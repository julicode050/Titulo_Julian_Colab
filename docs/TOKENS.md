# Especificación de los tokens A y B (Mecánica B)

**Versión:** 1 · **Fecha:** 2026-10-15 · **Estado:** diseñados, no fabricados.
**Fuente:** planos de Julián en Fusion (tokens A y B, y su posición al encajar).

Dos tokens pasivos, uno por persona. Cada uno toca la pantalla con 3 almohadillas conductoras. Al encajarse, sus patrones se superponen y los 6 puntos quedan en un arreglo aproximadamente circular. Así el par unido ocupa casi la misma superficie que un token solo.

## Almohadillas
- Diámetro: **5 mm** (las 6).

## Coordenadas (mm)
Los centros de las almohadillas se dan con el par **encajado**, con origen en el punto de abajo de A. El eje X va hacia la derecha y el eje Y hacia arriba.

| Punto | X | Y | Token |
|---|---|---|---|
| A1 | 0,00 | 0,00 | A (abajo) |
| A2 | 0,00 | 30,00 | A (arriba) |
| A3 | 25,98 | 15,00 | A (derecha) |
| B1 | −11,51 | 25,68 | B (izquierda) |
| B2 | 10,50 | 33,18 | B (arriba) |
| B3 | 23,49 | 25,68 | B (derecha) |

## Token A: triángulo equilátero
- Lados: **30 / 30 / 30 mm**.
- El clasificador del programa lo lee como **'A'**.

## Token B: triángulo escaleno
- Lados: B1–B3 = **35,00 mm** (horizontal), B2–B3 = **15,00 mm**, B1–B2 = **23,25 mm** (este último resulta de las otras restricciones).
- Ángulos: 18,8° en B1, 131,2° en B2 y 30,0° en B3.
- El clasificador lo lee como **'E'** (escaleno). Para que el programa lo reconozca como B, hay que agregar la letra E al token B en `settings.json` (`deteccion.tokenLetters`).

## Cómo se definen en el plano
- **A:** equilátero de 30 mm, con un lado vertical (A1–A2).
- **B respecto de A:**
  - el lado B2–B3 (15 mm) es **paralelo** al lado A2–A3;
  - está **desplazado 8 mm** hacia afuera de él;
  - y está **centrado** en él (la perpendicular desde el punto medio de B2–B3 cae en el punto medio de A2–A3).
- **B1:** está a la misma altura que B3, a **35 mm** hacia la izquierda. Así queda a 12,299 mm de A2 (cota de referencia).

## Par encajado
- **Huella** de los 6 puntos, con almohadillas: **42,5 × 38,2 mm**.
- **Separaciones más chicas** (centro a centro / borde a borde):

  | Par | Centro a centro | Borde a borde |
  |---|---|---|
  | A2–B2 | 10,97 mm | 5,97 mm |
  | A3–B3 | 10,97 mm | 5,97 mm |
  | A2–B1 | 12,30 mm | 7,30 mm |

  La primera versión del diseño tenía B1 a 30 mm de B3, y A2–B1 quedaba a solo 7,8 mm. Julián alejó B1 a 35 mm para ir a la segura.
- **Centros de los triángulos al encajar:** A en (8,66; 15,00), B en (7,49; 28,18). Están a 13,2 mm de distancia.

## Análisis de reconocimiento (hecho en el computador, 2026-10-15)
- **Por separado:** A se lee como 'A' y B como 'E'.
- **Encajados:** la pantalla ve un solo grupo de 6 puntos. Para saber qué punto es de cada token, se prueban los 10 repartos posibles en dos tríos y se elige el que mejor calza con las medidas conocidas de A y B. Simulando error de posición en cada punto (impresión + ruido de la pantalla), la identificación correcta fue:

  | Error por punto | 0,5 mm | 1,0 mm | 1,5 mm | 2,0 mm |
  |---|---|---|---|---|
  | Identificación correcta | 100 % | 99,8 % | 96,4 % | 84,6 % |

- **Importante:** el código actual **no** usa este método. Hoy divide el grupo de 6 puntos por cercanía espacial, lo que con puntos intercalados mezclaría los de A y B. Hay que implementar el calce de medidas en `TokenTracker` (ver `PENDING.md`).
- **"Encajados" vs. "muy cerca":** con el reparto correcto, el programa puede comprobar que los 6 puntos forman la configuración encajada de esta tabla y no dos tokens que solo están cerca. Esto resuelve una limitación abierta desde el inicio del proyecto.
- **Umbral de agrupación:** la mayor distancia dentro de un token es 35 mm (128 px), menor que el umbral actual de 150 px, así que cada token se sigue agrupando bien.

## Pruebas físicas pendientes (con probetas, antes de fabricar)
Con la app abierta y el panel de debug (tecla D), revisar "Contactos" y que se mantenga estable al mover y girar las probetas.
1. **Separación mínima:** dos almohadillas de Ø5 mm a **11 mm** entre centros. Es la separación más chica del par encajado. La pantalla debe contar 2 contactos, no 1.
2. **Tamaño de almohadilla:** que una almohadilla de Ø5 mm se registre de forma estable como contacto.
3. **Token A y token B por separado:** que el debug muestre 3 puntos y la forma esperada ('A' y 'E').
