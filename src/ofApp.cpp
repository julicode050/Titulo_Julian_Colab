#include "ofApp.h"


void ofApp::setup(){
    ofBackground(225);
    ofEnableDepthTest();

    // crear instancia de la herramienta de escala
    scaleTool = new ScaleTool(1);
    
    // shape
    shape.setup();

    // token (tu clase Token usa ofxTuio)
    token.setup(ofVec2f(0,0));

    // herramientas para rotación (mantener en tools vector)
    tools.emplace_back(std::make_unique<RotationTool>());       // index 0
    tools.emplace_back(std::make_unique<AxisSelectorTool>());   // index 1

    // mapear tokens a herramientas específicas (F, G, H)
    tokenTools['F'] = std::make_unique<ScaleTool>();
    tokenTools['G'] = std::make_unique<TranslateTool>();
    tokenTools['H'] = std::make_unique<ColorTool>();

    // luz / material
    light.setup();
    light.setPosition(300,300,600);
    material.setDiffuseColor(ofColor(180));
    material.setSpecularColor(ofColor(255));
    material.setShininess(64);

    cam.setDistance(600);
    // desactivar control con mouse (si quieres que token A lo controle)
    cam.disableMouseInput();

    // inicializar variables esféricas respecto al pivote = centro del shape
    {
        glm::vec3 pivot = shape.getPosition();
        glm::vec3 camPos = cam.getPosition(); // glm::vec3
        glm::vec3 rel = camPos - pivot;
        camRadius = glm::length(rel);
        if (camRadius < 1.0f) camRadius = 1.0f;
        camAzimuth = atan2(rel.z, rel.x); // azimuth alrededor del eje Y
        camElevation = atan2(rel.y, sqrt(rel.x*rel.x + rel.z*rel.z)); // elevación
        cam.lookAt(pivot);
    }

    
    
}

void ofApp::update(){
    token.update();

    auto current = token.getPoints();            // puntos actuales en pantalla (pix)
    auto locked  = token.getLockedPoints();      // snapshot (pix), vacío si no locked
    char id = token.getId();

    // punteros a rotation tools (en tools vector)
    auto* rotTool = dynamic_cast<RotationTool*>(tools[0].get());
    auto* axisTool = dynamic_cast<AxisSelectorTool*>(tools[1].get());

    // ------------ ROTACIÓN (token C) ------------
    if (id == 'C') {
        // crear menú fijo a partir del snapshot
        if (token.isLocked() && axisTool && !axisTool->hasLockedMenu()) {
            axisTool->setLockedPoints(locked);
        }
        // limpiar si token ya no locked
        if (!token.isLocked() && axisTool && axisTool->hasLockedMenu()) {
            axisTool->clearLocked();
        }

        // actualizar selección del eje con puntos actuales
        if (axisTool) axisTool->updateSelection(current);

        // rotator toma current points + eje seleccionado
        if (rotTool) {
            RotationAxis sel = axisTool ? axisTool->currentAxis : RotationAxis::None;
            rotTool->update(current, sel);
            // Aplicar rotación acumulada al shape
            shape.setRotationX(rotTool->rotationX);
            shape.setRotationY(rotTool->rotationY);
            shape.setRotationZ(rotTool->rotationZ);
        }
    } else {
        // Si no es token C, aseguramos que rotation tools no queden activos
        if (axisTool && axisTool->hasLockedMenu()) axisTool->clearLocked();
        if (rotTool) {
            // opcional: no acumular rotación si no está C
            // rotTool->lastAngle = 0; (si tienes esa variable accesible)
        }
    }

    // ------------ ESCALA (token F) ------------
    if (id == 'F') {
        if (!scaleTool->locked && !current.empty()) {
            // --- primera vez que se coloca el token ---
            glm::vec3 tokenCenter(0,0,0);
            for (auto &p : current) tokenCenter += glm::vec3(p.x, p.y, 0);
            tokenCenter /= current.size();

            scaleTool->origin = tokenCenter;            // pivote del escalado
            scaleTool->initialScale = shape.getScale(); // guardar escala inicial
            scaleTool->locked = true;
        }
        else if (scaleTool->locked && !current.empty()) {
            // --- calcular escalado relativo al pivote ---
            glm::vec3 currentCenter(0,0,0);
            for (auto &p : current) currentCenter += glm::vec3(p.x, p.y, 0);
            currentCenter /= current.size();

            glm::vec3 delta = currentCenter - scaleTool->origin;
            float distance = glm::length(delta);        // distancia desde el pivote
            float factor = 1.0f + distance / 200.0f;   // ajustar sensibilidad

            // aplicar escala al objeto
            shape.setScale(scaleTool->initialScale * factor);
        }
    }
    else {
        // token retirado → reset
        if (scaleTool->locked) scaleTool->locked = false;
    }



    // ------------ TRASLACIÓN (token G) ------------
    auto itG = tokenTools.find('G');
    if (itG != tokenTools.end()) {
        TranslateTool* tTool = dynamic_cast<TranslateTool*>(itG->second.get());
        if (tTool) {
            if (id == 'G' && token.isLocked() && !tTool->locked) {
                tTool->setLockedPoints(locked, shape.getPosition());
            }
            if (id != 'G' && tTool->locked) {
                tTool->reset();
            }
            tTool->update(current);
            shape.setPosition(tTool->getPosition());
            lastPosition = shape.getPosition();
        }
    }

    // ------------ COLOR (token H) ------------
    auto itH = tokenTools.find('H');
    if (itH != tokenTools.end()) {
        ColorTool* cTool = dynamic_cast<ColorTool*>(itH->second.get());
        if (cTool) {
            if (id == 'H' && token.isLocked() && !cTool->locked) {
                cTool->setLockedPoints(locked, shapeColor);
            }
            if (id != 'H' && cTool->locked) {
                cTool->reset();
            }
            cTool->update(current);
            shapeColor = cTool->getColor();
        }
    }
    
    // ------------ Cámara (token A) ------------
    if (id == 'A') {
        updateCameraWithToken(current, token.isLocked());
    }

}

