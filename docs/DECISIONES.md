# Registro de decisiones

Decisiones técnicas y de diseño del prototipo, en orden cronológico. Cada entrada indica qué se decidió, por qué, y quién lo decidió o qué lo motivó. Es la fuente del apartado "Decisiones" de cada reporte de progreso (`docs/reportes/`).

Formato: `### AAAA-MM-DD · Título` y luego **Decisión**, **Motivo** y, cuando aplica, **Commit** o **Reemplaza a**.

---

### 2026-10-01 · Reutilizar la detección capacitiva existente
**Decisión:** conservar sin reimplementar la recepción TUIO y el clasificador de formas (`Token::classifyShape`) del prototipo anterior. Construir encima un módulo de seguimiento (`TokenTracker`).
**Motivo:** el brief la define como la pieza más valiosa y validada del proyecto.
**Commit:** `2acff32`

### 2026-10-01 · Detección provisoria de tokens "unidos"
**Decisión:** dos tokens cuentan como unidos si sus contactos se funden en un grupo de 6 o más puntos (el grupo se divide con k-means para conservar la identidad de cada token), o si sus centros están a menos de 5 cm. La unión se confirma tras 5 cuadros y la separación tras 10.
**Motivo:** la detección existente solo reconocía tokens individuales, y al juntarlos se perdía la identidad de ambos. Julián acordó esta solución provisoria hasta fabricar tokens que se reconozcan como unidad al encajarse.
**Limitación aceptada:** no distingue "encajados" de "muy cerca".
**Commit:** `2acff32`

### 2026-10-01 · Tokens B y C como el mismo token físico
**Decisión:** mapear las letras `B` y `C` del clasificador al token físico B (`tokenLetters` en `settings.json`), sin modificar el clasificador.
**Motivo:** el clasificador etiqueta los triángulos isósceles como B o C según la rotación. Fence Builder ya aceptaba ambas letras para el mismo token.
**Commit:** `2acff32`

### 2026-10-01 · Parámetros en un archivo externo, en centímetros
**Decisión:** todos los umbrales y valores de juego viven en `bin/data/settings.json`, editable sin recompilar. La escala es 36,5 px/cm (1080 px / ~29,6 cm de alto). Ninguna regla compara contra un número fijo de tokens: se usan umbrales (`minJoinedTokens`, `fusionMinTokens`, `joinedMinPoints`).
**Motivo:** requisito del brief (número de piezas configurable) y necesidad de calibrar en bodystorming.
**Commit:** `2acff32`

### 2026-10-01 · Punto central de eventos para logging futuro
**Decisión:** todos los eventos de tokens (aparece, se retira, se une, se separa) y de juego pasan por `src/Events.h`. No se escribe ningún archivo.
**Motivo:** el brief excluye el logging automático en este sprint, pero pide que agregarlo después sea un cambio aislado.
**Commit:** `2acff32`

### 2026-10-01 · Simulador de escritorio
**Decisión:** agregar tokens virtuales A y B (tecla S) que generan puntos de contacto sintéticos y pasan por la misma detección que los tokens reales.
**Motivo:** probar sin la pantalla táctil ni los tokens, incluida la unión, que el hardware actual aún no produce.
**Commit:** `2acff32`

### 2026-10-01 · Eliminar Fence Builder, 2 tokens, textos en español
**Decisión:** se elimina el juego anterior. Se diseña para 2 tokens, uno por persona ("por ahora"). Los textos de pantalla van en español y la selección del menú se hace manteniendo un token 1,5 s sobre el botón.
**Motivo:** decisiones de Julián al revisar el brief. La permanencia sobre el botón evita activaciones accidentales, igual que la corrección `d6f2b1a` del prototipo anterior.
**Commit:** `2acff32`

### 2026-10-01 · Zona central de Ensamblaje de 6 cm de radio
**Decisión:** radio de 6 cm, ajustable.
**Motivo:** el brief pide ~25–30 cm de radio, pero la pantalla mide ~29,6 cm de alto, así que esa zona la cubriría entera.
**Commit:** `2acff32`

### 2026-10-01 · Reglas iniciales de los minijuegos (ambigüedades del brief)
**Decisión:**
- **Ensamblaje:** cada token lleva un fragmento a la vez. El éxito se mide como tiempo por ciclo. Un token que ya está sobre un fragmento al empezar el ciclo no puede recogerlo hasta salir de él.
- **Resonancia:** distancias de 6, 14 y 24 cm entre nodos individuales, repartidas en partes iguales y barajadas con semilla fija. Un par unido también activa nodos individuales.
- **Marea:** 1 token sostiene un punto en modo bajo; dos tokens sin unirse también cuentan como modo bajo. Un punto desaparece al vaciarse o a los 40 s. La fusión de puntos depende de `fusionMinTokens` (4).
**Motivo:** el brief dejaba estos puntos abiertos. Julián pidió completarlos y revisarlos durante las pruebas (ver `PENDING.md`).
**Commit:** `2acff32`

### 2026-10-03 · Ensamblaje: fragmentos sueltos hasta unir (revertida)
**Decisión:** al entrar a la zona, el fragmento se suelta y queda flotando. Se fusionan todos cuando los tokens se unen.
**Motivo:** en una prueba de bodystorming, Julián vio que la figura se armaba con un solo token, contrario al punto 7.1 del brief.
**Commit:** `b430cdf` · **Reemplazada el mismo día** (ver la entrada siguiente).

