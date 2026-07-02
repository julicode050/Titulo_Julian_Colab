#pragma once
#include "Token.hpp"
#include "Tools.hpp" // for clusterTouches
#include "ofMain.h"
#include <map>
#include <vector>

enum GameState { GAME_MENU, GAME_PLAYING, GAME_SUCCESS };
enum GameMode { COOP_MODE, SOLO_MODE };

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void keyPressed(int key) override;

	// Native Touch Event Callbacks
	void touchDown(ofTouchEventArgs & touch) override;
	void touchMoved(ofTouchEventArgs & touch) override;
	void touchUp(ofTouchEventArgs & touch) override;

	// --- Input & Tracking ---
	Token token; // Manages ofxTuio and touch lists
	std::map<int, ofVec2f> nativeTouches; // Maps touch ID to screen coords

	// --- Physics & Logs State ---
	struct FenceLog {
		glm::vec2 posA, posA_prev; // Top end (End A, grabs with Token A)
		glm::vec2 posB, posB_prev; // Bottom end (End B, grabs with Token B/C)
		bool grabbedA = false;
		bool grabbedB = false;
		bool isPlanted = false;
		int grabLossFramesA = 0;
		int grabLossFramesB = 0;
		// Solo mode properties
		bool grabbedSolo = false;
		int grabLossFramesSolo = 0;
		glm::vec2 soloGrabOffsetA;
		glm::vec2 soloGrabOffsetB;
	};
	std::vector<FenceLog> logs;

	float logLength = 240.0f;
	float logRadius = 30.0f;
	float groundY = 0.0f;
	float grabThreshold = 120.0f;

	// --- Target & Gameplay ---
	GameState gameState = GAME_MENU;
	GameMode currentMode = COOP_MODE;

	// Sheep Placeholder
	glm::vec2 sheepPos;
	glm::vec2 sheepVel;

	// --- Active Tokens Classification ---
	struct ActiveToken {
		char id;
		glm::vec2 worldPos;
		glm::vec2 actionPos;
		int clusterIdx;
	};
	std::vector<ActiveToken> activeTokens;
	std::map<char, glm::vec2> tokenOffsets; // Tracks initial offset (X for grabbing, Y for hammer)
	std::map<char, int> tokenTimeouts; // Tracks frames missing for each token
};
