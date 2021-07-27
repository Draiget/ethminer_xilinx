#include "ethminer_kernel.hpp"

#define COMPUTE_DAG_GENERATE_PARALLEL_COUNT 64

/*
 * HLS pipeline vs HLS unroll
 * https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/What-is-the-difference-between-unroll-and-pipeline/m-p/886038
 */

extern "C" void generate_dag(
        hash128_t* dag,
        uint32_t dag_size,
        hash64_t* light,
        uint32_t light_size,
        uint32_t work_size)
{
#pragma HLS resource variable = dag_size core = RAM_2P_LUTRAM
#pragma HLS resource variable = light_size core = RAM_2P_LUTRAM

    for (uint32_t compute_index = 0; compute_index < COMPUTE_DAG_GENERATE_PARALLEL_COUNT; ++compute_index){
#pragma HLS resource variable = compute_index core = RAM_2P_LUTRAM
#pragma HLS loop_tripcount min = 1 max = 64
#pragma HLS pipeline

        if (((compute_index >> 1U) & (~1U)) >= dag_size){
            continue;
        }

        hash128_t dag_node{};
        copy(dag_node.uint4s, light[compute_index % light_size].uint4s, 4);
        dag_node.words[0] ^= compute_index;
        SHA3_512(dag_node.uint2s);

        for (uint32_t i = 0; i != ETHASH_DATASET_PARENTS; ++i) {
#pragma HLS loop_tripcount min = 1 max = 256
#pragma HLS pipeline

            uint32_t parent_index = fnv(compute_index ^ i, dag_node.words[i % NODE_WORDS]) % light_size;
            auto parent = light[parent_index];

            for (uint32_t t = 0; t != NODE_WORDS; t++) {
#pragma HLS loop_tripcount min = 1 max = 16
#pragma HLS pipeline

                dag_node.words[t] = fnv(dag_node.words[t], parent.words[t]);
            }
        }

        SHA3_512(dag_node.uint2s);
        hash64_t* dag_nodes = (hash64_t*)dag;
        copy(dag_nodes[compute_index].uint4s, dag_node.uint4s, 4);
    }
}
