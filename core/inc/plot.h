#ifndef HERMES_PLOT_h
#define HERMES_PLOT_h

#ifdef __cplusplus
extern "C" {
#endif

/* TH: P1 vs P2 (Sensibilidad vs Distancia) */
void plot_profiles(const char *p1_csv, const char *p2_csv);

/* TVG: Ganancia vs Distancia */
void plot_tvg(const char *tvg_csv);

#ifdef __cplusplus
}
#endif

#endif // HERMES_PLOT_h
