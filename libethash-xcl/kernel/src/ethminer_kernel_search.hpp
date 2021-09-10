#pragma once

typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef uint __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;

/*
 * Search kernel result data structure
 */

struct search_result
{
    // One word for gid and 8 for mix hash
    uint32_t gid;
    uint32_t mix[8];
};

struct search_results
{
    search_result result;
    uint32_t count = 0;
};

struct debug_data
{
    bool computed;
    uint64_t nonce;
    uint32_t seed;
    uint32_t access_offsets[32];
    uint32_t access_mix_words[32];
    uint32_t access_words_mixed[64][32];
    uint32_t access_dag[64][32];
    uint64_t after_init_state[12];
    uint64_t mix_after_init[16];
    uint64_t mix_after_access[16];
    uint32_t mix_hash_after_reduce[8];
    uint64_t before_final_state[12];
    uint64_t reccak_res;
    uint64_t swab_res;
};

/*
 * Xilinx specific properties for miner kernels
 */

// Memory size for single HBM bank
#define XILINX_MEMORY_PER_HBM_BANK 256

// Amount of iterations will be performed for single search kernel execution
#define MAX_NONCE_SEARCH 8192 * 128

/*
 * FNV function and properties
 */

#define FNV_PRIME 0x01000193U

template<typename T>
auto fnv_fn(T x, T y) -> T{
#pragma HLS pipeline
    return (x) * FNV_PRIME ^ (y);
}

/*
 * Copy memory loop macros
 */
#define copy(dst, src, count) \
    for (int i = 0; i != count; ++i) { \
        (dst)[i] = (src)[i]; \
    }

/*
 * GPU-specific data structures
 */

struct uint2
{
    uint32_t x;
    uint32_t y;
};
using uint2_t = uint2;

struct uint4
{
    uint64_t d[2];
    uint32_t w[4];
};
using uint4_t = uint4;

/*
 * Ethash data structures
 */

using hash32_t = union
{
    uint4 uint4s[32 / sizeof(uint4)]; // 2
    uint32_t words[32 / sizeof(uint32_t)]; // 8
    uint64_t data64[32 / sizeof(uint64_t)]; // 4
};

using hash64_t = union
{
    uint64_t data64[64 / sizeof(uint64_t)];
    uint32_t words[64 / sizeof(uint32_t)];
    uint2_t uint2s[64 / sizeof(uint2_t)];
    uint4_t uint4s[64 / sizeof(uint4_t)];
};

using hash128_t = union
{
    uint64_t data64[128 / sizeof(uint64_t)]; // 16
    uint32_t words[128 / sizeof(uint32_t)]; // 32
    uint2_t uint2s[128 / sizeof(uint2_t)]; // 16
    uint4_t uint4s[128 / sizeof(uint4_t)]; // 8
};

union hash256_t
{
    uint64_t word64s[4];
    uint32_t word32s[8];
    uint8_t bytes[32];
    char str[32];
};

union hash512_t
{
    uint64_t word64s[8];
    uint32_t word32s[16];
    uint8_t bytes[64];
    char str[64];
};

union hash1024_t
{
    union hash512_t hash512s[2];
    uint64_t word64s[16];
    uint32_t word32s[32];
    uint8_t bytes[128];
    char str[128];
};

/*
 * Xilinx constants
 */

static auto constexpr dag_elements_per_hbm_bank = (XILINX_MEMORY_PER_HBM_BANK * 1024 * 1024) / sizeof(hash128_t);
static constexpr uint32_t elements_per_hbm_bank = (XILINX_MEMORY_PER_HBM_BANK * 1024 * 1024) / sizeof(hash64_t);

/*
 * Keccak hashing algorithm specific
 *
 * Reference: https://habr.com/ru/post/159073/
 */

static unsigned char const keccak_r[25] = {
        0, 36, 3, 41, 18,
        1, 44, 10, 45, 2,
        62, 6, 43, 15, 61,
        28, 55, 25, 21, 56,
        27, 20, 39, 8, 14
};

static uint64_t keccak_rc[24] = {
        0x0000000000000001,
        0x0000000000008082,
        0x800000000000808A,
        0x8000000080008000,
        0x000000000000808B,
        0x0000000080000001,
        0x8000000080008081,
        0x8000000000008009,
        0x000000000000008A,
        0x0000000000000088,
        0x0000000080008009,
        0x000000008000000A,
        0x000000008000808B,
        0x800000000000008B,
        0x8000000000008089,
        0x8000000000008003,
        0x8000000000008002,
        0x8000000000000080,
        0x000000000000800A,
        0x800000008000000A,
        0x8000000080008081,
        0x8000000000008080,
        0x0000000080000001,
        0x8000000080008008
};