void ofApp::draw(){
    // dibujar token y HUD 2D
    token.draw();

    // texto e info siempre debe ir sin luces/material
    ofDisableLighting();
    ofDisableDepthTest();
    ofSetColor(255);
    ofDrawBitmapString("Token detectado: " + string(1, token.getId()), 20, 70);

    // --- 3D ---
    ofEnableDepthTest();
    cam.begin();

        // grid
        ofSetColor(150);
        ofDrawGrid(200.0f, 10, true, false, true, false);

        light.enable();
        material.begin();

        ofSetColor(shapeColor);  // aquí sí color del objeto
        shape.draw();
    

        material.end();
        light.disable();

    cam.end();
    ofDisableDepthTest();

    if (scaleTool) {
            scaleTool->draw(20,20);
        }

    
    
    // --- 2D UI info según token activo ---
    ofDisableLighting();   // 🔑 evita que se aplique luz al texto
    
    // --- Dibujar gizmo de ejes en esquina superior derecha ---
    {
        ofPushStyle();
        ofPushMatrix();

        int gizmoSize = 100;
        int margin = 10;

        // viewport exclusivo del gizmo
        ofViewport(ofGetWidth() - gizmoSize - margin, margin, gizmoSize, gizmoSize);
        ofSetupScreen();

        ofTranslate(gizmoSize/2, gizmoSize/2, 0);
        ofMultMatrix(glm::toMat4(cam.getOrientationQuat()));

        ofSetLineWidth(3);
        int axisLen = 35;
        int textOffset = 10;

        ofSetColor(255,0,0);
        ofDrawLine(0,0,0, axisLen,0,0);
        ofDrawBitmapString("X", axisLen + textOffset, 0);

        ofSetColor(0,255,0);
        ofDrawLine(0,0,0, 0,axisLen,0);
        ofDrawBitmapString("Y", 0, axisLen + textOffset);

        ofSetColor(0,0,255);
        ofDrawLine(0,0,0, 0,0,axisLen);
        ofDrawBitmapString("Z", 0, textOffset, axisLen);

        ofPopMatrix();
        ofPopStyle();

        // restaurar viewport normal
        ofViewport();
        ofSetupScreen(); // 👈 asegura volver a la proyección estándar
    }
    
    ofSetColor(255);

    int y = 30;
    char id = token.getId();
    if (id == 'C') {
        RotationTool* rT = dynamic_cast<RotationTool*>(tools[0].get());
        AxisSelectorTool* aT = dynamic_cast<AxisSelectorTool*>(tools[1].get());
        if (aT) aT->drawMenu();
        if (rT) {
            ofDrawBitmapString("Rot X: " + ofToString(rT->rotationX,2), 20, y+=15);
            ofDrawBitmapString("Rot Y: " + ofToString(rT->rotationY,2), 20, y+=15);
            ofDrawBitmapString("Rot Z: " + ofToString(rT->rotationZ,2), 20, y+=15);
        }
    }
    else if (id == 'F') {
        auto itF = tokenTools.find('F');
        if (itF != tokenTools.end()){
            ScaleTool* sTool = dynamic_cast<ScaleTool*>(itF->second.get());
            if (sTool) ofDrawBitmapString("Escala: " + ofToString(sTool->getScale().x, 2), 20, y+=15);
        }
    }
    else if (id == 'G') {
        ofDrawBitmapString("Pos: " + ofToString(lastPosition.x,1) + ", " + ofToString(lastPosition.y,1), 20, y+=15);
    }
    else if (id == 'H') {
        ofDrawBitmapString("Color H: " + ofToString(shapeColor.getHue()), 20, y+=15);
    }
}



