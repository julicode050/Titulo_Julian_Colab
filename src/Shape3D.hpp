// Shape3D.hpp
#pragma once
#include "ofMain.h"

class Shape3D {
public:
    void setup();
    void draw();

    void setRotationX(float angle);
    void setRotationY(float angle);
    void setRotationZ(float angle);
    void setRotation(float angle, const glm::vec3& axis);

    void setPosition(const glm::vec3& pos);
    void setScale(const glm::vec3& scaleVec);
    void resetTransform();

    // getters que usamos desde ofApp
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getScale() const { return scale; }
    float getRotationX() const { return rotationX; }
    float getRotationY() const { return rotationY; }
    float getRotationZ() const { return rotationZ; }
    glm::vec3 getOrientationEulerDeg() const { return glm::vec3(rotationX, rotationY, rotationZ); }

private:
    ofBoxPrimitive box;
    glm::vec3 position;
    glm::vec3 scale;
    float rotationX = 0, rotationY = 0, rotationZ = 0;
};
