/**
 * @file vectorjoin_join_numa.cpp
 * @author ruichenhan (hanruichen@ruc.edu.cn)
 * @brief test Vector join algorithm based on Numa optimization
 * @version 0.1
 * @date 2024-09-04
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <sched.h>    /* CPU_ZERO, CPU_SET */
#include <pthread.h>  /* pthread_* */
#include <string.h>   /* memset */
#include <stdio.h>    /* printf */
#include <stdlib.h>   /* memalign */
#include <sys/time.h> /* gettimeofday */
#include <time.h>
#include "gendata_util.hpp"
#include "metadata.h"
#include "barrier.h"  /* pthread_barrier_* */
#include "affinity.h" /* pthread_attr_setaffinity_np */
// #include "../util/generator.h"          /* numa_localize() */
// #include "../util/no_partitioning_join.h" /* no partitioning joins: NPO, NPO_st */
// #include "../util/parallel_radix_join.h"  /* parallel radix joins: RJ, PRO, PRH, PRHO */
// #include "../util/generator.h"            /* create_relation_xk */
// #include "../util/Hashgroup.h"
// #include "../util/constants.h"     /* DEFAULT_R_SEED, DEFAULT_R_SEED */
// #include "../util/affinity.h"      /* pthread_attr_setaffinity_np & sched_setaffinity */
#include <iostream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <array>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <getopt.h> /* getopt */
#include <limits.h> /* INT_MAX */
#include <unistd.h> /* sysconf */
#include <unordered_map>
#include "paraforans.cpp"
#define BARRIER_ARRIVE(B, RV)                           \
    RV = pthread_barrier_wait(B);                       \
    if (RV != 0 && RV != PTHREAD_BARRIER_SERIAL_THREAD) \
    {                                                   \
        printf("Couldn't wait on barrier\n");           \
        exit(EXIT_FAILURE);                             \
    }

bool help_flag = false;
void parse_args(int argc, char **argv, param_join_t *cmd_params)
{

    int c, i, found;
    while (1)
    {
        static struct option long_options[] =
            {
                /* These options set a flag. */

                {"help", no_argument, 0, 'h'},
                // {"help", required_argument, 0, 'h'},
                /* These options don't set a flag.
                   We distinguish them by their indices. */
                {"nthreads", required_argument, 0, 'n'},
                {"s", required_argument, 0, 's'},
                {"r", required_argument, 0, 'r'},
                {"numa_partition", required_argument, 0, 'p'},
                {"test_bandwidth", required_argument, 0, 't'},
                {"col_num", required_argument, 0, 'c'},
                {"cachetest", required_argument, 0, 'l'},
                {0, 0, 0, 0}};
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "h:n:s:r:p:t:c:l",
                        long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;
        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'h':
            printf("\parameter:\n \t1.numa_partition: for numa test\n \t2.test_bandwidth: for bandwidth test\n \t3.cachetest: for cache test\te.g.:1 2 3\n");
            // cmd_params->help = atoi(optarg);
            help_flag = true;
            break;
        case 'n':
            cmd_params->nthreads = atoi(optarg);
            break;
        case 's':
            cmd_params->s_size = atoi(optarg);
            break;
        case 'r':
            cmd_params->r_size = atoi(optarg);
            break;
        case 'p':
            cmd_params->numa_partition = atoi(optarg);
            break;
        case 't':
            cmd_params->test_bandwidth = atoi(optarg);
            break;
        case 'c':
            cmd_params->col_num = atoi(optarg);
            break;
        case 'l':
            cmd_params->cachetest = atoi(optarg);
            break;
        default:
            break;
        }
    }
    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        std::cout << "non-option arguments: ";
        while (optind < argc)
            std::cout << argv[optind++] << " ";
        std::cout << std::endl;
    }
}
typedef struct arg_vec arg_vec;
struct arg_vec
{
    // uint64_t comlineR;
    int tid;
    pthread_barrier_t *barrier;
    int32_t *vec;

    uint64_t R_num_tuples;
    int32_t *R_key;
    int32_t *R_payload;

    bool R_tuples_euql_1;

