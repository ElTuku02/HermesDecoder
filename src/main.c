/*
 * HermesDecoder - PGA460 config frame decoder (C) + CSV export
 *
 * Frame format (your protocol):
 *   [0..1]  : prefix/mode (e.g., 0x5E02) -> ignored for register mapping
 *   [2..56] : 55 bytes -> REG1..REG55 in fixed order
 *
 * Default: prints "raw decode" (registers + bitfields)
 * --csv   : generates threshold profile CSVs (P1 and P2)
 *
 * CSV columns:
 *   stage,delta_us,t_us,dist_cm,value_pct,value_raw
 *
 * Extra raw decode feature:
 *   - After Px_THR_10 (REG34 for P1, REG50 for P2) it prints decoded L1..L8
 *     (5-bit packed values) both raw and percentage.
 *
 * Build:
 *   gcc -O2 -Wall -Wextra hermes_decoder.c -o HermesDecoder
 *
 * Run:
 *   echo "<HEX>" | ./HermesDecoder
 *   echo "<HEX>" | ./HermesDecoder --csv
 *   echo "<HEX>" | ./HermesDecoder --csv prefix
 */

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

/* ------------------ bit helpers ------------------ */
#define GET_BITS(byte, lsb, width) (((byte) >> (lsb)) & ((1u << (width)) - 1u))
#define HI_NIBBLE(b) (((b) >> 4) & 0x0F)
#define LO_NIBBLE(b) ((b) & 0x0F)

/* ------------------ hex parsing ------------------ */
static int hexval(char c){
    if ('0'<=c && c<='9') return c-'0';
    if ('a'<=c && c<='f') return 10 + (c-'a');
    if ('A'<=c && c<='F') return 10 + (c-'A');
    return -1;
}

