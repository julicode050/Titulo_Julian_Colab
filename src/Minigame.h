#pragma once
#include "Events.h"
#include "Settings.h"
#include "TokenTracker.h"
#include "Ui.h"
#include "ofMain.h"

// Base de los tres minijuegos. Cada juego solo lee el TokenTracker
// (tokens con posición y grupos unidos); nunca toca los puntos crudos.
class Minigame {
public:
	virtual ~Minigame() { }
	virtual std::string id() const = 0; // para eventos: "ensamblaje", ...
	virtual std::string title() const = 0; // texto visible
	virtual void start() = 0;
	virtual void update(float dt, const TokenTracker & tracker) = 0;
	virtual void draw(const TokenTracker & tracker) = 0;
	virtual bool isFinished() const = 0;
	virtual void drawResults() = 0;

	// Dibuja cada token (o grupo unido) con un marcador simple.
	static void drawTokens(const TokenTracker & tracker) {
		ofPushStyle();
		for (auto & g : tracker.getGroups()) {
			if (g.isJoined()) {
				ofNoFill();
				ofSetLineWidth(3);
				ofSetColor(Ui::accentJoined);
				ofDrawCircle(g.center, cm(2.6f));
				ofFill();
			}
			for (int uid : g.uids) {
				auto * t = tracker.findToken(uid);
				ofSetColor(g.isJoined() ? Ui::accentJoined : Ui::accent, t->visible ? 200 : 80);
				ofDrawCircle(t->pos, cm(0.5f));
			}
		}
		ofPopStyle();
	}

protected:
	void event(const std::string & name, const std::string & detail = "") {
		Events::notifyGame(id(), name, detail);
	}
	static float cm(float v) { return settings().cm(v); }
};