### 2026-10-03 · Ensamblaje: un fragmento fijado por unión
**Decisión:** el token sigue llevando su fragmento dentro de la zona. Al unirse con otro token dentro de la zona, se fija ese fragmento. Para fijar el siguiente, los tokens deben separarse y volver a unirse.
**Motivo:** en la prueba de Julián, la versión anterior soltaba el fragmento en el borde de la zona y no se podía volver a recoger, lo que hacía imposible completar el nivel. Además, esta regla implementa su idea de mecánica para aumentar la codependencia.
**Reemplaza a:** "fragmentos sueltos hasta unir".
**Commit:** `ac5b32b`

### 2026-10-03 · Panel de diagnóstico en pantalla
**Decisión:** panel arriba a la izquierda (tecla D) con cada token detectado: etiqueta, ID, letra cruda del clasificador, puntos de contacto, posición en cm y si está unido.
**Motivo:** pedido de Julián para las pruebas en la mesa.
**Commit:** `b430cdf`

### 2026-10-04 · No actualizar openFrameworks
**Decisión:** mantener openFrameworks 0.12.1.
**Motivo:** ya es la última versión estable, y cambiar de versión durante la tesis solo agrega riesgo.

### 2026-10-04 · Escala de pantalla según especificación del fabricante
**Decisión:** `pxPerCm` = 36,43, calculado a partir del pixel pitch de la ViewSonic TD2455 (modelo VS17978): 0,2745 mm, área visible 527 × 296,5 mm.
**Motivo:** reemplaza la estimación de 36,5 px/cm, hecha con medidas tomadas a mano. Los 50,3 cm de ancho medidos no calzaban con el panel; el ancho visible real es de 52,7 cm.

### 2026-10-04 · Ensamblaje: calce de rompecabezas por posición
**Decisión:**
- Cada pieza tiene un único lugar en la figura. Las figuras se cortan en ángulos irregulares (entre 0,6 y 1,4 veces el reparto parejo), así cada pieza tiene una forma propia.
- Una pieza se fija solo si el token que la lleva está sobre su lugar (a menos de 3,5 cm) al momento de la unión.
- Si el lugar es incorrecto, la pieza tiembla y se tiñe de rojo, y la unión no se gasta: unidos, pueden deslizarse al lugar correcto.
- Los contornos de cada lugar se ven en los primeros ciclos y se desvanecen hasta dejar solo la silueta.
**Motivo:** Julián pidió que hubiera calce de rompecabezas, en vez de que cualquier pieza sirviera en cualquier lugar. Se eligió calce por posición (no por rotación) para mantener la regla explicable en una frase.
**Commit:** `c4d9fd0`

### 2026-10-04 · Ensamblaje: figura y zona más grandes
**Decisión:** la figura pasa de 4,5 a 8,5 cm de radio y la zona de 6 a 10,5 cm. Las piezas esperan en columnas a ambos lados de la zona. Para fijar, solo el token que lleva la pieza debe estar dentro de la zona.
**Motivo:** con la figura chica, los lugares quedaban a 2–3 cm entre sí, menos que el ancho del token, así que apuntar a uno era impracticable. Exigir que ambos tokens estuvieran en la zona hacía que una unión hecha desde afuera, junto a un lugar del borde, se ignorara sin ninguna respuesta. Julián probará estas medidas con los tokens reales la semana del 2026-10-05.
**Commit:** `c4d9fd0`

### 2026-10-04 · Calce por rotación queda como opción futura
**Decisión:** no implementar por ahora el calce por rotación de la pieza. Queda anotado en `PENDING.md`.
**Motivo:** el token A (triángulo equilátero) no permite distinguir su rotación en pasos de 120°, y los tokens unidos giran juntos. Se reevaluará con los nuevos tokens.

### 2026-10-04 · Panel de instrucciones (tecla I)
**Decisión:** con la tecla I se muestra arriba a la derecha el objetivo, la mecánica y la condición para ganar del minijuego en curso. Empieza oculto. El texto se arma con los valores actuales de `settings.json`, así no queda desactualizado al calibrar. La línea de atajos del panel de debug ya no repite "debug".
**Motivo:** pedido de Julián para las pruebas. Es una ayuda para el investigador; el brief pide evitar instrucciones largas para los participantes, por eso empieza oculto.

### 2026-10-04 · Ensamblaje: los cortes de las figuras siguen siendo aleatorios (por ahora)
**Decisión:** se mantienen aleatorios, en cada sesión, los ángulos de corte de las figuras y la posición inicial de las piezas. Queda pendiente evaluar una semilla fija, como ya tienen Resonancia y Marea.
**Motivo:** hallazgo del reporte del 04-10. Con el calce de rompecabezas, la dificultad de cada ciclo depende de la forma de las piezas, así que dos grupos pueden enfrentar figuras de distinta dificultad y sus sesiones no son directamente comparables. Para el bodystorming no es un problema, pero conviene resolverlo antes de las sesiones con participantes.
**Commit:** `c4d9fd0` (introdujo los cortes irregulares).
