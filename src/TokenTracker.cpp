#include "TokenTracker.h"
#include "Settings.h"
#include "Tools.hpp" // clusterTouches

// Divide un grupo fusionado (dos tokens unidos) en k subgrupos con k-means.
// Semillas: el punto más lejano a los anteriores, para que sea determinista.
static std::vector<std::vector<ofVec2f>> splitCluster(const std::vector<ofVec2f> & pts, int k) {
	std::vector<ofVec2f> centers { pts[0] };
	while ((int)centers.size() < k) {
		ofVec2f farthest = pts[0];
		float best = -1;
		for (auto & p : pts) {
			float d = FLT_MAX;
			for (auto & c : centers)
				d = std::min(d, p.distance(c));
			if (d > best) {
				best = d;
				farthest = p;
			}
		}
		centers.push_back(farthest);
	}

	std::vector<std::vector<ofVec2f>> parts;
	for (int iter = 0; iter < 10; ++iter) {
		parts.assign(k, {});
		for (auto & p : pts) {
			int bestIdx = 0;
			for (int c = 1; c < k; ++c)
				if (p.distance(centers[c]) < p.distance(centers[bestIdx])) bestIdx = c;
			parts[bestIdx].push_back(p);
		}
		for (int c = 0; c < k; ++c) {
			if (parts[c].empty()) continue;
			ofVec2f m(0, 0);
			for (auto & p : parts[c])
				m += p;
			centers[c] = m / (float)parts[c].size();
		}
	}
	parts.erase(std::remove_if(parts.begin(), parts.end(), [](auto & p) { return p.empty(); }), parts.end());
	return parts;
}

void TokenTracker::setup(const Token * c) {
	classifier = c;
	tokens.clear();
	groups.clear();
	pairs.clear();
}

std::string TokenTracker::labelFor(char letter) const {
	if (letter == '?') return "?";
	for (auto & [label, letters] : settings().tokenLetters) {
		if (letters.find(letter) != std::string::npos) return label;
	}
	return std::string(1, letter);
}

std::vector<TokenTracker::Observation> TokenTracker::observe(const std::vector<ofVec2f> & pts) {
	std::vector<Observation> obs;
	fingers.clear();

	auto clusters = clusterTouches(pts, settings().clusterThresholdPx);
	for (int i = 0; i < (int)clusters.size(); ++i) {
		auto & cluster = clusters[i];
		if (cluster.size() < 3) {
			// 1-2 puntos: dedos u otros contactos, no son tokens
			for (auto & p : cluster)
				fingers.push_back(p);
			continue;
		}

		std::vector<std::vector<ofVec2f>> parts;
		int mergeId = -1;
		if ((int)cluster.size() >= settings().joinedMinPoints) {
			// Tokens unidos: sus contactos quedan dentro del mismo grupo.
			// Se separan para conservar la posición e identidad de cada uno.
			int k = std::max(2, (int)std::round(cluster.size() / 3.0f));
			parts = splitCluster(cluster, k);
			mergeId = i;
		} else {
			parts = { cluster };
		}

		for (auto & part : parts) {
			Observation o;
			o.points = part;
			o.pos = classifier->centroid(part);
			o.letter = part.size() >= 3 ? classifier->classifyShape(classifier->orderByAngle(part)) : '?';
			o.label = labelFor(o.letter);
			o.mergeId = parts.size() > 1 ? mergeId : -1;
			obs.push_back(o);
		}
	}
	return obs;
}

void TokenTracker::matchObservations(std::vector<Observation> & obs) {
	float maxJump = settings().cm(settings().trackMaxJumpCm);

	// Emparejamiento voraz por distancia; preferir la misma etiqueta.
	struct Candidate {
		float cost;
		int t, o;
	};
	std::vector<Candidate> cands;
	for (int t = 0; t < (int)tokens.size(); ++t) {
		for (int o = 0; o < (int)obs.size(); ++o) {
			float d = glm::distance(tokens[t].pos, obs[o].pos);
			if (d > maxJump) continue;
			bool sameLabel = obs[o].label == "?" || obs[o].label == tokens[t].label;
			cands.push_back({ d + (sameLabel ? 0.0f : maxJump * 0.5f), t, o });
		}
	}
	std::sort(cands.begin(), cands.end(), [](auto & a, auto & b) { return a.cost < b.cost; });

	std::vector<bool> tUsed(tokens.size(), false), oUsed(obs.size(), false);
	for (auto & c : cands) {
		if (tUsed[c.t] || oUsed[c.o]) continue;
		tUsed[c.t] = oUsed[c.o] = true;

		auto & tok = tokens[c.t];
		auto & o = obs[c.o];
		tok.pos = o.pos;
		tok.points = o.points;
		tok.mergeId = o.mergeId;
		tok.rawLetter = o.letter;
		tok.visible = true;
		tok.missingFrames = 0;

		// Histéresis de etiqueta: la letra cruda puede parpadear entre frames.
		if (o.label != "?" && o.label != tok.label) {
			if (o.label == tok.pendingLabel) {
				if (++tok.pendingCount >= settings().labelConfirmFrames) {
					tok.label = o.label;
					tok.pendingCount = 0;
				}
			} else {
				tok.pendingLabel = o.label;
				tok.pendingCount = 1;
			}
		} else {
			tok.pendingCount = 0;
		}
	}

	// Tokens no vistos este frame: periodo de gracia, luego se retiran.
	for (int t = (int)tokens.size() - 1; t >= 0; --t) {
		if (tUsed[t]) continue;
		auto & tok = tokens[t];
		tok.visible = false;
		tok.mergeId = -1;
		if (++tok.missingFrames > settings().tokenGraceFrames) {
			// Primero se rompen sus uniones, para que cada separación quede registrada.
			for (auto it = pairs.begin(); it != pairs.end();) {
				if (it->first.first == tok.uid || it->first.second == tok.uid) {
					if (it->second.joined) {
						int other = it->first.first == tok.uid ? it->first.second : it->first.first;
						emit(TokenEventArgs::SEPARATED, tok, findToken(other));
					}
					it = pairs.erase(it);
				} else {
					++it;
				}
			}
			emit(TokenEventArgs::REMOVED, tok);
			tokens.erase(tokens.begin() + t);
		}
	}

	// Observaciones nuevas: tokens nuevos.
	for (int o = 0; o < (int)obs.size(); ++o) {
		if (oUsed[o]) continue;
		TrackedToken tok;
		tok.uid = nextUid++;
		tok.label = obs[o].label;
		tok.pos = obs[o].pos;
		tok.points = obs[o].points;
		tok.mergeId = obs[o].mergeId;
		tok.rawLetter = obs[o].letter;
		tokens.push_back(tok);
		emit(TokenEventArgs::ADDED, tokens.back());
	}
}

