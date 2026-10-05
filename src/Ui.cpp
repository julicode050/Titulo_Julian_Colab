#include "Ui.h"

namespace Ui {

const ofColor background(18, 20, 28);
const ofColor neutral(120, 130, 150);
const ofColor accent(255, 190, 70);
const ofColor accentJoined(90, 230, 200);

static ofTrueTypeFont fonts[3];

void setup() {
	// Fuente sans del sistema, con rango Latin para tildes y ñ.
	int sizes[3] = { 14, 22, 40 };
	for (int i = 0; i < 3; ++i) {
		ofTrueTypeFontSettings s(OF_TTF_SANS, sizes[i]);
		s.antialiased = true;
		s.addRanges(ofAlphabet::Latin);
		fonts[i].load(s);
	}
}

void text(const std::string & s, float x, float y, int size) {
	auto & f = fonts[std::clamp(size, 0, 2)];
	if (!f.isLoaded()) {
		ofDrawBitmapString(s, x, y);
		return;
	}
	auto box = f.getStringBoundingBox(s, 0, 0);
	f.drawString(s, x - box.width * 0.5f - box.x, y - box.height * 0.5f - box.y);
}

void textLeft(const std::string & s, float x, float y, int size) {
	auto & f = fonts[std::clamp(size, 0, 2)];
	if (!f.isLoaded()) {
		ofDrawBitmapString(s, x, y);
		return;
	}
	f.drawString(s, x, y);
}

float textWidth(const std::string & s, int size) {
	auto & f = fonts[std::clamp(size, 0, 2)];
	return f.isLoaded() ? f.stringWidth(s) : s.size() * 8.0f;
}

void ring(const glm::vec2 & c, float radius, float progress, float thickness) {
	progress = ofClamp(progress, 0, 1);
	if (progress <= 0) return;
	ofPath arc;
	arc.setCircleResolution(90);
	float start = -90;
	float end = start + 360 * progress;
	arc.arc(c, radius + thickness * 0.5f, radius + thickness * 0.5f, start, end);
	arc.arcNegative(c, radius - thickness * 0.5f, radius - thickness * 0.5f, end, start);
	arc.close();
	arc.setFillColor(ofGetStyle().color);
	arc.draw();
}

void timerBar(float remaining01, const ofColor & color) {
	ofPushStyle();
	ofFill();
	ofSetColor(neutral, 60);
	ofDrawRectangle(0, 0, ofGetWidth(), 10);
	ofSetColor(color);
	ofDrawRectangle(0, 0, ofGetWidth() * ofClamp(remaining01, 0, 1), 10);
	ofPopStyle();
}

}
