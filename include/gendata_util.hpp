#ifndef Gendata_H
#define Gendata_H

#include "metadata.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/time.h>

/**
 * @brief generate random number in range [1, range]
 * 
 * @param[in] range 
 * @return int the generated random number 
 */
inline int rand_x(int range) {
    return ((rand() % (range)) + 1);
}
/**
 * @brief generate test data
 * 
 * @param[out] Ra column a range: 1-100 
 * @param[out] Rb column b range: 1-100 
 * @param[out] Rc column c range: 1-100 
 * @param[out] Rd column d range: 1-100 
 * @return int the number of lines generated
 */
idx gen_data(const idx& size_R, T* Ra, T* Rb, T* Rc, T* Rd) {
    timeval start, end;
    double ms;
    gettimeofday(&start, NULL);
    srand(time(NULL));
    size_t i;
    for(i = 0; i != size_R; ++i){
        
        Ra[i] = (rand_x(DATA_MAX_CONST));
        Rb[i] = (rand_x(DATA_MAX_CONST));
        Rc[i] = (rand_x(DATA_MAX_CONST));
        Rd[i] = 1; 
      
    }
    gettimeofday(&end, NULL);
    //ms = calc_ms(end, start);
    std::cout << ">>> Generated data " << i <<  " lines used time " << ms << "ms." << std::endl;
    return i;
}
/**
 * @brief generate conditions according to the selection rate of each test
 * 
 * @param conditions 
 * @return int number of conditions
 */
idx gen_conditions(std::vector<idx>& conditions) {
    int i;

  
    for(i = 0; i != CONST_TEST_NUM; ++i) {
        conditions.emplace_back(DATA_MAX_CONST * pow((CONST_BASE + (CONST_STRIDE * i)), 1.0/3));
    }
    
    std::cout << ">>> Generated conditions ";
    for(size_t j = 0; j != conditions.size(); ++j) {
        std::cout << conditions[j] << "%\t";
    }
    std::cout << std::endl;
    return i;
}
#endif