    uint64_t S_num_tuples;
    int32_t *S_key;
    int32_t *S_payload;
    int32_t **test_bandwidth_col;
    uint64_t startindex;
    unsigned long long (*test_bandwidth)(int32_t **, uint64_t, uint64_t);
    uint64_t results;
    double run_time;
};
void build_vector_mt(int32_t *vec, int32_t *key, int32_t *payload, uint64_t num_tuples)
{
    uint32_t i;
    // uint64_t sum=0;
    // printf("%d %d\n",rank,comline);
    // fflush(stdout);
    for (i = 0; i < num_tuples; i++)
    {
        vec[key[i] - 1] = payload[i];
    }
    return;
}
int64_t probe_vector(int32_t *vec, int32_t *key, int32_t *payload, uint64_t num_tuples)
{
    uint32_t i;
    int64_t matches;

    // const uint32_t hashmask = ht->hash_mask;
    // const uint32_t skipbits = ht->skip_bits;

    matches = 0;

    for (i = 0; i < num_tuples; i++)
    {

        int32_t idx = key[i];
        if (vec[idx - 1] != -1)
        { // if(rel->tuples[i].key == vec[idx-1]){  //--get predicate vector value
            // matches ++;
            matches += payload[i];
            // index[i]=vec[idx-1];
        }
    }
    // printf("first thread:%lld\n",matches);
    return matches;
}
int64_t probe_vector_0(int32_t *vec, int32_t *key, int32_t *payload, uint64_t num_tuples)
{
    uint32_t i;
    int64_t matches;

    // const uint32_t hashmask = ht->hash_mask;
    // const uint32_t skipbits = ht->skip_bits;

    matches = 0;

    for (i = 0; i < num_tuples; i++)
    {

        int32_t idx = key[i];
        if (idx)
        { // if(rel->tuples[i].key == vec[idx-1]){  //--get predicate vector value
            // matches ++;
            matches += payload[i];
            // index[i]=vec[idx-1];
        }
    }
    // printf("first thread:%lld\n",matches);
    return matches;
}
unsigned long long
test_bandwidth_1(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += test_bandwidth_col[0][i];
    return matches;
}
unsigned long long
test_bandwidth_2(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i]);
    return matches;
}
unsigned long long
test_bandwidth_3(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i]);
    return matches;
}
unsigned long long
test_bandwidth_4(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i]);
    return matches;
}
unsigned long long
test_bandwidth_5(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i] + test_bandwidth_col[4][i]);
    return matches;
}
unsigned long long
test_bandwidth_6(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i] + test_bandwidth_col[4][i] + test_bandwidth_col[5][i]);
    return matches;
}
unsigned long long
test_bandwidth_7(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i] + test_bandwidth_col[4][i] + test_bandwidth_col[5][i] + test_bandwidth_col[6][i]);
    return matches;
}
unsigned long long
test_bandwidth_8(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i] + test_bandwidth_col[4][i] + test_bandwidth_col[5][i] + test_bandwidth_col[6][i] + test_bandwidth_col[7][i]);
    return matches;
}
unsigned long long
test_bandwidth_9(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i] + test_bandwidth_col[4][i] + test_bandwidth_col[5][i] + test_bandwidth_col[6][i] + test_bandwidth_col[7][i] + test_bandwidth_col[8][i]);
    return matches;
}
unsigned long long
test_bandwidth_10(int32_t **test_bandwidth_col, uint64_t num_tuples, uint64_t startindex)
{
    uint64_t i;
    int64_t matches;
    matches = 0;
    for (i = startindex; i < startindex + num_tuples; i++)
        matches += (test_bandwidth_col[0][i] + test_bandwidth_col[1][i] + test_bandwidth_col[2][i] + test_bandwidth_col[3][i] + test_bandwidth_col[4][i] + test_bandwidth_col[5][i] + test_bandwidth_col[6][i] + test_bandwidth_col[7][i] + test_bandwidth_col[8][i] + test_bandwidth_col[9][i]);
    return matches;
}
void *
TESTBANDWIDTH_thread(void *param)
{
    struct timeval start, end;
    int rv;
    arg_vec *args = (arg_vec *)param;
    BARRIER_ARRIVE(args->barrier, rv); // TODO 仿照加同步
    if (args->tid == 0)
        gettimeofday(&start, NULL);

    args->results = args->test_bandwidth(args->test_bandwidth_col, args->S_num_tuples, args->startindex);
    BARRIER_ARRIVE(args->barrier, rv);
    if (args->tid == 0)
    {
        gettimeofday(&end, NULL);
        args->run_time = ((double)1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec) / 1000000;
        // printf("Thread %d: Run time: %.6f ms\n", args->tid, run_time_tmp);
    }

    return nullptr;
}
void *
VECTORJOIN_thread(void *param)
{
    int rv;
    arg_vec *args = (arg_vec *)param;

    BARRIER_ARRIVE(args->barrier, rv); // wqy

    if (!args->R_tuples_euql_1)
    {

        build_vector_mt(args->vec, args->R_key, args->R_payload, args->R_num_tuples);
        BARRIER_ARRIVE(args->barrier, rv);
        args->results = probe_vector(args->vec, args->S_key, args->S_payload, args->S_num_tuples);
        BARRIER_ARRIVE(args->barrier, rv);
    }
    else
    {
        args->results = probe_vector_0(args->vec, args->S_key, args->S_payload, args->S_num_tuples);
    }

    return nullptr;
}
double *VECTORJOIN(int32_t *R_key, int32_t *R_payload, int32_t *S_key, int32_t *S_payload, uint64_t R_num_tuples, uint64_t S_num_tuples)
{
    timeval start1, end1;
    timeval buildp_start, buildp_end;
    clock_t start, end;
    int nthreads = sysconf(_SC_NPROCESSORS_ONLN);
    int32_t numR, numS, numRthr, numSthr;
    int i, rv, j;
    float sum_p_create;
    cpu_set_t set;
    arg_vec args[nthreads];
    pthread_t tid[nthreads];
    pthread_attr_t attr;
    pthread_barrier_t barrier;

    numR = R_num_tuples;
    numS = S_num_tuples;

    numRthr = numR / nthreads;
    numSthr = numS / nthreads;

    int32_t *vec = new int32_t[R_num_tuples];
    memset(vec, 0xff, sizeof(int32_t) * R_num_tuples);
    rv = pthread_barrier_init(&barrier, NULL, nthreads);
    if (rv != 0)
    {
        printf("Couldn't create the barrier\n");
        exit(EXIT_FAILURE);
    }
    pthread_attr_init(&attr);

    start = clock();
    gettimeofday(&start1, NULL);
    for (i = 0; i < nthreads; i++)
    {
        int cpu_idx = i;
        CPU_ZERO(&set);
        CPU_SET(cpu_idx, &set);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

        // args[i].tid = i;
        args[i].vec = vec;
        if (R_num_tuples == 1)
            args[i].R_tuples_euql_1 = true;
        else
            args[i].R_tuples_euql_1 = false;
        // memset(args[i].vec, 0xff, sizeof(int32_t) * R_num_tuples);
        args[i].barrier = &barrier;
        args[i].R_num_tuples = (i == (nthreads - 1)) ? numR : numRthr;
        args[i].R_key = R_key + numRthr * i;
        args[i].R_payload = R_payload + numRthr * i;
        numR -= numRthr;

        args[i].S_num_tuples = (i == (nthreads - 1)) ? numS : numSthr;
        args[i].S_key = S_key + numSthr * i;
        args[i].S_payload = S_payload + numSthr * i;
        numS -= numSthr;

        args[i].results = 0;

        // ToDo start_time
        gettimeofday(&buildp_start, NULL);
        rv = pthread_create(&tid[i], &attr, VECTORJOIN_thread, (void *)&args[i]); // 初始化时间
        // ToDo end_time
        gettimeofday(&buildp_end, NULL);
        // printf("build pthread time:\t");
        sum_p_create = ((double)1000000 * (buildp_end.tv_sec - buildp_start.tv_sec) + buildp_end.tv_usec - buildp_start.tv_usec) / 1000;
        // printf("%lf\n",((double)1000000 * (buildp_end.tv_sec - buildp_start.tv_sec) + buildp_end.tv_usec - buildp_start.tv_usec) / 1000000);    // 单位：s

        if (rv)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rv);
            exit(-1);
        }
    }
    printf("build pthread time:\t");
    printf("%lf\ts\n", sum_p_create);
    uint64_t result = 0;
    for (i = 0; i < nthreads; i++)
    {
        pthread_join(tid[i], NULL);
        /* sum up results */
        result += args[i].results;
    }
    gettimeofday(&end1, NULL);
    end = clock();
    double *run_time;
    run_time = new double[2];
    run_time[1] = end - start;
    run_time[0] = ((double)1000000 * (end1.tv_sec - start1.tv_sec) + end1.tv_usec - start1.tv_usec) / 1000000 - sum_p_create / 1000;
    // std::cout << "VECTORJOIN: " << run_time2 << " ms" << std::endl;
    // std::cout << result << std::endl;
    std::cout << "result = " << result << std::endl;
    return run_time;
}
double *VECTORJOIN_numa(int32_t *R_key, int32_t *R_payload, int32_t **S_key, int32_t **S_payload, uint64_t R_num_tuples, uint64_t *S_num_tuples)
{
    clock_t start, end;
    timeval start1, end1;
    int nthreads = sysconf(_SC_NPROCESSORS_ONLN);
    int32_t numR, numRthr;
    int numa_regions = eth_hashjoin::get_num_numa_regions();
    uint64_t numS_numa[numa_regions];
    int32_t numSthr_numa[numa_regions];
    int i, rv, j;
    cpu_set_t set;
    arg_vec args[nthreads];
    pthread_t tid[nthreads];
    pthread_attr_t attr;
    pthread_barrier_t barrier;

    numR = R_num_tuples;
    // numS = S_num_tuples;
    for (i = 0; i < numa_regions; i++)
        numS_numa[i] = S_num_tuples[i];
    // numS_numa = S_num_tuples;
    numRthr = numR / nthreads;
    int nthreads_numa[numa_regions];
    int nthreadsPnuma = nthreads / numa_regions;
    for (i = 0; i < numa_regions; i++)
    {
        nthreads_numa[i] = (i == (numa_regions - 1)) ? (nthreads - nthreadsPnuma * i) : nthreadsPnuma;
    }
    // numSthr = numS / nthreads;
    for (i = 0; i < numa_regions; i++)
    {
        numSthr_numa[i] = numS_numa[i] / nthreads_numa[i];
    }

    int32_t *vec = new int32_t[R_num_tuples];
    memset(vec, 0xff, sizeof(int32_t) * R_num_tuples);
    rv = pthread_barrier_init(&barrier, NULL, nthreads);
    if (rv != 0)
    {
        printf("Couldn't create the barrier\n");
        exit(EXIT_FAILURE);
    }
    pthread_attr_init(&attr);
    // int num_nthreads[numa_regions] = {0};
    int num_nthreads[numa_regions];
    memset(num_nthreads, 0, sizeof(numa_regions * 4));
    start = clock();
    gettimeofday(&start1, NULL);
    for (i = 0; i < nthreads; i++)
    {
        int cpu_idx = i;
        int numa_id = eth_hashjoin::get_numa_id(cpu_idx);
        // std::cout << i << " " << numa_id << std::endl;
        CPU_ZERO(&set);
        CPU_SET(cpu_idx, &set);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

        // args[i].tid = i;
        args[i].vec = vec;
        if (R_num_tuples == 1)
            args[i].R_tuples_euql_1 = true;
        else
            args[i].R_tuples_euql_1 = false;
        // memset(args[i].vec, 0xff, sizeof(int32_t) * R_num_tuples);
        args[i].barrier = &barrier;
        args[i].R_num_tuples = (i == (nthreads - 1)) ? numR : numRthr;
        args[i].R_key = R_key + numRthr * i;
        args[i].R_payload = R_payload + numRthr * i;
        numR -= numRthr;

        args[i].S_num_tuples = (num_nthreads[numa_id] == (nthreads_numa[numa_id] - 1)) ? numS_numa[numa_id] : numSthr_numa[numa_id];
        args[i].S_key = S_key[numa_id] + numSthr_numa[numa_id] * num_nthreads[numa_id];
        args[i].S_payload = S_payload[numa_id] + numSthr_numa[numa_id] * num_nthreads[numa_id];
        // numS -= numSthr;
        numS_numa[numa_id] -= numSthr_numa[numa_id];
        num_nthreads[numa_id]++;
        args[i].results = 0;

        rv = pthread_create(&tid[i], &attr, VECTORJOIN_thread, (void *)&args[i]);
        if (rv)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rv);
            exit(-1);
        }
    }
    uint64_t result = 0;
    for (i = 0; i < nthreads; i++)
    {
        pthread_join(tid[i], NULL);
        /* sum up results */
        result += args[i].results;
    }
    gettimeofday(&end1, NULL);
    end = clock();
    double *run_time;
    run_time = new double[2];
    run_time[1] = end - start;
    run_time[0] = ((double)1000000 * (end1.tv_sec - start1.tv_sec) + end1.tv_usec - start1.tv_usec) / 1000000;
    // double run_time5 = end - start;
    // fprintf(fp, "%lld\t%lld\t%lf\n", R_num_tuples, S_num_tuples, run_time5);
    std::cout << "result = " << result << std::endl;
    return run_time;
}
double *Test_Bandwidth(int32_t **S_key, uint64_t num_tuples, int8_t col_num)
{
    // timeval start1, end1;
    clock_t start, end;
    int nthreads = sysconf(_SC_NPROCESSORS_ONLN);
    int32_t numS, numSthr;
    int i, rv, j;
    cpu_set_t set;
    arg_vec args[nthreads];
    pthread_t tid[nthreads];
    pthread_attr_t attr;
    pthread_barrier_t barrier;
    numS = num_tuples;
    numSthr = numS / nthreads;

    // std::cout << 111 << std::endl;
    rv = pthread_barrier_init(&barrier, NULL, nthreads);
    if (rv != 0)
    {
        printf("Couldn't create the barrier\n");
        exit(EXIT_FAILURE);
    }
    pthread_attr_init(&attr);
    start = clock();
    // gettimeofday(&start1, NULL);
    for (i = 0; i < nthreads; i++)
    {
        int cpu_idx = i;
        CPU_ZERO(&set);
        CPU_SET(cpu_idx, &set);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
        args[i].barrier = &barrier;
        args[i].S_num_tuples = (i == (nthreads - 1)) ? numS : numSthr;
        args[i].test_bandwidth_col = S_key;
        args[i].startindex = i * numSthr;
        // args[i].test_bandwidth = test_bandwidth_1;
        switch (col_num)
        {
        case 1:
            args[i].test_bandwidth = test_bandwidth_1;
            break;
        case 2:
            args[i].test_bandwidth = test_bandwidth_2;
            break;
        case 3:
            args[i].test_bandwidth = test_bandwidth_3;
            break;
        case 4:
            args[i].test_bandwidth = test_bandwidth_4;
            break;
        case 5:
            args[i].test_bandwidth = test_bandwidth_5;
            break;
        case 6:
            args[i].test_bandwidth = test_bandwidth_6;
            break;
        case 7:
            args[i].test_bandwidth = test_bandwidth_7;
            break;
        case 8:
            args[i].test_bandwidth = test_bandwidth_8;
            break;
        case 9:
            args[i].test_bandwidth = test_bandwidth_9;
            break;
        case 10:
            args[i].test_bandwidth = test_bandwidth_10;
            break;
        default:
            break;
        }
        numS -= numSthr;
        args[i].results = 0;

        rv = pthread_create(&tid[i], &attr, TESTBANDWIDTH_thread, (void *)&args[i]);
        if (rv)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rv);
            exit(-1);
        }
    }
    uint64_t result = 0;
    for (i = 0; i < nthreads; i++)
    {
        pthread_join(tid[i], NULL);
        /* sum up results */
        result += args[i].results;
    }
    // gettimeofday(&end1, NULL);
    end = clock();
    double *run_time;
    run_time = new double[2];
    run_time[1] = end - start;
    run_time[0] = args[0].run_time;

    std::cout << result << std::endl;
    return run_time;
}
double Test_Bandwidth_Numa(int32_t *S_key[][10], uint64_t *S_num_tuples, int col_num)
{
    timeval start, end;
    // timeval start1, end1;
    int nthreads = sysconf(_SC_NPROCESSORS_ONLN);
    // int32_t numR, numRthr;
    int numa_regions = eth_hashjoin::get_num_numa_regions();
    uint64_t numS_numa[numa_regions];
    int32_t numSthr_numa[numa_regions];
    int i, rv, j;
    cpu_set_t set;
    arg_vec args[nthreads];
    pthread_t tid[nthreads];
    pthread_attr_t attr;
    pthread_barrier_t barrier;

    // numR = R_num_tuples;
    // numS = S_num_tuples;
    for (i = 0; i < numa_regions; i++)
        numS_numa[i] = S_num_tuples[i];

    // numRthr = numR / nthreads;
    int nthreads_numa[numa_regions];
    int nthreadsPnuma = nthreads / numa_regions;
    for (i = 0; i < numa_regions; i++)
    {
        nthreads_numa[i] = (i == (numa_regions - 1)) ? (nthreads - nthreadsPnuma * i) : nthreadsPnuma;
    }
    // numSthr = numS / nthreads;
    for (i = 0; i < numa_regions; i++)
    {
        numSthr_numa[i] = numS_numa[i] / nthreads_numa[i];
    }

    // int32_t *vec = new int32_t[R_num_tuples];
    // memset(vec, 0xff, sizeof(int32_t) * R_num_tuples);
    // unsigned long long (*test_bandwidth)(int32_t **, uint64_t, uint64_t);

    rv = pthread_barrier_init(&barrier, NULL, nthreads);
    // double create_time = 0.0;
    if (rv != 0)
    {
        printf("Couldn't create the barrier\n");
        exit(EXIT_FAILURE);
    }
    pthread_attr_init(&attr);
    gettimeofday(&start, NULL);
    // int num_nthreads[numa_regions] = {0};
    int num_nthreads[numa_regions];
    memset(num_nthreads, 0, sizeof(numa_regions * 4));
    for (i = 0; i < nthreads; i++)
    {
        int cpu_idx = i;
        int numa_id = eth_hashjoin::get_numa_id(cpu_idx);
        // std::cout << i << " " << numa_id << std::endl;
        CPU_ZERO(&set);
        CPU_SET(cpu_idx, &set);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

        args[i].tid = i;
        // args[i].vec = vec;
        // // memset(args[i].vec, 0xff, sizeof(int32_t) * R_num_tuples);
        args[i].barrier = &barrier;
        // args[i].R_num_tuples = (i == (nthreads - 1)) ? numR : numRthr;
        // args[i].R_key = R_key + numRthr * i;
        // args[i].R_payload = R_payload + numRthr * i;
        // numR -= numRthr;
        switch (col_num)
        {
        case 1:
            args[i].test_bandwidth = test_bandwidth_1;
            break;
        case 2:
            args[i].test_bandwidth = test_bandwidth_2;
            break;
        case 3:
            args[i].test_bandwidth = test_bandwidth_3;
            break;
        case 4:
            args[i].test_bandwidth = test_bandwidth_4;
            break;
        case 5:
            args[i].test_bandwidth = test_bandwidth_5;
            break;
        case 6:
            args[i].test_bandwidth = test_bandwidth_6;
            break;
        case 7:
            args[i].test_bandwidth = test_bandwidth_7;
            break;
        case 8:
            args[i].test_bandwidth = test_bandwidth_8;
            break;
        case 9:
            args[i].test_bandwidth = test_bandwidth_9;
            break;
        case 10:
            args[i].test_bandwidth = test_bandwidth_10;
            break;
        default:
            break;
        }
        args[i].S_num_tuples = (num_nthreads[numa_id] == (nthreads_numa[numa_id] - 1)) ? numS_numa[numa_id] : numSthr_numa[numa_id];
        args[i].test_bandwidth_col = S_key[numa_id];
        args[i].startindex = numSthr_numa[numa_id] * num_nthreads[numa_id];
        // args[i].S_payload = S_payload[numa_id] + numSthr_numa[numa_id] * num_nthreads[numa_id];
        // numS -= numSthr;
        numS_numa[numa_id] -= numSthr_numa[numa_id];
        num_nthreads[numa_id]++;
        args[i].results = 0;
        // gettimeofday(&start1, NULL);
        rv = pthread_create(&tid[i], &attr, TESTBANDWIDTH_thread, (void *)&args[i]);
        // gettimeofday(&end1, NULL);
        // double run_time_tmp = ((double)1000000 * (end1.tv_sec - start1.tv_sec) + end1.tv_usec - start1.tv_usec) / 1000;
        // create_time += run_time_tmp;
        if (rv)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rv);
            exit(-1);
        }
    }
    // printf("Run time: %.3f ms\n", create_time);
    uint64_t result = 0;
    for (i = 0; i < nthreads; i++)
    {
        pthread_join(tid[i], NULL);
        /* sum up results */
        result += args[i].results;
    }
    gettimeofday(&end, NULL);
    double run_time5 = ((double)1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec) / 1000000;
    // fprintf(fp, "%lld\t%lld\t%lf\n", 0, 1, run_time5);
    std::cout << result << std::endl;
    return run_time5;
}
int main(int argc, char **argv)
{
    int nthreads = 32;
    unsigned long long s_size = pow(2, 30);
    // unsigned long long s_size = pow(2, 10);
    unsigned long long r_size = 32;

    FILE *fpans = fopen("./test_vecjoin_cpu_xlsx.txt", "w"); // cpu连接测试xlsx格式
    fprintf(fpans, "%s\t%s\t%s\t%s\t%s\t%s\n", "R_size", "s_size", "CPU", "NUMA", "Metric", "Values");

    string cpumodel;
#if defined __x86_64__
    cpumodel = getCpuInfo();
#elif defined __aarch64__
    cpumodel = "NULL";
#endif

    string numastat;

    string metricstat;

    int numa_regions = eth_hashjoin::get_num_numa_regions();

    // to change the parameter
    param_join_t cmd_params;
    cmd_params.nthreads = 32;
    cmd_params.s_size = pow(2, 30);
    // cmd_params.s_size = pow(2, 10);
    cmd_params.r_size = 32;
    // cmd_params.r_size = 8;

    cmd_params.numa_partition = 0;
    cmd_params.test_bandwidth = 0; // 测试带宽 --test_bandwidth=1
    cmd_params.col_num = 10;
    parse_args(argc, argv, &cmd_params);

    // printf("help val=%d\n\n",cmd_params.help);
    if (help_flag)
    {
        // printf("*********************\n");
        return 0;
    }

    if (!cmd_params.test_bandwidth && !cmd_params.numa_partition && !cmd_params.cachetest)
    {
        printf("IN NO_BANDWIDTH & NO_NUMA\n");
        FILE *fp = fopen("./test_vecjoin_cpu.txt", "w");
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\n", "R_size", "s_size", "time(s)", "tuples/s", "CPU_Clock", "tuple/CPU Clock");

        numastat = "NUMA-oblivious"; // NUMA-conscious

        for (int i = 0; i <= 30; i++)
        {
            double run_time[5] = {0.0};
            double run_clock[5] = {0.0};
            cmd_params.r_size = pow(2, i);
            relation_t *R, *S;
            R = new relation_t;
            R->key = new int32_t[cmd_params.r_size];
            R->payload = new int32_t[cmd_params.r_size];
            R->num_tuples = new uint64_t[1];
            *(R->num_tuples) = cmd_params.r_size;

            S = new relation_t;
            S->key = new int32_t[s_size];
            S->payload = new int32_t[s_size];
            S->num_tuples = new uint64_t[1];
            *(S->num_tuples) = cmd_params.s_size;

            std::cout << "Testing in progress: vectorjoin with the rows of table S: " << cmd_params.s_size << " and the table of R: " << cmd_params.r_size << std::endl;
            if (i != 0)
            {
                std::cout << "Generating R table data: " << cmd_params.r_size << " rows" << std::endl;
                gen_data(cmd_params.r_size, cmd_params.r_size, R, cmd_params.nthreads);
            }

            std::cout << "Generating S table data: " << cmd_params.s_size << " rows" << std::endl;
            int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
            gen_data(cmd_params.s_size, cmd_params.r_size, S, max_cores);
            // for (int j = 0; j < 5; j++)
            for (int j = 0; j < 5; j++)
            {
                double *run_time_tmp = VECTORJOIN(R->key, R->payload, S->key, S->payload, cmd_params.r_size, S->num_tuples[0]);
                run_clock[j] = run_time_tmp[1];
                run_time[j] = run_time_tmp[0];
            }

            delete[] S->key;
            delete[] S->payload;
            delete[] S->num_tuples;
            delete[] R->key;
            delete[] R->payload;
            delete[] R->num_tuples;
            delete S;
            delete R;
            // std::cout << "Run time: " << run_time[j] / CLOCKS_PER_SEC << " seconds" << std::endl;
            double min_run_time = *std::min_element(run_time, run_time + 5);
            double min_run_clock = *std::min_element(run_clock, run_clock + 5);

            metricstat = "Times(ms)"; // 不包括线程创建时间
            fprintf(fpans, "%d\t%lld\t%s\t%s\t%s\t%lf\n", i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), min_run_time * 1000);
            metricstat = "Throughput(GT/s)";
            fprintf(fpans, "%d\t%d\t%s\t%s\t%s\t%lf\n", i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), 1 / min_run_time);
            metricstat = "Cycles/Tuple";
            fprintf(fpans, "%d\t%lld\t%s\t%s\t%s\t%lf\n", i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), s_size / min_run_clock );

            /*
             * Metric:
             *   Throughput(GT/s):tuple/time(GT/s)
             *   Cycles/Tuple:CPU_clock/tuple
             *
             * 注：这里的
             *   tuple 单位为GT
             *   time 单位为s
             */
            fprintf(fp, "%d\t%lld\t%lf\t%lf\t%lf\t%lf\n", cmd_params.r_size, s_size, min_run_time, 1 / min_run_time, min_run_clock, min_run_clock / s_size);
            // R_size	s_size	time(s)	tuples/s	CPU Clock	tuple/CPU Clock
            if (i == 0)
                i += 4;
        }
    }
    if (cmd_params.test_bandwidth && !cmd_params.numa_partition && !cmd_params.cachetest)
    {
        printf("IN BANDWIDTH & NO_NUMA\n");
        FILE *fp = fopen("./test_bandwidth_cpu.txt", "w");
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", "col_num", "s_size", "time(s)", "tuples/s", "bandwidth(GB/ms)");
        double run_time[5] = {0.0};
        double run_clock[5] = {0.0};
        double bandwidth[10] = {0.0};
        int32_t *S_key[cmd_params.col_num];
        for (int i = 0; i < cmd_params.col_num; i++)
            S_key[i] = new int32_t[cmd_params.s_size];
        for (uint64_t i = 0; i < cmd_params.s_size; i++)
        {
            S_key[0][i] = 1;
            S_key[1][i] = 1;
            S_key[2][i] = 1;
            S_key[3][i] = 1;
            S_key[4][i] = 1;
            S_key[5][i] = 1;
            S_key[6][i] = 1;
            S_key[7][i] = 1;
            S_key[8][i] = 1;
            S_key[9][i] = 1;
        }
        for (int j = 1; j <= cmd_params.col_num; j++)
        {
            for (int i = 0; i < 5; i++)
            {
                double *run_time_tmp = Test_Bandwidth(S_key, cmd_params.s_size, j);
                run_clock[i] = run_time_tmp[1];
                run_time[i] = run_time_tmp[0];
            }

            double min_run_time = *std::min_element(run_time, run_time + 5);
            bandwidth[j] = j * 4 / min_run_time;
        }
        double max_bandwidth = 0.0;
        int best_colnum = 0;
        for (int j = 1; j <= cmd_params.col_num; j++)
            if (max_bandwidth < bandwidth[j])
            {
                max_bandwidth = bandwidth[j];
                best_colnum = j;
            }

        fprintf(fp, "%lld\t%d\t%lf\t%lf\t%lf\n", best_colnum, s_size, (double)best_colnum * 4 / max_bandwidth, max_bandwidth / (best_colnum * 4), max_bandwidth);
    }
    if (!cmd_params.test_bandwidth && cmd_params.numa_partition && !cmd_params.cachetest)
    {
        printf("IN NO_BANDWIDTH & NUMA\n");
        FILE *fp = fopen("./test_vecjoin_numa_cpu.txt", "w");
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\t%s\n", "R_size", "s_size", "time(s)", "tuples/s", "CPU Clock", "CPU Clock /CPU Clock");
        FILE *fpans_numa = fopen("./test_vecjoin_cpu_numa_xlsx.txt", "w"); // cpu连接测试xlsx格式
        fprintf(fpans_numa, "%s\t%s\t%s\t%s\t%s\t%s\n", "R_size", "s_size", "CPU", "NUMA", "Metric", "Values");
        numastat = "NUMA-conscious"; // NUMA-conscious

        double run_time[5] = {0.0};
        double run_clock[5] = {0.0};
        for (int i = 0; i <= 30; i++)
        {
            cmd_params.r_size = pow(2, i);
            double run_time[5] = {0.0};
            relation_t *R;
            relation_numa_t *S;
            R = new relation_t;
            R->key = new int32_t[cmd_params.r_size];
            R->payload = new int32_t[cmd_params.r_size];
            R->num_tuples = new uint64_t[1];
            *(R->num_tuples) = cmd_params.r_size;

            S = new relation_numa_t;

            uint64_t num_numa[numa_regions];
            int num_size_per_numa = s_size / numa_regions;
            for (int j = 0; j < numa_regions; j++)
                num_numa[j] = (j == numa_regions - 1) ? s_size - num_size_per_numa * (numa_regions - 1) : num_size_per_numa;
            // std::cout << num_numa[0] << std::endl;
            for (int j = 0; j < numa_regions; j++)
            {
                eth_hashjoin::bind_numa(j);
                S->key[j] = (int *)numa_alloc_onnode(num_numa[j] * sizeof(int), j);
                S->payload[j] = (int *)numa_alloc_onnode(num_numa[j] * sizeof(int), j);
            }

            std::cout << "Testing in progress: vectorjoin with the rows of table S: " << cmd_params.s_size << " and the table of R: " << cmd_params.r_size << std::endl;
            if (i != 0)
            {
                std::cout << "Generating R table data: " << cmd_params.r_size << " rows" << std::endl;
                gen_data(cmd_params.r_size, cmd_params.r_size, R, cmd_params.nthreads);
            }
            std::cout << "Generating S table data: " << cmd_params.s_size << " rows" << std::endl;
            int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
            gen_data_numa(num_numa, cmd_params.r_size, S, max_cores);

            // int32_t *S_key_numa[numa_regions];
            // int32_t *S_payload_numa[numa_regions];

            std::cout << num_numa[0] << " " << num_numa[1] << std::endl;
            for (int j = 0; j < 5; j++)
            {
                double *run_time_tmp = VECTORJOIN_numa(R->key, R->payload, S->key, S->payload, cmd_params.r_size, num_numa);
                run_clock[j] = run_time_tmp[1];
                run_time[j] = run_time_tmp[0];
            }

            for (int j = 0; j < numa_regions; j++)
            {
                // eth_hashjoin::bind_numa(j);
                numa_free(S->key[j], num_numa[j] * sizeof(int));
                numa_free(S->payload[j], num_numa[j] * sizeof(int));
                // S->key[j] = (int *)numa_alloc_onnode(num_numa[j] * sizeof(int), j);
                // S->payload[j] = (int *)numa_alloc_onnode(num_numa[j] * sizeof(int), j);
            }
            // delete[] S->key;
            // delete[] S->payload;
            // delete[] S->num_tuples;
            delete[] R->key;
            delete[] R->payload;
            delete[] R->num_tuples;
            delete S;
            delete R;

            double min_run_time = *std::min_element(run_time, run_time + 5);
            double min_run_clock = *std::min_element(run_clock, run_clock + 5);

            metricstat = "Times(ms)";
            fprintf(fpans_numa, "%d\t%d\t%s\t%s\t%s\t%lf\n", i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), min_run_time * 1000);
            metricstat = "Throughput(GT/s)";
            fprintf(fpans_numa, "%d\t%d\t%s\t%s\t%s\t%lf\n", i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), 1 / min_run_time);
            metricstat = "Cycles/Tuple";
            fprintf(fpans_numa, "%d\t%d\t%s\t%s\t%s\t%lf\n", i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), s_size / min_run_clock);

            fprintf(fp, "%d\t%d\t%lf\t%lf\t%lf\t%lf\n", cmd_params.r_size, s_size, min_run_time, 1 / min_run_time, min_run_clock, min_run_clock / s_size);
            if (i == 0)
                i += 4;
        }
    }
    if (cmd_params.test_bandwidth && cmd_params.numa_partition && !cmd_params.cachetest)
    {
        printf("IN BANDWIDTH & NUMA\n");
        FILE *fp = fopen("./test_bandwidth_numa_cpu.txt", "w");
        fprintf(fp, "%s\t%s\t%s\t%s\t%s\n", "col_num", "s_size", "time(s)", "tuples/s", "bandwidth(GB/s)");
        double run_time[5] = {0.0};
        double bandwidth[10] = {0.0};
        uint64_t num_numa[numa_regions];
        int num_size_per_numa = s_size / numa_regions;
        for (int j = 0; j < numa_regions; j++)
            num_numa[j] = (j == numa_regions - 1) ? s_size - num_size_per_numa * (numa_regions - 1) : num_size_per_numa;
        int32_t *S_key[numa_regions][10];
        for (int j = 0; j < numa_regions; j++)
        {
            eth_hashjoin::bind_numa(j);
            S_key[j][0] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][1] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][2] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][3] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][4] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][5] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][6] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][7] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][8] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            S_key[j][9] = (int32_t *)numa_alloc_onnode(num_numa[j] * sizeof(int32_t), j);
            for (uint64_t i = 0; i < num_numa[j]; i++)
            {
                S_key[j][0][i] = 1;
                S_key[j][1][i] = 1;
                S_key[j][2][i] = 1;
                S_key[j][3][i] = 1;
                S_key[j][4][i] = 1;
                S_key[j][5][i] = 1;
                S_key[j][6][i] = 1;
                S_key[j][7][i] = 1;
                S_key[j][8][i] = 1;
                S_key[j][9][i] = 1;
            }
        }
        for (int j = 1; j <= cmd_params.col_num; j++)
        {
            for (int i = 0; i < 5; i++)
                run_time[i] = Test_Bandwidth_Numa(S_key, num_numa, j);
            double min_run_time = *std::min_element(run_time, run_time + 5);
            bandwidth[j] = j * 4 / min_run_time;
        }
        double max_bandwidth = 0.0;
        int best_colnum = 0;
        for (int j = 1; j <= cmd_params.col_num; j++)
            if (max_bandwidth < bandwidth[j])
            {
                max_bandwidth = bandwidth[j];
                best_colnum = j;
            }

        fprintf(fp, "%lld\t%lld\t%lf\t%lf\t%lf\n", best_colnum, s_size, (double)best_colnum * 4 / max_bandwidth, max_bandwidth / (best_colnum * 4), max_bandwidth);
    }

    if (cmd_params.cachetest)
    {
        // get cache size
        int l1dsize, l2size, l3size;
        // int32_t *key; int32_t *payload; 一行访问的数据为64b=8B
        l1dsize = getCacheInfo(0) * 1024 / 8; // 恰好装满l1 cache时需要多少row
        l2size = getCacheInfo(2) * 1024 / 8;  // 恰好装满l2 cache时需要多少row
        // TODO if arm l3=l3/Die
        #if defined __x86_64__
            l3size = getCacheInfo(3) * 1024 / 8; // 恰好装满l3 cache时需要多少row
        #elif defined __aarch64__
            l3size = getCacheInfo(3) * 1024 / 8 / (numa_max_node()+1); // 恰好装满l3 cache时需要多少row
        #endif
        // l3size = getCacheInfo(3) * 1024 / 8; // 恰好装满l3 cache时需要多少row
        printf("l3size=%d\n\n",l3size);
        // get r rows
        // int r_l1_size=l1dsize/16;    // 100%
        int r_l1_size, r_l2_size, r_l3_size, r_mem_size; // 100%

        std::time_t ts;

        FILE *fpcache_1 = fopen("./test_vecjoin_cpu_cache_xlsx_l1.txt", "w"); // cache_size_test 连接测试xlsx格式
        fprintf(fpcache_1, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "time", "R_size", "s_size", "CPU", "NUMA", "Metric", "Values", "cache");
        FILE *fpcache_2 = fopen("./test_vecjoin_cpu_cache_xlsx_l2.txt", "w"); // cache_size_test 连接测试xlsx格式
        fprintf(fpcache_2, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "time", "R_size", "s_size", "CPU", "NUMA", "Metric", "Values", "cache");
        FILE *fpcache_3 = fopen("./test_vecjoin_cpu_cache_xlsx_l3.txt", "w"); // cache_size_test 连接测试xlsx格式
        fprintf(fpcache_3, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "time", "R_size", "s_size", "CPU", "NUMA", "Metric", "Values", "cache");
        FILE *fpcache_mem = fopen("./test_vecjoin_cpu_cache_xlsx_mem.txt", "w"); // cache_size_test 连接测试xlsx格式
        fprintf(fpcache_mem, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", "time", "R_size", "s_size", "CPU", "NUMA", "Metric", "Values", "cache");

        // numastat="NUMA-oblivious";    // NUMA-conscious
        /*---------- get L1 Cache test ----------*/
        cout << "L1 cache test start\n";
        for (int i = 1; i <= 30; i++) // 10:100%
        {
            r_l1_size = l1dsize * i / 10;
            float rate = i * 1.0 / 10;
            // cout<<"r_rows="<<r_l1_size<<endl;
            double run_time[5] = {0.0};
            double run_clock[5] = {0.0};
            // cmd_params.r_size = pow(2, i);
            relation_t *R, *S;
            R = new relation_t;
            R->key = new int32_t[r_l1_size];
            R->payload = new int32_t[r_l1_size];
            R->num_tuples = new uint64_t[1];
            *(R->num_tuples) = r_l1_size;

            S = new relation_t;
            S->key = new int32_t[cmd_params.s_size];
            S->payload = new int32_t[cmd_params.s_size];
            S->num_tuples = new uint64_t[1];
            *(S->num_tuples) = cmd_params.s_size;

            std::cout << "Testing in progress: vectorjoin with the rows of table S: " << cmd_params.s_size << " and the table of R: " << r_l1_size << std::endl;
            if (i != 0)
            {
                std::cout << "Generating R table data: " << r_l1_size << " rows" << std::endl;
                gen_data(r_l1_size, r_l1_size, R, cmd_params.nthreads);
            }

            std::cout << "Generating S table data: " << cmd_params.s_size << " rows" << std::endl;
            int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
            gen_data(cmd_params.s_size, r_l1_size, S, max_cores);

            ts = std::time(nullptr);
            // printf("ts=%d\n",ts);
            // for (int j = 0; j < 5; j++)
            for (int j = 0; j < 5; j++)
            {
                double *run_time_tmp = VECTORJOIN(R->key, R->payload, S->key, S->payload, r_l1_size, S->num_tuples[0]);
                run_clock[j] = run_time_tmp[1];
                run_time[j] = run_time_tmp[0];
            }
            delete[] S->key;
            delete[] S->payload;
            delete[] S->num_tuples;
            delete[] R->key;
            delete[] R->payload;
            delete[] R->num_tuples;
            delete S;
            delete R;

            // std::cout << "Run time: " << run_time[j] / CLOCKS_PER_SEC << " seconds" << std::endl;
            double min_run_time = *std::min_element(run_time, run_time + 5);
            double min_run_clock = *std::min_element(run_clock, run_clock + 5);

            // 下面可要
            metricstat = "Times(ms)";
            fprintf(fpcache_1, "%d\t%f\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), min_run_time * 1000, "L1");
            metricstat = "Throughput(GT/s)";
            fprintf(fpcache_1, "%d\t%f\t%d\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), 1 / min_run_time, "L1");
            metricstat = "Cycles/Tuple";
            fprintf(fpcache_1, "%d\t%f\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), s_size / min_run_clock, "L1");
        }
        fclose(fpcache_1);

        /*---------- get L2 Cache test ----------*/
        cout << "\nL2 cache test start\n";
        for (int i = 1; i <= 30; i++) // 10:100%
        {
            r_l2_size = l2size * i / 10;
            float rate = i * 1.0 / 10;
            // cout<<"r_rows="<<r_l2_size<<endl;
            double run_time[5] = {0.0};
            double run_clock[5] = {0.0};
            // cmd_params.r_size = pow(2, i);
            relation_t *R, *S;
            R = new relation_t;
            R->key = new int32_t[r_l2_size];
            R->payload = new int32_t[r_l2_size];
            R->num_tuples = new uint64_t[1];
            *(R->num_tuples) = r_l2_size;

            S = new relation_t;
            S->key = new int32_t[cmd_params.s_size];
            S->payload = new int32_t[cmd_params.s_size];
            S->num_tuples = new uint64_t[1];
            *(S->num_tuples) = cmd_params.s_size;

            std::cout << "Testing in progress: vectorjoin with the rows of table S: " << cmd_params.s_size << " and the table of R: " << r_l2_size << std::endl;
            if (i != 0)
            {
                std::cout << "Generating R table data: " << r_l2_size << " rows" << std::endl;
                gen_data(r_l2_size, r_l2_size, R, cmd_params.nthreads);
            }

            std::cout << "Generating S table data: " << cmd_params.s_size << " rows" << std::endl;
            int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
            gen_data(cmd_params.s_size, r_l2_size, S, max_cores);

            ts = std::time(nullptr);
            // for (int j = 0; j < 5; j++)
            for (int j = 0; j < 5; j++)
            {
                double *run_time_tmp = VECTORJOIN(R->key, R->payload, S->key, S->payload, r_l2_size, S->num_tuples[0]);
                run_clock[j] = run_time_tmp[1];
                run_time[j] = run_time_tmp[0];
            }

            delete[] S->key;
            delete[] S->payload;
            delete[] S->num_tuples;
            delete[] R->key;
            delete[] R->payload;
            delete[] R->num_tuples;
            delete S;
            delete R;

            // std::cout << "Run time: " << run_time[j] / CLOCKS_PER_SEC << " seconds" << std::endl;
            double min_run_time = *std::min_element(run_time, run_time + 5);
            double min_run_clock = *std::min_element(run_clock, run_clock + 5);

            // 下面可要
            metricstat = "Times(ms)";
            fprintf(fpcache_2, "%d\t%f\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), min_run_time * 1000, "L2");
            metricstat = "Throughput(GT/s)";
            fprintf(fpcache_2, "%d\t%f\t%d\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), 1 / min_run_time, "L2");
            metricstat = "Cycles/Tuple";
            fprintf(fpcache_2, "%d\t%f\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), s_size / min_run_clock, "L2");
        }
        fclose(fpcache_2);

        /*---------- get L3 Cache test ----------*/
        cout << "\nL3 cache test start\n";
        for (int i = 1; i <= 30; i++) // 10:100%
        {
            r_l3_size = l3size * i / 10;
            float rate = i * 1.0 / 10;
            // cout<<"r_rows="<<r_l3_size<<endl;
            double run_time[5] = {0.0};
            double run_clock[5] = {0.0};
            // cmd_params.r_size = pow(2, i);
            relation_t *R, *S;
            R = new relation_t;
            R->key = new int32_t[r_l3_size];
            R->payload = new int32_t[r_l3_size];
            R->num_tuples = new uint64_t[1];
            *(R->num_tuples) = r_l3_size;

            S = new relation_t;
            S->key = new int32_t[cmd_params.s_size];
            S->payload = new int32_t[cmd_params.s_size];
            S->num_tuples = new uint64_t[1];
            *(S->num_tuples) = cmd_params.s_size;

            std::cout << "Testing in progress: vectorjoin with the rows of table S: " << cmd_params.s_size << " and the table of R: " << r_l3_size << std::endl;
            if (i != 0)
            {
                std::cout << "Generating R table data: " << r_l3_size << " rows" << std::endl;
                gen_data(r_l3_size, r_l3_size, R, cmd_params.nthreads);
            }

            std::cout << "Generating S table data: " << cmd_params.s_size << " rows" << std::endl;
            int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
            gen_data(cmd_params.s_size, r_l3_size, S, max_cores);

            ts = std::time(nullptr);
            // for (int j = 0; j < 5; j++)
            for (int j = 0; j < 5; j++)
            {
                double *run_time_tmp = VECTORJOIN(R->key, R->payload, S->key, S->payload, r_l3_size, S->num_tuples[0]);
                run_clock[j] = run_time_tmp[1];
                run_time[j] = run_time_tmp[0];
            }

            delete[] S->key;
            delete[] S->payload;
            delete[] S->num_tuples;
            delete[] R->key;
            delete[] R->payload;
            delete[] R->num_tuples;
            delete S;
            delete R;

            // std::cout << "Run time: " << run_time[j] / CLOCKS_PER_SEC << " seconds" << std::endl;
            double min_run_time = *std::min_element(run_time, run_time + 5);
            double min_run_clock = *std::min_element(run_clock, run_clock + 5);

            // 下面可要
            metricstat = "Times(ms)";
            fprintf(fpcache_3, "%d\t%f\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), min_run_time * 1000, "L3");
            metricstat = "Throughput(GT/s)";
            fprintf(fpcache_3, "%d\t%f\t%d\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), 1 / min_run_time, "L3");
            metricstat = "Cycles/Tuple";
            fprintf(fpcache_3, "%d\t%f\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, rate, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), s_size / min_run_clock, "L3");
        }
        fclose(fpcache_3);

        /*---------- get Memory test ----------*/
        cout << "\nMemory test\n";
        for (int i = 1; i <= 5; i++) // 10:100%
        {
            r_mem_size = l3size * i;
            // float rate=i*1.0/10;
            // cout<<"r_rows="<<r_mem_size<<endl;
            double run_time[5] = {0.0};
            double run_clock[5] = {0.0};
            // cmd_params.r_size = pow(2, i);
            relation_t *R, *S;
            R = new relation_t;
            R->key = new int32_t[r_mem_size];
            R->payload = new int32_t[r_mem_size];
            R->num_tuples = new uint64_t[1];
            *(R->num_tuples) = r_mem_size;

            S = new relation_t;
            S->key = new int32_t[cmd_params.s_size];
            S->payload = new int32_t[cmd_params.s_size];
            S->num_tuples = new uint64_t[1];
            *(S->num_tuples) = cmd_params.s_size;

            std::cout << "Testing in progress: vectorjoin with the rows of table S: " << cmd_params.s_size << " and the table of R: " << r_mem_size << std::endl;
            if (i != 0)
            {
                std::cout << "Generating R table data: " << r_mem_size << " rows" << std::endl;
                gen_data(r_mem_size, r_mem_size, R, cmd_params.nthreads);
            }

            std::cout << "Generating S table data: " << cmd_params.s_size << " rows" << std::endl;
            int max_cores = sysconf(_SC_NPROCESSORS_ONLN);
            gen_data(cmd_params.s_size, r_mem_size, S, max_cores);

            ts = std::time(nullptr);
            // for (int j = 0; j < 5; j++)
            for (int j = 0; j < 5; j++)
            {
                double *run_time_tmp = VECTORJOIN(R->key, R->payload, S->key, S->payload, r_mem_size, S->num_tuples[0]);
                run_clock[j] = run_time_tmp[1];
                run_time[j] = run_time_tmp[0];
            }

            delete[] S->key;
            delete[] S->payload;
            delete[] S->num_tuples;
            delete[] R->key;
            delete[] R->payload;
            delete[] R->num_tuples;
            delete S;
            delete R;

            // std::cout << "Run time: " << run_time[j] / CLOCKS_PER_SEC << " seconds" << std::endl;
            double min_run_time = *std::min_element(run_time, run_time + 5);
            double min_run_clock = *std::min_element(run_clock, run_clock + 5);

            // 下面可要
            metricstat = "Times(ms)";
            fprintf(fpcache_mem, "%d\t%d\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), min_run_time * 1000, "mem");
            metricstat = "Throughput(GT/s)";
            fprintf(fpcache_mem, "%d\t%d\t%d\t%s\t%s\t%s\t%lf\t%s\n", ts, i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), 1 / min_run_time, "mem");
            metricstat = "Cycles/Tuple";
            fprintf(fpcache_mem, "%d\t%d\t%lld\t%s\t%s\t%s\t%lf\t%s\n", ts, i, s_size, cpumodel.c_str(), numastat.c_str(), metricstat.c_str(), s_size / min_run_clock, "mem");
        }
        fclose(fpcache_mem);
    }
    return 0;
}
