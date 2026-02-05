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

 //gcc -O2 -Wall -Wextra -Icore/inc core/src/*.c -o hermesdecoder

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "plot.h"
#include "export.h"
#include "utils.h"
#include "decoder.h"
#include "usage.h"

int main(int argc, char **argv){
    int want_plot_th = 0;
    int want_plot_tvg = 0;
    int want_export_csv = 0;
    int want_export_json = 0;
    const char *csv_prefix = NULL;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "--export-csv") == 0){
            want_export_csv = 1;
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0){
                csv_prefix = argv[++i];
            }

        } else if (strcmp(argv[i], "--export-json") == 0){
            want_export_json = 1;
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0){
                csv_prefix = argv[++i];
            }

        } else if (strcmp(argv[i], "--plot") == 0){
            want_plot_th = 1;
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0){
                csv_prefix = argv[++i];
            }

        } else if (strcmp(argv[i], "--plot-tvg") == 0){
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

     // --- Export/plot section ---
    if (want_export_csv || want_plot_th || want_plot_tvg){

        // Rutas CSV "export" (solo si want_export_csv)
        char p1_export[256], p2_export[256], tvg_export[256];
        char p1_json[256], p2_json[256], tvg_json[256];

        if (csv_prefix && csv_prefix[0]){
            snprintf(p1_export, sizeof(p1_export), "%s_p1_profile.csv", csv_prefix);
            snprintf(p2_export, sizeof(p2_export), "%s_p2_profile.csv", csv_prefix);
            snprintf(tvg_export, sizeof(tvg_export), "%s_tvg_profile.csv", csv_prefix);
        } else {
            snprintf(p1_export, sizeof(p1_export), "p1_profile.csv");
            snprintf(p2_export, sizeof(p2_export), "p2_profile.csv");
            snprintf(tvg_export, sizeof(tvg_export), "tvg_profile.csv");
        }

        if (csv_prefix && csv_prefix[0]){
            snprintf(p1_json, sizeof(p1_json), "%s_p1_profile.json", csv_prefix);
            snprintf(p2_json, sizeof(p2_json), "%s_p2_profile.json", csv_prefix);
            snprintf(tvg_json, sizeof(tvg_json), "%s_tvg_profile.json", csv_prefix);
        } else {
            snprintf(p1_json, sizeof(p1_json), "p1_profile.json");
            snprintf(p2_json, sizeof(p2_json), "p2_profile.json");
            snprintf(tvg_json, sizeof(tvg_json), "tvg_profile.json");
        }

        // Rutas CSV temporales para plot (no exporta)
        const char *p1_tmp  = "/tmp/hermes_p1_profile.csv";
        const char *p2_tmp  = "/tmp/hermes_p2_profile.csv";
        const char *tvg_tmp = "/tmp/hermes_tvg_profile.csv";

        int ok_p1_plot = 0, ok_p2_plot = 0, ok_tvg_plot = 0;
        int ok_p1_exp  = 0, ok_p2_exp  = 0, ok_tvg_exp  = 0;

        // --- Generar CSVs para PLOT (temporales) ---
        if (want_plot_th){
            ok_p1_plot = write_th_profile_csv(p1_tmp, reg, 0);
            ok_p2_plot = write_th_profile_csv(p2_tmp, reg, 1);
        }
        if (want_plot_tvg){
            ok_tvg_plot = write_tvg_csv(tvg_tmp, reg);
        }

        // --- Plot (usa SIEMPRE los temporales) ---
        if (want_plot_th){
            if (ok_p1_plot==0 && ok_p2_plot==0){
                printf("\nMostrando gráfica TH (P1 vs P2)...\n");
                plot_profiles(p1_tmp, p2_tmp);
            } else {
                printf("\nNo se puede plotear TH: error generando CSV temporal.\n");
            }
        }

        if (want_plot_tvg){
            if (ok_tvg_plot==0){
                printf("\nMostrando gráfica TVG...\n");
                plot_tvg(tvg_tmp);
            } else {
                printf("\nNo se puede plotear TVG: error generando CSV temporal.\n");
            }
        }

        // Borrar temporales al finalizar
        if (want_plot_th){
            remove(p1_tmp);
            remove(p2_tmp);
        }
        if (want_plot_tvg){
            remove(tvg_tmp);
        }

        // --- Generar CSVs para EXPORT (persistentes) ---
        if (want_export_csv){
            // Política: exportar siempre todo (TH + TVG)
            ok_p1_exp = write_th_profile_csv(p1_export, reg, 0);
            ok_p2_exp = write_th_profile_csv(p2_export, reg, 1);
            ok_tvg_exp = write_tvg_csv(tvg_export, reg);

            printf("\nCSV exportados:\n");
            printf("  %s %s\n", (ok_p1_exp==0) ? "OK " : "ERR", p1_export);
            printf("  %s %s\n", (ok_p2_exp==0) ? "OK " : "ERR", p2_export);
            printf("  %s %s\n", (ok_tvg_exp==0) ? "OK " : "ERR", tvg_export);
        }

        if (want_export_json){
            int ok_j1 = write_th_profile_json(p1_json, reg, 0);
            int ok_j2 = write_th_profile_json(p2_json, reg, 1);
            int ok_j3 = write_tvg_json(tvg_json, reg);

            printf("\nJSON exportados:\n");
            printf("  %s %s\n", (ok_j1==0) ? "OK " : "ERR", p1_json);
            printf("  %s %s\n", (ok_j2==0) ? "OK " : "ERR", p2_json);
            printf("  %s %s\n", (ok_j3==0) ? "OK " : "ERR", tvg_json);
        }
    }
    return 0;
}