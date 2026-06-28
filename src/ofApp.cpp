#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofBackground(35);
    ofEnableDepthTest();
    ofSetWindowTitle("Titulo Julian Colab");

    // Camera setup
    cam.setDistance(600);
    cam.lookAt(glm::vec3(0, 0, 0));
    cam.disableMouseInput(); // Keep view locked for touchscreen gameplay

    // Light and material
    light.setup();
    light.setPosition(200, 500, 400);

    material.setDiffuseColor(ofColor(139, 90, 43)); // Wood brown
    material.setSpecularColor(ofColor(255));
    material.setShininess(32);

    // Initialize token tracking
    token.setup(ofVec2f(0,0));

    // Reset game state and log position
    keyPressed('r');
}

//--------------------------------------------------------------
glm::vec3 ofApp::screenToWorldPlane(float x, float y, float planeZ) {
    glm::vec3 pNear = cam.screenToWorld(glm::vec3(x, y, 0.0f));
    glm::vec3 pFar = cam.screenToWorld(glm::vec3(x, y, 1.0f));
    glm::vec3 dir = pFar - pNear;
    if (abs(dir.z) < 0.0001f) return pNear;
    float t = (planeZ - pNear.z) / dir.z;
    return pNear + t * dir;
}

//--------------------------------------------------------------
void ofApp::update(){
    // Update token tracker
    token.update();

    // Get screen points (pixels)
    auto pts = token.getPoints();

    // Add points from native macOS touchscreen events
    for (auto const& pair : nativeTouches) {
        pts.push_back(pair.second);
    }

    // Fallback: If no TUIO or native points are detected, use the mouse/single-touch cursor
    if (pts.empty() && ofGetMousePressed()) {
        pts.push_back(ofVec2f(ofGetMouseX(), ofGetMouseY()));
    }
    
    // A threshold of 150px works well for standard touchscreens
    auto clusters = clusterTouches(pts, 150.0f);

    // Classify each cluster and populate activeTokens list
    activeTokens.clear();
    for (int i = 0; i < (int)clusters.size(); ++i) {
        if (clusters[i].empty()) continue;
        
        // 1. Centroid in screen pixels
        ofVec2f centroid(0, 0);
        for (auto &p : clusters[i]) centroid += p;
        centroid /= (float)clusters[i].size();

        // 2. Classify shape of this cluster
        char tokenID = '?';
        if (clusters[i].size() >= 3) {
            auto ordered = token.orderByAngle(clusters[i]);
            tokenID = token.classifyShape(ordered);
        } else if (clusters[i].size() == 1) {
            // Debug simulation: Hold key A/C/B/F/G/H while dragging to simulate a token
            if (ofGetKeyPressed('a') || ofGetKeyPressed('A')) tokenID = 'A';
            else if (ofGetKeyPressed('c') || ofGetKeyPressed('C')) tokenID = 'C';
            else if (ofGetKeyPressed('b') || ofGetKeyPressed('B')) tokenID = 'B';
            else if (ofGetKeyPressed('f') || ofGetKeyPressed('F')) tokenID = 'F';
            else if (ofGetKeyPressed('g') || ofGetKeyPressed('G')) tokenID = 'G';
            else if (ofGetKeyPressed('h') || ofGetKeyPressed('H')) tokenID = 'H';
        }

        // 3. Project to 3D world plane
        glm::vec3 wPos = screenToWorldPlane(centroid.x, centroid.y, 0.0f);
        activeTokens.push_back({tokenID, wPos, i});
    }

    // --- Grabbing logic ---
    // Release End A if the token 'A' is no longer near posA
    if (grabbedA) {
        bool found = false;
        for (auto &t : activeTokens) {
            if (t.id == 'A' && glm::distance(t.worldPos, posA) < grabThreshold) {
                posA = t.worldPos;
                found = true;
                break;
            }
        }
        if (!found) grabbedA = false;
    }

    // Release End B if token 'C' or 'B' is no longer near posB
    if (grabbedB) {
        bool found = false;
        for (auto &t : activeTokens) {
            if ((t.id == 'C' || t.id == 'B') && glm::distance(t.worldPos, posB) < grabThreshold) {
                posB = t.worldPos;
                found = true;
                break;
            }
        }
        if (!found) grabbedB = false;
    }

    // Try grabbing End A with Token A
    if (!grabbedA) {
        for (auto &t : activeTokens) {
            if (t.id == 'A' && glm::distance(t.worldPos, posA) < grabThreshold) {
                grabbedA = true;
                posA = t.worldPos;
                break;
            }
        }
    }

    // Try grabbing End B with Token C or B
    if (!grabbedB) {
        for (auto &t : activeTokens) {
            if ((t.id == 'C' || t.id == 'B') && glm::distance(t.worldPos, posB) < grabThreshold) {
                grabbedB = true;
                posB = t.worldPos;
                break;
            }
        }
    }

    // --- Physics update ---
    if (gameState == GAME_PLAYING) {
        float dt = ofGetLastFrameTime();
        if (dt > 0.1f) dt = 0.1f; // Cap dt to prevent instability
        
        glm::vec3 gravity(0, -800.0f, 0); // Ground-directed gravity

        // Verlet Integration for free endpoints
        if (!grabbedA) {
            glm::vec3 temp = posA;
            posA += (posA - posA_prev) + gravity * dt * dt;
            posA_prev = temp;
        } else {
            // Smoothly track current velocity to avoid huge kinetic energy on release
            posA_prev = posA - (posA - posA_prev) * 0.4f;
        }

        if (!grabbedB) {
            glm::vec3 temp = posB;
            posB += (posB - posB_prev) + gravity * dt * dt;
            posB_prev = temp;
        } else {
            posB_prev = posB - (posB - posB_prev) * 0.4f;
        }

        // Ground Collision resolution
        float bounce = 0.4f;
        float friction = 0.85f;
        if (posA.y < groundY) {
            glm::vec3 vel = posA - posA_prev;
            posA.y = groundY;
            vel.y = -vel.y * bounce;
            vel.x *= friction;
            vel.z *= friction;
            posA_prev = posA - vel;
        }
        if (posB.y < groundY) {
            glm::vec3 vel = posB - posB_prev;
            posB.y = groundY;
            vel.y = -vel.y * bounce;
            vel.x *= friction;
            vel.z *= friction;
            posB_prev = posB - vel;
        }

        // Rigid length constraint relaxation (maintain logLength)
        for (int step = 0; step < 4; ++step) {
            glm::vec3 delta = posA - posB;
            float currentDist = glm::length(delta);
            if (currentDist > 0.001f) {
                float diff = logLength - currentDist;
                glm::vec3 offset = (delta / currentDist) * diff * 0.5f;

                if (!grabbedA && !grabbedB) {
                    posA += offset;
                    posB -= offset;
                } else if (grabbedA) {
                    // Endpoint A is fixed by player, offset goes entirely to B
                    posB -= offset * 2.0f;
                } else if (grabbedB) {
                    // Endpoint B is fixed by player, offset goes entirely to A
                    posA += offset * 2.0f;
                }
            }
        }

        // --- Victory check ---
        // Player must release the log, one end must stand in target field, vertical orientation
        if (!grabbedA && !grabbedB) {
            // Check if either end stands on the floor in the circle
            glm::vec3 bottomEnd = (posA.y < posB.y) ? posA : posB;
            glm::vec3 topEnd = (posA.y < posB.y) ? posB : posA;

            bool onGround = (bottomEnd.y <= groundY + 5.0f);
            float distToTarget = glm::distance(glm::vec2(bottomEnd.x, bottomEnd.z), glm::vec2(targetPos.x, targetPos.z));
            bool inTarget = (distToTarget < targetRadius);

            glm::vec3 logDir = glm::normalize(topEnd - bottomEnd);
            bool isVertical = (logDir.y > 0.94f); // vertical angle close to 90 degrees

            // Velocity checks for stability
            float speedA = glm::length(posA - posA_prev);
            float speedB = glm::length(posB - posB_prev);
            bool isStable = (speedA < 0.25f && speedB < 0.25f);

            if (onGround && inTarget && isVertical && isStable) {
                gameState = GAME_SUCCESS;
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    // Draw viewport and HUD
    ofDisableLighting();
    ofDisableDepthTest();
    
    // Draw 3D scene
    ofEnableDepthTest();
    cam.begin();

        // 1. Draw Grid Ground
        ofSetColor(80);
        ofDrawGrid(100.0f, 12, true, false, true, false);

        // 2. Draw Target Field (Green Circle flat on the floor)
        ofPushMatrix();
        ofTranslate(targetPos);
        ofRotateXDeg(90);
        ofNoFill();
        ofSetLineWidth(3);
        ofSetColor(0, 230, 110);
        ofDrawCircle(0, 0, targetRadius);
        ofFill();
        ofSetColor(0, 230, 110, 40); // transparent fill
        ofDrawCircle(0, 0, targetRadius);
        ofPopMatrix();

        // Enable light for log
        light.enable();
        material.begin();

            // 3. Draw Log Cylinder
            glm::vec3 center = (posA + posB) * 0.5f;
            glm::vec3 dir = posA - posB;
            float len = glm::length(dir);
            
            if (gameState == GAME_SUCCESS) {
                ofSetColor(50, 220, 100); // Bright green
            } else if (grabbedA && grabbedB) {
                ofSetColor(240, 130, 20); // Bright orange when fully grabbed
            } else if (grabbedA || grabbedB) {
                ofSetColor(210, 180, 50); // Muted gold when partially grabbed
            } else {
                ofSetColor(139, 90, 43); // Wood brown when free
            }

            ofPushMatrix();
            ofTranslate(center);
            
            // Rotate cylinder to align Y axis with direction vector
            glm::vec3 localY(0, 1, 0);
            ofQuaternion q;
            q.makeRotate(localY, dir);
            float rotAngle;
            ofVec3f rotAxis;
            q.getRotate(rotAngle, rotAxis);
            ofRotateDeg(rotAngle, rotAxis.x, rotAxis.y, rotAxis.z);
            
            ofDrawCylinder(logRadius, len);
            ofPopMatrix();

        material.end();
        light.disable();

        // 4. Draw Grabbing Spheres on ends (Visual aids)
        ofNoFill();
        ofSetLineWidth(2);
        
        // End A
        ofSetColor(grabbedA ? ofColor::red : ofColor::lightGray);
        ofDrawSphere(posA, grabbedA ? 15.0f : 8.0f);
        
        // End B
        ofSetColor(grabbedB ? ofColor::red : ofColor::lightGray);
        ofDrawSphere(posB, grabbedB ? 15.0f : 8.0f);

        // 5. Draw active touchscreen/token projected coordinates in 3D space
        ofFill();
        for (auto &t : activeTokens) {
            ofSetColor(255, 100, 100, 180);
            ofDrawSphere(t.worldPos, 12.0f);
            
            // Draw classified token ID
            string label = "Token: ";
            label += t.id;
            if (t.id == '?') label = "Finger/Unknown";
            
            ofSetColor(255);
            ofDrawBitmapString(label, t.worldPos.x + 15, t.worldPos.y + 15, t.worldPos.z);
        }

    cam.end();
    ofDisableDepthTest();
    ofDisableLighting();

    // --- 2D UI HUD Overlay ---
    ofSetColor(255);
    ofDrawBitmapString("Titulo Julian Colab - Log Lift & Place Game", 25, 40);
    
    int yOffset = 70;
    
    // Aggregate native touches for points display
    auto ptsDraw = token.getPoints();
    for (auto const& pair : nativeTouches) {
        ptsDraw.push_back(pair.second);
    }
    if (ptsDraw.empty() && ofGetMousePressed()) {
        ptsDraw.push_back(ofVec2f(ofGetMouseX(), ofGetMouseY()));
    }

    ofDrawBitmapString("Total Touch Points: " + ofToString(ptsDraw.size()), 25, yOffset += 20);
    ofDrawBitmapString("Active Token Clusters: " + ofToString(activeTokens.size()), 25, yOffset += 20);
    
    yOffset += 10;
    ofDrawBitmapString("Recognized Tokens:", 25, yOffset += 20);
    if (activeTokens.empty()) {
        ofDrawBitmapString("  (None)", 25, yOffset += 15);
    } else {
        for (auto &t : activeTokens) {
            string tokenInfo = "  - Token " + string(1, t.id);
            if (t.id == '?') tokenInfo = "  - Finger / Unknown '?'";
            ofDrawBitmapString(tokenInfo, 25, yOffset += 15);
        }
    }

    yOffset += 10;
    ofSetColor(grabbedA ? ofColor::green : ofColor::red);
    ofDrawBitmapString("End A: " + string(grabbedA ? "GRABBED (Token A)" : "FREE (Needs Token A)"), 25, yOffset += 20);
    ofSetColor(grabbedB ? ofColor::green : ofColor::red);
    ofDrawBitmapString("End B: " + string(grabbedB ? "GRABBED (Token B/C)" : "FREE (Needs Token B/C)"), 25, yOffset += 20);
    
    // Draw helper prompts
    yOffset += 40;
    if (gameState == GAME_SUCCESS) {
        ofSetColor(80, 255, 120);
        ofDrawBitmapString("SUCCESS! Log placed upright in target field!", 25, yOffset);
        ofSetColor(200);
        ofDrawBitmapString("Press 'R' on keyboard to reset and play again.", 25, yOffset + 25);
    } else {
        ofSetColor(200);
        ofDrawBitmapString("INSTRUCTIONS:", 25, yOffset);
        ofDrawBitmapString("- Place Token A on End A (left) and Token B/C on End B (right) to lift it.", 25, yOffset + 20);
        ofDrawBitmapString("- Move both tokens to carry it to the green target circle.", 25, yOffset + 35);
        ofDrawBitmapString("- Let go of both tokens to stand the log upright in the center.", 25, yOffset + 50);
        ofDrawBitmapString("- (Debug: Hold 'A' or 'C' key while dragging mouse to simulate tokens)", 25, yOffset + 65);
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == 'r' || key == 'R') {
        gameState = GAME_PLAYING;
        
        // Start log lying flat on the left side
        posA = glm::vec3(-250.0f, groundY, 0.0f);
        posB = glm::vec3(-250.0f + logLength, groundY, 0.0f);
        posA_prev = posA;
        posB_prev = posB;
        
        grabbedA = false;
        grabbedB = false;
        grabberIdA = -1;
        grabberIdB = -1;
    }
}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs & touch){
    nativeTouches[touch.id] = ofVec2f(touch.x, touch.y);
}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs & touch){
    nativeTouches[touch.id] = ofVec2f(touch.x, touch.y);
}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs & touch){
    nativeTouches.erase(touch.id);
}

