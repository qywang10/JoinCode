#ifndef Gendata_H
#define Gendata_H
#include "metadata.h"
#include "cpu_mapping.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h> /* sysconf */
#define RAND_RANGE48(N, STATE) ((double)nrand48(STATE) / ((double)RAND_MAX + 1) * (N))
static int seeded = 0;
static unsigned int seedValue;
/**
 * @brief generate random number in range [1, range]
 *
 * @param[in] range
 * @return int the generated random number
 */

inline int rand_x(int range)
{
  return ((rand() % (range)) + 1);
}
/**
 * @brief generate random number in range [range_min, range_max]
 *
 * @param[in] range_min
 * @param[in] range_max
 * @return int the generated random number
 */
inline int rand_x(int range_min, int range_max)
{
  return (rand() % (range_max - range_min + 1) + range_min);
}
/**
 * @brief generate random number in range [min, max], for double
 *
 * @param max
 * @param min
 * @return double the generated random number
 */
inline double rand_x(double min, double max)
{
  return min + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX / (max - min)));
}
/**
 * @Randomly assign values to seed and seedValue
 *
 * @return void
 */
void seed_generator(unsigned int seed)
{
  srand(seed);
  seedValue = seed;
  seeded = 1;
}
/**
 * @Check wheter seeded, if not seed the generator with current time
 *
 * @return void
 */
static void
check_seed()
{
  if (!seeded)
  {
    seedValue = time(NULL);
    srand(seedValue);
    seeded = 1;
  }
}
/**
 * @brief generate test data
 *
 * @param s_size
 * @param groups
 * @param VecInx
 * @param M1
 * @param M2
 * @return int number of records generated
 */
int gen_data(const int &s_size, const int &groups, int *VecInx, int *M1, int *M2)
{
  timeval start, end;
  double ms;
  gettimeofday(&start, NULL);
  srand(time(NULL));
  int i;
  for (i = 0; i != s_size; ++i)
  {
    VecInx[i] = (rand_x(groups));
    M1[i] = M_VALUE;
    M2[i] = M_VALUE;
  }
  gettimeofday(&end, NULL);
  // ms = calc_ms(end, start);
  std::cout << ">>> Generated data " << i << " lines used time " << ms << "ms." << std::endl;
  return i;
}
/**
 * @brief randomly shuffle elements
 * @param[in]  state A parameter used to determine a random number
 * @param[out] relation Data structure for storing table data
 * @return void
 */
void knuth_shuffle48(relation_t *relation, unsigned short *state)
{
  uint64_t i;
  for (i = *(relation->num_tuples) - 1; i > 0; i--)
  {
    int64_t j = RAND_RANGE48(i, state);
    int tmp = relation->key[i];
    relation->key[i] = relation->key[j];
    relation->key[j] = tmp;
  }
}
/**
 * @brief Create random unique keys starting from firstkey
 * @param[in]  args Data structure for storing parameter data
 * @return void*
 */
