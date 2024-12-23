#ifndef Metadata_H
#define Metadata_H
#include <string>
typedef int8_t idx;
#define STACKSIZE 8388608
#define HASH(X, Y, Z) ((X - Z) % Y)
constexpr int8_t DIM_NULL = INT8_MAX;
constexpr int GROUP_NULL = INT16_MAX;
const int M_VALUE = 5;                                                                          // value of M1 and M2
const int size_v = 1024;                                                                        // Vector length
const char *VECJOIN_JOINALGO_TEST_TIME_FILE = "../log/operator_test/vecjoin_joinalgo_test.tsv"; // the result time file for vecjoin join test
const char *CRYSTAL_JOINALGO_TEST_TIME_FILE = "../log/operator_test/crystal_joinalgo_test.tsv"; // the result time file for crystal join test
const char *VECJOIN_AGGALGO_TEST_TIME_FILE = "../log/operator_test/vecjoin_aggalgo_test.tsv";   // the result time file for vecjoin agg test
const char *CRYSTAL_AGGALGO_TEST_TIME_FILE = "../log/operator_test/crystal_aggalgo_test.tsv";   // the result time file for crystal agg test
const char *OLAPCORE_TEST_TIME_FILE = "./log/benchmark_test/olapcore_test.tsv";                 // the result time file for OLAPcore test
/*Star Schema Benchmark*/
const size_t LINEORDER_BASE = 6000000; // lineorder base num
const size_t CUSTOMER_BASE = 30000;    // customer base num
const size_t SUPPLIER_BASE = 2000;     // supplier base num
const size_t PART_BASE = 200000;       // part base num
const size_t DATE_BASE = 7 * 365;      // date base num
/*TPC-H*/
const size_t LINEITEM_BASE = 6000000;    // lineitem base num
const size_t PARTSUPP_BASE = 800000;     // partsupp base num
const size_t ORDERS_BASE = 1500000;      // orders base num
const size_t PARTTPCH_BASE = 200000;     // TPCH-part base num
const size_t SUPPLIERTPCH_BASE = 10000;  // TPCH-supplier base num
const size_t CUSTOMERTPCH_BASE = 150000; // TPCH-customer base num
const size_t NATION_BASE = 25;           // nation base num
const size_t REGION_BASE = 5;            // region base num
enum TABLE_NAME
{
  customer,
  supplier,
  part,
  date,
  lineorder
};
struct relation_t    // 128b=16B
{
  int32_t *key;
  int32_t *payload;
  uint64_t *num_tuples;
};
struct relation_numa_t
{
  int32_t *key[10];
  int32_t *payload[10];
  uint64_t *num_tuples;
};
struct create_arg_t
{
  relation_t rel;
  int64_t firstkey;
  int64_t maxid;
  uint64_t ridstart;
  pthread_barrier_t *barrier;
  int tid;
};
struct param_join_t
{
  //int help=0;    // help
  uint32_t nthreads;
  int r_size;
  int s_size;
  int groups;
  int8_t numa_partition;
  int8_t col_num;
  int8_t test_bandwidth;
  int8_t cachetest;
};
struct param_t
{
  uint32_t nthreads;
  double sf;
  uint32_t d_groups;
  uint32_t s_groups;
  uint32_t p_groups;
  uint32_t c_groups;
  double d_sele;
  double s_sele;
  double p_sele;
  double c_sele;
  int d_bitmap;
  int s_bitmap;
  int p_bitmap;
  int c_bitmap;
  int basic_numa;
  int sqlnum;
};
struct pth_cwmjoint
{
  int join_id;
  int64_t start;
  int64_t num_tuples;
  int8_t *dimvec;
  int32_t *fk;
  int64_t *OID;
  int16_t *groupID;
  int *index;
  int factor;
  int tid;
};
struct pth_rowolapcoret
{
  int join_id;
  int64_t start;
  int64_t num_tuples;
  int8_t **dimvec_array;
  int32_t **fk_array;
  int dimvec_nums;
  const int *orders;
  uint32_t *group_vector;
  int32_t *M1;
  int32_t *M2;
  const int *factor;
};
struct pth_vwmolapcoret
{
  int join_id;
  int64_t start;
  int64_t num_tuples;
  int8_t **dimvec_array;
  int32_t **fk_array;
  int dimvec_nums;
  int *orders;
  int64_t *OID;
  int16_t *groupID;
  uint32_t *group_vector;
  int32_t *M1;
  int32_t *M2;
  int *index;
  int *factor;
};
struct pth_cwmaggt
{
  int64_t start;
  int64_t num_tuples;
  int64_t *OID;
  int16_t *groupID;
  int *index;
  int32_t *M1;
  int32_t *M2;
  uint32_t *group_vector;
};
struct Select_Node;
struct Select_Data
{
  const void *sel_col1;
  const void *sel_col2;
  int8_t op;        // Filter symbols
  int8_t col2_flag; // Single value or column
  int8_t select_flag;
  int8_t count;
  int *pre_bmp;
  int *res_bmp;
  void *(*select)(void *);
};
struct Select_Node
{
  int select_num;
  Select_Data select_data[5];
  int col_length;
  std::string tablename;
};
struct Group_Data_gt
{
  const void *gro_col;
  void *com_dic_t;
  int location;
  int dic_location;
  std::string tablename;
};
struct Group_Data
{
  int *group_count;
  const void *gro_col;
  const void *key_col;
  void *com_dic_t;
  int *pre_vec;
  int *res_vec;
  int ht_len;
  int ht_min;
  std::string colname;
  std::string tablename;
  int table_size;
};
struct Group_Node
{
  int tablenum;
  int *group_total_num;
  Group_Data group_data[7];
};
struct Join_Node
{
  int *join_col[4];
  int *pre_vec[4];
  void *(*join[4])(void *);
  int32_t *OID;
  int16_t *groupID;
  int table_size;
  int8_t factor[4];
  int8_t join_col_num;
};

struct pth_st
{
  pthread_barrier_t *barrier;
  const void *sel_col1;
  const void *sel_col2;
  std::string tablename;
  int8_t op;
  int8_t col2_flag;
  int8_t select_flag;
  int *pre_bmp;
  int *res_bmp;
  int8_t logic;
  int startindex;
  int comline; // 维表行数
};
struct pth_gt
{
  int colnum;
  int *group_count;
  int startindex;
  int comline; // 维表行数
  int ht_len;
  int ht_min;
  pthread_mutex_t *mut;
  pthread_barrier_t *barrier;
  std::string tablename;
  std::string colname;
  const void *gro_col;
  const void *key_col;
  void *com_dic_t;
  int *res_vec;
  int *pre_vec;
};

#endif
