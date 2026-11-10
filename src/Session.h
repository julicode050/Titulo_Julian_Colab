#pragma once
#include "Events.h"
#include "ofMain.h"
#include <fstream>

// Sesión de prueba con una pareja: código de la pareja, un único reloj desde el inicio y el
// registro automático de eventos en CSV (formato fijo para la planilla de Excel de Julián).
// Escucha el punto central de eventos (Events.h); no hace falta tocar los juegos para registrar.
class Session {
public:
	void setup();
	void exit();

	void start(const std::string & pairCode);
	void stop();
	bool started() const { return active; }

	const std::string & pairCode() const { return pair; }
	float elapsed() const { return active ? ofGetElapsedTimef() - t0 : 0; }
	std::string clockText() const; // mm:ss
	const std::string & filePath() const { return mainPath; }

private:
	void onGame(GameEventArgs & e);
	void onToken(TokenEventArgs & e);
	void writeRow(std::ofstream & out, const std::string & game, float t, const std::string & event, const std::string & token);
	std::string seconds(float t) const;

	bool active = false;
	std::string pair;
	float t0 = 0;
	std::string currentGame; // "Ensamblaje", "Resonancia", "Marea" o "" fuera de un juego
	bool skipNextPhase = false; // el primer cambio de fase coincide con el inicio del juego
	std::ofstream mainFile, tokensFile;
	std::string mainPath;
};
