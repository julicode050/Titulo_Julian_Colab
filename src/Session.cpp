#include "Session.h"
#include "Settings.h"

// Nombres de los juegos y textos de los eventos tal como los espera la planilla.
static std::string gameName(const std::string & id) {
	if (id == "ensamblaje") return "Ensamblaje";
	if (id == "resonancia") return "Resonancia";
	if (id == "marea") return "Marea";
	return "";
}

void Session::setup() {
	ofAddListener(Events::game(), this, &Session::onGame);
	ofAddListener(Events::token(), this, &Session::onToken);
}

void Session::exit() {
	stop();
	ofRemoveListener(Events::game(), this, &Session::onGame);
	ofRemoveListener(Events::token(), this, &Session::onToken);
}

void Session::start(const std::string & pairCode) {
	stop();
	auto & r = settings().registro;
	pair = pairCode;
	t0 = ofGetElapsedTimef();
	currentGame.clear();
	active = true;

	std::string folder = ofToDataPath(r.folder, true);
	ofDirectory::createDirectory(folder, false, true);
	std::string base = folder + "/" + pair + "_" + ofGetTimestampString("%Y-%m-%d_%H-%M-%S");
	mainPath = base + ".csv";

	// UTF-8 con BOM y encabezado fijo, para que Excel en español lo abra bien.
	const std::string bom = "\xEF\xBB\xBF";
	const std::string & sep = r.separator;
	std::string header = "Pareja" + sep + "Juego" + sep + "t (s)" + sep + "Evento" + sep + "Token\n";
	mainFile.open(mainPath, std::ios::binary);
	mainFile << bom << header << std::flush;
	if (r.tokensFile) {
		tokensFile.open(base + "_tokens.csv", std::ios::binary);
		tokensFile << bom << header << std::flush;
	}
	ofLogNotice("Session") << "Registro en " << mainPath;
}

void Session::stop() {
	if (!active) return;
	if (!currentGame.empty()) writeRow(mainFile, currentGame, elapsed(), "Juego interrumpido", "No aplica");
	mainFile.close();
	if (tokensFile.is_open()) tokensFile.close();
	active = false;
}

std::string Session::clockText() const {
	int s = (int)elapsed();
	return ofToString(s / 60, 2, '0') + ":" + ofToString(s % 60, 2, '0');
}

std::string Session::seconds(float t) const {
	std::string s = ofToString(t, 1);
	std::replace(s.begin(), s.end(), '.', settings().registro.decimal.empty() ? '.' : settings().registro.decimal[0]);
	return s;
}

void Session::writeRow(std::ofstream & out, const std::string & game, float t, const std::string & event, const std::string & token) {
	if (!out.is_open()) return;
	const std::string & sep = settings().registro.separator;
	out << pair << sep << game << sep << seconds(t) << sep << event << sep << token << "\n" << std::flush;
}

void Session::onGame(GameEventArgs & e) {
	if (!active) return;
	std::string game = gameName(e.game);
	if (game.empty()) return;
	float t = e.time - t0;

	std::string event;
	if (e.name == "game_start") {
		currentGame = game;
		skipNextPhase = true;
		event = "Inicio de juego";
	} else if (e.name == "game_end") {
		event = "Fin de juego";
	} else if (e.name == "game_aborted") {
		event = "Juego interrumpido";
	} else if (e.name == "fragment_fixed") {
		event = "Pieza fijada";
	} else if (e.name == "node_completed") {
		event = "Nodo completado";
	} else if (e.name == "point_emptied") {
		event = "Punto perdido";
	} else if (e.name == "phase_start") {
		if (skipNextPhase) { // la primera fase empieza junto con el juego
			skipNextPhase = false;
			return;
		}
		event = "Cambio de fase";
	} else {
		return; // el resto de los eventos internos no va a la planilla
	}
	writeRow(mainFile, game, t, event, e.token);
	if (e.name == "game_end" || e.name == "game_aborted") currentGame.clear();
}

void Session::onToken(TokenEventArgs & e) {
	if (!active) return;
	float t = e.time - t0;
	switch (e.type) {
	case TokenEventArgs::JOINED:
		if (!currentGame.empty()) writeRow(mainFile, currentGame, t, "Unión", "Par");
		break;
	case TokenEventArgs::SEPARATED:
		// Toda separación se registra como voluntaria; Julián corrige las accidentales con el video.
		if (!currentGame.empty()) writeRow(mainFile, currentGame, t, "Separación voluntaria", "Par");
		break;
	case TokenEventArgs::ADDED:
		writeRow(tokensFile, currentGame.empty() ? "Menú" : currentGame, t, "Aparece", e.labelA);
		break;
	case TokenEventArgs::REMOVED:
		writeRow(tokensFile, currentGame.empty() ? "Menú" : currentGame, t, "Desaparece", e.labelA);
		break;
	}
}
