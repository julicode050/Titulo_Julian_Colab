#pragma once
#include "Events.h"
#include "Token.hpp"
#include "ofMain.h"
#include <map>
#include <vector>

// Convierte los puntos de contacto crudos en tokens persistentes y grupos de tokens unidos.
// Reutiliza la detección existente: clusterTouches (Tools) para agrupar puntos y
// Token::orderByAngle / Token::classifyShape para identificar cada patrón.
//
// Es el único lugar del código que decide cuándo un token aparece, desaparece,
// se une o se separa; cada cambio se emite por Events::token().
class TokenTracker {
public:
	struct TrackedToken {
		int uid = -1;
		std::string label; // etiqueta del token físico ("A", "B") o la letra cruda si no está mapeada
		glm::vec2 pos; // centroide en px
		bool visible = true; // false mientras está en periodo de gracia
		int missingFrames = 0;
		int mergeId = -1; // >= 0 si este frame vino de un grupo de puntos fusionado
		std::vector<ofVec2f> points;
		std::string pendingLabel;
		int pendingCount = 0;
	};

	struct Group {
		std::vector<int> uids;
		glm::vec2 center;
		int size() const { return (int)uids.size(); }
		bool isJoined() const { return uids.size() >= 2; }
	};

	void setup(const Token * classifier);
	void update(const std::vector<ofVec2f> & pts);
	void drawDebug() const;

	const std::vector<TrackedToken> & getTokens() const { return tokens; }
	const std::vector<Group> & getGroups() const { return groups; }
	const std::vector<glm::vec2> & getFingers() const { return fingers; }
	const TrackedToken * findToken(int uid) const;
	int groupIndexOf(int uid) const;

private:
	struct Observation {
		std::string label;
		char letter = '?';
		glm::vec2 pos;
		int mergeId = -1;
		std::vector<ofVec2f> points;
	};
	struct PairState {
		bool joined = false;
		int counter = 0;
	};

	std::vector<Observation> observe(const std::vector<ofVec2f> & pts);
	std::string labelFor(char letter) const;
	void matchObservations(std::vector<Observation> & obs);
	void updatePairs();
	void rebuildGroups();
	void emit(TokenEventArgs::Type type, const TrackedToken & a, const TrackedToken * b = nullptr);

	const Token * classifier = nullptr;
	std::vector<TrackedToken> tokens;
	std::vector<Group> groups;
	std::vector<glm::vec2> fingers;
	std::map<std::pair<int, int>, PairState> pairs;
	int nextUid = 1;
};
