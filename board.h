#ifndef GOL_BOARD2_H
#define GOL_BOARD2_H

#if defined(__INTEL_COMPILER)
#include <malloc.h>
#else
#include <mm_malloc.h>
#endif // defined(__GNUC__)

#include <cstdlib>
#include <iostream>

// extended board with additional borders

class board {
public:
    int* matrix;
    int m_width;
    int m_height;

    board(int rows, int columns) {
        m_width = columns+2;
        m_height = rows+2;
        auto size = m_width * m_height * sizeof(int);
        size += size%64;
        matrix = (int*) _mm_malloc(size, 64);
    }

    ~board() {
        _mm_free(matrix);
    }

    void set_random() {
        // second row starting index
        int start = m_width;
        // last row starting index
        int end = (m_height-1)*m_width;

        // fill the inner part (game part) with random value
        int line_count = 0;
        for (int i = start; i < end ; ++i) {
            // (line_count == 0 || line_count == m_width-1) => boarder
            matrix[i] = (line_count == 0 || line_count == m_width-1) ? -1 : rand()%2;
            line_count = (line_count+1)%m_width;
        }

        // set left and right border
        int left, right;
        for (int i = 1; i < m_height ; i++) {
            left = i*m_width;
            right = left+m_width-1;
            matrix[left] = matrix[right-1];
            matrix[right] = matrix[left+1];
        }

        // set upper and bottom border
        for (int j = 0; j < start; ++j) {
            // copy last row in first row
            matrix[j] = matrix[end-m_width+j];
            // copy first row in last row
            matrix[end+j] = matrix[start+j];
        }
    }

    // periodic conf
    void set_blinker() {
        auto i = m_width*3 + m_height/2;
        matrix[i-1] = 1;
        matrix[i] = 1;
        matrix[i+1] = 1;
    }

    // static conf
    void set_beehive() {
        auto i = m_width*2 + m_height/3;
        matrix[i-m_width+1] = 1;
        matrix[i-m_width+2] = 1;
        matrix[i] = 1;
        matrix[i+3] = 1;
        matrix[i+m_width+1] = 1;
        matrix[i+m_width+2] = 1;
    }

    void print() {
        auto line_count = 0;
        for (auto i = 0; i < m_width*m_height; ++i) {

            std::cout << matrix[i] << " ";
            line_count++;

            if (line_count == m_width){
                std::cout << std::endl;
                line_count = 0;
            }

        }
        std::cout << std::endl;
    }

    // check if this is obtained from "in"
    bool test(board* in) {

        // check board dimensions
        if (in->m_width != m_width || in->m_height != m_height) {
            return false;
        }

        // check matrix
        int ind, cell_value, expected_value, sum;
        int* origin = in->matrix;
        for (int i = 1; i < m_height; ++i) {
            ind = i*m_width;
            for (int j = 1; j < m_width; ++j) {

                cell_value = origin[ind+j];
                sum =   origin[ind+j-1-m_width] + origin[ind+j-m_width] + origin[ind+j+1-m_width] +
                        origin[ind+j-1] + origin[ind+j+1] +
                        origin[ind+j-1+m_width] + origin[ind+j+m_width] + origin[ind+j+1+m_width];
                expected_value = (sum == 3) || (sum+cell_value == 3) ? 1 : 0;

                if(expected_value != matrix[ind+j]){
                    return false;
                }

            }
        }

        // TODO: check borders


        return true;

    }

};

#endif //GOL_BOARD2_H
