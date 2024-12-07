#ifndef Metadata_H
#define Metadata_H

typedef int idx;
typedef int T;
idx DATA_NUM = 1 << 24;
const int DATA_MAX_CONST = 100;     // max for const stride selection rate test
const idx CONST_TEST_NUM = 10;   // 10 tests with const change of selection rate stride
const float CONST_BASE = 0.1;       // const stride base selection rate
const float CONST_STRIDE = 0.1;     // stride of selection rate
const char* TIME_FILE = "./log/time_array_vectorizing_4096.tsv"; // the result time file
const idx size_v = 1024;//Vector length
enum Branch {
    NON_BRANCH = 0,
    BRANCH_TWO,
    BRANCH_THREE,
    BRANCH_ALL
};
const Branch BRANCH[] = {NON_BRANCH, BRANCH_TWO, BRANCH_THREE, BRANCH_ALL};
#endif