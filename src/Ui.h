#pragma once
#include "ofMain.h"

// Ayudas de dibujo compartidas por el menú y los minijuegos.
namespace Ui {
void setup();
// Texto centrado en (x, y). size: 0 = pequeño, 1 = mediano, 2 = grande
void text(const std::string & s, float x, float y, int size = 1);
// Anillo de progreso (0..1) empezando arriba, en sentido horario.
void ring(const glm::vec2 & c, float radius, float progress, float thickness);
// Barra horizontal de tiempo restante en el borde superior.
void timerBar(float remaining01, const ofColor & color);

// Paleta común
extern const ofColor background;
extern const ofColor neutral;
extern const ofColor accent;
extern const ofColor accentJoined;
}
