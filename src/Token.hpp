#pragma once
#include "ofMain.h"
#include "ofxTuio.h"

class Token {
public:
	Token();
	~Token();

	void setup(const ofVec2f & pos);
	void update();
	void draw();

	void setPoints(const vector<ofVec2f> & newPts); // opcional (debug)
	vector<ofVec2f> getPoints() const; // puntos actuales (escala a pantalla)
	vector<ofVec2f> getLockedPoints() const; // snapshot inicial (si hay)
	bool isLocked() const;
	char getId() const; // A..Z o '?'

	// TUIO callbacks
	void tuioAdded(ofxTuioCursor & c);
	void tuioUpdated(ofxTuioCursor & c);
	void tuioRemoved(ofxTuioCursor & c);
	// ----------- NUEVAS VARIABLES PARA ZOOM POR ROTACIÓN ------------
	float prevAngle = 0.0f; // último ángulo registrado del token
	bool hasInitialAngle = false; // indica si ya inicializamos prevAngle

	// clasificación & utils public para ofApp
	char classifyShape(const vector<ofVec2f> & pts) const;
	float angleBetween(const ofVec2f & a, const ofVec2f & b, const ofVec2f & c) const;
	bool approxEqual(float a, float b, float tolerance = 12.0f) const;
	ofVec2f centroid(const vector<ofVec2f> & pts) const;
	vector<ofVec2f> orderByAngle(const vector<ofVec2f> & pts) const;

private:
	struct Touch {
		ofVec2f pos;
		long id;
	};
	vector<Touch> cursors; // coords normalizadas 0..1
	vector<ofVec2f> scaledCursors; // coords en pixeles (current)

	// locked (snapshot) data
	bool locked = false;
	vector<ofVec2f> lockedPoints; // snapshot en pixeles (solo si locked==true)
	char detectedId = '?';

	ofxTuioReceiver tuio;
};
