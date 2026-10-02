#pragma once
#include "ofMain.h"

// Simulador de escritorio: dos tokens virtuales que se arrastran con el mouse.
// Genera puntos de contacto sintéticos con la forma de cada token (A = triángulo
// equilátero, B = isósceles), que entran al mismo pipeline que los puntos reales
// (clusterTouches -> classifyShape -> TokenTracker). Sirve para probar los minijuegos
// sin la pantalla táctil ni los tokens físicos.
class Simulator {
public:
	bool active = false;

	void setup();
	void update(float dt);
	std::vector<ofVec2f> getPoints() const;
	// Desliza el token i hacia `target` (los tokens reales se mueven de forma continua,
	// así el tracker conserva su identidad).
	void moveTo(int i, const glm::vec2 & target);
	bool arrived(int i) const;
	void draw() const;

	// Devuelven true si el simulador consumió el evento.
	bool mousePressed(int x, int y);
	bool mouseDragged(int x, int y);
	void mouseReleased();
	bool keyPressed(int key);

private:
	struct SimToken {
		std::string label;
		glm::vec2 pos;
		glm::vec2 target;
		float angle = 0; // radianes
		bool present = true;
	};
	std::vector<ofVec2f> shapePoints(const SimToken & t) const;
	glm::vec2 joinedOffset() const;
	void applyJoin();

	SimToken tokens[2];
	bool joined = false;
	int dragging = -1;
	int lastTouched = 0;
	glm::vec2 dragOffset;
};
