
#include "../include/metadata.h"
//#include "../include/time_util.h"
//#include "../include/log_util.h"
#include "../include/gendata_util.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/time.h>

/**
 * @brief calculate ms using timeval struct
 * 
 * @param end end_time
 * @param start start_time
 * @return double end - start
 */
inline double calc_ms(timeval end, timeval start) {
    return (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000.0;
}
idx dynamic_vector_col_branch(idx condition, const idx& size_R,
                          const T* Ra, 
                          const T* Rb, 
                          const T* Rc, 
                          const T* Rd,
                          std::vector<T>& ret1,
                          std::vector<T>& ret2,
                          std::vector<T>& ret3,
                          const Branch& branch){
    idx count = 0;
    // std::vector<int> result1, result2, result3;
    idx i;
    idx result_size = size_R;
    if(branch == Branch::BRANCH_ALL){
        // std::cout << "          Branching all: " << std::endl;
        // select on Ra
        for(i = 0; i != result_size; ++i) {
            if(Ra[i] <= condition) {
                ret1.emplace_back(i);
            }
        }
        result_size = ret1.size();
        // select on Rb
        for(i = 0; i != result_size; ++i){
            if(Rb[ret1[i]] <= condition) {
                ret2.emplace_back(ret1[i]);
            } 
        }
        result_size = ret2.size();
        // select on Rc
        for(i = 0; i != result_size; ++i){
            if(Rc[ret2[i]] <= condition) {
                ret3.emplace_back(ret2[i]);
            }
        }
        result_size = ret3.size();
        // select on Rd
        for(i = 0; i != result_size; ++i){
            count += Rd[ret3[i]];
        }

    } else if (branch == Branch::BRANCH_THREE) {
        // std::cout << "          Branching three: " << std::endl;
        // select on Ra and Rb
        for(i = 0; i != result_size; ++i) {
            if(Ra[i] <= condition && Rb[i] <= condition) {
                ret2.emplace_back(i);
            }
        }
        // select on Rc
        result_size = ret2.size();
        for(i = 0; i != result_size; ++i){
            if(Rc[ret2[i]] <= condition) {
                ret3.emplace_back(ret2[i]);
            }
        }
        result_size = ret3.size();
        // select on Rd
        for(i = 0; i != result_size; ++i){
            count += Rd[ret3[i]];
        }
    } else if (branch == Branch::BRANCH_TWO) {
        // std::cout << "          Branching two: " << std::endl;
        // select on Ra, Rb and Rc
        for(i = 0; i != result_size; ++i) {
            if(Ra[i] <= condition && Rb[i] <= condition && Rc[i] <= condition) {
                ret3.emplace_back(i);
            }
        }
        result_size = ret3.size();
        // select on Rd
        for(i = 0; i != result_size; ++i){
            count += Rd[ret3[i]];
        }
    } else {
        // std::cout << "          Branching non: " << std::endl;
        // NON_BRANCH
        for(i = 0; i != result_size; ++i) {
            if(Ra[i] <= condition && Rb[i] <= condition && Rc[i] <= condition) {
                count += Rd[i];
            }
        }
    }
    
    ret1.clear();
    ret2.clear();
    ret3.clear();
    return count;
}

/**
 * @brief perform select using static vector
 * @param condition determine the selection rate
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @return int the count of selection result
 */
idx static_vector_col_branch(idx condition, const idx& size_R,
                         const T* Ra, const T* Rb, 
                         const T* Rc, const T* Rd,
                         idx* ret, const Branch& branch){
    idx curr_size = size_R;
    idx count = 0;
    // select on Ra
    idx write_idx = 0;
    idx read_idx = 0;
    if(branch == Branch::BRANCH_ALL){
        // std::cout << "          Branching all: " << std::endl;
        for(read_idx = 0; read_idx != curr_size; ++read_idx) {
            if(Ra[read_idx] <= condition) {
                ret[write_idx] = read_idx;
                ++write_idx;
            }
        }
        // select on Rb
        curr_size = write_idx;
        write_idx = 0;
        for(read_idx = 0; read_idx != curr_size; ++read_idx){
            if(Rb[ret[read_idx]] <= condition) {
                ret[write_idx] = ret[read_idx];
                ++write_idx;
            }
        }
        // select on Rc
        curr_size = write_idx;
        write_idx = 0;
        for(read_idx = 0; read_idx != curr_size; ++read_idx){
            if(Rc[ret[read_idx]] <= condition) {
                ret[write_idx] = ret[read_idx];
                ++write_idx;
            }
        }
        // select on Rd
        curr_size = write_idx;
        for(read_idx = 0; read_idx != curr_size; ++read_idx){
            count += Rd[ret[read_idx]];
        }
    } else if (branch == Branch::BRANCH_THREE) {
        // std::cout << "          Branching three: " << std::endl;
        // select on Ra and Rb
        for(read_idx = 0; read_idx != curr_size; ++read_idx) {
            if(Ra[read_idx] <= condition && Rb[read_idx] <= condition) {
                ret[write_idx] = read_idx;
                ++write_idx;
            }
        }
        // select on Rc
        curr_size = write_idx;
        write_idx = 0;
        for(read_idx = 0; read_idx != curr_size; ++read_idx){
            if(Rc[ret[read_idx]] <= condition) {
                ret[write_idx] = ret[read_idx];
                ++write_idx;
            }
        }
        // select on Rd
        curr_size = write_idx;
        for(read_idx = 0; read_idx != curr_size; ++read_idx){
            count += Rd[ret[read_idx]];
        }
    } else if (branch == Branch::BRANCH_TWO) {
        // std::cout << "          Branching two: " << std::endl;
        // select on Ra, Rb and Rc
        for(read_idx = 0; read_idx != curr_size; ++read_idx) {
            if(Ra[read_idx] <= condition && Rb[read_idx] <= condition && Rc[read_idx] <= condition) {
                ret[write_idx] = read_idx;
                ++write_idx;
            }
        }
        // select on Rd
        curr_size = write_idx;
        for(read_idx = 0; read_idx != curr_size; ++read_idx){
            count += Rd[ret[read_idx]];
        }
    } else {
        // std::cout << "          Branching non: " << std::endl;
        // NON_BRANCH
         // select on all
        for(read_idx = 0; read_idx != curr_size; ++read_idx) {
            if(Ra[read_idx] <= condition && Rb[read_idx] <= condition && Rc[read_idx] <= condition) {
                count += Rd[read_idx];
                ++write_idx;
            }
        }
    }
    
    return count;
}
/**
 * @brief perform select using shared bitmap
 * 
 * @param condition determine the selection rate
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @return int the count of selection result
 */
idx shared_bitmap_col_branch(idx condition,const idx& size_R,
                         const T* Ra, const T* Rb, 
                         const T* Rc, const T* Rd,
                         std::vector<bool>& bitmap,
                         const Branch& branch){
    idx len = size_R;
    idx count = 0;
    idx i;
    if(branch == Branch::BRANCH_ALL){
        // std::cout << "          Branching all: " << std::endl;
        // select on Ra
        for(i = 0; i != len; ++i){
            if(Ra[i] <= condition)
            {
              bitmap[i] =  1;
            }
            else
            {
              bitmap[i] =  0;
            }
              
        }
        // select on Rb
        for(i = 0; i != len; ++i){
            if(bitmap[i]){
              if(Rb[i] <= condition)
              {
                bitmap[i] = 1;
              }
              else
              {
                bitmap[i] = 0;
              }
            
                
            }
            
        }

        // select on Rc
        for(i = 0; i != len; ++i){
            if(bitmap[i]){
              if((Rc[i] <= condition))
              {
                bitmap[i] = 1;
              }
              else
              {
                bitmap[i] = 0;
              }
                
            }
        }

        // select on Rd
        for(i = 0; i != len; ++i){
            if(bitmap[i]) {
                count += Rd[i];
            }
        }
        
    } else if (branch == Branch::BRANCH_THREE) {
        // std::cout << "          Branching three: " << std::endl;
        // select on Ra and Rb
        for(i = 0; i != len; ++i){
            if((Ra[i] <= condition) & (Rb[i] <= condition))
            {
              bitmap[i] = 1;
            }
            else
            {
              bitmap[i] = 0;
            }
            
        }
    
        // select on Rc
        for(i = 0; i != len; ++i){
            if(bitmap[i]){
              if(Rc[i] <= condition)
              {
                bitmap[i] = 1;
              }
              else
              {
                bitmap[i] = 0;
              }
                
            }
        }

        // select on Rd
        for(i = 0; i != len; ++i){
            if(bitmap[i]) {
                count += Rd[i];
            }
        }
        
    } else if (branch == Branch::BRANCH_TWO) {
        // std::cout << "          Branching two: " << std::endl;
        // select on Ra, Rb and Rc
        for(i = 0; i != len; ++i){
          if((Ra[i] <= condition) & (Rb[i] <= condition) & (Rc[i] <= condition))
          {
             bitmap[i] = 1;
          }
          else
          {
            bitmap[i] = 0;
          }
           
        }

        // select on Rd
        for(i = 0; i != len; ++i){
            if(bitmap[i]) {
                count += Rd[i];
            }
        }
        
    } else {
        // NON_BRANCH
        // std::cout << "          Branching non: " << std::endl;
        // select on all
        for(i = 0; i != len; ++i){
            if ( (Ra[i] <= condition) && (Rb[i] <= condition) && (Rc[i] <= condition)) {
                count += Rd[i];
            }
        }
        
    }
    return count;
}
/**
 * @brief perform select using seperate bitmap
 * 
 * @param condition determine the selection rate
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @return int the count of selection result
 */
idx seperate_bitmap_col_branch(idx condition,const idx& size_R,
                           const T* Ra, const T* Rb, 
                           const T* Rc, const T* Rd,
                           std::vector<bool>& bitmap_Ra, 
                           std::vector<bool>& bitmap_Rb,
                           std::vector<bool>& bitmap_Rc, 
                           std::vector<bool>& bitmap, 
                           const Branch& branch){
    idx len = size_R;
    int count = 0;
    
    if(branch == Branch::BRANCH_ALL){
        // std::cout << "          Branching all: " << std::endl;
        // select on Ra
        for(idx i = 0; i != len; ++i){
          if(Ra[i] <= condition)
          {
            bitmap_Ra[i] = 1;
          }
          else
          {
            bitmap_Ra[i] = 0;
          }
            
        }

        // select on Rb
        for(idx i = 0; i != len; ++i){
          if(Rb[i] <= condition)
          {
            bitmap_Rb[i] = 1;
          }
          else
          {
            bitmap_Rb[i] = 0;
          }
            
        }

        // select on Rc
        for(idx i = 0; i != len; ++i){
          if(Rc[i] <= condition)
          {
            bitmap_Rc[i] = 1;
          
          }
          else
          {
            bitmap_Rc[i] = 0;
          }
            
        }

        // merge results of Ra and Rb and Rc
        for(idx i = 0; i != len; ++i) {
          if((bitmap_Ra[i]&bitmap_Rb[i]&bitmap_Rc[i]))
          {
            bitmap[i] = 1;
          }
          else
          {
            bitmap[i] = 0;
          }
            
        }

        // select on Rd
        for(idx i = 0; i != len; ++i){
            if(bitmap[i]) {
                count += Rd[i];
            }
        }
        
    } else if (branch == Branch::BRANCH_THREE) {
        // std::cout << "          Branching three: " << std::endl;
        // select on Ra and Rb
        for(idx i = 0; i != len; ++i){
          if((Ra[i] <= condition) & (Rb[i] <= condition))
          {
            bitmap_Rb[i] = 1;
          }
          else
          {
            bitmap_Rb[i] = 0;
          }
            
        }

        // select on Rd
        for(idx i = 0; i != len; ++i){
          if(Rc[i] <= condition)
          {
            bitmap_Rc[i] = 1;
          }
          else
          {
            bitmap_Rc[i] = 0;
          }
            
        }

        // merge results of Ra and Rb and Rc
        for(idx i = 0; i != len; ++i) {
          if((bitmap_Rb[i]&bitmap_Rc[i]))
          {
            bitmap[i] = 1;
          }
          else
          {
            bitmap[i] = 0;
          }
            
        }

        // select on Rd
        for(idx i = 0; i != len; ++i){
            if(bitmap[i]) {
                count += Rd[i];
            }
        }
        
    } else if (branch == Branch::BRANCH_TWO) {
        // std::cout << "          Branching two: " << std::endl;
        // select on Ra, Rb and Rc
        for(idx i = 0; i != len; ++i){
          if((Ra[i] <= condition) & (Rb[i] <= condition) & (Rc[i] <= condition))
          {
            bitmap_Rc[i] = 1;
          }
          else
          {
            bitmap_Rc[i] = 0;          
          }
            
        }

        // merge results of Ra and Rb and Rc
        for(idx i = 0; i != len; ++i) {
            bitmap[i] = bitmap_Rc[i];
        }

        // select on Rd
        for(idx i = 0; i != len; ++i){
            if(bitmap[i]) {
                count += Rd[i];
            }
        }
        
    } else {
        // NON_BRANCH
        // std::cout << "          Branching non: " << std::endl;
        // select on all
        for(size_t i = 0; i != len; ++i){
            if ((Ra[i] <= condition) && (Rb[i] <= condition) && (Rc[i] <= condition)) {
                count += Rd[i];
            }
        }
    }
    return count;
}
/**
 * @brief test1: select using dynamic vector, the stride of selection rate change is 
 *               10% per test for const stride
 * 
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @param branch
 * @return void
 */
void test_dynamic_vector_col_branch(const idx& size_R,
                                const T* Ra, const T* Rb, 
                                const T* Rc, const T* Rd,
                                const std::vector<idx>& conditions,
                                const Branch& branch, std::ofstream& timefile) {
                                    
    std::cout << ">>> Start test using dynamic vector" << std::endl;  
    timefile << "dynamic vector\t";
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 3)*100 << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);
        std::vector<int> ret1, ret2, ret3;
        count = dynamic_vector_col_branch(conditions[select_idx], DATA_NUM, Ra, Rb, Rc, Rd, ret1, ret2, ret3, branch);
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        timefile << ms << "\t";
    }
    timefile << std::endl;
}

