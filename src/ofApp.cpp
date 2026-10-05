#include "ofApp.h"
#include "Ensamblaje.h"
#include "Marea.h"
#include "Resonancia.h"
#include "Settings.h"
#include "Ui.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetWindowTitle("Titulo Julian Colab - Minijuegos");
	ofSetFrameRate(60);
	ofSetCircleResolution(64);
	ofEnableAlphaBlending();

	settings().load(ofToDataPath("settings.json", true));
	Ui::setup();

	// Initialize token tracking
	token.setup(ofVec2f(0, 0));
	tracker.setup(&token);
	simulator.setup();

	games.push_back(std::make_unique<Ensamblaje>());
	games.push_back(std::make_unique<Resonancia>());
	games.push_back(std::make_unique<Marea>());

	layoutButtons();
}

void ofApp::layoutButtons() {
	float w = settings().cm(13.0f);
	float h = settings().cm(5.0f);
	float gap = settings().cm(1.5f);
	float total = games.size() * w + (games.size() - 1) * gap;
	float x = (ofGetWidth() - total) * 0.5f;
	float y = ofGetHeight() * 0.5f - h * 0.5f;

	menuButtons.clear();
	for (size_t i = 0; i < games.size(); ++i)
		menuButtons.push_back({ ofRectangle(x + i * (w + gap), y, w, h), games[i]->title() });

	resultButtons = { { ofRectangle(ofGetWidth() * 0.5f - w * 0.5f, ofGetHeight() * 0.75f, w, h * 0.7f), "Volver al menú" } };
}

//--------------------------------------------------------------
void ofApp::update() {
	float dt = std::min((float)ofGetLastFrameTime(), 0.1f);

	// Update token tracker (TUIO)
	token.update();

	simulator.update(dt);

	// Todos los contactos: TUIO + táctil nativo de macOS + simulador
	auto pts = token.getPoints();
	for (auto const & pair : nativeTouches)
		pts.push_back(pair.second);
	auto simPts = simulator.getPoints();
	pts.insert(pts.end(), simPts.begin(), simPts.end());

	contactCount = pts.size();
	tracker.update(pts);

	if (state == APP_MENU) {
		int chosen = updateDwell(menuButtons, dt);
		if (chosen >= 0) startGame(chosen);
	} else if (state == APP_PLAYING) {
		games[currentGame]->update(dt, tracker);
		if (games[currentGame]->isFinished()) {
			state = APP_RESULTS;
			for (auto & b : resultButtons)
				b.dwell = 0;
		}
	} else if (state == APP_RESULTS) {
		if (updateDwell(resultButtons, dt) >= 0) goToMenu();
	}
}

int ofApp::updateDwell(std::vector<Button> & buttons, float dt) {
	// Un token debe permanecer sobre el botón para elegirlo; evita activaciones accidentales.
	for (int i = 0; i < (int)buttons.size(); ++i) {
		bool covered = false;
		for (auto & t : tracker.getTokens())
			if (t.visible && buttons[i].rect.inside(t.pos.x, t.pos.y)) covered = true;
		buttons[i].dwell = covered ? buttons[i].dwell + dt : 0;
		if (buttons[i].dwell >= settings().menuDwellSec) {
			buttons[i].dwell = 0;
			return i;
		}
	}
	return -1;
}

void ofApp::startGame(int index) {
	currentGame = index;
	state = APP_PLAYING;
	games[index]->start();
}

void ofApp::goToMenu() {
	if (state == APP_PLAYING) Events::notifyGame(games[currentGame]->id(), "game_aborted");
	state = APP_MENU;
	for (auto & b : menuButtons)
		b.dwell = 0;
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(Ui::background);

	if (state == APP_MENU) {
		drawButtons(menuButtons);
	} else if (state == APP_PLAYING) {
		games[currentGame]->draw(tracker);
	} else if (state == APP_RESULTS) {
		games[currentGame]->drawResults();
		drawButtons(resultButtons);
	}
	if (state != APP_PLAYING) Minigame::drawTokens(tracker);

	simulator.draw();

	if (showDebug) {
		tracker.drawDebug();
		drawDebugPanel();
	}
	if (showInstructions) drawInstructionsPanel();
}

// La fuente bitmap del panel de debug no tiene tildes: se reemplazan por letras simples.
static std::string asciiFold(std::string s) {
	static const std::vector<std::pair<std::string, std::string>> map = {
		{ "á", "a" }, { "é", "e" }, { "í", "i" }, { "ó", "o" }, { "ú", "u" }, { "ü", "u" }, { "ñ", "n" },
		{ "Á", "A" }, { "É", "E" }, { "Í", "I" }, { "Ó", "O" }, { "Ú", "U" }, { "Ñ", "N" },
	};
	for (auto & [from, to] : map)
		ofStringReplace(s, from, to);
	return s;
}

void ofApp::drawInstructionsPanel() {
	// Mismo estilo que el panel de debug, alineado a la derecha.
	std::vector<std::string> lines;
	if (state == APP_MENU || currentGame < 0) {
		lines = { "INSTRUCCIONES (I para ocultar)", "",
			"Elige un minijuego para ver sus instrucciones:",
			"teclas 1/2/3, clic, o un token 1,5 s sobre el boton." };
	} else {
		lines = games[currentGame]->instructions();
		lines.insert(lines.begin(), { "INSTRUCCIONES: " + ofToUpper(games[currentGame]->title()) + " (I para ocultar)", "" });
	}

	size_t maxChars = 0;
	for (auto & l : lines) {
		l = asciiFold(l);
		maxChars = std::max(maxChars, l.size());
	}
	float x = ofGetWidth() - 16 - maxChars * 8; // la fuente bitmap mide 8 px por carácter

	ofPushStyle();
	int y = 24;
	for (auto & l : lines) {
		if (!l.empty()) ofDrawBitmapStringHighlight(l, x, y, ofColor(0, 0, 0, 180), ofColor(255));
		y += 18;
	}
	ofPopStyle();
}