void *
random_unique_gen_thread(void *args)
{
  create_arg_t *arg = (create_arg_t *)args;
  relation_t *rel = &arg->rel;
  int64_t firstkey = arg->firstkey;
  int64_t maxid = arg->maxid;
  uint64_t ridstart = arg->ridstart;
  uint64_t i;

  /* for randomly seeding nrand48() */
  unsigned short state[3] = {0, 0, 0};
  unsigned int seed = time(NULL) + *(unsigned int *)pthread_self();
  memcpy(state, &seed, sizeof(seed));

  for (i = 0; i < *rel->num_tuples; i++)
  {
    if (firstkey % (maxid + 1) == 0)
      firstkey = 1;
    firstkey = firstkey % (maxid + 1);
    if (firstkey == 0)
      printf("%d %d\n", arg->firstkey, i);
    rel->key[i] = firstkey;
    rel->payload[i] = 1;
    // printf("%d ", firstkey);

    firstkey++;
  }

  /* randomly shuffle elements */
  knuth_shuffle48(rel, state);
  return 0;
}
void *
random_unique_gen_0_thread(void *args)
{
  create_arg_t *arg = (create_arg_t *)args;
  relation_t *rel = &arg->rel;
  // int64_t firstkey = arg->firstkey;
  // int64_t maxid = arg->maxid;
  // uint64_t ridstart = arg->ridstart;
  uint64_t i;

  // /* for randomly seeding nrand48() */
  // unsigned short state[3] = {0, 0, 0};
  // unsigned int seed = time(NULL) + *(unsigned int *)pthread_self();
  // memcpy(state, &seed, sizeof(seed));

  for (i = 0; i < *rel->num_tuples; i++)
  {
    rel->key[i] = 1;
    rel->payload[i] = 1;
    // printf("%d ", i);

    // firstkey++;
  }

  /* randomly shuffle elements */
  // knuth_shuffle48(rel, state);
  return nullptr;
}
/**
 * @brief generate random number in range [1, range]
 *
 * @param[in] table
 * @param[in] SF
 * @return int the generated random number
 */
inline int size_of_table(const TABLE_NAME &table, const double &SF)
{
  switch (table)
  {
  case customer:
    return CUSTOMER_BASE * SF;
  case supplier:
    return SUPPLIER_BASE * SF;
  case part:
    return (int)(PART_BASE * (double)(1 + log2(SF)));
  case date:
    return DATE_BASE;
  case lineorder:
    return LINEORDER_BASE * SF;
  }
  return 0;
}
/**
 * @brief generate test data for OLAPcore test
 *
 * @param c_sele selection rate
 * @param s_sele
 * @param p_sele
 * @param d_sele
 * @param SF
 * @param c_bitmap
 * @param s_bitmap
 * @param p_bitmap
 * @param d_bitmap
 * @param c_groups
 * @param s_groups
 * @param p_groups
 * @param d_groups
 * @param dimvec_c
 * @param dimvec_s
 * @param dimvec_p
 * @param dimvec_d
 * @param fk_c
 * @param fk_s
 * @param fk_p
 * @param fk_d
 * @param M1
 * @param M2
 * @return int
 */
