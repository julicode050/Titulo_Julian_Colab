#include "Diagnostic.h"
#include "Settings.h"
#include "Ui.h"

static const float kHistorySec = 5.0f; // ventana para el mínimo y máximo de contactos
static const float kBriefSec = 0.5f; // un contacto que dura menos que esto cuenta como breve (parpadeo)
static const size_t kMaxEvents = 16;

// Número con un decimal y coma decimal (la fuente bitmap no tiene tildes, pero sí coma).
static std::string dec(float v) {
	std::string s = ofToString(v, 1);
	std::replace(s.begin(), s.end(), '.', ',');
	return s;
}
static float toMm(float px) {
	return px / (settings().pxPerCm / 10.0f);
}

void Diagnostic::setup() {
	ofAddListener(Events::token(), this, &Diagnostic::onToken);
	resetCounters();
}

void Diagnostic::exit() {
	ofRemoveListener(Events::token(), this, &Diagnostic::onToken);
}

void Diagnostic::resetCounters() {
	t0 = ofGetElapsedTimef();
	appearances = disappearances = briefContacts = 0;
	countHistory.clear();
	events.clear();
	for (auto & c : contacts)
		c.since = 0;
	log("contadores reiniciados");
}

void Diagnostic::log(const std::string & line) {
	events.push_front("[" + dec(now()) + " s] " + line);
	while (events.size() > kMaxEvents)
		events.pop_back();
}

void Diagnostic::onToken(TokenEventArgs & e) {
	if (!active) return;
	std::string a = e.labelA + " #" + ofToString(e.uidA);
	std::string b = e.labelB + " #" + ofToString(e.uidB);
	switch (e.type) {
	case TokenEventArgs::ADDED: log("TOKEN " + a + " aparece"); break;
	case TokenEventArgs::REMOVED: log("TOKEN " + a + " desaparece"); break;
	case TokenEventArgs::JOINED: log("UNION " + a + " + " + b); break;
	case TokenEventArgs::SEPARATED: log("SEPARACION " + a + " / " + b); break;
	}
}

void Diagnostic::update(const std::vector<ofVec2f> & pts) {
	if (!active) return;
	float t = now();

	// Numeración estable: cada contacto conserva su número mientras exista (vecino más cercano).
	float maxJump = settings().cm(1.5f);
	std::vector<bool> used(pts.size(), false);
	std::vector<Contact> next;
	for (auto & c : contacts) {
		int best = -1;
		float bestDist = maxJump;
		for (int i = 0; i < (int)pts.size(); ++i) {
			float d = glm::distance(c.pos, glm::vec2(pts[i].x, pts[i].y));
			if (!used[i] && d < bestDist) bestDist = d, best = i;
		}
		if (best >= 0) {
			used[best] = true;
			next.push_back({ c.id, glm::vec2(pts[best].x, pts[best].y), c.since });
		} else {
			float lasted = t - c.since;
			disappearances++;
			if (lasted < kBriefSec) briefContacts++;
			log("contacto #" + ofToString(c.id) + " desaparece (duro " + dec(lasted) + " s)");
		}
	}
	for (int i = 0; i < (int)pts.size(); ++i) {
		if (used[i]) continue;
		next.push_back({ nextId, glm::vec2(pts[i].x, pts[i].y), t });
		appearances++;
		log("contacto #" + ofToString(nextId) + " aparece");
		nextId++;
	}
	contacts = next;

	countHistory.push_back({ t, (int)contacts.size() });
	while (!countHistory.empty() && countHistory.front().first < t - kHistorySec)
		countHistory.pop_front();
}

