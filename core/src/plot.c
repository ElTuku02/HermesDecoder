#include <stdio.h>
#include <stdlib.h>
#include "plot.h"

/* ------------------ Gnuplot plotting ------------------ */
void plot_profiles(const char *p1_csv, const char *p2_csv){
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

void plot_tvg(const char *tvg_csv){
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


