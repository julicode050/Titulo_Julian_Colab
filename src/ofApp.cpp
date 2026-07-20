#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofBackground(135, 206, 235); // Sky blue
	ofSetWindowTitle("Titulo Julian Colab - Fence Builder");

	groundY = ofGetHeight() - 150.0f;

	// Sheep Placeholder
	sheepPos = glm::vec2(ofGetWidth() * 0.75f, groundY - 30.0f);
	sheepVel = glm::vec2(-0.5f, 0.0f);

	// Initialize token tracking
	token.setup(ofVec2f(0, 0));

	// Initialize 2 logs
	logs.clear();
	for (int i = 0; i < 2; ++i) {
		FenceLog log;
		log.posA = glm::vec2(100.0f + (i * 300.0f), groundY);
		log.posB = glm::vec2(100.0f + (i * 300.0f) + logLength, groundY);
		log.posA_prev = log.posA;
		log.posB_prev = log.posB;
		logs.push_back(log);
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	// Update token tracker
	token.update();

	// Get screen points (pixels)
	auto pts = token.getPoints();

	// Add points from native macOS touchscreen events
	for (auto const & pair : nativeTouches) {
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
		for (auto & p : clusters[i])
			centroid += p;
		centroid /= (float)clusters[i].size();

		// 2. Classify shape of this cluster
		char tokenID = '?';
		if (clusters[i].size() >= 3) {
			auto ordered = token.orderByAngle(clusters[i]);
			tokenID = token.classifyShape(ordered);
		} else if (clusters[i].size() == 1) {
			// Debug simulation: Hold key A/C/B/F/G/H/D while dragging to simulate a token
			if (ofGetKeyPressed('a') || ofGetKeyPressed('A'))
				tokenID = 'A';
			else if (ofGetKeyPressed('c') || ofGetKeyPressed('C'))
				tokenID = 'C';
			else if (ofGetKeyPressed('b') || ofGetKeyPressed('B'))
				tokenID = 'B';
			else if (ofGetKeyPressed('d') || ofGetKeyPressed('D'))
				tokenID = 'D'; // Hammer
		}

		// 3. Project to 2D world plane (screen coordinates)
		glm::vec2 wPos(centroid.x, centroid.y);

		// Track offset lifetime based on origin
		if (tokenOffsets.find(tokenID) == tokenOffsets.end()) {
			if (tokenID == 'D') {
				// Hammer action area must be below the hammer (positive Y)
				tokenOffsets[tokenID] = glm::vec2(0.0f, 150.0f);
			} else {
				// Grabbing tokens depend on the half they originated in (X axis)
				float xOffset = (wPos.x < ofGetWidth() / 2.0f) ? 150.0f : -150.0f;
				tokenOffsets[tokenID] = glm::vec2(xOffset, 0.0f);
			}
		}

		glm::vec2 aPos = wPos + tokenOffsets[tokenID];
		activeTokens.push_back({ tokenID, wPos, aPos, i });

		tokenTimeouts[tokenID] = 0; // reset timeout since it's currently active
	}

	// --- Menu Logic ---
	if (gameState == GAME_MENU) {
		float btnW = 300;
		float btnH = 100;
		float btnX1 = ofGetWidth() / 2.0f - btnW - 20.0f;
		float btnX2 = ofGetWidth() / 2.0f + 20.0f;
		float btnY = ofGetHeight() / 2.0f - btnH / 2.0f;

		auto checkClick = [&](float x, float y) {
			if (y > btnY && y < btnY + btnH) {
				if (x > btnX1 && x < btnX1 + btnW) {
					currentMode = COOP_MODE;
					gameState = GAME_PLAYING;
				} else if (x > btnX2 && x < btnX2 + btnW) {
					currentMode = SOLO_MODE;
					gameState = GAME_PLAYING;
				}
			}
		};

		if (ofGetMousePressed()) checkClick(ofGetMouseX(), ofGetMouseY());
		for (auto & t : activeTokens)
			checkClick(t.actionPos.x, t.actionPos.y);

		return; // Skip game logic while in menu
	}

	// --- Hammer Logic (Token D) ---
	for (auto & t : activeTokens) {
		if (t.id == 'D') {
			glm::vec2 actionPos = t.actionPos;

			for (auto & log : logs) {
				if (!log.isPlanted) {
					// Check if Hammer action point is hitting the top end (End A)
					if (glm::distance(actionPos, log.posA) < grabThreshold) {
						// Optional: Ensure it's roughly vertical before planting
						glm::vec2 logDir = glm::normalize(log.posA - log.posB);
						if (logDir.y < -0.7f && log.posB.y >= groundY - 10.0f) {
							log.isPlanted = true;
							log.grabbedA = false;
							log.grabbedB = false;
							// Lock it exactly vertical
							log.posA.x = log.posB.x;
							log.posA_prev = log.posA;
							log.posB_prev = log.posB;
						}
					}
				}
			}
		}
	}

	// --- Grabbing logic ---
	auto updateGrabState = [&](bool & grabbed, glm::vec2 & pos, int & lossFrames, char targetToken1, char targetToken2) {
		if (!grabbed) {
			for (auto & t : activeTokens) {
				if ((t.id == targetToken1 || t.id == targetToken2) && glm::distance(t.actionPos, pos) < grabThreshold) {
					grabbed = true;
					lossFrames = 0;
					pos = t.actionPos;
					break;
				}
			}
		} else {
			bool found = false;
			for (auto & t : activeTokens) {
				if ((t.id == targetToken1 || t.id == targetToken2) && glm::distance(t.actionPos, pos) < grabThreshold) {
					pos = t.actionPos;
					found = true;
					lossFrames = 0; // reset
					break;
				}
			}
			if (!found) {
				lossFrames++;
				if (lossFrames > 30) {
					grabbed = false;
					lossFrames = 0;
				}
			}
		}
	};

	if (currentMode == COOP_MODE) {
		for (auto & log : logs) {
			if (log.isPlanted) continue;
			updateGrabState(log.grabbedA, log.posA, log.grabLossFramesA, 'A', 'A');
			updateGrabState(log.grabbedB, log.posB, log.grabLossFramesB, 'B', 'C');
		}
	} else {
		// SOLO MODE logic
		for (auto & log : logs) {
			if (log.isPlanted) continue;

			glm::vec2 center = (log.posA + log.posB) * 0.5f;

			if (!log.grabbedSolo) {
				for (auto & t : activeTokens) {
					if ((t.id == 'A' || t.id == 'B' || t.id == 'C') && glm::distance(t.actionPos, center) < grabThreshold * 1.5f) {
						log.grabbedSolo = true;
						log.grabLossFramesSolo = 0;
						log.soloGrabOffsetA = log.posA - t.actionPos;
						log.soloGrabOffsetB = log.posB - t.actionPos;
						break;
					}
				}
			} else {
				bool found = false;
				for (auto & t : activeTokens) {
					if (t.id == 'A' || t.id == 'B' || t.id == 'C') {
						glm::vec2 expectedCenter = t.actionPos + (log.soloGrabOffsetA + log.soloGrabOffsetB) * 0.5f;
						if (glm::distance(expectedCenter, center) < grabThreshold * 1.5f) {
							log.posA = t.actionPos + log.soloGrabOffsetA;
							log.posB = t.actionPos + log.soloGrabOffsetB;
							log.posA_prev = log.posA;
							log.posB_prev = log.posB;
							found = true;
							log.grabLossFramesSolo = 0;
							break;
						}
					}
				}
				if (!found) {
					log.grabLossFramesSolo++;
					if (log.grabLossFramesSolo > 30) {
						log.grabbedSolo = false;
						log.grabLossFramesSolo = 0;
					}
				}
			}
		}
	}

	// --- Physics update ---
	if (gameState == GAME_PLAYING) {
		float dt = ofGetLastFrameTime();
		if (dt > 0.1f) dt = 0.1f; // Cap dt to prevent instability

		glm::vec2 gravity(0, 800.0f); // Ground-directed gravity (Y is down)

		for (auto & log : logs) {
			if (log.isPlanted) continue;

			// Verlet Integration for free endpoints
			bool isGrabbedA = log.grabbedA || log.grabbedSolo;
			bool isGrabbedB = log.grabbedB || log.grabbedSolo;

			if (!isGrabbedA) {
				glm::vec2 temp = log.posA;
				log.posA += (log.posA - log.posA_prev) + gravity * dt * dt;
				log.posA_prev = temp;
			} else {
				log.posA_prev = log.posA - (log.posA - log.posA_prev) * 0.4f;
			}

			if (!isGrabbedB) {
				glm::vec2 temp = log.posB;
				log.posB += (log.posB - log.posB_prev) + gravity * dt * dt;
				log.posB_prev = temp;
			} else {
				log.posB_prev = log.posB - (log.posB - log.posB_prev) * 0.4f;
			}

			// Ground Collision resolution
			float bounce = 0.4f;
			float friction = 0.85f;
			if (log.posA.y > groundY) {
				glm::vec2 vel = log.posA - log.posA_prev;
				log.posA.y = groundY;
				vel.y = -vel.y * bounce;
				vel.x *= friction;
				log.posA_prev = log.posA - vel;
			}
			if (log.posB.y > groundY) {
				glm::vec2 vel = log.posB - log.posB_prev;
				log.posB.y = groundY;
				vel.y = -vel.y * bounce;
				vel.x *= friction;
				log.posB_prev = log.posB - vel;
			}

			// Rigid length constraint relaxation (maintain logLength)
			for (int step = 0; step < 4; ++step) {
				glm::vec2 delta = log.posA - log.posB;
				float currentDist = glm::length(delta);
				if (currentDist > 0.001f) {
					float diff = logLength - currentDist;
					glm::vec2 offset = (delta / currentDist) * diff * 0.5f;

					if (log.grabbedSolo) {
						// Skip relaxation, rigid body handles it
					} else if (!log.grabbedA && !log.grabbedB) {
						log.posA += offset;
						log.posB -= offset;
					} else if (log.grabbedA) {
						log.posB -= offset * 2.0f;
					} else if (log.grabbedB) {
						log.posA += offset * 2.0f;
					}
				}
			}
		}

		// Update Sheep Placeholder (Static)
	}

	// Clean up token offsets for tokens no longer on screen
	for (auto it = tokenOffsets.begin(); it != tokenOffsets.end();) {
		char id = it->first;
		bool active = false;
		for (auto & t : activeTokens) {
			if (t.id == id) active = true;
		}
		if (!active) {
			tokenTimeouts[id]++;
			if (tokenTimeouts[id] > 30) { // roughly 0.5 seconds at 60fps
				tokenTimeouts.erase(id);
				it = tokenOffsets.erase(it);
				continue;
			}
		}
		++it;
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofSetLineWidth(1);

	if (gameState == GAME_MENU) {
		ofBackground(40, 50, 60);
		ofSetColor(255);
		ofDrawBitmapString("FENCE BUILDER", ofGetWidth() / 2.0f - 50, ofGetHeight() / 2.0f - 150);
		ofDrawBitmapString("Select Game Mode to Start", ofGetWidth() / 2.0f - 90, ofGetHeight() / 2.0f - 120);

		float btnW = 300;
		float btnH = 100;
		float btnX1 = ofGetWidth() / 2.0f - btnW - 20.0f;
		float btnX2 = ofGetWidth() / 2.0f + 20.0f;
		float btnY = ofGetHeight() / 2.0f - btnH / 2.0f;

		ofSetColor(100, 200, 100);
		ofDrawRectangle(btnX1, btnY, btnW, btnH);
		ofSetColor(100, 100, 200);
		ofDrawRectangle(btnX2, btnY, btnW, btnH);

		ofSetColor(255);
		ofDrawBitmapString("Mode 1: Co-Op", btnX1 + 100, btnY + 45);
		ofDrawBitmapString("(2 Tokens per Log)", btnX1 + 80, btnY + 65);

		ofDrawBitmapString("Mode 2: Solo", btnX2 + 100, btnY + 45);
		ofDrawBitmapString("(1 Token per Log)", btnX2 + 80, btnY + 65);

		// Draw active token action points so they can click buttons
		for (auto & t : activeTokens) {
			ofSetColor(255, 200, 50, 220);
			ofDrawCircle(t.actionPos, 15.0f);
		}
		return;
	}

	// 1. Draw Landscape
	// Sky is background color. Draw Grass:
	ofSetColor(34, 139, 34); // Forest green
	ofDrawRectangle(0, groundY, ofGetWidth(), ofGetHeight() - groundY);

	// 2. Draw Sheep Placeholder
	ofSetColor(255);
	ofDrawCircle(sheepPos, 30.0f);
	ofSetColor(0);
	ofDrawBitmapString("SHEEP", sheepPos.x - 20, sheepPos.y);

	// 3. Draw Logs
	for (auto & log : logs) {
		if (log.isPlanted) {
			ofSetColor(101, 67, 33); // Darker wood brown for planted logs
		} else if (log.grabbedA && log.grabbedB) {
			ofSetColor(240, 130, 20); // Bright orange when fully grabbed
		} else if (log.grabbedA || log.grabbedB) {
			ofSetColor(210, 180, 50); // Muted gold when partially grabbed
		} else {
			ofSetColor(139, 90, 43); // Wood brown when free
		}

		// Robust thick line drawing using geometry
		glm::vec2 delta = log.posB - log.posA;
		float dist = glm::length(delta);
		float angle = atan2(delta.y, delta.x);

		ofFill();
		// Draw the main body as a rotated rectangle
		ofPushMatrix();
		ofTranslate(log.posA);
		ofRotateZRad(angle);
		ofDrawRectangle(0, -logRadius, dist, logRadius * 2.0f);
		ofPopMatrix();

		// Draw rounded ends
		ofDrawCircle(log.posA, logRadius);
		ofDrawCircle(log.posB, logRadius);

		ofSetLineWidth(1);

		// Draw Grabbing Spheres on ends (Visual aids) if not planted
		if (!log.isPlanted) {
			ofNoFill();
			ofSetLineWidth(2);

			// End A
			ofSetColor(log.grabbedA ? ofColor::red : ofColor::lightGray);
			ofDrawCircle(log.posA, log.grabbedA ? 15.0f : 8.0f);

			// End B
			ofSetColor(log.grabbedB ? ofColor::red : ofColor::lightGray);
			ofDrawCircle(log.posB, log.grabbedB ? 15.0f : 8.0f);
			ofFill();
		}
	}

	// 4. Draw active touchscreen/token projected coordinates
	ofFill();
	for (auto & t : activeTokens) {
		if (t.id == 'D') {
			ofSetColor(100, 100, 255, 200); // Blue for Hammer base
		} else {
			ofSetColor(255, 100, 100, 180);
		}
		ofDrawCircle(t.worldPos, 12.0f);

		// Draw action area for ALL tokens
		ofNoFill();
		ofSetLineWidth(2);
		ofSetColor(255, 200, 50, 220); // Yellow/Orange action circle
		ofDrawCircle(t.actionPos, grabThreshold);
		ofDrawLine(t.worldPos, t.actionPos); // Draw connecting line
		ofFill();

		// Draw classified token ID
		string label = "Token: ";
		label += t.id;
		if (t.id == '?') label = "Finger/Unknown";
		if (t.id == 'D') label += " (Hammer)";

		ofSetColor(0);
		ofDrawBitmapString(label, t.worldPos.x + 15, t.worldPos.y + 15);
	}

	// --- 2D UI HUD Overlay ---
	ofSetColor(0);
	ofDrawBitmapString("Titulo Julian Colab - Fence Builder Game", 25, 40);

	int yOffset = 70;

	// Aggregate native touches for points display
	auto ptsDraw = token.getPoints();
	for (auto const & pair : nativeTouches) {
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
		for (auto & t : activeTokens) {
			string tokenInfo = "  - Token " + string(1, t.id);
			if (t.id == '?') tokenInfo = "  - Finger / Unknown '?'";
			if (t.id == 'D') tokenInfo += " (Hammer)";
			ofDrawBitmapString(tokenInfo, 25, yOffset += 15);
		}
	}

	// Draw helper prompts
	yOffset += 40;
	ofDrawBitmapString("INSTRUCTIONS:", 25, yOffset);
	ofDrawBitmapString("- Grab logs with Tokens A (top) and B/C (bottom).", 25, yOffset + 20);
	ofDrawBitmapString("- Stand logs upright, then use Token D (Hammer) on top to plant them.", 25, yOffset + 35);
	ofDrawBitmapString("- Build a fence to keep the sheep in!", 25, yOffset + 50);
	ofDrawBitmapString("- (Debug: Hold 'A', 'B', 'C', or 'D' key while dragging mouse to simulate tokens)", 25, yOffset + 65);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == 'r' || key == 'R') {
		gameState = GAME_PLAYING;

		logs.clear();
		for (int i = 0; i < 2; ++i) {
			FenceLog log;
			log.posA = glm::vec2(100.0f + (i * 300.0f), groundY);
			log.posB = glm::vec2(100.0f + (i * 300.0f) + logLength, groundY);
			log.posA_prev = log.posA;
			log.posB_prev = log.posB;
			logs.push_back(log);
		}
	}
}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs & touch) {
	nativeTouches[touch.id] = ofVec2f(touch.x, touch.y);
}

//--------------------------------------------------------------
void ofApp::touchMoved(ofTouchEventArgs & touch) {
	nativeTouches[touch.id] = ofVec2f(touch.x, touch.y);
}

//--------------------------------------------------------------
void ofApp::touchUp(ofTouchEventArgs & touch) {
	nativeTouches.erase(touch.id);
}
