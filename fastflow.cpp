#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <ff/parallel_for.hpp>

#include "board.h"

// cache efficient version, the board is extended with additional border to allow
// a linear scan of the memory with tree indices that compute the neighbour sum
//
// the two board (implemented as contiguous memory) are read and write in a perfect linear way
//
// this version can be vectorized but with fastflow parallel for the compiler does not consider it convenient
// compile with flag "-vec-report5" for further.
//
// performances are the same as a non vectorized version ("-no-vec"), excepted for the case with 1 thread

// #define PRINT

int main(int argc, char* argv[]) {

    // board size
    auto rows = atoi(argv[1]);
    auto cols = atoi(argv[2]);
    // iteration number
    auto it_num = atoi(argv[3]);
    // parallelism degree
    auto th_num = atoi(argv[4]);

    // data structures
    board in(rows, cols);
    board out(rows, cols);
    // data pointers
    board * p_in = &in;
    board * p_out = &out;
    // memory data pointer
    int* matrix_in;
    int* matrix_out;

    int width = in.m_width;

    in.set_random();

    ff::ParallelFor pf(th_num, true);

    #ifdef PRINT
    std::time_t time = std::time(nullptr);
    std::ofstream file;
    std::string filename = std::to_string((long long)time)+"_seq.test.txt";
    file.open(filename);
    file << in.m_height << std::endl;
    file << in.m_width << std::endl;
    file << it_num << std::endl;
    in.print_file(file);
    #endif

    // time start
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    // game iteration
    for (int k = 0; k < it_num; ++k) {

        matrix_in = p_in->matrix;
        matrix_out = p_out->matrix;

        pf.parallel_for(1, rows+1, [matrix_in,matrix_out,width,rows,cols](const long i) {

            // current, upper and lower indices
            auto up_p = (i-1)*width + 1;
            auto curr_p = up_p + width;
            auto low_p = curr_p + width;

            #pragma ivdep
            for (int j = 0; j < cols; ++j) {
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
            int left = i*width;
            int right = left+width-1;
            matrix_out[left] = matrix_out[right-1];
            matrix_out[right] = matrix_out[left+1];

            // first row must be copied as bottom border
            if (i == 1) {
                const auto fr_index = width;
                const auto bb_index = (rows+1)*width;
                #pragma ivdep
                for (int z = 0; z < width; ++z) {
                    matrix_out[bb_index+z] = matrix_out[fr_index+z];
                }
            }
            // last row must be copied as upper border
            if (i == rows) {
                const auto lr_index = (rows)*width;
                #pragma ivdep
                for (int ub_index = 0; ub_index < width; ++ub_index) {
                    matrix_out[ub_index] = matrix_out[lr_index+ub_index];
                }
            }

        }, th_num);

        #ifdef PRINT
        (*p_out).print_file(file);
        #endif
        // swap pointer
        board * tmp = p_in;
        p_in = p_out;
        p_out = tmp;

    }

    // time end
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << std::endl;
    std::cout << "game execution time is: " << duration << " milliseconds" << std::endl;
    std::cout << std::endl;

    return 0;
}
