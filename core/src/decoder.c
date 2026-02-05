
#include <stdio.h>
#include <stdint.h>

#include "utils.h"
#include "decoder.h"

/* ------------------ Raw decode (FULL 1..55) ------------------ */
void decode_reg(const uint8_t reg[55], int idx /*1..55*/, uint8_t b){
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
