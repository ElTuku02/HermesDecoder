#include <stdio.h>
#include <stdint.h>

#include "export.h"
#include "utils.h"

int write_th_profile_csv(const char *path, const uint8_t reg[55], int is_p2){
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

int write_tvg_csv(const char *path, const uint8_t reg[55]){
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

int write_th_profile_json(const char *path, const uint8_t reg[55], int is_p2){
    int delta_us[12];
    int L5[8], L8[4];
    int raw[12];

    extract_T12_us(reg, is_p2, delta_us);
    extract_L1_L8_5bit(reg, is_p2, L5);
    extract_L9_L12_8bit(reg, is_p2, L8);

    for (int i = 0; i < 8; i++) raw[i] = L5[i];
    for (int i = 0; i < 4; i++) raw[8+i] = L8[i];

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "{\n");
    fprintf(f, "  \"profile\": \"%s\",\n", is_p2 ? "P2" : "P1");
    fprintf(f, "  \"units\": {\"x\": \"cm\", \"time\": \"us\", \"y\": \"percent\"},\n");
    fprintf(f, "  \"points\": [\n");

    int acc = 0;
    for (int i = 0; i < 12; i++){
        int stage = i + 1;
        acc += delta_us[i];
        double dist_cm = tof_us_to_cm(acc);
        double pct = value_to_pct(stage, raw[i]);

        fprintf(f,
            "    {\"stage\": %d, \"delta_us\": %d, \"t_us\": %d, \"dist_cm\": %.4f, \"value_pct\": %.2f, \"value_raw\": %d}%s\n",
            stage, delta_us[i], acc, dist_cm, pct, raw[i],
            (i == 11) ? "" : ","
        );
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    return 0;
}

int write_tvg_json(const char *path, const uint8_t reg[55]){
    int t_us[6];
    int g_raw[5];
    int g_max[5];

    // tiempos (T0..T5)
    uint8_t b0 = reg[0], b1 = reg[1], b2 = reg[2];
    t_us[0] = nibble_to_us(HI_NIBBLE(b0));
    t_us[1] = nibble_to_us(LO_NIBBLE(b0));
    t_us[2] = nibble_to_us(HI_NIBBLE(b1));
    t_us[3] = nibble_to_us(LO_NIBBLE(b1));
    t_us[4] = nibble_to_us(HI_NIBBLE(b2));
    t_us[5] = nibble_to_us(LO_NIBBLE(b2));

    // ganancias según Excel (G2/G3 partidos)
    uint8_t tvg3 = reg[3], tvg4 = reg[4], tvg5 = reg[5], tvg6 = reg[6];

    int g2_hi2 = (int)GET_BITS(tvg3, 0, 2);   // TVGAIN3 b1..b0
    int g2_lo4 = (int)GET_BITS(tvg4, 4, 4);   // TVGAIN4 b7..b4
    int g3_hi4 = (int)GET_BITS(tvg4, 0, 4);   // TVGAIN4 b3..b0
    int g3_lo2 = (int)GET_BITS(tvg5, 6, 2);   // TVGAIN5 b7..b6

    g_raw[0] = (int)GET_BITS(tvg3, 2, 6);         g_max[0] = 63; // G1
    g_raw[1] = (g2_hi2 << 4) | g2_lo4;            g_max[1] = 63; // G2
    g_raw[2] = (g3_hi4 << 2) | g3_lo2;            g_max[2] = 63; // G3
    g_raw[3] = (int)GET_BITS(tvg5, 0, 6);         g_max[3] = 63; // G4
    g_raw[4] = (int)GET_BITS(tvg6, 2, 6);         g_max[4] = 63; // G5

    int reserved  = (int)GET_BITS(tvg6, 1, 1);
    int freq_shift = (int)GET_BITS(tvg6, 0, 1);

    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "{\n");
    fprintf(f, "  \"profile\": \"TVG\",\n");
    fprintf(f, "  \"units\": {\"x\": \"cm\", \"time\": \"us\", \"y\": \"percent\"},\n");
    fprintf(f, "  \"flags\": {\"reserved\": %d, \"freq_shift\": %d},\n", reserved, freq_shift);
    fprintf(f, "  \"points\": [\n");

    int acc_us = 0;
    for (int i = 0; i < 6; i++){
        acc_us += t_us[i];

        int gain_idx = (i < 5) ? i : 4; // cola mantiene G5
        double gain_pct = (g_raw[gain_idx] / (double)g_max[gain_idx]) * 100.0;
        double dist_cm = tof_us_to_cm(acc_us);

        fprintf(f,
            "    {\"stage\": %d, \"delta_us\": %d, \"t_us\": %d, \"dist_cm\": %.4f, \"gain_pct\": %.2f, \"gain_raw\": %d, \"gain_raw_max\": %d}%s\n",
            i + 1, t_us[i], acc_us, dist_cm, gain_pct, g_raw[gain_idx], g_max[gain_idx],
            (i == 5) ? "" : ","
        );
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    return 0;
}