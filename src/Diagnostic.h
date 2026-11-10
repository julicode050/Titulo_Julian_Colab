#pragma once
#include "Events.h"
#include "TokenTracker.h"
#include "ofMain.h"
#include <deque>

// Modo diagnóstico (tecla P): para probetas y pruebas de agarre, sin reglas de juego.
// Muestra cada contacto crudo con su número y posición, las distancias entre contactos en mm,
// qué reconoce el sistema (A, B leído como E, o el par encajado) y un registro con hora de
// las apariciones y desapariciones de contactos y tokens.
class Diagnostic {
public:
	bool active = false;

	void setup();
	void exit();
	void update(const std::vector<ofVec2f> & pts);
	void draw(const TokenTracker & tracker) const;
	void resetCounters();

private:
	struct Contact {
		int id;
		glm::vec2 pos;
		float since; // segundo (desde el reinicio) en que apareció
	};

	void onToken(TokenEventArgs & e);
	void log(const std::string & line);
	float now() const { return ofGetElapsedTimef() - t0; }

	std::vector<Contact> contacts;
	int nextId = 1;
	float t0 = 0;
	int appearances = 0, disappearances = 0, briefContacts = 0;
	std::deque<std::pair<float, int>> countHistory; // (tiempo, contactos) de los últimos segundos
	std::deque<std::string> events; // más reciente primero
};
