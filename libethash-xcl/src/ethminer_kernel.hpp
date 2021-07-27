#pragma once

#include <hls_math.h>

// #include <stdint.h>
// #include <clc.h>
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

#define ETHASH_DATASET_PARENTS 256
#define NODE_WORDS (64 / 4)

#define FNV_PRIME 0x01000193U
#define fnv(x, y) ((x)*FNV_PRIME ^ (y))

#define copy(dst, src, count)        \
    for (int i = 0; i != count; ++i) \
    {                                \
        (dst)[i] = (src)[i];         \
    }

// __attribute__ ((aligned(2*sizeof(uint32_t))))
typedef union uint2
{
    uint64_t d;
    uint32_t x;
    uint32_t y;

    uint2() {}
    uint2(uint32_t x, uint32_t y){
        this->x = x;
        this->y = y;
    }
} uint2_t;

//  __attribute__ ((aligned(4 * sizeof(uint32_t))))
typedef union uint4
{
    uint64_t d;
    uint64_t e;

    uint32_t x;
    uint32_t y;
    uint32_t z;
    uint32_t w;
} uint4_t;

typedef struct
{
    uint4_t uint4s[1]; // 32 / sizeof(uint4_t)
} hash32_t;

typedef union
{
    uint64_t data64[128 / sizeof(uint64_t)];
    uint32_t words[128 / sizeof(uint32_t)];
    uint2_t uint2s[128 / sizeof(uint2_t)];
    uint4_t uint4s[128 / sizeof(uint4_t)];
} hash128_t;

typedef union
{
    uint64_t data64[64 / sizeof(uint64_t)];
    uint32_t words[64 / sizeof(uint32_t)];
    uint2_t uint2s[64 / sizeof(uint2_t)];
    uint4_t uint4s[64 / sizeof(uint4_t)];
} hash64_t;

uint2 ROL2(const uint2 v, const int n) {
    uint2 result;
    if (n <= 32) {
        result.y = ((v.y << (n)) | (v.x >> (32 - n)));
        result.x = ((v.x << (n)) | (v.y >> (32 - n)));
    } else {
        result.y = ((v.x << (n - 32)) | (v.y >> (64 - n)));
        result.x = ((v.y << (n - 32)) | (v.x >> (64 - n)));
    }
    return result;
}

uint32_t chi(const uint2 a, const uint2 b, const uint2 c){
    return a.x ^ ((~b.x) & c.x);
}

static uint2_t const keccak_round_constants[24] = {
        { 0x00000001, 0x00000000 }, { 0x00008082, 0x00000000 }, { 0x0000808a, 0x80000000 }, { 0x80008000, 0x80000000 },
        { 0x0000808b, 0x00000000 }, { 0x80000001, 0x00000000 }, { 0x80008081, 0x80000000 }, { 0x00008009, 0x80000000 },
        { 0x0000008a, 0x00000000 }, { 0x00000088, 0x00000000 }, { 0x80008009, 0x00000000 }, { 0x8000000a, 0x00000000 },
        { 0x8000808b, 0x00000000 }, { 0x0000008b, 0x80000000 }, { 0x00008089, 0x80000000 }, { 0x00008003, 0x80000000 },
        { 0x00008002, 0x80000000 }, { 0x00000080, 0x80000000 }, { 0x0000800a, 0x00000000 }, { 0x8000000a, 0x80000000 },
        { 0x80008081, 0x80000000 }, { 0x00008080, 0x80000000 }, { 0x80000001, 0x00000000 }, { 0x80008008, 0x80000000 }
};

static uint2_t make_uint2(uint32_t x, uint32_t y){
    uint2_t r;
    r.x = x;
    r.y = y;
    return r;
}

/*
 * More about Keccak: https://habr.com/ru/post/159073/
 */

#define KECCAK_1600_SHA3_ROUND_COUNT 24

