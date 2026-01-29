# HermesDecoder

HermesDecoder es una herramienta CLI escrita en C para la **decodificación, inspección y validación de tramas de configuración**
utilizadas en dispositivos AIoT basados en **STM32 + PGA460** para medición ultrasónica.

El objetivo principal del proyecto es transformar tramas binarias (en formato hexadecimal)
en información **legible y estructurada**, permitiendo analizar registros, campos de bits y parámetros
internos del PGA460 de forma clara y fiable.

---

## Características

- Decodificación de tramas de configuración del PGA460
- Interpretación a nivel de **registro y subcampos (bitfields)**
- Soporte para tablas de umbrales (P1 / P2)
- Salida orientada a depuración y validación
- Diseño modular y extensible
- Pensado para entornos de laboratorio y banco de pruebas

---

## Estado del proyecto

**En desarrollo temprano**

El proyecto se encuentra en fase inicial y está evolucionando activamente.

---

## Hoja de ruta (Roadmap)

- [ ] Decodificación completa de registros del PGA460
- [ ] Interpretación detallada de campos de bits
- [ ] Validación de valores reservados
- [ ] Verificación de checksum / integridad
- [ ] Exportación a JSON y CSV
- [ ] Encoder inverso (campos → trama)
- [ ] Herramientas auxiliares para análisis de señales

---

## Motivación

La configuración del PGA460 implica una gran cantidad de registros y campos empaquetados a nivel de bits,
lo que dificulta la depuración directa desde tramas hexadecimales.

HermesDecoder nace como una herramienta para:

- reducir errores de configuración
- acelerar el proceso de ajuste y calibración
- facilitar el análisis técnico durante el desarrollo

---
