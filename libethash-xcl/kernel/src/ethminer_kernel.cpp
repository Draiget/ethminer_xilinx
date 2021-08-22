#include "ethminer_kernel.hpp"

void ethash_calculate_dag_item(
        hash64_t* ret,
        uint32_t node_index,
        hash64_t* light,
        uint32_t light_size);

/*
 * HLS pipeline vs HLS unroll
 * https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/What-is-the-difference-between-unroll-and-pipeline/m-p/886038
 *
 * HBM access
 * https://forums.xilinx.com/t5/Alveo-Accelerator-Cards/Usage-scenario-of-AXI-crossbar-on-U50/td-p/1075369
 *
 * Optimizations
 * https://www.xilinx.com/html_docs/xilinx2021_1/vitis_doc/optimizingperformance.html#fhe1553474153030
 */

constexpr uint32_t elements_per_hbm_bank = (XILINX_MEMORY_PER_HBM_BANK * 1024 * 1024) / sizeof(hash64_t);

/**
 * \brief Generate DAG in the size of single HBM memory bank (256Mb)
 */
extern "C" void generate_dag(
        hash128_t* dag,
        uint32_t start_index,
        uint32_t count,
        hash64_t* light,
        uint32_t light_size)
{
#pragma HLS INTERFACE s_axilite port=start_index bundle=control
#pragma HLS INTERFACE s_axilite port=count bundle=control
#pragma HLS INTERFACE s_axilite port=light_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    for (auto node_index = start_index; node_index != start_index + count; ++node_index) {
#pragma HLS loop_tripcount min = 1 max = 4194304
#pragma HLS pipeline

        // auto *tmp_node = (hash64_t *) &(dag0[node_index]);
        ethash_calculate_dag_item((hash64_t *)dag, node_index, light, light_size);
    }
}

void ethash_calculate_dag_item(
        hash64_t* ret,
        uint32_t node_index,
        hash64_t* light,
        uint32_t light_size)
{
    uint32_t tmp_node[16];
//#pragma HLS BIND_STORAGE variable=tmp_node type=RAM_1P impl=BRAM
#pragma HLS ARRAY_PARTITION variable=tmp_node complete dim=0
//#pragma HLS BIND_STORAGE variable=tmp_node type=ram_2p impl=auto

#pragma HLS INTERFACE s_axilite port=node_index bundle=control
#pragma HLS INTERFACE s_axilite port=light_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    // uint32_t num_parent_nodes = light_size
    auto const light_index = node_index % light_size;

tmp_node_copy_from_light_loop:
    for (int i = 0; i < 16; ++i) {
#pragma HLS loop_tripcount min = 16 max = 16
#pragma HLS unroll
        tmp_node[i] = light[light_index].words[i];
    }

    tmp_node[0] ^= node_index;
    SHA3_512(((hash64_t*)&tmp_node[0])->data64);

    uint32_t indices[256];
#pragma HLS BIND_STORAGE variable=indices type=RAM_1P impl=LUTRAM
#pragma HLS array_partition variable=indices complete
    for (uint8_t i = 0; i < (uint8_t)ETHASH_DATASET_PARENTS; ++i) {
        indices[i] = fnv_fn(node_index ^ i, tmp_node[i % NODE_WORDS]) % light_size;
    }

ethash_parents_set_256_loop:
    for (uint8_t i = 0; i < (uint8_t)ETHASH_DATASET_PARENTS; ++i) {
#pragma HLS loop_tripcount min = 256 max = 256
#pragma HLS unroll
        auto parent_index = indices[i];

ethash_parents_set_node_words_loop:
        for (uint32_t w = 0; w != NODE_WORDS; ++w) {
#pragma HLS loop_tripcount min = 16 max = 16
#pragma HLS unroll
            tmp_node[w] = fnv_fn(tmp_node[w], light[parent_index].words[w]);
        }
    }

    SHA3_512(((hash64_t*)&tmp_node[0])->data64);

tmp_node_copy_to_return_value_loop:
    for (int i = 0; i < 16; ++i) {
#pragma HLS loop_tripcount min = 4 max = 4
#pragma HLS unroll
        ret->words[i] = tmp_node[i];
    }
}

