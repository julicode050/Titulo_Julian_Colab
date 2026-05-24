#include "Shape3D.hpp"

void Shape3D::setup() {
    box.set(150);
    box.setResolution(2);
    resetTransform();
}

void Shape3D::draw() {
    ofPushMatrix();
    ofTranslate(position);
    ofScale(scale);
    ofRotateXDeg(rotationX);
    ofRotateYDeg(rotationY);
    ofRotateZDeg(rotationZ);
    box.draw();
    ofPopMatrix();
}

void Shape3D::setRotationX(float angle) { rotationX = angle; }
void Shape3D::setRotationY(float angle) { rotationY = angle; }
void Shape3D::setRotationZ(float angle) { rotationZ = angle; }
void Shape3D::setRotation(float angle, const glm::vec3& axis) { /* opcional */ }
void Shape3D::setPosition(const glm::vec3& pos) { position = pos; }
void Shape3D::setScale(const glm::vec3& scaleVec) { scale = scaleVec; }
void Shape3D::resetTransform() {
    position = glm::vec3(0);
    scale = glm::vec3(1);
    rotationX = rotationY = rotationZ = 0;
}
