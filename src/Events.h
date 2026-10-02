#pragma once
#include "ofMain.h"

// Punto único por donde pasan todos los eventos del sistema.
// Para agregar logging (CSV/JSON) más adelante basta con escuchar estos dos eventos:
//   ofAddListener(Events::token(), this, &Logger::onToken);
//   ofAddListener(Events::game(),  this, &Logger::onGame);
// No hace falta tocar el tracker ni los minijuegos.

struct TokenEventArgs {
	enum Type { ADDED, REMOVED, JOINED, SEPARATED };
	Type type;
	int uidA = -1; // token principal
	int uidB = -1; // segundo token (solo JOINED / SEPARATED)
	std::string labelA;
	std::string labelB;
	glm::vec2 pos; // px
	float time = 0; // segundos desde el inicio de la app
};

struct GameEventArgs {
	std::string game; // "ensamblaje", "resonancia", "marea", "menu"
	std::string name; // p. ej. "cycle_start", "node_completed"
	std::string detail; // texto libre (valores, ids)
	float time = 0;
};

namespace Events {
inline ofEvent<TokenEventArgs> & token() {
	static ofEvent<TokenEventArgs> e;
	return e;
}
inline ofEvent<GameEventArgs> & game() {
	static ofEvent<GameEventArgs> e;
	return e;
}
inline void notifyGame(const std::string & game, const std::string & name, const std::string & detail = "") {
	GameEventArgs a { game, name, detail, ofGetElapsedTimef() };
	ofNotifyEvent(Events::game(), a);
}
}