void ofApp::updateCameraWithToken(const std::vector<ofVec2f>& current, bool isLocked) {
    static const float smoothing = 0.1f; // factor de suavizado [0..1]
    
    if (isLocked && !current.empty()) {
        // --- Centro del token ---
        ofVec2f c(0,0);
        for (auto &p: current) c += p;
        c /= (float)current.size();

        // --- Reinicio al aparecer el token ---
        if (!camTokenActive) {
            lastCamPos = c;                  // reset posición previa del token
            token.prevAngle = 0.0f;          // reset ángulo previo
            token.hasInitialAngle = false;   // permitirá inicializar ángulo correctamente
        }

        // -------- ORBITA DE LA CÁMARA --------
        if (camTokenActive || !token.hasInitialAngle) {
            ofVec2f delta = c - lastCamPos;

            camAzimuth  += -delta.x * camSensitivity;
            camElevation += -delta.y * camSensitivity;

            // limitar elevación a ±89°
            float maxElev = glm::radians(89.0f);
            camElevation = glm::clamp(camElevation, -maxElev, maxElev);

            // coordenadas esféricas → cartesianas
            float cosEl = cos(camElevation);
            glm::vec3 relNew;
            relNew.x = camRadius * cosEl * cos(camAzimuth);
            relNew.y = camRadius * sin(camElevation);
            relNew.z = camRadius * cosEl * sin(camAzimuth);

            glm::vec3 pivot = shape.getPosition();
            glm::vec3 targetCamPos = pivot + relNew;

            // --- suavizado ---
            glm::vec3 smoothPos = cam.getPosition() * (1.0f - smoothing) + targetCamPos * smoothing;
            cam.setPosition(smoothPos);
            cam.lookAt(pivot);
        }

        // -------- ZOOM CON ROTACIÓN DEL TOKEN --------
        if (current.size() >= 2) {
            glm::vec2 p0 = current[0];
            glm::vec2 p1 = current[1];
            glm::vec2 dir = glm::normalize(p1 - p0);

            float angle = atan2(dir.y, dir.x);

            if (!token.hasInitialAngle) {
                token.prevAngle = angle;
                token.hasInitialAngle = true;
            } else {
                float deltaAngle = angle - token.prevAngle;

                // normalizar ángulo en [-PI, PI]
                if (deltaAngle > PI) deltaAngle -= TWO_PI;
                if (deltaAngle < -PI) deltaAngle += TWO_PI;

                // ajustar velocidad de zoom
                float zoomSpeed = 1500.0f;
                float targetRadius = camRadius - deltaAngle * zoomSpeed;

                // limitar rango
                targetRadius = glm::clamp(targetRadius, 100.0f, 2000.0f);

                // suavizado del zoom
                camRadius = camRadius * (1.0f - smoothing) + targetRadius * smoothing;

                token.prevAngle = angle;
            }
        }

        lastCamPos = c;
        camTokenActive = true;
    } else {
        camTokenActive = false;
        token.hasInitialAngle = false; // reset al retirar token
    }
}

