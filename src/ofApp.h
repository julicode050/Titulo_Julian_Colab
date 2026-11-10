#pragma once
#include "Diagnostic.h"
#include "Minigame.h"
#include "Session.h"
#include "Simulator.h"
#include "Token.hpp"
#include "TokenTracker.h"
#include "ofMain.h"
#include <map>
#include <memory>
#include <vector>

enum AppState { APP_SESSION_START, APP_MENU, APP_PLAYING, APP_RESULTS };

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	void keyPressed(int key) override;
	void mousePressed(int x, int y, int button) override;
	void mouseDragged(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;

	// Native Touch Event Callbacks
	void touchDown(ofTouchEventArgs & touch) override;
	void touchMoved(ofTouchEventArgs & touch) override;
	void touchUp(ofTouchEventArgs & touch) override;

private:
	struct Button {
		ofRectangle rect;
		std::string label;
		float dwell = 0; // segundos con un token encima
	};

	void startGame(int index);
	void goToMenu();
	void layoutButtons();
	// Avanza la permanencia de tokens sobre los botones; devuelve el índice elegido o -1.
	int updateDwell(std::vector<Button> & buttons, float dt);
	void drawButtons(const std::vector<Button> & buttons);
	void drawDebugPanel();
	void drawInstructionsPanel();
	void drawClock();
	void drawSessionStart();
	void abortGameIfRunning();
	void startTestMode();
	bool startTestStep(); // inicia el juego del paso actual; false si ya no quedan

	// --- Input & Tracking ---
	Token token; // recepción TUIO y clasificación de formas
	std::map<int, ofVec2f> nativeTouches; // Maps touch ID to screen coords
	TokenTracker tracker;
	Simulator simulator;
	Diagnostic diagnostic;
	Session session;
	std::string typedCode; // código de pareja mientras se escribe

	// --- Juegos ---
	std::vector<std::unique_ptr<Minigame>> games;
	int currentGame = -1;
	AppState state = APP_SESSION_START;
	std::vector<Button> menuButtons;
	std::vector<Button> resultButtons;
	bool showDebug = true;
	bool showInstructions = false;
	bool testMode = false; // "Modo testeo": los juegos seguidos en orden fijo
	int testStep = 0;
	float resultsTime = 0;
	size_t contactCount = 0; // puntos de contacto crudos del último frame
};
