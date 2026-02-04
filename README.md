# HermesDecoder

**HermesDecoder** es una herramienta CLI escrita en **C** para la **decodificación, análisis y validación avanzada de tramas de configuración**
utilizadas en dispositivos **AIoT** basados en **STM32 + PGA460** para medición ultrasónica.

El proyecto permite transformar tramas binarias (en formato hexadecimal) en información **legible, estructurada y analizable**,
facilitando tanto la **depuración de configuración** como el **ajuste fino de perfiles de medición** (TH / TVG).

---

## Características principales

- ✅ Decodificación completa de tramas de configuración del **PGA460**
- ✅ Interpretación a nivel de **registro y subcampos (bitfields)**
- ✅ Soporte completo para:
  - **Perfiles de umbrales (TH)**: Preset 1 y Preset 2
  - **Ganancia dependiente del tiempo (TVG)**
- ✅ Decodificación de campos empaquetados en bits (4b / 5b / 6b / 8b)
- ✅ Validación y aviso de bits **RESERVED** mal configurados
- ✅ Exportación de datos a:
  - **CSV** (perfiles TH y TVG)
  - **JSON** (estructura completa de perfiles y parámetros)
- ✅ Visualización gráfica mediante **gnuplot**:
  - TH: sensibilidad (%) vs distancia (P1 y P2 en la misma gráfica)
  - TVG: ganancia (%) vs distancia
- ✅ Generación automática de CSV temporales para plotting (sin ensuciar el proyecto)
- ✅ Herramienta orientada a **laboratorio, banco de pruebas y calibración**
- ✅ Código modular y extensible

---

## Uso básico

HermesDecoder lee una **trama hexadecimal por `stdin`**:

```bash
echo "5E02..." | ./hermesdecoder [opciones]
```

## Opciones disponibles

### Visualización

```bash
--plot
```

Muestra la gráfica de umbrales TH (Preset 1 y Preset 2) usando **gnuplot**.
La gráfica representa:

- Eje X → distancia (cm)
- Eje Y → sensibilidad (%)

```bash
--plot-tvg
```

Muestra la gráfica de TVG (ganancia dependiente del tiempo) usando gnuplot.
La gráfica representa:

- Eje X → distancia (cm) obtenida a partir del tiempo TVG
- Eje Y → ganancia (%)

Ambas opciones pueden combinarse:

```bash
--plot --plot-tvg
```

En este caso se mostrarán ambas gráficas, cada una en su ventana.

>[!WARNING]
>Las opciones de `--plot` generan archivos CSV **temporales** en `/tmp` que se eliminan automáticamente al finalizar.

### Exportación

#### Exportación CSV

```bash
--export-csv [prefijo]
```

Exporta los perfiles decodificados a archivos CSV **persistentes**

Archivos generados:

- `<prefijo>`_p1_profile.csv
- `<prefijo>`_p2_profile.csv
- `<prefijo>`_tvg_profile.csv

Si no se indica prefijo, se usan nombres por defecto:

- p1_profile.csv
- p2_profile.csv
- tvg_profile.csv

>[!IMPORTANT]
>Las opciones `--plot` y `--plot-tvg` **NO exportan CSV por defecto**.
>Para guardar archivos es obligatorio usar `--export-csv`.

#### Exportación JSON

```bash
--export-json [prefijo]
```

Exporta la información decodificada a archivos **JSON persistentes**.

Archivos generados:

- `<prefijo>`_p1_profile.json
- `<prefijo>`_p2_profile.json
- `<prefijo>`_tvg_profile.json

Si no se indica prefijo, se usan nombres por defecto:

- p1_profile.json
- p2_profile.json
- tvg_profile.json

## Ayuda

```Bash
--help
-h
```

Muestra la ayuda de uso

## Formato de salida

### Decodificación RAW

La salida principal del programa muestra:

- Listado secuencial de registros (1–55)
- Decodificación de subcampos y bitfields
- Interpretación de:
  - tiempos (µs)
  - umbrales (%)
  - ganancias (%)

Avisos de:

- bits RESERVED mal configurados
- valores sospechosos o fuera de rango

Esta salida está pensada para depuración y validación **directa de tramas**.

### CSV/JSON - Perfiles TH

Campos de los umbrales:

```url
stage, delta_us, t_us, dist_cm, value_pct, value_raw
```

- `stage` → etapa del perfil
- `delta_us` → duración del tramo
- `t_us` → tiempo acumulado
- `dist_cm` → distancia equivalente
- `value_pct` → umbral en porcentaje
- `value_raw` → valor raw del registro

### CSV/JSON - Perfiles TVG

```url
stage, delta_us, t_us, dist_cm_tvg, gain_pct, gain_raw, gain_raw_max
```

- `stage` → etapa TVG
- `delta_us` → duración del tramo
- `t_us` → tiempo acumulado
- `dist_cm_tvg` → distancia equivalente
- `gain_pct` → ganancia en porcentaje
- `gain_raw` → valor raw de ganancia
- `gain_raw_max` → valor máximo posible del campo

## Estado del proyecto

🟢 **Funcional y en uso activo**

El proyecto ha superado la fase inicial y se utiliza como herramienta real de análisis y calibración.
Actualmente se encuentra en fase de **mejora continua y refinamiento**.

## Motivación

La configuración del PGA460 implica una gran cantidad de registros y campos empaquetados a nivel de bits,
lo que hace difícil y propenso a errores el ajuste directo mediante tramas hexadecimales.

HermesDecoder nace para:

- reducir errores de configuración
- acelerar el proceso de ajuste y calibración
- visualizar perfiles TH y TVG de forma clara
- facilitar el análisis técnico durante el desarrollo
- servir como herramienta de apoyo a firmware y hardware

## Hoja de ruta (Roadmap)

- [x] Decodificación completa de registros del PGA460
- [x] Interpretación detallada de campos de bits
- [x] Validación de bits reservados
- [x] Exportación a CSV
- [x] Visualización gráfica con gnuplot
- [x] Exportación a JSON