/**
 * @brief test2: select using static array, test in one group
 *               for comparison of the allocation efficiency
 * @param Ra
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @return void
 */
void test_static_vector_col_branch(const idx& size_R,
                                            const T* Ra, const T* Rb, 
                                            const T* Rc, const T* Rd,
                                            const std::vector<idx>& conditions,
                                            const Branch& branch, std::ofstream& timefile) {
    const idx vector_size = size_R;
    int* ret = new int[vector_size];
    timefile << "static vector (group)\t";
    std::cout << ">>> Start test using static vector" << std::endl;
    for(size_t select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);
        count = static_vector_col_branch(conditions[select_idx], DATA_NUM, Ra, Rb, Rc, Rd, ret, branch);
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        // time_results.emplace_back(ms);
        timefile << ms << "\t";
    }
    delete[] ret;
    timefile << std::endl;
}
/**
 * @brief test3: select using shared bitmap, use only one bitmap
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @return void 
 */
void test_shared_bitmap_col_branch( const idx& size_R,
                               const T* Ra, const T* Rb, 
                               const T* Rc, const T* Rd,
                               const std::vector<idx>& conditions, 
                               const Branch& branch, std::ofstream& timefile) {
    std::cout << ">>> Start test using shared bitmap" << std::endl;
    timefile << "shared bitmap\t";
    // bool* bitmap = new bool[c1.size()];
    std::vector<bool> bitmap;
    bitmap.reserve(DATA_NUM);
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);
        count = shared_bitmap_col_branch(conditions[select_idx], DATA_NUM, Ra, Rb, Rc, Rd, bitmap, branch);

        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        // time_results.emplace_back(ms);
        timefile << ms << "\t";
    }
    timefile << std::endl;
    // }
    
    // delete[] bitmap;
}
/**
 * @brief test4: select using seperate bitmap, 
 *               for every column use a bitmap to record the selection 
 *               calculate the & of them to get the final result
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @return void
 */
