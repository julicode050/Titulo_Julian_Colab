// Tools.hpp
#pragma once
#include "ofMain.h"

// ---------------- ENUMS ----------------
enum class RotationAxis { None, X, Y, Z };

// ---------------- BASE TOOL ----------------
class Tool {
public:
    int id = -1;
    Tool() {}
    Tool(int id) : id(id) {}
    virtual ~Tool() {}

    virtual void update(const vector<ofVec2f>& pts, RotationAxis axis = RotationAxis::None) {}
    virtual void draw(int x, int y) {}
};

// ---------------- ROTATION TOOL ----------------
class RotationTool : public Tool {
public:
    bool  active    = false;
    float lastAngle = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;

    void update(const vector<ofVec2f>& pts, RotationAxis currentAxis = RotationAxis::None) override;
    void draw(int x, int y) override;
};

// ---------------- AXIS SELECTOR TOOL ----------------
class AxisSelectorTool : public Tool {
public:
    RotationAxis currentAxis = RotationAxis::None;
    bool menuActive = false;
    bool lockedMenu = false;
    ofVec2f fixedMenuCenter;
    float buttonRadius  = 140.0f;
    float buttonSpacing = 80.0f;

    void update(const vector<ofVec2f>& pts, RotationAxis axis = RotationAxis::None) override;
    void draw(int x, int y) override;
    void drawMenu();

    void setLockedPoints(const vector<ofVec2f>& lockedPts);
    void clearLocked();
    bool hasLockedMenu() const;
    void updateSelection(const vector<ofVec2f>& currentPts);

private:
    ofVec2f buttonPos(const string& axis) const;
    bool insideCircle(const ofVec2f& p, const ofVec2f& c) const;
    void drawButton(const string& label, const ofColor& color);
   
};

// ---------------- SCALE TOOL ----------------
class ScaleTool : public Tool {
public:
    glm::vec3 origin = glm::vec3(0,0,0);        // centro inicial del token al colocarlo
    glm::vec3 initialScale = glm::vec3(1.0f);   // escala del objeto al colocar el token
    bool locked = false;                         // ya existente

    float initialAvg = 0.0f;
    glm::vec3 scaleVec = glm::vec3(1.0f);

    ScaleTool() : Tool() {}
    ScaleTool(int id) : Tool(id) {}

    void setLockedPoints(const vector<ofVec2f>& lockedPts, const glm::vec3& currentScale);
    void reset();
    void update(const vector<ofVec2f>& pts, RotationAxis axis = RotationAxis::None) override;
    void draw(int x, int y) override;

    glm::vec3 getScale() const { return scaleVec; }
};

// ---------------- TRANSLATE TOOL ----------------
class TranslateTool : public Tool {
public:
    bool locked = false;
    ofVec2f initialCenter;
    glm::vec3 initialPos = glm::vec3(0.0f);

    void setLockedPoints(const vector<ofVec2f>& lockedPts, const glm::vec3& currentPos);
    void reset();
    void update(const vector<ofVec2f>& pts, RotationAxis axis = RotationAxis::None) override;
    void draw(int x, int y) override;
    glm::vec3 getPosition() const { return position; }

private:
    glm::vec3 position = glm::vec3(0.0f);
};

// ---------------- COLOR TOOL ----------------
class ColorTool : public Tool {
public:
    bool locked = false;
    ofVec2f initialCenter;
    ofColor currentColor = ofColor::white;

    void setLockedPoints(const vector<ofVec2f>& lockedPts, const ofColor& startColor);
    void reset();
    void update(const vector<ofVec2f>& pts, RotationAxis axis = RotationAxis::None) override;
    void draw(int x, int y) override;
    ofColor getColor() const { return currentColor; }
};

// ---------------- UTILS ----------------
std::vector<std::vector<ofVec2f>> clusterTouches(const std::vector<ofVec2f>& pts, float threshold);