void Diagnostic::draw(const TokenTracker & tracker) const {
	if (!active) return;
	float pxPerCm = settings().pxPerCm;
	ofPushStyle();

	// Grilla de 1 cm como referencia
	ofSetLineWidth(1);
	ofSetColor(255, 255, 255, 18);
	for (float x = 0; x < ofGetWidth(); x += pxPerCm)
		ofDrawLine(x, 0, x, ofGetHeight());
	for (float y = 0; y < ofGetHeight(); y += pxPerCm)
		ofDrawLine(0, y, ofGetWidth(), y);

	// Distancias entre contactos cercanos (los que podrían ser del mismo token)
	std::vector<std::tuple<float, int, int>> pairs;
	for (size_t i = 0; i < contacts.size(); ++i) {
		for (size_t j = i + 1; j < contacts.size(); ++j) {
			float d = glm::distance(contacts[i].pos, contacts[j].pos);
			pairs.push_back({ d, contacts[i].id, contacts[j].id });
			if (d > settings().clusterThresholdPx) continue;
			ofSetColor(120, 200, 255, 90);
			ofDrawLine(contacts[i].pos, contacts[j].pos);
		}
	}
	std::sort(pairs.begin(), pairs.end());

	// Contactos: círculo del tamaño de una almohadilla (Ø5 mm), número y posición en mm
	for (auto & c : contacts) {
		ofNoFill();
		ofSetLineWidth(2);
		ofSetColor(255, 220, 80);
		ofDrawCircle(c.pos, settings().cm(0.25f));
		ofFill();
		ofDrawCircle(c.pos, 3);
		ofSetColor(255);
		ofDrawBitmapString("#" + ofToString(c.id), c.pos.x + 10, c.pos.y - 8);
	}

	// Qué reconoce el sistema, junto a cada token / par
	for (auto & g : tracker.getGroups()) {
		if (g.isJoined()) {
			ofNoFill();
			ofSetLineWidth(3);
			ofSetColor(90, 230, 200);
			ofDrawCircle(g.center, settings().cm(3.0f));
			ofDrawBitmapStringHighlight("PAR ENCAJADO", g.center.x - 48, g.center.y - settings().cm(3.3f), ofColor(0, 90, 70), ofColor(255));
		}
	}
	for (auto & t : tracker.getTokens()) {
		std::string label = t.label + (t.label == "B" && t.rawLetter != 'B' ? " (leido como " + std::string(1, t.rawLetter) + ")" : "");
		ofColor bg = t.visible ? ofColor(150, 90, 0) : ofColor(80);
		// A la izquierda (A) o a la derecha (B) del token, para no tapar sus contactos
		float dx = t.label == "A" ? -settings().cm(4.5f) : settings().cm(3.2f);
		ofDrawBitmapStringHighlight(label, t.pos.x + dx, t.pos.y + 4, bg, ofColor(255));
	}

	// Panel izquierdo
	int minC = INT_MAX, maxC = 0;
	for (auto & h : countHistory)
		minC = std::min(minC, h.second), maxC = std::max(maxC, h.second);
	if (countHistory.empty()) minC = 0;

	std::vector<std::string> left = {
		"MODO DIAGNOSTICO (P para salir, C reinicia contadores)",
		"",
		"Contactos ahora: " + ofToString(contacts.size()) + "   (min " + ofToString(minC) + " / max " + ofToString(maxC)
			+ " en los ultimos " + ofToString((int)kHistorySec) + " s)",
		"Apariciones: " + ofToString(appearances) + "   Desapariciones: " + ofToString(disappearances)
			+ "   Breves (<" + dec(kBriefSec) + " s): " + ofToString(briefContacts),
		"",
		"Reconocido:",
	};
	if (tracker.getTokens().empty()) left.push_back("  (ningun token)");
	bool pair = false;
	for (auto & g : tracker.getGroups())
		if (g.isJoined()) pair = true;
	for (auto & t : tracker.getTokens()) {
		left.push_back("  " + t.label + " #" + ofToString(t.uid) + "  forma '" + std::string(1, t.rawLetter) + "'  "
			+ ofToString(t.points.size()) + " pts  (" + dec(toMm(t.pos.x)) + "; " + dec(toMm(t.pos.y)) + ") mm"
			+ (t.visible ? "" : "  [perdido " + ofToString(t.missingFrames) + " cuadros]"));
	}
	left.push_back("  Par encajado: " + std::string(pair ? "SI" : "no"));
	left.push_back("");
	left.push_back("Contactos (posicion desde la esquina superior izquierda):");
	if (contacts.empty()) left.push_back("  (ninguno)");
	for (auto & c : contacts)
		left.push_back("  #" + ofToString(c.id) + "  (" + dec(toMm(c.pos.x)) + "; " + dec(toMm(c.pos.y)) + ") mm   visible hace "
			+ dec(now() - c.since) + " s");
	left.push_back("");
	left.push_back("Distancias entre contactos (centro a centro):");
	if (pairs.empty()) left.push_back("  (menos de 2 contactos)");
	for (size_t i = 0; i < pairs.size() && i < 15; ++i) {
		auto & [d, a, b] = pairs[i];
		left.push_back("  #" + ofToString(a) + " - #" + ofToString(b) + "   " + dec(toMm(d)) + " mm   (borde a borde "
			+ dec(toMm(d) - 5.0f) + " mm)");
	}
	int y = 24;
	for (auto & l : left) {
		if (!l.empty()) ofDrawBitmapStringHighlight(l, 16, y, ofColor(0, 0, 0, 190), ofColor(255));
		y += 18;
	}

	// Panel derecho: eventos recientes
	size_t maxChars = 7;
	for (auto & e : events)
		maxChars = std::max(maxChars, e.size());
	float x = ofGetWidth() - 16 - maxChars * 8;
	y = 24;
	ofDrawBitmapStringHighlight("EVENTOS", x, y, ofColor(0, 0, 0, 190), Ui::accent);
	y += 22;
	for (auto & e : events) {
		ofDrawBitmapStringHighlight(e, x, y, ofColor(0, 0, 0, 190), ofColor(235));
		y += 18;
	}
	ofPopStyle();
}