void test_seperate_bitmap_col_branch(const idx& size_c,
                                        const T* Ra, const T* Rb, 
                                        const T* Rc, const T* Rd,
                                        const std::vector<idx>& conditions, 
                                        const Branch& branch, std::ofstream& timefile) {
    
    std::cout << ">>> Start test using seperate bitmap" << std::endl;

    timefile << "seperate bitmap\t";
    std::vector<bool> bitmap_Ra, bitmap_Rb, bitmap_Rc, bitmap;
    bitmap_Ra.reserve(DATA_NUM);
    bitmap_Rb.reserve(DATA_NUM);
    bitmap_Rc.reserve(DATA_NUM);
    bitmap.reserve(DATA_NUM);
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "%" << std::endl;
        int count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);
        count = seperate_bitmap_col_branch(conditions[select_idx], DATA_NUM, Ra, Rb, Rc, Rd, 
                                        bitmap_Ra, bitmap_Rb, bitmap_Rc, bitmap, branch);
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        // time_results.emplace_back(ms);
        timefile << ms << "\t";
    }
    timefile << std::endl;
  
}

/**
 * @brief test5: select using static array with Vectorization processing
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @return void 
 */
void test_dynamic_vector_vec_branch(const idx& size_R,
                                                const T* Ra, const T* Rb, 
                                                const T* Rc, const T* Rd,
                                                const std::vector<idx>& conditions,
                                                const Branch& branch, std::ofstream& timefile) {
    std::cout << ">>> Start test using dynamic vector with Vectorization processing" << std::endl;  
    timefile << "dynamic vector with Vectorization processing\t" ;
    std::vector<idx> ret1, ret2, ret3;
    ret1.reserve(size_v);
    ret2.reserve(size_v);
    ret3.reserve(size_v);
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 3)*100 << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);;
        idx vec_num = DATA_NUM / size_v;
        for(idx i = 0; i != vec_num; ++i) {
            count += dynamic_vector_col_branch(conditions[select_idx], size_v, Ra+i*size_v, Rb+i*size_v, Rc+i*size_v, Rd+i*size_v, 
                                                  ret1, ret2, ret3, branch);
        }
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        timefile << ms << "\t";
    }
    timefile << std::endl;
    //     time_results.emplace_back(ms);
    // }
}
/**
 * @brief test6: select using static array with Vectorization processing
 * @param Ra
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @return void 
 */
