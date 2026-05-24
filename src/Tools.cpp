#include "Tools.hpp"

// ---------------- UTILS ----------------
std::vector<std::vector<ofVec2f>> clusterTouches(const std::vector<ofVec2f>& pts, float threshold) {
    std::vector<std::vector<ofVec2f>> clusters;
    for (const auto& p : pts) {
        bool found = false;
        for (auto& group : clusters) {
            for (const auto& gp : group) {
                if (p.distance(gp) < threshold) {
                    group.push_back(p);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found) clusters.push_back({p});
    }
    return clusters;
}

// ---------------- ROTATION TOOL ----------------
void RotationTool::update(const vector<ofVec2f>& pts, RotationAxis currentAxis) {
    active = (pts.size() >= 2);
    if (!(active && currentAxis != RotationAxis::None)) {
        lastAngle = 0.0f;
        return;
    }

    ofVec2f p1 = pts[0];
    ofVec2f p2 = pts[1];
    ofVec2f delta = p2 - p1;
    float newAngle = atan2(delta.y, delta.x) * RAD_TO_DEG;

    if (lastAngle == 0.0f) {
        lastAngle = newAngle;
        return;
    }

    float d = newAngle - lastAngle;
    if (d > 180)  d -= 360;
    if (d < -180) d += 360;

    switch (currentAxis) {
        case RotationAxis::X: rotationX += d; break;
        case RotationAxis::Y: rotationY += d; break;
        case RotationAxis::Z: rotationZ += d; break;
        default: break;
    }
    lastAngle = newAngle;
}

void RotationTool::draw(int x, int y) {
    ofDrawBitmapString("Rotator: " + string(active ? "SI" : "NO"), x, y);
}

// ---------------- AXIS SELECTOR TOOL ----------------
void AxisSelectorTool::update(const vector<ofVec2f>& pts, RotationAxis) { updateSelection(pts); }
void AxisSelectorTool::draw(int x, int y) {
    string axisName = "Ninguno";
    if      (currentAxis == RotationAxis::X) axisName = "X";
    else if (currentAxis == RotationAxis::Y) axisName = "Y";
    else if (currentAxis == RotationAxis::Z) axisName = "Z";
    ofDrawBitmapString("Eje seleccionado: " + axisName, x, y);
}

void AxisSelectorTool::setLockedPoints(const vector<ofVec2f>& lockedPts) {
    if (lockedPts.size() < 3) return;
    ofVec2f c(0,0);
    for (auto &p: lockedPts) c += p;
    c /= (float)lockedPts.size();
    fixedMenuCenter = c;

    float sum = 0; int count = 0;
    for (size_t i = 0; i < lockedPts.size(); ++i) {
        for (size_t j = i+1; j < lockedPts.size(); ++j) {
            sum += lockedPts[i].distance(lockedPts[j]);
            ++count;
        }
    }
    float avgDist = (count>0) ? (sum / count) : 100.0f;
    buttonRadius = ofClamp(avgDist * 0.9f, 80.0f, 200.0f);
    buttonSpacing = avgDist * 1.4f;

    lockedMenu = true;
    menuActive = true;
    currentAxis = RotationAxis::None;
}
void AxisSelectorTool::clearLocked() { lockedMenu = false; menuActive = false; currentAxis = RotationAxis::None; }
bool AxisSelectorTool::hasLockedMenu() const { return lockedMenu; }
ofVec2f AxisSelectorTool::buttonPos(const string& axis) const {
    if (axis == "X") return fixedMenuCenter + ofVec2f(-buttonSpacing, 0);
    if (axis == "Y") return fixedMenuCenter + ofVec2f(0, -buttonSpacing);
    if (axis == "Z") return fixedMenuCenter + ofVec2f(buttonSpacing, 0);
    return fixedMenuCenter;
}
bool AxisSelectorTool::insideCircle(const ofVec2f& p, const ofVec2f& c) const { return p.distance(c) < buttonRadius; }
void AxisSelectorTool::drawButton(const string& label, const ofColor& color) {
    ofVec2f pos = buttonPos(label);
    ofNoFill(); ofSetLineWidth(3); ofSetColor(color);
    ofDrawCircle(pos, buttonRadius);
    ofSetColor(255);
    ofDrawBitmapString(label, pos + ofVec2f(-6, 6));
    if ((label == "X" && currentAxis == RotationAxis::X) ||
        (label == "Y" && currentAxis == RotationAxis::Y) ||
        (label == "Z" && currentAxis == RotationAxis::Z)) {
        ofNoFill(); ofSetLineWidth(4); ofSetColor(255,230,0);
        ofDrawCircle(pos, buttonRadius + 10);
    }
}
void AxisSelectorTool::updateSelection(const vector<ofVec2f>& currentPts) {
    if (!lockedMenu) return;
    if (currentPts.empty()) { currentAxis = RotationAxis::None; return; }
    for (auto &pt : currentPts) {
        if (insideCircle(pt, buttonPos("X"))) { currentAxis = RotationAxis::X; return; }
        if (insideCircle(pt, buttonPos("Y"))) { currentAxis = RotationAxis::Y; return; }
        if (insideCircle(pt, buttonPos("Z"))) { currentAxis = RotationAxis::Z; return; }
    }
    currentAxis = RotationAxis::None;
}
void AxisSelectorTool::drawMenu() {
    if (!menuActive || !lockedMenu) return;
    drawButton("X", ofColor::red);
    drawButton("Y", ofColor::green);
    drawButton("Z", ofColor::blue);
}

// ---------------- SCALE TOOL ----------------
void ScaleTool::setLockedPoints(const vector<ofVec2f>& lockedPts, const glm::vec3& currentScale) {
    if (lockedPts.size() < 2) return;
    initialAvg = 0.0f; int count = 0;
    for (size_t i = 0; i < lockedPts.size(); ++i) {
        for (size_t j = i + 1; j < lockedPts.size(); ++j) {
            initialAvg += lockedPts[i].distance(lockedPts[j]);
            count++;
        }
    }
    if (count > 0) initialAvg /= count;
    if (initialAvg <= 0) initialAvg = 1.0f;
    scaleVec = currentScale;
    locked = true;
}
void ScaleTool::reset() { locked = false; initialAvg = 0.0f; scaleVec = glm::vec3(1.0f); }
void ScaleTool::update(const vector<ofVec2f>& pts, RotationAxis) {
    if (!locked || pts.empty()) return;
    float sum = 0; int count = 0;
    for (size_t i = 0; i < pts.size(); ++i) {
        for (size_t j = i + 1; j < pts.size(); ++j) {
            sum += pts[i].distance(pts[j]); count++;
        }
    }
    float avg = (count > 0) ? (sum / count) : initialAvg;
    float factor = (initialAvg > 0) ? (avg / initialAvg) : 1.0f;
    factor = std::max(0.01f, factor);
    scaleVec = glm::vec3(factor);
}
void ScaleTool::draw(int x, int y) { ofDrawBitmapString("Scale tool: " + ofToString(scaleVec.x, 2), x, y); }

// ---------------- TRANSLATE TOOL ----------------
void TranslateTool::setLockedPoints(const vector<ofVec2f>& lockedPts, const glm::vec3& currentPos) {
    if (lockedPts.empty()) return;
    ofVec2f c(0,0);
    for (auto &p : lockedPts) c += p;
    c /= (float)lockedPts.size();
    initialCenter = c;
    initialPos = currentPos;
    locked = true;
}
void TranslateTool::reset() { locked = false; initialCenter = ofVec2f(0,0); initialPos = glm::vec3(0.0f); }
void TranslateTool::update(const vector<ofVec2f>& pts, RotationAxis) {
    if (!locked || pts.empty()) return;
    ofVec2f c(0,0);
    for (auto &p: pts) c += p;
    c /= (float)pts.size();
    ofVec2f d = c - initialCenter;
    float worldX = d.x;
    float worldY = -d.y;
    position = initialPos + glm::vec3(worldX, worldY, 0.0f);
}
void TranslateTool::draw(int x, int y) {
    ofDrawBitmapString("Translate tool: " + ofToString(position.x,1) + ", " + ofToString(position.y,1), x, y);
}

// ---------------- COLOR TOOL ----------------
void ColorTool::setLockedPoints(const vector<ofVec2f>& lockedPts, const ofColor& startColor) {
    if (lockedPts.empty()) return;
    ofVec2f c(0,0);
    for (auto &p: lockedPts) c += p;
    c /= (float)lockedPts.size();
    initialCenter = c;
    currentColor = startColor;
    locked = true;
}
void ColorTool::reset() { locked = false; initialCenter = ofVec2f(0,0); }
void ColorTool::update(const vector<ofVec2f>& pts, RotationAxis) {
    if (!locked || pts.empty()) return;
    ofVec2f c(0,0);
    for (auto &p: pts) c += p;
    c /= (float)pts.size();
    float hue = ofMap(c.x, 0, ofGetWidth(), 0, 255, true);
    float sat = ofMap(c.y, 0, ofGetHeight(), 255, 50, true);
    currentColor.setHsb((int)hue, (int)sat, 200);
}
void ColorTool::draw(int x, int y) { ofDrawBitmapString("Color tool: H" + ofToString(currentColor.getHue()) , x, y); }