void TokenTracker::updatePairs() {
	float joinDist = settings().cm(settings().joinDistanceCm);
	for (int i = 0; i < (int)tokens.size(); ++i) {
		for (int j = i + 1; j < (int)tokens.size(); ++j) {
			auto & a = tokens[i];
			auto & b = tokens[j];
			// Si uno está oculto (gracia), su estado de unión se congela.
			if (!a.visible || !b.visible) continue;

			bool raw = (a.mergeId >= 0 && a.mergeId == b.mergeId)
				|| (joinDist > 0 && glm::distance(a.pos, b.pos) < joinDist);

			auto key = std::make_pair(std::min(a.uid, b.uid), std::max(a.uid, b.uid));
			auto & st = pairs[key];
			if (raw == st.joined) {
				st.counter = 0;
				continue;
			}
			int needed = st.joined ? settings().separateConfirmFrames : settings().joinConfirmFrames;
			if (++st.counter >= needed) {
				st.joined = raw;
				st.counter = 0;
				emit(raw ? TokenEventArgs::JOINED : TokenEventArgs::SEPARATED, a, &b);
			}
		}
	}
}

void TokenTracker::rebuildGroups() {
	// Union-find sobre los pares unidos.
	std::map<int, int> parent;
	for (auto & t : tokens)
		parent[t.uid] = t.uid;
	std::function<int(int)> find = [&](int x) { return parent[x] == x ? x : parent[x] = find(parent[x]); };
	for (auto & [key, st] : pairs) {
		if (st.joined && parent.count(key.first) && parent.count(key.second))
			parent[find(key.first)] = find(key.second);
	}

	std::map<int, Group> byRoot;
	for (auto & t : tokens)
		byRoot[find(t.uid)].uids.push_back(t.uid);

	groups.clear();
	for (auto & [root, g] : byRoot) {
		g.center = glm::vec2(0);
		for (int uid : g.uids)
			g.center += findToken(uid)->pos;
		g.center /= (float)g.uids.size();
		groups.push_back(g);
	}
}

void TokenTracker::update(const std::vector<ofVec2f> & pts) {
	auto obs = observe(pts);
	matchObservations(obs);
	updatePairs();
	rebuildGroups();
}

const TokenTracker::TrackedToken * TokenTracker::findToken(int uid) const {
	for (auto & t : tokens)
		if (t.uid == uid) return &t;
	return nullptr;
}

int TokenTracker::groupIndexOf(int uid) const {
	for (int g = 0; g < (int)groups.size(); ++g)
		for (int u : groups[g].uids)
			if (u == uid) return g;
	return -1;
}

void TokenTracker::emit(TokenEventArgs::Type type, const TrackedToken & a, const TrackedToken * b) {
	TokenEventArgs args;
	args.type = type;
	args.uidA = a.uid;
	args.labelA = a.label;
	args.pos = a.pos;
	if (b) {
		args.uidB = b->uid;
		args.labelB = b->label;
		args.pos = (a.pos + b->pos) * 0.5f;
	}
	args.time = ofGetElapsedTimef();
	ofNotifyEvent(Events::token(), args);
}

void TokenTracker::drawDebug() const {
	ofPushStyle();
	ofSetColor(255, 255, 255, 120);
	for (auto & f : fingers)
		ofDrawCircle(f, 6);

	for (auto & g : groups) {
		if (g.isJoined()) {
			ofNoFill();
			ofSetLineWidth(2);
			ofSetColor(80, 255, 160);
			ofDrawCircle(g.center, settings().cm(3.0f));
			for (size_t i = 1; i < g.uids.size(); ++i)
				ofDrawLine(findToken(g.uids[0])->pos, findToken(g.uids[i])->pos);
			ofFill();
		}
	}
	for (auto & t : tokens) {
		ofSetColor(t.visible ? ofColor(255, 220, 80) : ofColor(150, 150, 150));
		for (auto & p : t.points)
			ofDrawCircle(p, 5);
		ofDrawBitmapString(t.label + " #" + ofToString(t.uid), t.pos.x + 20, t.pos.y - 20);
	}
	ofPopStyle();
}