void test_static_vector_vec_branch(const idx& size_R,
                                                        const T* Ra, const T* Rb, 
                                                        const T* Rc, const T* Rd,
                                                        const std::vector<idx>& conditions, 
                                                        const Branch& branch, std::ofstream& timefile) {
    std::cout << ">>> Start test using static vector with Vectorization processing" << std::endl;  
    timefile << "static vector with Vectorization processing\t";
    int* ret = new int[size_v];
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 3)*100 << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);;
        idx vec_num = DATA_NUM / size_v;
        for(idx i = 0; i != vec_num; ++i) {
            count += static_vector_col_branch(conditions[select_idx], size_v, Ra+i*size_v, Rb+i*size_v, Rc+i*size_v, Rd+i*size_v, ret, branch);
        }
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        timefile << ms << "\t";
    }
    timefile << std::endl;
    //     time_results.emplace_back(ms);
    // }
    delete[] ret;
}
/**
 * @brief test7: select using shared bitmap with Vectorization processing
 * @param Ra
 * @param Rb
 * @param Rc
 * @param Rd
 * @param conditions
 * @return void 
 */
void test_shared_bitmap_vec_branch(const idx& size_R,
                                                const T* Ra, const T* Rb, 
                                                const T* Rc, const T* Rd,
                                                const std::vector<idx>& conditions, 
                                                const Branch& branch, std::ofstream& timefile) {
    std::cout << ">>> Start test using shared bitmap with Vectorization processing" << std::endl;  
    timefile << "shared bitmap partition\t";
    // bool* bitmap = new bool[c1.size()];
    std::vector<bool> bitmap;
    bitmap.reserve(size_v);
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 3)*100 << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);;
        idx vec_num = DATA_NUM / size_v;
        for(idx i = 0; i != vec_num; ++i) {
            count += shared_bitmap_col_branch(conditions[select_idx], size_v, Ra+i*size_v, Rb+i*size_v, Rc+i*size_v, Rd+i*size_v, bitmap, branch);
        }
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        // time_results.emplace_back(ms);
        timefile << ms << "\t";
    }
    timefile << std::endl;
    // }
    // delete[] bitmap;
}
/**
 * @brief test8: select using seperate bitmap with Vectorization processing
 * @param Ra 
 * @param Rb 
 * @param Rc 
 * @param Rd 
 * @param conditions
 * @return void
 */