/*
 * Last round constants of SHA-512 for keccak init step
 */
static uint16_t sha_512_init_fr_theta_constants[10] = {
        0, 10, 6, 16, 12, 22, 3, 18, 9, 24
};

/*
 * Indexing utilities
 */

#define index_of(i, j) j * 5 + i

/*
 * CUDA specific utilities
 */

#define cuda_swab64(x)                                          \
    ((uint64_t)((((uint64_t)(x)&0xff00000000000000ULL) >> 56) | \
                (((uint64_t)(x)&0x00ff000000000000ULL) >> 40) | \
                (((uint64_t)(x)&0x0000ff0000000000ULL) >> 24) | \
                (((uint64_t)(x)&0x000000ff00000000ULL) >> 8) |  \
                (((uint64_t)(x)&0x00000000ff000000ULL) << 8) |  \
                (((uint64_t)(x)&0x0000000000ff0000ULL) << 24) | \
                (((uint64_t)(x)&0x000000000000ff00ULL) << 40) | \
                (((uint64_t)(x)&0x00000000000000ffULL) << 56)))

/*
 * Hashing functions
 */

static uint64_t ROT(uint64_t x, unsigned char n, unsigned char w) {
    return ((x << (n % w)) | (x >> (w - (n % w))));
}

void SHA3_512_round_init_last(uint64_t a[25], unsigned char rc_i) {
    uint64_t c[5]{};
    uint64_t d[5]{};
    uint64_t b[25]{};

#pragma HLS ARRAY_PARTITION variable=a complete dim=0
#pragma HLS ARRAY_PARTITION variable=c complete dim=0
#pragma HLS ARRAY_PARTITION variable=d complete dim=0
#pragma HLS ARRAY_PARTITION variable=b complete dim=0

    // theta step
    for (auto i = 0; i < 5; i++){
        c[i] = a[index_of(i, 0)] ^ a[index_of(i, 1)] ^ a[index_of(i, 2)] ^ a[index_of(i, 3)] ^ a[index_of(i, 4)];
    }

    for (auto i = 0; i < 5; i++){
        d[i] = c[(i + 4) % 5] ^ ROT(c[(i + 1) % 5], 1, 64);
    }
    for (auto i = 0; i < 10; i++){
        auto index = sha_512_init_fr_theta_constants[i];
        a[index] ^= d[i / 2];
    }

    // rho and pi steps
    for (auto i = 0; i < 5; i++){
        for (auto j = 0; j < 5; j++) {
            auto b_index = index_of(j, (2 * i + 3 * j) % 5);
            if (b_index < 10) {
                b[b_index] = ROT(a[index_of(i, j)], keccak_r[index_of(j, i)], 64);
            }
        }
    }

    // copy from b
    for (auto i = 8; i < 10; i++){
        a[i] = b[i];
    }

    // chi step
    for (auto i = 0; i < 5; i++){
        for (auto j = 0; j < 5; j++) {
            if (index_of(i, j) < 8) {
                a[index_of(i, j)] = b[index_of(i, j)] ^ ((~b[index_of((i + 1) % 5, j)]) & b[index_of((i + 2) % 5, j)]);
            }
        }
    }

    // iota step
    a[0] ^= keccak_rc[rc_i];
}

void SHA3_512_round(uint64_t a[25], unsigned char rc_i) {
    uint64_t c[5]{};
    uint64_t d[5]{};
    uint64_t b[25]{};

#pragma HLS ARRAY_PARTITION variable=a complete dim=0
#pragma HLS ARRAY_PARTITION variable=c complete dim=0
#pragma HLS ARRAY_PARTITION variable=d complete dim=0
#pragma HLS ARRAY_PARTITION variable=b complete dim=0

    // theta step
    for (auto i = 0; i < 5; i++){
        c[i] = a[index_of(i, 0)] ^ a[index_of(i, 1)] ^ a[index_of(i, 2)] ^ a[index_of(i, 3)] ^ a[index_of(i, 4)];
    }

    for (auto i = 0; i < 5; i++){
        d[i] = c[(i + 4) % 5] ^ ROT(c[(i + 1) % 5], 1, 64);
    }
    for (auto i = 0; i < 5; i++){
        for (auto j = 0; j < 5; j++) {
            a[index_of(i, j)] ^= d[i];
        }
    }

    // rho and pi steps
    for (auto i = 0; i < 5; i++){
        for (auto j = 0; j < 5; j++) {
            b[index_of(j, (2 * i + 3 * j) % 5)] = ROT(a[index_of(i, j)], keccak_r[index_of(j, i)], 64);
        }
    }

    // chi step
    for (auto i = 0; i < 5; i++){
        for (auto j = 0; j < 5; j++) {
            a[index_of(i, j)] = b[index_of(i, j)] ^ ((~b[index_of((i + 1) % 5,j)]) & b[index_of((i + 2) % 5,j)]);
        }
    }

    // iota step
    a[0] ^= keccak_rc[rc_i];
}