// Converts "AA BB" or "AABB" to bytes. Returns byte count or -1 on error.
static int parse_hex_bytes(const char *s, uint8_t *out, int max_out){
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

/* ------------------ time mapping (4b -> us) ------------------ */
static const int TIME_US[16] = {
    100, 200, 300, 400,
    600, 800, 1000, 1200,
    1400, 2000, 2400, 3200,
    4000, 5200, 6400, 8000
};

static int nibble_to_us(uint8_t n){
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

static double tof_us_to_cm(int t_us){
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

static void extract_T12_us(const uint8_t reg[55], int is_p2, int t_us[12]){
    int base = is_p2 ? 39 : 23; // P2: REG40..45, P1: REG24..29
    for (int i = 0; i < 6; i++){
        uint8_t b = reg[base + i];
        t_us[i*2 + 0] = nibble_to_us(HI_NIBBLE(b)); // T(1+2i)
        t_us[i*2 + 1] = nibble_to_us(LO_NIBBLE(b)); // T(2+2i)
    }
}

static void extract_L1_L8_5bit(const uint8_t reg[55], int is_p2, int L[8]){
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

static void extract_L9_L12_8bit(const uint8_t reg[55], int is_p2, int L[4]){
    int base = is_p2 ? 50 : 34; // P2: REG51..54, P1: REG35..38
    for (int i = 0; i < 4; i++){
        L[i] = reg[base + i]; // 0..255
    }
}

static double value_to_pct(int stage /*1..12*/, int raw){
    if (stage <= 8) return (raw / 31.0) * 100.0;   // 5-bit
    return (raw / 255.0) * 100.0;                  // 8-bit
}

/* Prints decoded L1..L8 (raw and %) for P1 or P2 */
static void print_L1_L8_decoded(const uint8_t reg[55], int is_p2){
    int L[8];
    extract_L1_L8_5bit(reg, is_p2, L);

    printf("    Decoded %s L1..L8 (5-bit):\n", is_p2 ? "P2" : "P1");
    for (int i = 0; i < 8; i++){
        int stage = i + 1; // 1..8
        double pct = value_to_pct(stage, L[i]);
        printf("      L%d = %2d  (%.2f%%)\n", i + 1, L[i], pct);
    }
}

static int write_th_profile_csv(const char *path, const uint8_t reg[55], int is_p2){
    int delta_us[12];
    int L5[8], L8[4];
    int value_raw[12];

    extract_T12_us(reg, is_p2, delta_us);
    extract_L1_L8_5bit(reg, is_p2, L5);
    extract_L9_L12_8bit(reg, is_p2, L8);

    for (int i = 0; i < 8; i++) value_raw[i] = L5[i];
    for (int i = 0; i < 4; i++) value_raw[8 + i] = L8[i];

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "stage,delta_us,t_us,dist_cm,value_pct,value_raw\n");

    int acc_us = 0;
    for (int i = 0; i < 12; i++){
        int stage = i + 1;
        acc_us += delta_us[i];

        double dist_cm = tof_us_to_cm(acc_us);
        double pct = value_to_pct(stage, value_raw[i]);

        fprintf(f, "%d,%d,%d,%.4f,%.2f,%d\n",
                stage,
                delta_us[i],
                acc_us,
                dist_cm,
                pct,
                value_raw[i]);
    }

    fclose(f);
    return 0;
}

static int write_tvg_csv(const char *path, const uint8_t reg[55]){
    int t_us[6];
    int g_raw[5];
    int g_max[5];

    /* Extraer tiempos TVG (T0..T5) */
    uint8_t b0 = reg[0]; // TVGAIN0
    uint8_t b1 = reg[1]; // TVGAIN1
    uint8_t b2 = reg[2]; // TVGAIN2

    t_us[0] = nibble_to_us(HI_NIBBLE(b0));
    t_us[1] = nibble_to_us(LO_NIBBLE(b0));
    t_us[2] = nibble_to_us(HI_NIBBLE(b1));
    t_us[3] = nibble_to_us(LO_NIBBLE(b1));
    t_us[4] = nibble_to_us(HI_NIBBLE(b2));
    t_us[5] = nibble_to_us(LO_NIBBLE(b2));

    /* Extraer ganancias TVG (G1..G5) - SEGÚN EXCEL (G2 y G3 PARTIDOS) */
    uint8_t tvg3 = reg[3]; // TVGAIN3
    uint8_t tvg4 = reg[4]; // TVGAIN4
    uint8_t tvg5 = reg[5]; // TVGAIN5
    uint8_t tvg6 = reg[6]; // TVGAIN6

    // Partes de G2 y G3 (ver Excel)
    int g2_hi2 = (int)GET_BITS(tvg3, 0, 2); // TVGAIN3 b1..b0 -> G2[5:4]
    int g2_lo4 = (int)GET_BITS(tvg4, 4, 4); // TVGAIN4 b7..b4 -> G2[3:0]

    int g3_hi4 = (int)GET_BITS(tvg4, 0, 4); // TVGAIN4 b3..b0 -> G3[5:2]
    int g3_lo2 = (int)GET_BITS(tvg5, 6, 2); // TVGAIN5 b7..b6 -> G3[1:0]

    // Ganancias finales (todas 6 bits)
    g_raw[0] = (int)GET_BITS(tvg3, 2, 6);        g_max[0] = 63; // G1 = TVGAIN3 b7..b2
    g_raw[1] = (g2_hi2 << 4) | g2_lo4;           g_max[1] = 63; // G2 (6b) = (hi2<<4) | lo4
    g_raw[2] = (g3_hi4 << 2) | g3_lo2;           g_max[2] = 63; // G3 (6b) = (hi4<<2) | lo2
    g_raw[3] = (int)GET_BITS(tvg5, 0, 6);        g_max[3] = 63; // G4 = TVGAIN5 b5..b0
    g_raw[4] = (int)GET_BITS(tvg6, 2, 6);        g_max[4] = 63; // G5 = TVGAIN6 b7..b2

    // Flags TVGAIN6
    int reserved   = (int)GET_BITS(tvg6, 1, 1);
    int freq_shift = (int)GET_BITS(tvg6, 0, 1);
    if (reserved != 0){
        printf("WARNING: TVGAIN6 RESERVED bit != 0 (%d)\n", reserved);
    }
    (void)freq_shift; // por ahora no se usa

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "stage,delta_us,t_us,dist_cm_tvg,gain_pct,gain_raw,gain_raw_max\n");

    int acc_us = 0;
    for (int i = 0; i < 6; i++){
        acc_us += t_us[i];

        int gain_idx = (i < 5) ? i : 4; // último tramo mantiene G5
        double gain_pct = (g_raw[gain_idx] / (double)g_max[gain_idx]) * 100.0;
        double dist_cm_tvg = tof_us_to_cm(acc_us);

        fprintf(
            f,
            "%d,%d,%d,%.4f,%.2f,%d,%d\n",
            i + 1,
            t_us[i],
            acc_us,
            dist_cm_tvg,
            gain_pct,
            g_raw[gain_idx],
            g_max[gain_idx]
        );
    }

    fclose(f);
    return 0;
}

/* ------------------ Gnuplot plotting ------------------ */
static void plot_profiles(const char *p1_csv, const char *p2_csv){
    FILE *gp = popen("gnuplot -persist", "w");
    if (!gp){
        printf("ERROR: no se pudo lanzar gnuplot (¿instalado?).\n");
        return;
    }

    fprintf(gp, "set datafile separator ','\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set xlabel 'Distancia (cm)'\n");
    fprintf(gp, "set ylabel 'Sensibilidad (%%)'\n");
    fprintf(gp, "set title 'Perfil de sensibilidad P1 vs P2'\n");
    fprintf(gp, "set key outside top right vertical\n");

    fprintf(gp, "set style line 1 lc rgb '#1f77b4' lw 2 pt 7\n");
    fprintf(gp, "set style line 2 lc rgb '#d62728' lw 2 pt 5\n");

    fprintf(
        gp,
        "plot '%s' using 4:5 with linespoints ls 1 title 'P1', "
        "'%s' using 4:5 with linespoints ls 2 title 'P2'\n",
        p1_csv,
        p2_csv
    );

    fflush(gp);
    pclose(gp);
}

static void plot_tvg(const char *tvg_csv){
    FILE *gp = popen("gnuplot -persist", "w");
    if (!gp){
        printf("ERROR: no se pudo lanzar gnuplot (¿instalado?).\n");
        return;
    }

    fprintf(gp, "set datafile separator ','\n");
    fprintf(gp, "set grid\n");
    fprintf(gp, "set title 'TVG: Ganancia vs Distancia'\n");
    fprintf(gp, "set xlabel 'Distancia (cm)'\n");
    fprintf(gp, "set ylabel 'Ganancia (%%)'\n");
    fprintf(gp, "set yrange [0:100]\n");
    fprintf(gp, "set xrange [0:*]\n");
    fprintf(gp, "set key outside top right vertical\n");
    fprintf(gp,
        "plot "
        "'%s' using (0):(column(5)) every ::0::0 with steps lw 2 notitle, "
        "'%s' using 4:5 with steps lw 2 title 'TVG', "
        "'%s' using 4:5 with points pt 7 ps 1.2 lc rgb 'black' title 'Puntos TVG'\n",
        tvg_csv,
        tvg_csv,
        tvg_csv
    );

    fflush(gp);
    pclose(gp);
}




/* ------------------ Raw decode (FULL 1..55) ------------------ */
static void decode_reg(const uint8_t reg[55], int idx /*1..55*/, uint8_t b){
    switch (idx){
        case 1:
            printf("  TVGAIN0:       0x%02X | TVG_T0=%u TVG_T1=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 2:
            printf("  TVGAIN1:       0x%02X | TVG_T2=%u TVG_T3=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 3:
            printf("  TVGAIN2:       0x%02X | TVG_T4=%u TVG_T5=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 4:
            printf("  TVGAIN3:       0x%02X | TVG_G1=%u TVG_G2=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 5:
            printf("  TVGAIN4:       0x%02X | TVG_G2=%u TVG_G3=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 6:
            printf("  TVGAIN5:       0x%02X | TVG_G3=%u TVG_G4=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 7: {
            uint8_t tvg_g5     = (uint8_t)GET_BITS(b, 2, 6);
            uint8_t reserved   = (uint8_t)GET_BITS(b, 1, 1);
            uint8_t freq_shift = (uint8_t)GET_BITS(b, 0, 1);
            printf("  TVGAIN6:       0x%02X | TVG_G5=%u RESERVED=%u FREQ_SHIFT=%u\n",
                   b, tvg_g5, reserved, freq_shift);
            if (reserved != 0) printf("    WARNING: RESERVED bit is not 0\n");
            break;
        }
        case 8: {
            uint8_t bpf_bw   = (uint8_t)GET_BITS(b, 6, 2);
            uint8_t gain_ini = (uint8_t)GET_BITS(b, 0, 6);
            printf("  INIT_GAIN:     0x%02X | BPF_BW=%u GAIN_INIT=%u\n", b, bpf_bw, gain_ini);
            break;
        }
        case 9:
            printf("  FREQUENCY:     0x%02X | FREQ=%u\n", b, b);
            break;
        case 10:
            printf("  DEADTIME:      0x%02X | THR_CMP_DEGLTCH=%u PULSE_DT=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 11: {
            uint8_t io_if_sel = (uint8_t)GET_BITS(b, 7, 1);
            uint8_t uart_diag = (uint8_t)GET_BITS(b, 6, 1);
            uint8_t io_dis    = (uint8_t)GET_BITS(b, 5, 1);
            uint8_t p1_pulse  = (uint8_t)GET_BITS(b, 0, 5);
            printf("  PULSE_P1:      0x%02X | IO_IF_SEL=%u UART_DIAG=%u IO_DIS=%u P1_PULSE=%u\n",
                   b, io_if_sel, uart_diag, io_dis, p1_pulse);
            break;
        }
        case 12:
            printf("  PULSE_P2:      0x%02X | UART_ADDR=%u P2_PULSE=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 13: {
            uint8_t dis_cl = (uint8_t)GET_BITS(b, 7, 1);
            uint8_t res    = (uint8_t)GET_BITS(b, 6, 1);
            uint8_t lim1   = (uint8_t)GET_BITS(b, 0, 6);
            printf("  CURR_LIM_P1:   0x%02X | DIS_CL=%u RESERVED=%u CURR_LIM1=%u\n", b, dis_cl, res, lim1);
            if (res != 0) printf("    WARNING: RESERVED bit is not 0\n");
            break;
        }
        case 14: {
            uint8_t lpf_co = (uint8_t)GET_BITS(b, 6, 2);
            uint8_t lim2   = (uint8_t)GET_BITS(b, 0, 6);
            printf("  CURR_LIM_P2:   0x%02X | LPF_CO=%u CURR_LIM2=%u\n", b, lpf_co, lim2);
            break;
        }
        case 15:
            printf("  REC_LENGTH:    0x%02X | P1_REC=%u P2_REC=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 16:
            printf("  FREQ_DIAG:     0x%02X | FDIAG_LEN=%u FDIAG_START=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 17: {
            uint8_t fdiag_err = (uint8_t)GET_BITS(b, 5, 3);
            uint8_t sat_th    = (uint8_t)GET_BITS(b, 1, 4);
            uint8_t p1_nls    = (uint8_t)GET_BITS(b, 0, 1);
            printf("  SAT_FDIAG_TH:  0x%02X | FDIAG_ERR_TH=%u SAT_TH=%u P1_NLS_EN=%u\n", b, fdiag_err, sat_th, p1_nls);
            break;
        }
        case 18: {
            uint8_t p2_nls     = (uint8_t)GET_BITS(b, 7, 1);
            uint8_t vpwr_ov_th = (uint8_t)GET_BITS(b, 5, 2);
            uint8_t lmp_tmr    = (uint8_t)GET_BITS(b, 3, 2);
            uint8_t fvolt_err  = (uint8_t)GET_BITS(b, 0, 3);
            printf("  FVOLT_DEC:     0x%02X | P2_NLS_EN=%u VPWR_OV_TH=%u LMP_TMR=%u FVOLT_ERR_TH=%u\n",
                   b, p2_nls, vpwr_ov_th, lmp_tmr, fvolt_err);
            break;
        }
        case 19: {
            uint8_t afe_gain = (uint8_t)GET_BITS(b, 6, 2);
            uint8_t lpm_en   = (uint8_t)GET_BITS(b, 5, 1);
            uint8_t sel      = (uint8_t)GET_BITS(b, 4, 1);
            uint8_t decpl_t  = (uint8_t)GET_BITS(b, 0, 4);
            printf("  DECPL_TEMP:    0x%02X | AFE_GAIN_RNG=%u LPM_EN=%u DECPL_TEMP_SEL=%u DECPL_T=%u\n",
                   b, afe_gain, lpm_en, sel, decpl_t);
            break;
        }
        case 20: {
            uint8_t noise = (uint8_t)GET_BITS(b, 3, 5);
            uint8_t k     = (uint8_t)GET_BITS(b, 2, 1);
            uint8_t n     = (uint8_t)GET_BITS(b, 0, 2);
            printf("  DSP_SCALE:     0x%02X | NOISE_LVL=%u SCALE_K=%u SCALE_N=%u\n", b, noise, k, n);
            break;
        }
        case 21:
            printf("  TEMP_TRIM:     0x%02X | TEMP_GAIN=%u TEMP_OFF=%u\n", b, HI_NIBBLE(b), LO_NIBBLE(b));
            break;
        case 22: {
            uint8_t st = (uint8_t)GET_BITS(b, 6, 2);
            uint8_t lr = (uint8_t)GET_BITS(b, 3, 3);
            uint8_t sr = (uint8_t)GET_BITS(b, 0, 3);
            printf("  P1_GAIN_CTRL:  0x%02X | P1_DIG_GAIN_LR_ST=%u P1_DIG_GAIN_LR=%u P1_DIG_GAIN_SR=%u\n", b, st, lr, sr);
            break;
        }
        case 23: {
            uint8_t st = (uint8_t)GET_BITS(b, 6, 2);
            uint8_t lr = (uint8_t)GET_BITS(b, 3, 3);
            uint8_t sr = (uint8_t)GET_BITS(b, 0, 3);
            printf("  P2_GAIN_CTRL:  0x%02X | P2_DIG_GAIN_LR_ST=%u P2_DIG_GAIN_LR=%u P2_DIG_GAIN_SR=%u\n", b, st, lr, sr);
            break;
        }

        /* P1 times */
        case 24: printf("  P1_THR_0:      0x%02X | (TIEMPOS) T1=%dus T2=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 25: printf("  P1_THR_1:      0x%02X | (TIEMPOS) T3=%dus T4=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 26: printf("  P1_THR_2:      0x%02X | (TIEMPOS) T5=%dus T6=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 27: printf("  P1_THR_3:      0x%02X | (TIEMPOS) T7=%dus T8=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 28: printf("  P1_THR_4:      0x%02X | (TIEMPOS) T9=%dus T10=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 29: printf("  P1_THR_5:      0x%02X | (TIEMPOS) T11=%dus T12=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;

        /* P1 values packed (L1..L8) */
        case 30: printf("  P1_THR_6:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 31: printf("  P1_THR_7:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 32: printf("  P1_THR_8:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 33: printf("  P1_THR_9:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 34:
            printf("  P1_THR_10:     0x%02X | (VALORES L1..L8, 5-bit packed)\n", b);
            print_L1_L8_decoded(reg, 0);
            break;

        /* P1 L9..L12 */
        case 35: printf("  P1_THR_11:     0x%02X | TH_P1_L9=%u\n",  b, b); break;
        case 36: printf("  P1_THR_12:     0x%02X | TH_P1_L10=%u\n", b, b); break;
        case 37: printf("  P1_THR_13:     0x%02X | TH_P1_L11=%u\n", b, b); break;
        case 38: printf("  P1_THR_14:     0x%02X | TH_P1_L12=%u\n", b, b); break;
        case 39: printf("  P1_THR_15:     0x%02X | RESERVED/TH_P1_OFF(?)\n", b); break;

        /* P2 times */
        case 40: printf("  P2_THR_0:      0x%02X | (TIEMPOS) T1=%dus T2=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 41: printf("  P2_THR_1:      0x%02X | (TIEMPOS) T3=%dus T4=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 42: printf("  P2_THR_2:      0x%02X | (TIEMPOS) T5=%dus T6=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 43: printf("  P2_THR_3:      0x%02X | (TIEMPOS) T7=%dus T8=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 44: printf("  P2_THR_4:      0x%02X | (TIEMPOS) T9=%dus T10=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;
        case 45: printf("  P2_THR_5:      0x%02X | (TIEMPOS) T11=%dus T12=%dus\n", b, nibble_to_us(HI_NIBBLE(b)), nibble_to_us(LO_NIBBLE(b))); break;

        /* P2 values packed (L1..L8) */
        case 46: printf("  P2_THR_6:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 47: printf("  P2_THR_7:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 48: printf("  P2_THR_8:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 49: printf("  P2_THR_9:      0x%02X | (VALORES L1..L8, 5-bit packed)\n", b); break;
        case 50:
            printf("  P2_THR_10:     0x%02X | (VALORES L1..L8, 5-bit packed)\n", b);
            print_L1_L8_decoded(reg, 1);
            break;

        /* P2 L9..L12 */
        case 51: printf("  P2_THR_11:     0x%02X | TH_P2_L9=%u\n",  b, b); break;
        case 52: printf("  P2_THR_12:     0x%02X | TH_P2_L10=%u\n", b, b); break;
        case 53: printf("  P2_THR_13:     0x%02X | TH_P2_L11=%u\n", b, b); break;
        case 54: printf("  P2_THR_14:     0x%02X | TH_P2_L12=%u\n", b, b); break;
        case 55: printf("  P2_THR_15:     0x%02X | RESERVED/TH_P2_OFF(?)\n", b); break;

        default:
            printf("  REG_%02d:       0x%02X\n", idx, b);
            break;
    }
}

/* ------------------ CLI ------------------ */
static void usage(const char *prog){
    fprintf(stderr,
        "Uso:\n"
        "  %s [--csv [prefix]]\n"
        "  %s --plot [prefix]\n\n"
        "Lee una trama HEX por stdin.\n"
        "  --csv            Genera CSV (sin gráfica)\n"
        "  --plot           Genera CSV TH y muestra gráfica (P1 vs P2)\n"
        "  --plot-tvg       Genera CSV TVG y muestra gráfica (ganancia vs distancia)\n"
        "  Puedes combinar flags: --plot --plot-tvg\n"
        "  --help, -h      Muestra esta ayuda\n",
        prog, prog
    );
}


int main(int argc, char **argv){
    int want_csv = 0;
    int want_plot = 0;
    int want_plot_tvg = 0;
    const char *csv_prefix = NULL;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "--csv") == 0){
            want_csv = 1;
            // prefijo opcional si el siguiente no empieza por --
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0){
                csv_prefix = argv[++i];
            }
        } else if (strcmp(argv[i], "--plot") == 0){
            want_csv = 1;
            want_plot = 1;
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0){
                csv_prefix = argv[++i];
            }
        } else if (strcmp(argv[i], "--plot-tvg") == 0){
            want_csv = 1;
            want_plot_tvg = 1;
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0){
                csv_prefix = argv[++i];
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0){
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Argumento no reconocido: %s\n\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }


    uint8_t buf[512];
    char line[4096];

    if (!fgets(line, sizeof(line), stdin)){
        fprintf(stderr, "No se recibió entrada por stdin.\n");
        return 1;
    }

    int n = parse_hex_bytes(line, buf, (int)sizeof(buf));
    if (n < 0){
        fprintf(stderr, "Error parseando hex.\n");
        return 1;
    }

    if (n < 2 + 55){
        fprintf(stderr, "Trama demasiado corta: %d bytes. Se esperan al menos 57 (2 + 55).\n", n);
        return 1;
    }

    uint16_t prefix = (uint16_t)((buf[0] << 8) | buf[1]);

    uint8_t reg[55];
    for (int i = 0; i < 55; i++){
        reg[i] = buf[2 + i];
    }

    printf("HermesDecoder\n");
    printf("Prefix/mode: 0x%04X (ignorado para el mapeo de registros)\n", prefix);
    printf("Bytes totales: %d\n", n);
    if (n != 57){
        printf("AVISO: longitud esperada = 57 bytes (2 + 55). Recibida = %d bytes.\n", n);
        printf("      Se decodificarán los primeros 55 bytes de registros igualmente.\n");
    }
    printf("\n");

    for (int i = 0; i < 55; i++){
        int idx = i + 1;
        printf("[%02d]", idx);
        decode_reg(reg, idx, reg[i]);
    }

    if (want_csv){
        char p1_path[256], p2_path[256], tvg_csv[256];
        
        if (csv_prefix && csv_prefix[0]){
            snprintf(p1_path, sizeof(p1_path), "%s_p1_profile.csv", csv_prefix);
            snprintf(p2_path, sizeof(p2_path), "%s_p2_profile.csv", csv_prefix);
            snprintf(tvg_csv, sizeof(tvg_csv), "%s_tvg_profile.csv", csv_prefix);
        } else {
            snprintf(p1_path, sizeof(p1_path), "p1_profile.csv");
            snprintf(p2_path, sizeof(p2_path), "p2_profile.csv");
            snprintf(tvg_csv, sizeof(tvg_csv), "tvg_profile.csv");
        }

        int ok_p1 = 0, ok_p2 = 0, ok_tvg = 0;

        // Si quieres TH (por --plot) o si --csv sin más y tú quieres seguir generando TH siempre,
        // entonces genera TH cuando corresponda:
        if (want_plot || (want_csv && !want_plot_tvg)) {
            ok_p1 = write_th_profile_csv(p1_path, reg, 0);
            ok_p2 = write_th_profile_csv(p2_path, reg, 1);
        }

        // Si quieres TVG (por --plot-tvg) o si quieres que --csv también lo incluya:
        if (want_plot_tvg || want_csv) {
            ok_tvg = write_tvg_csv(tvg_csv, reg);
        }

        if (want_plot){
            if (ok_p1==0 && ok_p2==0){
                printf("\nMostrando gráfica TH (P1 vs P2)...\n");
                plot_profiles(p1_path, p2_path); // tu función de TH
            } else {
                printf("\nNo se puede plotear TH: CSVs incorrectos.\n");
            }
        }

        if (want_plot_tvg){
            if (ok_tvg==0){
                printf("\nMostrando gráfica TVG...\n");
                plot_tvg(tvg_csv); // tu función de TVG
            } else {
                printf("\nNo se puede plotear TVG: CSV incorrecto.\n");
            }
        }
        
        if (want_plot){
            printf("\nMostrando gráfica con gnuplot...\n");
            printf("\nColumnas CSV:\n");
            printf("  stage,delta_us,t_us,dist_cm,value_pct,value_raw\n");
        }
        if (want_plot_tvg && ok_tvg == 0){
            printf("\nMostrando gráfica TVG con gnuplot...\n");
            printf("\nColumnas CSV:\n");
            printf("stage,delta_us,t_us,dist_cm_tvg,gain_pct,gain_raw,gain_raw_max\n");

        }

        printf("\nCSV:\n");
        if (want_plot || (want_csv && !want_plot_tvg)){
            printf("  %s %s\n", (ok_p1==0) ? "OK " : "ERR", p1_path);
            printf("  %s %s\n", (ok_p2==0) ? "OK " : "ERR", p2_path);
        }
        if (want_plot_tvg || want_csv){
            printf("  %s %s\n", (ok_tvg==0) ? "OK " : "ERR", tvg_csv);
        }
    }

    return 0;
}