static void SHA3_512(uint2 *a)
{
    uint2_t c[5], u, v;

    for (uint32_t i = 8; i < 25; i++) {
        a[i] = make_uint2(0, 0);
    }

    a[8].x = 1;
    a[8].y = 0x80000000;

    for (int i = 0; i < KECCAK_1600_SHA3_ROUND_COUNT - 1; i++) {
        //#pragma HLS resource variable = i core = RAM_2P_LUTRAM
        //#pragma HLS loop_tripcount min = 1 max = 24
        //#pragma HLS pipeline
        /*
         * ROUND
         * θ 'theta' step
         * C[x] = A[x,0] xor A[x,1] xor A[x,2] xor A[x,3] xor A[x,4];
         */
        c[0].x = a[0].x ^ a[5].x ^ a[10].x ^ a[15].x ^ a[20].x;
        c[1].x = a[1].x ^ a[6].x ^ a[11].x ^ a[16].x ^ a[21].x;
        c[2].x = a[2].x ^ a[7].x ^ a[12].x ^ a[17].x ^ a[22].x;
        c[3].x = a[3].x ^ a[8].x ^ a[13].x ^ a[18].x ^ a[23].x;
        c[4].x = a[4].x ^ a[9].x ^ a[14].x ^ a[19].x ^ a[24].x;

        /*
         * theta: d[i] = c[i+4] ^ rotl(c[i+1],1)
         * theta: a[0,i], a[1,i], .. a[4,i] ^= d[i]
         */
        u.x = c[4].x ^ ROL2(c[1], 1).x;
        a[0].x ^= u.x;
        a[5].x ^= u.x;
        a[10].x ^= u.x;
        a[15].x ^= u.x;
        a[20].x ^= u.x;

        u.x = c[0].x ^ ROL2(c[2], 1).x;
        a[1].x ^= u.x;
        a[6].x ^= u.x;
        a[11].x ^= u.x;
        a[16].x ^= u.x;
        a[21].x ^= u.x;

        u.x = c[1].x ^ ROL2(c[3], 1).x;
        a[2].x ^= u.x;
        a[7].x ^= u.x;
        a[12].x ^= u.x;
        a[17].x ^= u.x;
        a[22].x ^= u.x;

        u.x = c[2].x ^ ROL2(c[4], 1).x;
        a[3].x ^= u.x;
        a[8].x ^= u.x;
        a[13].x ^= u.x;
        a[18].x ^= u.x;
        a[23].x ^= u.x;

        u.x = c[3].x ^ ROL2(c[0], 1).x;
        a[4].x ^= u.x;
        a[9].x ^= u.x;
        a[14].x ^= u.x;
        a[19].x ^= u.x;
        a[24].x ^= u.x;

        /*
         * rho pi: b[..] = rotl(a[..], ..)
         */
        u.x = a[1].x;

        a[1] = ROL2(a[6], 44);
        a[6] = ROL2(a[9], 20);
        a[9] = ROL2(a[22], 61);
        a[22] = ROL2(a[14], 39);
        a[14] = ROL2(a[20], 18);
        a[20] = ROL2(a[2], 62);
        a[2] = ROL2(a[12], 43);
        a[12] = ROL2(a[13], 25);
        a[13] = ROL2(a[19], 8);
        a[19] = ROL2(a[23], 56);
        a[23] = ROL2(a[15], 41);
        a[15] = ROL2(a[4], 27);
        a[4] = ROL2(a[24], 14);
        a[24] = ROL2(a[21], 2);
        a[21] = ROL2(a[8], 55);
        a[8] = ROL2(a[16], 45);
        a[16] = ROL2(a[5], 36);
        a[5] = ROL2(a[3], 28);
        a[3] = ROL2(a[18], 21);
        a[18] = ROL2(a[17], 15);
        a[17] = ROL2(a[11], 10);
        a[11] = ROL2(a[7], 6);
        a[7] = ROL2(a[10], 3);
        a[10] = ROL2(u, 1);

        /*
         * chi: a[i,j] ^= ~b[i,j+1] & b[i,j+2]
         */
        u = a[0];
        v = a[1];
        a[0].x = chi(a[0], a[1], a[2]);
        a[1].x = chi(a[1], a[2], a[3]);
        a[2].x = chi(a[2], a[3], a[4]);
        a[3].x = chi(a[3], a[4], u);
        a[4].x = chi(a[4], u, v);

        u = a[5];
        v = a[6];
        a[5].x = chi(a[5], a[6], a[7]);
        a[6].x = chi(a[6], a[7], a[8]);
        a[7].x = chi(a[7], a[8], a[9]);
        a[8].x = chi(a[8], a[9], u);
        a[9].x = chi(a[9], u, v);

        u = a[10];
        v = a[11];
        a[10].x = chi(a[10], a[11], a[12]);
        a[11].x = chi(a[11], a[12], a[13]);
        a[12].x = chi(a[12], a[13], a[14]);
        a[13].x = chi(a[13], a[14], u);
        a[14].x = chi(a[14], u, v);

        u = a[15];
        v = a[16];
        a[15].x = chi(a[15], a[16], a[17]);
        a[16].x = chi(a[16], a[17], a[18]);
        a[17].x = chi(a[17], a[18], a[19]);
        a[18].x = chi(a[18], a[19], u);
        a[19].x = chi(a[19], u, v);

        u = a[20];
        v = a[21];
        a[20].x = chi(a[20], a[21], a[22]);
        a[21].x = chi(a[21], a[22], a[23]);
        a[22].x = chi(a[22], a[23], a[24]);
        a[23].x = chi(a[23], a[24], u);
        a[24].x = chi(a[24], u, v);

        /*
         * iota: a[0,0] ^= round constant
         */
        a[0].x ^= keccak_round_constants[i].x;
    }

    /*
     * theta: c = a[0,i] ^ a[1,i] ^ .. a[4,i]
     */
    c[0].x = a[0].x ^ a[5].x ^ a[10].x ^ a[15].x ^ a[20].x;
    c[1].x = a[1].x ^ a[6].x ^ a[11].x ^ a[16].x ^ a[21].x;
    c[2].x = a[2].x ^ a[7].x ^ a[12].x ^ a[17].x ^ a[22].x;
    c[3].x = a[3].x ^ a[8].x ^ a[13].x ^ a[18].x ^ a[23].x;
    c[4].x = a[4].x ^ a[9].x ^ a[14].x ^ a[19].x ^ a[24].x;

    /*
     * theta: d[i] = c[i+4] ^ rotl(c[i+1],1)
     * theta: a[0,i], a[1,i], .. a[4,i] ^= d[i]
     */

    u.x = c[4].x ^ ROL2(c[1], 1).x;
    a[0].x ^= u.x;
    a[10].x ^= u.x;

    u.x = c[0].x ^ ROL2(c[2], 1).x;
    a[6].x ^= u.x;
    a[16].x ^= u.x;

    u.x = c[1].x ^ ROL2(c[3], 1).x;
    a[12].x ^= u.x;
    a[22].x ^= u.x;

    u.x = c[2].x ^ ROL2(c[4], 1).x;
    a[3].x ^= u.x;
    a[18].x ^= u.x;

    u.x = c[3].x ^ ROL2(c[0], 1).x;
    a[9].x ^= u.x;
    a[24].x ^= u.x;

    /*
     * rho pi: b[..] = rotl(a[..], ..)
     */

    u = a[1];

    a[1].x = ROL2(a[6], 44).x;
    a[6].x = ROL2(a[9], 20).x;
    a[9].x = ROL2(a[22], 61).x;
    a[2].x = ROL2(a[12], 43).x;
    a[4].x = ROL2(a[24], 14).x;
    a[8].x = ROL2(a[16], 45).x;
    a[5].x = ROL2(a[3], 28).x;
    a[3].x = ROL2(a[18], 21).x;
    a[7].x = ROL2(a[10], 3).x;

    /*
     * chi: a[i,j] ^= ~b[i,j+1] & b[i,j+2]
     */

    u = a[0];
    v = a[1];
    a[0].x = chi(a[0], a[1], a[2]);
    a[1].x = chi(a[1], a[2], a[3]);
    a[2].x = chi(a[2], a[3], a[4]);
    a[3].x = chi(a[3], a[4], u);
    a[4].x = chi(a[4], u, v);
    a[5].x = chi(a[5], a[6], a[7]);
    a[6].x = chi(a[6], a[7], a[8]);
    a[7].x = chi(a[7], a[8], a[9]);

    /*
     * iota: a[0,0] ^= round constant
     */
    a[0].x ^= keccak_round_constants[23].x;
}
