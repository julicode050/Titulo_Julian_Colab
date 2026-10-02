#pragma once
#include "Minigame.h"
#include "Simulator.h"
#include "Token.hpp"
#include "TokenTracker.h"
#include "ofMain.h"
#include <map>
#include <memory>
#include <vector>

enum AppState { APP_MENU, APP_PLAYING, APP_RESULTS };

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
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

	// --- Input & Tracking ---
	Token token; // recepción TUIO y clasificación de formas
	std::map<int, ofVec2f> nativeTouches; // Maps touch ID to screen coords
	TokenTracker tracker;
	Simulator simulator;

	// --- Juegos ---
	std::vector<std::unique_ptr<Minigame>> games;
	int currentGame = -1;
	AppState state = APP_MENU;
	std::vector<Button> menuButtons;
	std::vector<Button> resultButtons;
	bool showDebug = false;
};