void SHA3_512(uint64_t a[8])
{
//#pragma HLS INTERFACE m_axi depth=8 port=a offset=slave
    uint64_t res[25];
#pragma HLS ARRAY_PARTITION variable=res complete dim=0
// BRAM

    for (auto i = 0; i < 8; i++) {
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS unroll
#pragma HLS DEPENDENCE variable=res intra RAW true
#pragma HLS DEPENDENCE variable=a intra RAW true
        res[i] = a[i];
    }

    for (auto i = 8; i < 25; i++) {
#pragma HLS loop_tripcount min = 16 max = 16
#pragma HLS unroll
#pragma HLS DEPENDENCE variable=res intra RAW true
        res[i] = 0;
    }

    res[8] = 0x8000000000000001;

    for (auto i = 0; i < 24; i++) {
#pragma HLS loop_tripcount min = 24 max = 24
#pragma HLS DEPENDENCE variable=res intra RAW true
// https://www.xilinx.com/html_docs/xilinx2021_1/vitis_doc/hls_pragmas.html#dxe1504034360397
        SHA3_512_round(res, i);
    }

    for (auto i = 0; i < 8; i++) {
#pragma HLS loop_tripcount min = 8 max = 8
#pragma HLS unroll
#pragma HLS DEPENDENCE variable=res intra RAW true
#pragma HLS DEPENDENCE variable=a intra RAW true
        a[i] = res[i];
    }
}

void keccak_f1600_init(uint64_t* in_state, uint64_t nonce, uint64_t* header) {
    uint64_t res[25];
#pragma HLS ARRAY_PARTITION variable=res complete dim=0

    for (auto i = 0; i < 4; i++) {
        res[i] = header[i];
    }

    res[4] = nonce; // in_state[4];
    res[5] = 0x0000000000000001;

    for (auto i = 6; i < 25; i++){
        res[i] = 0;
    }

    res[8] = 0x8000000000000000;

    for (auto i = 0; i < 24; i++) {
        if (i == 23) {
            SHA3_512_round_init_last(res, 23);
        } else {
            SHA3_512_round(res, i);
        }
    }

    for (auto i = 0; i < 12; i++) {
        in_state[i] = res[i];
    }
}

uint64_t keccak_f1600_final(uint64_t* in_state) {
    uint64_t res[25];
#pragma HLS ARRAY_PARTITION variable=res complete dim=0

    for (auto i = 0; i < 12; i++) {
        res[i] = in_state[i];
    }

    res[12] = 0x0000000000000001;

    for (auto i = 13; i < 25; i++){
        res[i] = 0;
    }

    res[16] = 0x8000000000000000;

    for (auto i = 0; i < 24; i++) {
#pragma HLS loop_tripcount min = 23 max = 23
#pragma HLS unroll
        SHA3_512_round(res, i);
    }

    return res[0];
}

/*
 * Kernel helper functions below
 */

static hash1024_t* select_dag_bank(
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
        uint32_t offset
){
    const auto index = offset % dag_elements_per_hbm_bank;

    switch (offset / dag_elements_per_hbm_bank) {
        case 0: return (hash1024_t*)&dag0[index];
        case 1: return (hash1024_t*)&dag1[index];
        case 2: return (hash1024_t*)&dag2[index];
        case 3: return (hash1024_t*)&dag3[index];
        case 4: return (hash1024_t*)&dag4[index];
        case 5: return (hash1024_t*)&dag5[index];
        case 6: return (hash1024_t*)&dag6[index];
        case 7: return (hash1024_t*)&dag7[index];
        case 8: return (hash1024_t*)&dag8[index];
        case 9: return (hash1024_t*)&dag9[index];
        case 10: return (hash1024_t*)&dag10[index];
        case 11: return (hash1024_t*)&dag11[index];
        case 12: return (hash1024_t*)&dag12[index];
        case 13: return (hash1024_t*)&dag13[index];
        case 14: return (hash1024_t*)&dag14[index];
        case 15: return (hash1024_t*)&dag15[index];
        case 16: return (hash1024_t*)&dag16[index];
        case 17: return (hash1024_t*)&dag17[index];
        case 18: return (hash1024_t*)&dag18[index];
        case 19: return (hash1024_t*)&dag19[index];
        case 20: return (hash1024_t*)&dag20[index];
        case 21: return (hash1024_t*)&dag21[index];
        case 22: return (hash1024_t*)&dag22[index];
        default: return (hash1024_t*)&dag23[index];
    }
}