void test_seperate_bitmap_vec_branch(const idx& size_R,
                                                const T* Ra, const T* Rb, 
                                                const T* Rc, const T* Rd,
                                                const std::vector<idx>& conditions, 
                                                const Branch& branch, std::ofstream& timefile) {
    std::cout << ">>> Start test using seperate bitmap with Vectorization processing" << std::endl;  
    timefile << "seperate bitmap with Vectorization processing\t";
    std::vector<bool> bitmap_Ra, bitmap_Rb, bitmap_Rc, bitmap;
    bitmap_Ra.reserve(size_v);
    bitmap_Rb.reserve(size_v);
    bitmap_Rc.reserve(size_v);
    bitmap.reserve(size_v);
    for(idx select_idx = 0; select_idx != conditions.size(); select_idx++) {
        std::cout << "      column selection rate " <<  conditions[select_idx] << 
                    "%, total selection rate " << pow(conditions[select_idx], 3)/pow(100, 3)*100 << "%" << std::endl;
        idx count = 0;
        timeval start, end;
        gettimeofday(&start, NULL);;
        size_t vec_num = DATA_NUM / size_v;
        for(idx i = 0; i != vec_num; ++i) {
            count += seperate_bitmap_col_branch(conditions[select_idx], size_v, Ra+i*size_v, Rb+i*size_v, Rc+i*size_v, Rd+i*size_v, 
                                            bitmap_Ra, bitmap_Rb, bitmap_Rc, bitmap, branch);
        }
        gettimeofday(&end, NULL);
        double ms = calc_ms(end, start);
        std::cout << "          Result count of selection rate " << pow(conditions[select_idx], 3)/pow(100, 2) << "% is " << count << "/" << DATA_NUM << std::endl;
        std::cout << "          Time: " << ms << "ms" << std::endl;
        timefile << ms << "\t";
    }
    timefile << std::endl;
    //     time_results.emplace_back(ms);
    // }
    // delete[] bitmap;
    // delete[] bitmap_c1;
    // delete[] bitmap_c2;
    // delete[] bitmap_c3;
}
int main() {
    std::ofstream timefile;
    timefile.open(TIME_FILE, std::ios::out | std::ios::trunc );

    bool is_exp = false;

    timefile << "stride exp: " << is_exp << std::endl;
        
    T* Ra = new T[DATA_NUM];
    T* Rb = new T[DATA_NUM];
    T* Rc = new T[DATA_NUM];
    T* Rd = new T[DATA_NUM];
    std::vector<int> conditions;
    gen_data(DATA_NUM, Ra, Rb, Rc, Rd);
    gen_conditions(conditions);
        
    for(const auto branch : BRANCH) {
      timefile << "non-partitioning, branch: " << (int)branch << std::endl;
      /*1. test dynamic select vector*/
      test_dynamic_vector_col_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*2. test static select vector*/
      test_static_vector_col_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*3. test shared bitmap*/
      test_shared_bitmap_col_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*4. test seperate bitmap*/
      test_seperate_bitmap_col_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*5. test dynamic select vector with Vectorization processing*/
      test_dynamic_vector_vec_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*6. test static select vector with Vectorization processing*/
      test_static_vector_vec_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*7. test shared bitmap with Vectorization processing*/
      test_shared_bitmap_vec_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      /*8. test seperate bitmap with partition*/
      test_seperate_bitmap_vec_branch(DATA_NUM, Ra, Rb, Rc, Rd, conditions, branch, timefile);
      
      timefile << std::endl;
      }
        
        // auto branch = Branch::BRANCH_ALL;
        // branch
  
  
        timefile << std::endl;
        delete[] Ra;
        delete[] Rb;
        delete[] Rc;
        delete[] Rd;
        conditions.clear();
    
    // std::cout << "\ntime results" << std::endl;
    // for(size_t i = 0; i != time_results.size(); ++i) {
    //     std::cout << time_results[i] << std::endl;
    // }
    // std::cout << std::endl;
    // time_results.clear();
    timefile.close();
}