/*


hash128_t* ethash_select_dag_input(
        hash128_t* dag0,
        hash128_t* dag1,
        hash128_t* dag2,
        hash128_t* dag3,
        hash128_t* dag4,
        hash128_t* dag5,
        hash128_t* dag6,
        hash128_t* dag7,
        hash128_t* dag8,
        hash128_t* dag9,
        hash128_t* dag10,
        hash128_t* dag11,
        hash128_t* dag12,
        hash128_t* dag13,
        hash128_t* dag14,
        hash128_t* dag15,
        hash128_t* dag16,
        hash128_t* dag17,
        hash128_t* dag18,
        hash128_t* dag19,
        hash128_t* dag20,
        hash128_t* dag21,
        hash128_t* dag22,
        hash128_t* dag23,
        uint32_t index);

extern "C" void generate_dag(
        hash128_t* dag0,
        hash128_t* dag1,
        hash128_t* dag2,
        hash128_t* dag3,
        hash128_t* dag4,
        hash128_t* dag5,
        hash128_t* dag6,
        hash128_t* dag7,
        hash128_t* dag8,
        hash128_t* dag9,
        hash128_t* dag10,
        hash128_t* dag11,
        hash128_t* dag12,
        hash128_t* dag13,
        hash128_t* dag14,
        hash128_t* dag15,
        hash128_t* dag16,
        hash128_t* dag17,
        hash128_t* dag18,
        hash128_t* dag19,
        hash128_t* dag20,
        hash128_t* dag21,
        hash128_t* dag22,
        hash128_t* dag23,
        uint32_t start,
        uint32_t count,
        hash64_t* light,
        uint32_t light_size)
{
#pragma HLS INTERFACE s_axilite port=start bundle=control
#pragma HLS INTERFACE s_axilite port=count bundle=control
#pragma HLS INTERFACE s_axilite port=light_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    for (auto node_index = start; node_index != start + count; ++node_index) {
#pragma HLS loop_tripcount min = 1 max = 4194304
#pragma HLS pipeline
        auto dag_index = node_index % elements_per_hbm_bank;
        auto *tmp_node = ethash_select_dag_input(dag0, dag1, dag2, dag3, dag4, dag5, dag6, dag7, dag8, dag9, dag10, dag11, dag12, dag13, dag14, dag15, dag16, dag17, dag18, dag19, dag20, dag21, dag22, dag23, dag_index);

        // auto *tmp_node = (hash64_t *) &(dag0[node_index]);
        ethash_calculate_dag_item((hash64_t *)tmp_node, node_index, light, light_size);
    }
}



hash128_t* ethash_select_dag_input(
        hash128_t* dag0,
        hash128_t* dag1,
        hash128_t* dag2,
        hash128_t* dag3,
        hash128_t* dag4,
        hash128_t* dag5,
        hash128_t* dag6,
        hash128_t* dag7,
        hash128_t* dag8,
        hash128_t* dag9,
        hash128_t* dag10,
        hash128_t* dag11,
        hash128_t* dag12,
        hash128_t* dag13,
        hash128_t* dag14,
        hash128_t* dag15,
        hash128_t* dag16,
        hash128_t* dag17,
        hash128_t* dag18,
        hash128_t* dag19,
        hash128_t* dag20,
        hash128_t* dag21,
        hash128_t* dag22,
        hash128_t* dag23,
        uint32_t index)
{
#pragma HLS INTERFACE s_axilite port=index bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    switch (index) {
        case 1: return dag1;
        case 2: return dag2;
        case 3: return dag3;
        case 4: return dag4;
        case 5: return dag5;
        case 6: return dag6;
        case 7: return dag7;
        case 8: return dag8;
        case 9: return dag9;
        case 10: return dag10;
        case 11: return dag11;
        case 12: return dag12;
        case 13: return dag13;
        case 14: return dag14;
        case 15: return dag15;
        case 16: return dag16;
        case 17: return dag17;
        case 18: return dag18;
        case 19: return dag19;
        case 20: return dag20;
        case 21: return dag21;
        case 22: return dag22;
        case 23: return dag23;
        default: return dag0;
    }
}
 */