void ofApp::drawDebugPanel() {
	// Texto solo ASCII: la fuente bitmap no tiene tildes.
	std::vector<std::string> lines;
	std::string screen = state == APP_MENU ? "Menu"
		: state == APP_PLAYING              ? "Jugando: " + games[currentGame]->id()
											: "Resultados: " + games[currentGame]->id();
	lines.push_back("DEBUG (D para ocultar)");
	lines.push_back("FPS " + ofToString(ofGetFrameRate(), 0) + "  |  " + screen);
	lines.push_back("Contactos: " + ofToString(contactCount) + "  |  Dedos/sueltos: " + ofToString(tracker.getFingers().size())
		+ "  |  Simulador: " + (simulator.active ? "ON" : "off"));
	lines.push_back("");

	auto & tokens = tracker.getTokens();
	lines.push_back("Tokens detectados: " + ofToString(tokens.size()));
	if (tokens.empty()) lines.push_back("  (ninguno)");
	for (auto & t : tokens) {
		std::string line = "  " + t.label + " #" + ofToString(t.uid)
			+ "  forma '" + std::string(1, t.rawLetter) + "'"
			+ "  " + ofToString(t.points.size()) + " pts"
			+ "  (" + ofToString(t.pos.x / settings().pxPerCm, 1) + ", " + ofToString(t.pos.y / settings().pxPerCm, 1) + ") cm";
		if (!t.visible) line += "  [perdido " + ofToString(t.missingFrames) + " frames]";

		int g = tracker.groupIndexOf(t.uid);
		if (g >= 0 && tracker.getGroups()[g].isJoined()) {
			std::string others;
			for (int uid : tracker.getGroups()[g].uids) {
				if (uid == t.uid) continue;
				auto * o = tracker.findToken(uid);
				others += (others.empty() ? "" : ", ") + o->label + "#" + ofToString(uid);
			}
			line += "  UNIDO con " + others;
		}
		lines.push_back(line);
	}

	int joined = 0;
	for (auto & g : tracker.getGroups())
		if (g.isJoined()) joined++;
	lines.push_back("Grupos unidos: " + ofToString(joined));
	lines.push_back("");
	lines.push_back("1/2/3 juego | M menu | R reiniciar | S simulador | I instrucciones | F pantalla completa");

	ofPushStyle();
	int y = 24;
	for (auto & l : lines) {
		if (!l.empty()) ofDrawBitmapStringHighlight(l, 16, y, ofColor(0, 0, 0, 180), ofColor(255));
		y += 18;
	}
	ofPopStyle();
}

void ofApp::drawButtons(const std::vector<Button> & buttons) {
	ofPushStyle();
	for (auto & b : buttons) {
		ofNoFill();
		ofSetLineWidth(2);
		ofSetColor(Ui::neutral);
		ofDrawRectRounded(b.rect, 12);
		ofSetColor(255);
		Ui::text(b.label, b.rect.getCenter().x, b.rect.getCenter().y, 1);

		if (b.dwell > 0) {
			ofSetColor(Ui::accent);
			Ui::ring(glm::vec2(b.rect.getCenter().x, b.rect.getBottom() - settings().cm(1.0f)), settings().cm(0.5f),
				b.dwell / settings().menuDwellSec, 4);
		}
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (simulator.keyPressed(key)) return;

	switch (key) {
	case '1':
	case '2':
	case '3':
		startGame(key - '1');
		break;
	case 'm':
	case 'M':
		goToMenu();
		break;
	case 'r':
	case 'R':
		if (currentGame >= 0 && state != APP_MENU) startGame(currentGame);
		break;
	case 's':
	case 'S':
		simulator.active = !simulator.active;
		break;
	case 'i':
	case 'I':
		showInstructions = !showInstructions;
		break;
	case 'd':
	case 'D':
		showDebug = !showDebug;
		break;
	case 'f':
	case 'F':
		ofToggleFullscreen();
		break;
	}
}

void ofApp::mousePressed(int x, int y, int button) {
	if (simulator.mousePressed(x, y)) return;

	// Clic directo del investigador (sin permanencia)
	auto & buttons = state == APP_MENU ? menuButtons : resultButtons;
	if (state == APP_PLAYING) return;
	for (int i = 0; i < (int)buttons.size(); ++i) {
		if (!buttons[i].rect.inside(x, y)) continue;
		if (state == APP_MENU)
			startGame(i);
		else
			goToMenu();
		return;
	}
}

void ofApp::mouseDragged(int x, int y, int button) {
	simulator.mouseDragged(x, y);
}

void ofApp::mouseReleased(int x, int y, int button) {
	simulator.mouseReleased();
}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs & touch) {
	nativeTouches[touch.id] = ofVec2f(touch.x, touch.y);
}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs & touch) {
	nativeTouches[touch.id] = ofVec2f(touch.x, touch.y);
}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs & touch) {
	nativeTouches.erase(touch.id);
}
