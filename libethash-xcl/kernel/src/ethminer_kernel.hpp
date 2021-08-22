#pragma once

#include <hls_math.h>

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

#define XILINX_MEMORY_PER_HBM_BANK 256

#define ETHASH_MIX_BYTES 128
#define ETHASH_DATASET_PARENTS 256
#define MIX_WORDS (ETHASH_MIX_BYTES / 4)
#define NODE_WORDS (64 / 4)

#define FNV_PRIME 0x01000193U
#define fnv(x, y) ((x)*FNV_PRIME ^ (y))

template<typename T>
auto fnv_fn(T x, T y) -> T{
    return (x) * FNV_PRIME ^ (y);
}

#define copy(dst, src, count)        \
    for (int i = 0; i != count; ++i) \
    {                                \
        (dst)[i] = (src)[i];         \
    }

struct uint2
{
    uint32_t x;
    uint32_t y;

    uint2() {}
};
using uint2_t = uint2;

struct uint4 {
    uint64_t d[2];
    uint32_t w[4];
};
typedef uint4 uint4_t;

using hash32_t = struct
{
    uint4_t uint4s[1]; // 32 / sizeof(uint4_t)
};

using hash128_t = union
{
    uint64_t data64[128 / sizeof(uint64_t)];
    uint32_t words[128 / sizeof(uint32_t)];
    uint2_t uint2s[128 / sizeof(uint2_t)];
    uint4_t uint4s[128 / sizeof(uint4_t)];
};

using hash64_t = union
{
    uint64_t data64[64 / sizeof(uint64_t)];
    uint32_t words[64 / sizeof(uint32_t)];
    uint2_t uint2s[64 / sizeof(uint2_t)];
    uint4_t uint4s[64 / sizeof(uint4_t)];
};

static auto make_uint2(uint32_t x, uint32_t y) -> uint2_t {
    uint2_t r;
    r.x = x;
    r.y = y;
    return r;
}

/*
 * More about Keccak: https://habr.com/ru/post/159073/
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

static uint64_t ROT(uint64_t x, unsigned char n, unsigned char w) {
    return ((x << (n % w)) | (x >> (w - (n % w))));
}

#define index_of(i, j) j * 5 + i

void SHA3_512_round(uint64_t a[25], unsigned char rc_i) {
    uint64_t c[5];
    uint64_t d[5];
    uint64_t b[25];
#pragma HLS BIND_STORAGE variable=c type=ram_2p impl=auto
#pragma HLS BIND_STORAGE variable=d type=ram_2p impl=auto
#pragma HLS BIND_STORAGE variable=b type=ram_2p impl=auto
// BRAM
#pragma HLS ARRAY_PARTITION variable=a complete dim=0

    // theta step
    for (auto i = 0; i < 5; i++){
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
        c[i] = a[index_of(i, 0)] ^ a[index_of(i, 1)] ^ a[index_of(i, 2)] ^ a[index_of(i, 3)] ^ a[index_of(i, 4)];
    }

    for (auto i = 0; i < 5; i++){
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
        d[i] = c[(i + 4) % 5] ^ ROT(c[(i + 1) % 5], 1, 64);
    }
    for (auto i = 0; i < 5; i++){
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
        for (auto j = 0; j < 5; j++) {
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
            a[index_of(i, j)] ^= d[i];
        }
    }

    // rho and pi steps
    for (auto i = 0; i < 5; i++){
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
        for (auto j = 0; j < 5; j++) {
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
            b[index_of(j, (2 * i + 3 * j) % 5)] = ROT(a[index_of(i, j)], keccak_r[index_of(j, i)], 64);
        }
    }

    // chi step
    for (auto i = 0; i < 5; i++){
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
        for (auto j = 0; j < 5; j++) {
#pragma HLS loop_tripcount min = 5 max = 5
#pragma HLS unroll
            a[index_of(i, j)] = b[index_of(i, j)] ^ ((~b[index_of((i + 1) % 5, j)]) & b[index_of((i + 2) % 5, j)]);
        }
    }

    // iota step
    a[0] ^= keccak_rc[rc_i];
}

void SHA3_512(uint64_t a[8])
{
//#pragma HLS INTERFACE m_axi depth=8 port=a offset=slave
    uint64_t res[25];
//#pragma HLS BIND_STORAGE variable=res type=ram_2p impl=auto
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

