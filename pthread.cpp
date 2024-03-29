#include <iostream>
#include <fstream>
#include <unistd.h>
#include <pthread.h>
#include <chrono>

#if defined(__INTEL_COMPILER)
#include <malloc.h>
#else
#include <mm_malloc.h>
#endif // defined(__GNUC__)

#include "board.h"

//#define PRINT

struct thread_data{
    int _start;
    int _stop;
};

// synchronization point
pthread_barrier_t barrier;

// game boards
board * in;
board * out;
// game iteration
int iter_num;

#ifdef PRINT
std::ofstream file;
#endif

void* body(void* arg) {

    // get data from arg
    thread_data* data = (thread_data*) arg;
    board * p_in = in;
    board * p_out = out;
    int start = data->_start;
    int stop = data->_stop;

    // matrix dimensions
    const auto cols = p_in->m_width;
    const auto rows = p_in->m_height;

    // data pointers
    int* matrix_in;
    int* matrix_out;

    // thread chunk
    const auto assigned_row_num = (stop-start+1);

    // game iteration
    for (int k = 0; k < iter_num; ++k) {

        matrix_in = p_in->matrix;
        matrix_out = p_out->matrix;

        // current, upper and lower indices
        auto up_p = start*cols + 1;
        auto curr_p = up_p + cols;
        auto low_p = curr_p + cols;

        #pragma ivdep
        for (int i = 1; i < assigned_row_num*cols; ++i) {

            // compute alive neighbours
            auto sum = matrix_in[up_p-1] + matrix_in[up_p] + matrix_in[up_p+1]
                       + matrix_in[curr_p-1] + matrix_in[curr_p+1]
                       + matrix_in[low_p-1] + matrix_in[low_p] + matrix_in[low_p+1];

            // set the current cell state
            matrix_out[curr_p] = (sum == 3) || (sum+matrix_in[curr_p] == 3) ? 1 : 0;

            // move the pointers
            up_p++;
            curr_p++;
            low_p++;
        }

        // set left and right border
        int left = start*cols + cols;
        int right = left+cols - 1;
        // no vectorization here (noncontiguous memory access make it inefficient)
        for (int i = 0; i < assigned_row_num; i++) {
            matrix_out[left] = matrix_out[right-1];
            matrix_out[right] = matrix_out[left+1];
            left += cols;
            right += cols;
        }

        // thread that compute the first row have to copy it as bottom border
        if (start == 0) {
            const auto sr_index = cols;
            const auto bb_index = (rows-1)*cols;
            #pragma ivdep
            for (int i = 0; i < cols; ++i) {
                matrix_out[bb_index+i] = matrix_out[sr_index+i];
            }
        }
        // thread that compute the last row have to copy it as upper border
        if (stop == rows-3) {
            const auto slr_index = (rows-2)*cols;
            #pragma ivdep
            for (int ub_index = 0; ub_index < cols; ++ub_index) {
                matrix_out[ub_index] = matrix_out[slr_index+ub_index];
            }
        }

        // swap board pointers
        board * tmp = p_in;
        p_in = p_out;
        p_out = tmp;

        // synchronization point
        int res = pthread_barrier_wait(&barrier);
        if(res == PTHREAD_BARRIER_SERIAL_THREAD) {
            // one of the forked threads pass here when threads exit from barrier, so this is not a real serial part
            // but until all threads (including the "serial" thread) have reached again the barrier
            // this portion of code is not executed again.
            //
            // so we can insert print here if we read safe data structures (data that are not write in the next iteration)
            #ifdef PRINT
            (*p_in).print_file(file);
            #endif
        } else if(res != 0) {
            std::cout << "Barrier error n." << res << std::endl;
            pthread_exit(NULL);
        }

    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {

    // board size
    auto rows = atoi(argv[1]);
    auto cols = atoi(argv[2]);
    // iteration number
    iter_num = atoi(argv[3]);
    // parallelism degree
    auto th_num = atoi(argv[4]);

    // data structures
    in = new board(rows, cols);
    out = new board(rows, cols);

    in->set_random();

    #ifdef PRINT
    std::time_t time = std::time(nullptr);
    std::string filename = std::to_string((long long)time)+"_seq.test.txt";
    file.open(filename);
    file << in->m_height << std::endl;
    file << in->m_width << std::endl;
    file << iter_num << std::endl;
    in->print_file(file);
    #endif

    // time start
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    // split parameaters
    auto th_rows = rows / th_num;
    auto remains = rows % th_num;

    // thread pool setup
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // initialize a barrier
    pthread_barrier_init(&barrier, NULL, th_num);

    // build thread chunk
    int start, stop = 0;
    thread_data* t_data = (thread_data*) malloc(sizeof(t_data)*th_num);
    for(auto i=0; i<th_num; i++) {
        start = stop;
        stop = (remains > 0) ? start + th_rows : start + th_rows -1;
        t_data[i]  = {start, stop};
        #ifdef PRINT
        std::cout << "Thread n." << i << " get rows from " << start << " to " << stop << std::endl;
        #endif
        remains--;
        stop++;
    }

    // run the threads
    pthread_t* tid = (pthread_t*) malloc(sizeof(pthread_t)*th_num);
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

    // clean up threads data structures
    pthread_attr_destroy(&attr);
    pthread_barrier_destroy(&barrier);

    // time end
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "game execution time is: " << duration << " milliseconds" << std::endl;
    std::cout << std::endl;

    // game data structures clean
    delete in;
    delete out;

    free(t_data);

    return 0;
}
