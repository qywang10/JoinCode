/**
 * @file    cpu_mapping.h
 * @author  Cagri Balkesen <cagri.balkesen@inf.ethz.ch>
 * @date    Tue May 22 16:35:12 2012
 * @version $Id: cpu_mapping.h 4548 2013-12-07 16:05:16Z bcagri $
 *
 * @brief  Provides cpu mapping utility function.
 *
 *
 */
#ifndef CPU_MAPPING_H
#define CPU_MAPPING_H
#include <stdio.h>  /* FILE, fopen */
#include <stdlib.h> /* exit, perror */
#include <unistd.h> /* sysconf */
#include <numaif.h> /* get_mempolicy() */
#include <string.h>
#include <numa.h>
/**
 * @defgroup cpumapping CPU mapping tool
 * @{
 */

#ifdef __cplusplus
namespace eth_hashjoin
{
    extern "C"
    {
#endif // __cplusplus

        /**
         * Returns SMT aware logical to physical CPU mapping for a given thread id.
         */
        int get_cpu_id(int thread_id);

        /**
         * Returns the NUMA id of the given thread id returned from get_cpu_id(int)
         *
         * @param mytid
         *
         * @return
         */
        int
        get_numa_id(int mytid);
        void
        bind_numa(int numaid);

        /**
         * Returns number of NUMA regions.
         *
         * @return
         */
        int
        get_num_numa_regions(void);

        /**
         * Returns the NUMA-node id of a given memory address
         */
        int
        get_numa_node_of_address(void *ptr);

#define MAX_NODES 512
        static int inited = 0;
        static int max_cpus;
        static int node_mapping[MAX_NODES];
        static int socket_num = 0;
        static int numa[10][200] = {{}, {}};
        static int numa_num[10] = {0};
        static int numa_count[10] = {0};

        void split(char *src, const char *separator, char **dest, int *num)
        {
            /*
                src 源字符串的首地址(buf的地址)
                separator 指定的分割字符
                dest 接收子字符串的数组
                num 分割后子字符串的个数
            */
            char *pNext;
            int count = 0;
            if (src == NULL || strlen(src) == 0) // 如果传入的地址为空或长度为0，直接终止
                return;
            if (separator == NULL || strlen(separator) == 0) // 如未指定分割的字符串，直接终止
                return;
            pNext = (char *)strtok(src, separator); // 必须使用(char *)进行强制类型转换(虽然不写有的编译器中不会出现指针错误)
            while (pNext != NULL)
            {
                *dest++ = pNext;
                ++count;
                pNext = (char *)strtok(NULL, separator); // 必须使用(char *)进行强制类型转换
            }
            *num = count;
        }
        void
        bind_numa(int numaid)
        {
            int nr_nodes = socket_num;
            struct bitmask *new_nodes;
            new_nodes = numa_bitmask_alloc(nr_nodes);
            numa_bitmask_setbit(new_nodes, numaid);
            numa_bind(new_nodes);
        }

        /**
         * Initializes the cpu mapping from the file defined by CUSTOM_CPU_MAPPING.
         * The mapping used for our machine Intel L5520 is = "8 0 1 2 3 8 9 10 11".
         */
        // static int
        // init_mappings_from_file()
        // {
        //     FILE *cfg;
        //     int i;

        //     cfg = fopen(CUSTOM_CPU_MAPPING, "r");
        //     if (cfg != NULL)
        //     {
        //         if (fscanf(cfg, "%d", &max_cpus) <= 0)
        //         {
        //             perror("Could not parse input!\n");
        //         }

        //         for (i = 0; i < max_cpus; i++)
        //         {
        //             if (fscanf(cfg, "%d", &node_mapping[i]) <= 0)
        //             {
        //                 perror("Could not parse input!\n");
        //             }
        //         }

        //         fclose(cfg);
        //         return 1;
        //     }

        //     /* perror("Custom cpu mapping file not found!\n"); */
        //     return 0;
        // }

        /**
         * Try custom cpu mapping file first, if does not exist then round-robin
         * initialization among available CPUs reported by the system.
         */
        // static void
        // init_mappings()
        // {
        //     if (init_mappings_from_file() == 0)
        //     {
        //         int i;

        //         max_cpus = sysconf(_SC_NPROCESSORS_ONLN);
        //         for (i = 0; i < max_cpus; i++)
        //         {
        //             node_mapping[i] = i;
        //         }
        //     }
        // }
        static void
        get_numa_info()
        {
            socket_num = numa_max_node() + 1;
            int i, j;
            max_cpus = sysconf(_SC_NPROCESSORS_ONLN);
            for (i = 0; i < max_cpus; i++)
            {
                numa[numa_node_of_cpu(i)][numa_num[numa_node_of_cpu(i)]++] = i;
            }
        }
        /** @} */

        /**
         * Returns SMT aware logical to physical CPU mapping for a given thread id.
         */
        int
        get_cpu_id(int thread_id)
        {
            if (!inited)
            {
                get_numa_info();
                // init_mappings();
                inited = 1;
            }
            int numa_id = thread_id % socket_num;
            int result = numa[numa_id][numa_count[numa_id]];
            numa_count[numa_id]++;
            if (numa_count[numa_id] == numa_num[numa_id])
                numa_count[numa_id] = 0;
            return result;
        }
        void
        numa_init()
        {
            int i;
            for (i = 0; i < socket_num; i++)
            {
                numa_count[i] = 0;
            }
        }

/* TODO: These are just place-holder implementations. */
/**
 * Topology of Intel E5-4640
 node 0 cpus: 0 4 8 12 16 20 24 28 32 36 40 44 48 52 56 60
 node 1 cpus: 1 5 9 13 17 21 25 29 33 37 41 45 49 53 57 61
 node 2 cpus: 2 6 10 14 18 22 26 30 34 38 42 46 50 54 58 62
 node 3 cpus: 3 7 11 15 19 23 27 31 35 39 43 47 51 55 59 63
*/
#define INTEL_E5 1

        int
        get_numa_id(int mytid)
        {
#if INTEL_E5
            if (!inited)
            {
                get_numa_info();
                // init_mappings();
                inited = 1;
            }
            int ret = 0;

            for (int i = 0; i < socket_num; i++)
                for (int j = 0; j < numa_num[i]; j++)
                    if (numa[i][j] == mytid)
                    {
                        ret = i;
                        return ret;
                    }

#else
    return 0;
#endif
        }

        int
        get_num_numa_regions(void)
        {
            /* TODO: FIXME automate it from the system config. */
#if INTEL_E5
            if (!inited)
            {
                get_numa_info();
                // init_mappings();
                inited = 1;
            }
            return socket_num;
#else
    return 1;
#endif
        }

        int
        get_numa_node_of_address(void *ptr)
        {
            int numa_node = -1;
            get_mempolicy(&numa_node, NULL, 0, ptr, MPOL_F_NODE | MPOL_F_ADDR);
            return numa_node;
        }

#ifdef __cplusplus
    } // extern "C"
} // namespace eth_hashjoin
#endif // __cplusplus
#endif /* CPU_MAPPING_H */