#include <iostream>
#include <pthread.h>

#include "board.h"

/*
 *
 *
 * compile with: g++ pthread.cpp -std=c++11 -O3 -pthread -o pthread.exe
 * run with: ./threads.exe @row_number @colum_number @iteration_number @parallelism degree [@configuration_number (from 1 to 4)]
 *
 *
 * */

struct thread_data{
    board* _in;
    board* _out;
    int _start;
    int _stop;
    int _iter;
    int _id;
};

// synchronization lock (some kind of barrier)
pthread_barrier_t barrier;

void update(int i, int j, board& in, board& out) {

    // upper and lower row indices
    auto up_p = (i-1) >= 0 ? (i-1) : (in.m_height-1);
    auto low_p = (i+1) % in.m_height;
    // left and right column indices
    auto left_p = (j-1) >= 0 ? (j-1) : (in.m_width-1);
    auto right_p = (j+1) % in.m_width;

    auto sum = in[up_p][left_p] + in[up_p][j] + in[up_p][right_p]
               + in[i][left_p] + in[i][right_p]
               + in[low_p][left_p] + in[low_p][j] + in[low_p][right_p];

    // empty cell
    if (in[i][j] == 0) {
        // with exactly 3 alive neighbours alive otherwise die
        out[i][j] = (sum == 3) ? 1 : 0;
        return;
    }
    // alive cell
    if (in[i][j] == 1) {
        // with less then 2 or more than 3 alive neighbours then die
        // otherwise keep alive
        out[i][j] = (sum < 2 || sum > 3) ? 0 : 1;
        return;
    }
}

void* body(void* arg) {

    thread_data* data = (thread_data*) arg;
    board* p_in = data->_in;
    board* p_out = data->_out;
    int start = data->_start;
    int stop = data->_stop;
    int iter = data->_iter;

    // columns number
    const auto col = p_in->m_width;

    // game iteration
    while(iter > 0) {

        for (auto i = start; i <= stop; ++i) {
            for (auto j = 0; j < col; ++j) {
                update(i,j,*p_in,*p_out);
            }
        }

        // swap pointer
        board* tmp = p_in;
        p_in = p_out;
        p_out = tmp;
        // decrease iteration count
        iter--;

        int res = pthread_barrier_wait(&barrier);
        if(res == PTHREAD_BARRIER_SERIAL_THREAD) {
            (*p_in).print();
        } else if(res != 0) {
            // error occurred
            std::cout << "Barrier error n." << res << std::endl;
        }

    }

    pthread_exit(NULL);
}

void set_random_conf(board& b) {
    for(auto i=0; i<b.m_width; i++)
        for(auto j=0; j<b.m_height; j++)
            b[i][j] = rand()%2;
}

// Beacon test conf (periodic)
void set_test_conf_1(board& b) {
    b[0][0] = 1;
    b[0][1] = 1;
    b[1][0] = 1;
    b[1][1] = 1;

    b[2][2] = 1;
    b[2][3] = 1;
    b[3][2] = 1;
    b[3][3] = 1;
}

// Blinker test conf (periodic)
void set_test_conf_2(board& b) {
    b[b.m_height/2][b.m_width/2] = 1;
    b[b.m_height/2][b.m_width/2+1] = 1;
    b[b.m_height/2][b.m_width/2-1] = 1;
}

// Glider test conf (dynamic)
// note: in a 10x10 matrix come back to initial conf in 40 iterations
void set_test_conf_3(board& b) {
    b[0][1] = 1;
    b[1][2] = 1;
    b[2][0] = 1;
    b[2][1] = 1;
    b[2][2] = 1;
}

// Beehive test conf (static)
void set_test_conf_4(board& b) {
    b[0][1] = 1;
    b[0][2] = 1;
    b[1][0] = 1;
    b[1][3] = 1;
    b[2][1] = 1;
    b[2][2] = 1;
}

int main(int argc, char* argv[]) {

    // board size
    auto rows = atoi(argv[1]);
    auto cols = atoi(argv[2]);
    // iteration number
    auto it_num = atoi(argv[3]);
    // parallelism degree
    auto th_num = atoi(argv[4]);
    // board initial configuration
    auto conf_num = atoi(argv[5]);

    // data structures
    board in(rows,cols);
    board out(rows,cols);

    switch (conf_num) {
        case 0:
            set_random_conf(in);
            break;
        case 1 :
            set_test_conf_1(in);
            break;
        case 2 :
            set_test_conf_2(in);
            break;
        case 3:
            set_test_conf_3(in);
            break;
        case 4:
            set_test_conf_4(in);
            break;
    }
    in.print();

    // TODO: think about static splitting [particular case es: th_rows<1 ...]

    auto th_rows = rows / th_num;
    auto remains = rows % th_num;

    // thread pool setup
    /* Initialize and set thread detached attribute */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t)*th_num);
    // initialize a barrier
    pthread_barrier_init(&barrier, NULL, th_num);

    int start, stop = 0;
    thread_data* t_data = (thread_data*) malloc(sizeof(t_data)*th_num);
    for(auto i=0; i<th_num; i++) {
        start = stop;
        stop = (remains > 0) ? start + th_rows : start + th_rows -1;
        t_data[i]  = {&in, &out, start, stop, it_num, i};
        std::cout << "Thread n." << i << " get rows from " << start << " to " << stop << std::endl;
        remains--;
        stop++;
    }

    for(auto i=0; i<th_num; i++) {
        auto rc = pthread_create(&tid[i], NULL, body, (void *)&t_data[i]);
        if (rc){
            std::cout << "ERROR; return code from pthread_create() is " << rc << std::endl;
        }
    }

    // await termination
    void *status;
    for(auto i=0; i<th_num; i++) {
        auto rc = pthread_join(tid[i], &status);
        if (rc) {
            std::cout << "ERROR; return code from pthread_join() is " << rc << std::endl;
        }
    }

    // clean up
    pthread_attr_destroy(&attr);
    pthread_barrier_destroy(&barrier);

    return 0;
}
