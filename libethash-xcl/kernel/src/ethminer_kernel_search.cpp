#include "ethminer_kernel_search.hpp"

static uint32_t d_dag_size;
static uint64_t d_target;
static hash256_t d_header;

#define NUM_WORDS_PER_MIX 32
#define ACCESSES 64
#define THREADS_PER_HASH 1
#define PARALLEL_HASH 4

static bool compute_hash(
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
        uint64_t nonce,
        hash256_t* mix_hash,
        debug_data* debug_data)
{
    uint64_t state[12]{};
#pragma HLS ARRAY_PARTITION variable=state complete dim=0

    debug_data->nonce = nonce;

    keccak_f1600_init(state, nonce, d_header.word64s);

    for (auto i = 0; i < 12; i++){
        debug_data->after_init_state[i] = state[i];
    }

    const auto seed_init = (uint32_t)(state[0] & 0x00000000FFFFFFFF);
    debug_data->seed = seed_init;

    uint32_t mix[NUM_WORDS_PER_MIX];
#pragma HLS ARRAY_PARTITION variable=mix complete dim=0

    for (auto i = 0, j = 0; i < 8; i++, j += 2){
#pragma HLS loop_tripcount min = 8 max = 8
        auto a = (uint32_t)(state[i] >> 32);
        auto b = (uint32_t)(state[i] & 0xFFFFFFFFLL);
        mix[j + 0] = b;
        mix[j + 1] = a;
        mix[j + 16] = b;
        mix[j + 17] = a;
    }

    for (auto i = 0; i < 16; i++){
#pragma HLS loop_tripcount min = 16 max = 16
#pragma HLS unroll
        debug_data->mix_after_init[i] = mix[i];
    }

    for (uint32_t a = 0; a < ACCESSES; a++){
#pragma HLS loop_tripcount min = 64 max = 64
        debug_data->access_mix_words[a] = mix[a % NUM_WORDS_PER_MIX];
        auto offset = fnv_fn(seed_init ^ a, mix[a % NUM_WORDS_PER_MIX]) % d_dag_size;
        debug_data->access_offsets[a] = offset;

        auto *dag = select_dag_bank(
                dag0, dag1, dag2, dag3, dag4, dag5, dag6, dag7, dag8, dag9, dag10, dag11, dag12,
                dag13, dag14, dag15, dag16, dag17, dag18, dag19, dag20, dag21, dag22, dag23,
                offset);

        for (auto w = 0; w < NUM_WORDS_PER_MIX; w++){
#pragma HLS loop_tripcount min = 32 max = 32
            debug_data->access_dag[a][w] = dag->word32s[w];
            mix[w] = fnv_fn(mix[w], dag->word32s[w]);
            debug_data->access_words_mixed[a][w] = mix[w];
        }
    }

    for (auto i = 0; i < 16; i++){
#pragma HLS loop_tripcount min = 16 max = 16
#pragma HLS unroll
        debug_data->mix_after_access[i] = mix[i];
    }

    for (auto i = 0; i < NUM_WORDS_PER_MIX; i += 4){
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS unroll
        auto h3 = fnv_fn(fnv_fn(fnv_fn(mix[i], mix[i + 1]), mix[i + 2]), mix[i + 3]);
        mix_hash->word32s[i / 4] = h3;
    }

    for (auto i = 0; i < 8; i++){
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS unroll
        debug_data->mix_hash_after_reduce[i] = mix_hash->word32s[i];
    }

    for (auto i = 0; i < 4; i++){
#pragma HLS loop_tripcount min = 4 max = 4
#pragma HLS unroll
        state[i + 8] = mix_hash->word64s[i];
    }

    for (auto i = 0; i < 12; i++){
#pragma HLS loop_tripcount min = 12 max = 12
#pragma HLS unroll
        debug_data->before_final_state[i] = state[i];
    }

    debug_data->reccak_res = keccak_f1600_final(state);
    debug_data->swab_res = cuda_swab64(debug_data->reccak_res);

    if (cuda_swab64(keccak_f1600_final(state)) > d_target){
        return true;
    }

    for (auto i = 0; i < 4; i++){
#pragma HLS loop_tripcount min = 4 max = 4
#pragma HLS unroll
        mix_hash->word64s[i] = state[8 + i];
    }

    return false;
}

extern "C" void search(
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
        uint32_t dag_size,
        search_results* output_data,
        debug_data* debug_data,
        uint64_t start_nonce,
        uint64_t target,
        hash256_t header)
{
#pragma HLS INTERFACE m_axi port=dag0 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag1 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag2 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag3 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag4 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag5 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag6 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag7 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag8 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag9 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag10 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag11 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag12 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag13 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag14 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag15 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag16 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag17 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag18 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag19 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag20 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag21 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag22 offset=slave bundle=gmem0
#pragma HLS INTERFACE m_axi port=dag23 offset=slave bundle=gmem0

#pragma HLS INTERFACE m_axi port=output_data offset=slave bundle=gmem1
    //num_write_outstanding=300

    d_header = header;
    d_dag_size = dag_size;

#pragma HLS INTERFACE m_axi port=start_nonce bundle=gmem0
#pragma HLS INTERFACE m_axi port=target bundle=gmem0
#pragma HLS INTERFACE m_axi port=return bundle=gmem0

    d_target = target;

    hash256_t mix_hash{};
    for (auto i = 0; i < MAX_NONCE_SEARCH; i++) {
#pragma HLS loop_tripcount min = 1 max = 1048576

        auto computed = compute_hash(
                dag0, dag1, dag2, dag3, dag4, dag5, dag6, dag7, dag8, dag9, dag10, dag11, dag12, dag13, dag14, dag15, dag16, dag17, dag18, dag19, dag20, dag21, dag22, dag23,
                start_nonce + i,
                &mix_hash,
                debug_data);

        debug_data->computed = computed;
        if (computed) {
            return;
        }

        output_data->result.gid = i;
        for (auto x = 0; x < 8; x++){
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS unroll
            output_data->result.mix[x] = mix_hash.word32s[x];
        }
        output_data->count++;
    }
}