void gen_data(int32_t *dimvec, int32_t *fk, int32_t *raw_col, int32_t dimvec_length, int32_t fk_length)
{
  for (int i = 0; i < dimvec_length; i++)
  {
    dimvec[i] = i + 1;
  }
  for (int i = 0; i < fk_length; i++)
  {
    raw_col[i] = rand_x(1, dimvec_length);
    fk[i] = rand_x(1, dimvec_length);
  }
  return;
}
int gen_data(const double &c_sele, const double &s_sele, const double &p_sele, const double &d_sele,
             const double &SF, const int &c_bitmap, const int &s_bitmap, const int &p_bitmap, const int &d_bitmap,
             const int &c_groups, const int &s_groups, const int &p_groups, const int &d_groups,
             int8_t *&dimvec_c, int8_t *&dimvec_s, int8_t *&dimvec_p, int8_t *&dimvec_d,
             int32_t *&fk_c, int32_t *&fk_s, int32_t *&fk_p, int32_t *&fk_d,
             int32_t *&M1, int32_t *&M2)
{
  timeval start, end;
  double ms;
  int size_customer = size_of_table(TABLE_NAME::customer, SF);
  int size_supplier = size_of_table(TABLE_NAME::supplier, SF);
  int size_part = size_of_table(TABLE_NAME::part, SF);
  int size_date = size_of_table(TABLE_NAME::date, SF);
  dimvec_c = new int8_t[size_customer];
  dimvec_s = new int8_t[size_supplier];
  dimvec_p = new int8_t[size_part];
  dimvec_d = new int8_t[size_date];
  int size_lineorder = size_of_table(TABLE_NAME::lineorder, SF);
  fk_c = new int32_t[size_lineorder];
  fk_s = new int32_t[size_lineorder];
  fk_p = new int32_t[size_lineorder];
  fk_d = new int32_t[size_lineorder];
  M1 = new int32_t[size_lineorder];
  M2 = new int32_t[size_lineorder];

  gettimeofday(&start, NULL);
  srand(time(NULL));

  // generate data for customer table
  if (c_bitmap)
  {
    for (size_t i = 0; i != size_customer; ++i)
    {
      dimvec_c[i] = rand_x(0.0, 1.0) <= c_sele ? (int8_t)rand_x(0, c_groups - 1) : DIM_NULL;
    }
  }
  else
  {
    for (size_t i = 0; i != size_customer; ++i)
    {
      dimvec_c[i] = rand_x(0.0, 1.0) <= c_sele ? 0 : DIM_NULL;
    }
  }

  // generate data for supplier table
  if (s_bitmap)
  {
    for (size_t i = 0; i != size_supplier; ++i)
    {
      dimvec_s[i] = rand_x(0.0, 1.0) <= s_sele ? (int8_t)rand_x(0, s_groups - 1) : DIM_NULL;
    }
  }
  else
  {
    for (size_t i = 0; i != size_supplier; ++i)
    {
      dimvec_s[i] = rand_x(0.0, 1.0) <= s_sele ? 0 : DIM_NULL;
    }
  }

  // generate data for part table
  if (p_bitmap)
  {
    for (size_t i = 0; i != size_part; ++i)
    {
      dimvec_p[i] = rand_x(0.0, 1.0) <= p_sele ? (int8_t)rand_x(0, p_groups - 1) : DIM_NULL;
    }
  }
  else
  {
    for (size_t i = 0; i != size_part; ++i)
    {
      dimvec_p[i] = rand_x(0.0, 1.0) <= p_sele ? 0 : DIM_NULL;
    }
  }
  // generate data for date table
  if (d_bitmap)
  {
    for (size_t i = 0; i != size_date; ++i)
    {
      dimvec_d[i] = rand_x(0.0, 1.0) <= d_sele ? (int8_t)rand_x(0, d_groups - 1) : DIM_NULL;
    }
  }
  else
  {
    for (size_t i = 0; i != size_date; ++i)
    {
      dimvec_d[i] = rand_x(0.0, 1.0) <= d_sele ? 0 : DIM_NULL;
    }
  }

  // generate data for lineorder table
  for (size_t i = 0; i != size_lineorder; ++i)
  {
    fk_c[i] = rand_x(0, size_customer);
    fk_s[i] = rand_x(0, size_supplier);
    fk_p[i] = rand_x(0, size_part);
    fk_d[i] = rand_x(0, size_date);
    M1[i] = 5;
    M2[i] = 5;
  }

  gettimeofday(&end, NULL);
  // ms = calc_ms(end, start);
  std::cout << ">>> Generated data " << size_lineorder << " lines in lineorder table used time " << ms << "ms." << std::endl;
  return 0;
}

/**
 * @brief generate test data for join test
 * @param[in]  size size of the table (rows)
 * @param[in] maxid The maximum random value
 * @param[in] nthreads The concurrent execution granularity
 * @param[out] relation The data structure for storing table data
 * @return int the number of lines generated
 */
