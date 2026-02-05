#include <stdio.h>
#include <stdint.h>

/* ------------------ CLI ------------------ */
void usage(const char *prog){
    fprintf(stderr,
        "Uso:\n"
        "  %s [opciones]\n\n"

        "Descripción:\n"
        "  Lee una trama HEX por stdin y decodifica la configuración del PGA460.\n"
        "  Puede mostrar gráficas interactivas y/o exportar perfiles a disco.\n\n"

        "Opciones:\n"
        "  --plot [prefix]        Muestra gráfica TH (P1 vs P2).\n"
        "                         No exporta CSV ni JSON.\n\n"

        "  --plot-tvg [prefix]    Muestra gráfica TVG (ganancia vs distancia).\n"
        "                         No exporta CSV ni JSON.\n\n"

        "  --export-csv [prefix]  Exporta perfiles TH (P1/P2) y TVG en CSV.\n"
        "                         No muestra gráficas.\n\n"

        "  --export-json [prefix] Exporta perfiles TH (P1/P2) y TVG en JSON.\n"
        "                         No muestra gráficas.\n\n"

        "  --help, -h             Muestra esta ayuda.\n\n"

        "Notas:\n"
        "  - Las opciones --plot y --plot-tvg pueden combinarse.\n"
        "  - Las opciones --export-csv y --export-json pueden combinarse\n"
        "    entre sí y con --plot / --plot-tvg.\n"
        "  - El prefijo es opcional y se usa para nombrar los ficheros exportados.\n\n"

        "Ejemplos:\n"
        "  %s --plot\n"
        "  %s --plot-tvg\n"
        "  %s --plot --plot-tvg\n"
        "  %s --export-csv test\n"
        "  %s --export-json test\n"
        "  %s --plot --export-csv test\n"
        "  %s --plot --plot-tvg --export-json test\n",
        prog, prog, prog, prog, prog, prog, prog, prog
    );
}


