#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include "utils.h"

/* ------------------ bit helpers ------------------ */
#define GET_BITS(byte, lsb, width) (((byte) >> (lsb)) & ((1u << (width)) - 1u))
#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b) & 0x0F)

/* ------------------ time mapping (4b -> us) ------------------ */
static const int TIME_US[16] = {
    100, 200, 300, 400,
    600, 800, 1000, 1200,
    1400, 2000, 2400, 3200,
    4000, 5200, 6400, 8000
};

int nibble_to_us(uint8_t n){
    return TIME_US[n & 0x0F];
}

/* ------------------ distance conversion ------------------ */
/*
 * Distance (cm) from time-of-flight (us):
 *   d = v * t / 2
 * Default v = 343 m/s
 * d_cm = t_us * (34300 cm/s) * 1e-6 / 2 = t_us * 0.01715
 */

#ifndef SPEED_OF_SOUND_M_S
#define SPEED_OF_SOUND_M_S 343.0
#endif

double tof_us_to_cm(int t_us){
    return (double)t_us * ((SPEED_OF_SOUND_M_S * 100.0) / 1e6) / 2.0;
}

/* ------------------ Threshold profile extraction (Excel-aligned) ------------------
 * Px_THR_0..5  : 12 nibbles of time (T1..T12) -> mapped using TIME_US[]
 * Px_THR_6..10 : L1..L8 packed as 8 values of 5 bits (total 40 bits = 5 bytes)
 * Px_THR_11..14: L9..L12 stored as full bytes
 * Px_THR_15    : reserved/offsets (not used in profile curve here)
 *
 * Indexing:
 *   reg[0]  = REG1
 *   reg[23] = REG24 (P1_THR_0)
 *   reg[39] = REG40 (P2_THR_0)
 */

void extract_T12_us(const uint8_t reg[55], int is_p2, int t_us[12]){
    int base = is_p2 ? 39 : 23; // P2: REG40..45, P1: REG24..29
    for (int i = 0; i < 6; i++){
        uint8_t b = reg[base + i];
        t_us[i*2 + 0] = nibble_to_us(HI_NIBBLE(b)); // T(1+2i)
        t_us[i*2 + 1] = nibble_to_us(LO_NIBBLE(b)); // T(2+2i)
    }
}

void extract_L1_L8_5bit(const uint8_t reg[55], int is_p2, int L[8]){
    int base = is_p2 ? 45 : 29; // P2: REG46..50, P1: REG30..34

    // Concatenate 5 bytes into a 40-bit MSB-first bitstream
    uint64_t bits = 0;
    for (int i = 0; i < 5; i++){
        bits = (bits << 8) | (uint64_t)reg[base + i];
    }

    // Extract 8 groups of 5 bits from MSB to LSB: [L1][L2]...[L8]
    for (int i = 0; i < 8; i++){
        int shift = (40 - 5) - (i * 5);
        L[i] = (int)((bits >> shift) & 0x1F); // 0..31
    }
}

void extract_L9_L12_8bit(const uint8_t reg[55], int is_p2, int L[4]){
    int base = is_p2 ? 50 : 34; // P2: REG51..54, P1: REG35..38
    for (int i = 0; i < 4; i++){
        L[i] = reg[base + i]; // 0..255
    }
}

double value_to_pct(int stage /*1..12*/, int raw){
    if (stage <= 8) return (raw / 31.0) * 100.0;   // 5-bit
    return (raw / 255.0) * 100.0;                  // 8-bit
}

/* Prints decoded L1..L8 (raw and %) for P1 or P2 */
void print_L1_L8_decoded(const uint8_t reg[55], int is_p2){
    int L[8];
    extract_L1_L8_5bit(reg, is_p2, L);

    printf("    Decoded %s L1..L8 (5-bit):\n", is_p2 ? "P2" : "P1");
    for (int i = 0; i < 8; i++){
        int stage = i + 1; // 1..8
        double pct = value_to_pct(stage, L[i]);
        printf("      L%d = %2d  (%.2f%%)\n", i + 1, L[i], pct);
    }
}

/* ------------------ hex parsing ------------------ */
int hexval(char c){
    if ('0'<=c && c<='9') return c-'0';
    if ('a'<=c && c<='f') return 10 + (c-'a');
    if ('A'<=c && c<='F') return 10 + (c-'A');
    return -1;
}

// Converts "AA BB" or "AABB" to bytes. Returns byte count or -1 on error.
int parse_hex_bytes(const char *s, uint8_t *out, int max_out){
    int n = 0;
    while (*s){
        while (*s && (isspace((unsigned char)*s) || *s==':' || *s=='-' || *s==',')) s++;
        if (!*s) break;

        int hi = hexval(*s++);
        while (*s && isspace((unsigned char)*s)) s++; // allow "A A"
        int lo = hexval(*s++);
        if (hi < 0 || lo < 0) return -1;
        if (n >= max_out) return -1;

        out[n++] = (uint8_t)((hi<<4) | lo);
    }
    return n;
}
