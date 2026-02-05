#ifndef HERMES_UTILS_H
#define HERMES_UTILS_H

#include <stdint.h>

#define GET_BITS(byte, lsb, width) (((byte) >> (lsb)) & ((1u << (width)) - 1u))
#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b) & 0x0F)

int nibble_to_us(uint8_t n);
double tof_us_to_cm(int t_us);

void extract_T12_us(const uint8_t reg[55], int is_p2, int t_us[12]);
void extract_L1_L8_5bit(const uint8_t reg[55], int is_p2, int L[8]);
void extract_L9_L12_8bit(const uint8_t reg[55], int is_p2, int L[4]);
void print_L1_L8_decoded(const uint8_t reg[55], int is_p2);
double value_to_pct(int stage /*1..12*/, int raw);
int parse_hex_bytes(const char *line, uint8_t *buf, int max_bytes);
int hexval(char c);

#endif
