#include "ethminer_kernel_dag.hpp"

void calculate_dag_item(
        hash64_t* ret,
        uint32_t node_index,
        hash64_t* light,
        uint32_t light_size);

hash128_t* determine_target_hbm_bank(
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
        uint8_t index)
{
#pragma HLS inline
    switch (index) {
        case 0: return dag0;
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
        default: return dag23;
    }
}

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

/**
 * \brief Generate DAG in the size of single HBM memory bank (256Mb)
 */
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
        uint32_t dag_bank_index,
        uint32_t count,
        hash64_t* light,
        uint32_t light_size)
{
#pragma HLS INTERFACE s_axilite port=dag_bank_index bundle=control
#pragma HLS INTERFACE s_axilite port=count bundle=control
#pragma HLS INTERFACE s_axilite port=light_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    for (uint32_t node_index = 0; node_index < count; ++node_index) {
#pragma HLS loop_tripcount min = 1 max = 4194304
//#pragma HLS pipeline
        hash64_t* dag_bank = (hash64_t*)determine_target_hbm_bank(
                dag0,dag1,dag2,dag3,dag4,dag5,dag6,dag7,dag8,dag9,dag10,dag11,dag12,
                dag13,dag14,dag15,dag16,dag17,dag18,dag19,dag20,dag21,dag22,dag23,
                dag_bank_index);

        auto dag_index = node_index % elements_per_hbm_bank;
        calculate_dag_item(&dag_bank[dag_index], node_index, light, light_size);
    }
}

void calculate_dag_item_parents(
        uint32_t* tmp_words_node,
        uint32_t node_index,
        hash64_t* light,
        uint32_t light_size)
{
    for (uint16_t i = 0; i < ETHASH_DATASET_PARENTS; ++i) {
#pragma HLS loop_tripcount min = 256 max = 256
#pragma HLS pipeline
        auto parent_index = fnv_fn(node_index ^ i, tmp_words_node[i % NODE_WORDS]) % light_size;

        for (uint8_t w = 0; w < NODE_WORDS; ++w) {
#pragma HLS loop_tripcount min = 16 max = 16
#pragma HLS pipeline
            tmp_words_node[w] = fnv_fn(tmp_words_node[w], light[parent_index].words[w]);
        }
    }
}

void calculate_dag_item(
        hash64_t* ret,
        uint32_t node_index,
        hash64_t* light,
        uint32_t light_size)
{
    //uint32_t tmp_node[16]{};
    uint64_t tmp_node[25]{};
    uint32_t* tmp_words_node = (uint32_t*)&tmp_node;
#pragma HLS ARRAY_PARTITION variable=tmp_node complete dim=0

#pragma HLS INTERFACE s_axilite port=node_index bundle=control
#pragma HLS INTERFACE s_axilite port=light_size bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

    auto const light_index = node_index % light_size;

    for (int i = 0; i < 8; ++i) {
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS pipeline
        tmp_node[i] = light[light_index].data64[i];
    }

    tmp_node[0] ^= node_index;
    SHA3_512(&tmp_node[0]);

    calculate_dag_item_parents(tmp_words_node, node_index, light, light_size);

    SHA3_512(&tmp_node[0]);

    for (uint8_t i = 0; i < 8; ++i) {
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS pipeline
#pragma HLS DEPENDENCE variable=tmp_node intra RAW true
        ret->data64[i] = tmp_node[i];
    }
}
