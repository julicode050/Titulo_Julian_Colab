#pragma once
#include "ofMain.h"
#include "Token.hpp"
#include "Tools.hpp"
#include "Shape3D.hpp"
#include <vector>
#include <memory>
#include <map>

class ofApp : public ofBaseApp {
public:
    void setup()  override;
    void update() override;
    void draw()   override;

    void updateCameraWithToken(const std::vector<ofVec2f>& current, bool isLocked);
    

    // --- Members principales ---
    Token token;
    std::vector<std::unique_ptr<Tool>> tools; // [0]=RotationTool, [1]=AxisSelectorTool
    std::map<char, std::unique_ptr<Tool>> tokenTools; // mapping token char -> tool (F,G,H)
    Shape3D shape;
    ofLight light;
    ofMaterial material;
    ofEasyCam cam;

    // --- Estado / UI ---
    glm::vec3 lastScale = glm::vec3(1.0f);
    glm::vec3 lastPosition = glm::vec3(0.0f);
    ofColor shapeColor = ofColor(200);

    // --- Cámara con token A ---
    ofVec2f lastCamPos = ofVec2f(0,0);   // última posición del token
    bool camTokenActive = false;          // está activo el token A?

    // Spherical coords para orbit
    float camRadius = 600.0f;             // distancia al centro del cubo
    float camAzimuth = 0.0f;              // ángulo horizontal (radianes)
    float camElevation = 0.0f;            // ángulo vertical (radianes)
    float camSensitivity = 0.005f;        // ajuste fino (radianes por pixel)
    
    // herramientas
    ScaleTool* scaleTool = nullptr;

    // lista de toques actuales para el token de escala
    std::vector<ofVec2f> scaleTokenTouches;
    
    
};
