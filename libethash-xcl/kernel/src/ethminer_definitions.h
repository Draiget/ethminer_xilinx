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
    uint64_t data64[128 / sizeof(uint64_t)];
    uint32_t words[128 / sizeof(uint32_t)];
    uint2_t uint2s[128 / sizeof(uint2_t)];
    uint4_t uint4s[128 / sizeof(uint4_t)];
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

static uint64_t ROT(uint64_t x, unsigned char n, unsigned char w);

void SHA3_512(uint64_t a[8]);
void SHA3_512_round(uint64_t a[25], unsigned char rc_i);
