#pragma once
#include "ofMain.h"
#include "Token.hpp"
#include "Tools.hpp" // for clusterTouches
#include <vector>
#include <map>

enum GameState { GAME_PLAYING, GAME_SUCCESS };

class ofApp : public ofBaseApp {
public:
    void setup()  override;
    void update() override;
    void draw()   override;
    void keyPressed(int key) override;

    // Native Touch Event Callbacks
    void touchDown(ofTouchEventArgs & touch) override;
    void touchMoved(ofTouchEventArgs & touch) override;
    void touchUp(ofTouchEventArgs & touch) override;

    // --- Input & Tracking ---
    Token token; // Manages ofxTuio and touch lists
    std::map<int, ofVec2f> nativeTouches; // Maps touch ID to screen coords

    // --- Physics & Log State ---
    glm::vec2 posA, posA_prev;
    glm::vec2 posB, posB_prev;
    float logLength = 250.0f;
    float logRadius = 20.0f;
    float groundY = 0.0f;

    // --- Grabbing State ---
    bool grabbedA = false;
    bool grabbedB = false;
    int grabberIdA = -1; // index of touch cluster grabbing End A
    int grabberIdB = -1; // index of touch cluster grabbing End B
    float grabThreshold = 100.0f;

    // --- Target & Gameplay ---
    GameState gameState = GAME_PLAYING;
    glm::vec2 targetPos;
    float targetRadius = 90.0f;

    // --- Active Tokens Classification ---
    struct ActiveToken {
        char id;
        glm::vec2 worldPos;
        int clusterIdx;
    };
    std::vector<ActiveToken> activeTokens;
};
