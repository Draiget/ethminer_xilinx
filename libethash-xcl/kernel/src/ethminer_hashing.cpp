#pragma once

#include "ethminer_definitions.h"

static uint64_t ROT(uint64_t x, unsigned char n, unsigned char w) {
    return ((x << (n % w)) | (x >> (w - (n % w))));
}

void SHA3_512_round(uint64_t a[25], unsigned char rc_i) {
    uint64_t c[5];
    uint64_t d[5];
    uint64_t b[25];

#pragma HLS BIND_STORAGE variable=c type=ram_2p impl=auto
#pragma HLS BIND_STORAGE variable=d type=ram_2p impl=auto
#pragma HLS BIND_STORAGE variable=b type=ram_2p impl=auto

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