int gen_data(const int &size, const int &maxid,
             relation_t *relation, const int &nthreads)
{
  int rv;
  uint32_t i;
  uint64_t offset = 0;
  create_arg_t args[nthreads];
  pthread_t tid[nthreads];
  cpu_set_t set;
  pthread_attr_t attr;
  pthread_barrier_t barrier;
  uint64_t ntuples_perthr;
  uint64_t ntuples_lastthr;
  ntuples_perthr = size / nthreads;
  ntuples_lastthr = size - ntuples_perthr * (nthreads - 1);
  pthread_attr_init(&attr);
  rv = pthread_barrier_init(&barrier, NULL, nthreads);
  if (rv != 0)
  {
    printf("[ERROR] Couldn't create the barrier\n");
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < nthreads; i++)
  {
    int cpu_idx = i;
    CPU_ZERO(&set);
    CPU_SET(cpu_idx, &set);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
    args[i].firstkey = offset + 1;
    args[i].maxid = maxid;
    args[i].ridstart = offset;
    args[i].rel.key = relation->key + offset;
    args[i].rel.payload = relation->payload + offset;
    args[i].rel.num_tuples = new uint64_t[1];
    *(args[i].rel.num_tuples) = (i == nthreads - 1) ? ntuples_lastthr
                                                    : ntuples_perthr;
    args[i].barrier = &barrier;
    offset += ntuples_perthr;
    if (maxid != 1)
      rv = pthread_create(&tid[i], &attr, random_unique_gen_thread,
                          (void *)&args[i]);
    else
      rv = pthread_create(&tid[i], &attr, random_unique_gen_0_thread,
                          (void *)&args[i]);
    if (rv)
    {
      fprintf(stderr, "[ERROR] pthread_create() return code is %d\n", rv);
      exit(-1);
    }
  }
  for (i = 0; i < nthreads; i++)
  {
    pthread_join(tid[i], NULL);
  }
  return 0;
}

int gen_data_numa(uint64_t *size, const int &maxid,
                  relation_numa_t *relation, const int &nthreads)
{
  int rv;
  uint32_t i;
  uint64_t offset = 0;
  create_arg_t args[nthreads];
  pthread_t tid[nthreads];
  int numa_regions = eth_hashjoin::get_num_numa_regions();
  cpu_set_t set;
  pthread_attr_t attr;
  pthread_barrier_t barrier;
  uint64_t *numS_numa;
  int32_t numSthr_numa[numa_regions];
  numS_numa = new uint64_t[numa_regions];
  for (i = 0; i < numa_regions; i++)
    numS_numa[i] = size[i];

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

  pthread_attr_init(&attr);
  rv = pthread_barrier_init(&barrier, NULL, nthreads);
  int num_nthreads[numa_regions] = {0};
  if (rv != 0)
  {
    printf("[ERROR] Couldn't create the barrier\n");
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < nthreads; i++)
  {
    int cpu_idx = i;
    int numa_id = eth_hashjoin::get_numa_id(cpu_idx);
    CPU_ZERO(&set);
    CPU_SET(cpu_idx, &set);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);
    args[i].firstkey = numSthr_numa[numa_id] * num_nthreads[numa_id] + 1;
    args[i].maxid = maxid;
    args[i].ridstart = numSthr_numa[numa_id] * num_nthreads[numa_id];
    args[i].rel.key = relation->key[numa_id] + numSthr_numa[numa_id] * num_nthreads[numa_id];
    args[i].rel.payload = relation->payload[numa_id] + numSthr_numa[numa_id] * num_nthreads[numa_id];
    args[i].rel.num_tuples = new uint64_t[1];
    *(args[i].rel.num_tuples) = (num_nthreads[numa_id] == (nthreads_numa[numa_id] - 1)) ? numS_numa[numa_id] : numSthr_numa[numa_id];
    args[i].barrier = &barrier;
    // offset += ntuples_perthr;
    numS_numa[numa_id] -= numSthr_numa[numa_id];
    num_nthreads[numa_id]++;
    if (maxid == 1)
      rv = pthread_create(&tid[i], &attr, random_unique_gen_thread,
                          (void *)&args[i]);
    else
      rv = pthread_create(&tid[i], &attr, random_unique_gen_0_thread,
                          (void *)&args[i]);
    if (rv)
    {
      fprintf(stderr, "[ERROR] pthread_create() return code is %d\n", rv);
      exit(-1);
    }
  }
  for (i = 0; i < nthreads; i++)
  {
    pthread_join(tid[i], NULL);
  }
  return 0;
}
#endif