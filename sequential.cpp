#include <iostream>

#include "board.h"
#include "game_conf.h"

/*
 *
 * compile with: g++ -std=c++11 -O3 sequential.cpp -o seq.exe
 * run with: ./seq.exe @row_number @colum_number @iteration_number [@configuration_number (from 1 to 4)]
 *
 * */

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

int main(int argc, char* argv[]) {

    // board size
    auto rows = atoi(argv[1]);
    auto cols = atoi(argv[2]);
    // iteration number
    auto it_num = atoi(argv[3]);
    // starting configuration
    auto conf_num = (argc>4) ? atoi(argv[4]) : 0;

    // data structures
    board in(rows,cols);
    board out(rows,cols);
    // data pointers
    board* p_in = &in;
    board* p_out = &out;

    switch (conf_num) {
        case 0:
            game_conf::set_random_conf(in);
            break;
        case 1 :
            game_conf::set_test_conf_1(in);
            break;
        case 2 :
            game_conf::set_test_conf_2(in);
            break;
        case 3:
            game_conf::set_test_conf_3(in);
            break;
        case 4:
            game_conf::set_test_conf_4(in);
            break;
    }
    //in.print();


    // game iteration
    while(it_num > 0) {

        for (auto i = 0; i < rows; ++i) {
            for (auto j = 0; j < cols ; ++j) {
                update(i,j,*p_in,*p_out);
                //std::cout << (*p_out)[i][j] << " ";
            }
            //std::cout << std::endl;
        }

        //swap pointer
        board* tmp = p_in;
        p_in = p_out;
        p_out = tmp;

        it_num--;
        //std::cout << std::endl;
    }

    return 0;
}