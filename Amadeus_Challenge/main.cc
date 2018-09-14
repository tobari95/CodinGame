#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <algorithm>
#include <random>
#include <string>
#include <utility>
#include <random>
using namespace std;

const int MAX_PLANET_COUNT = 90;
const int INF = (1 << 28);

// Moveのtype
const int NORMAL      = 0;
const int UNIT_SPREAD = 1;

int planetCount;
int edgeCount;

vector<int> G[MAX_PLANET_COUNT];    // グラフ構造を表す。

struct Move {
    int type;           // 0: 通常 1: unit spread
    int target;         // output == "NONE"のときは-1
    string output;

    Move(int type, int target) : type(type), target(target) {
        if (type == UNIT_SPREAD and target < 0) output = "NONE";
        else output = to_string(target);
    }
};

using Strategy = vector<Move>;

struct GameState {
    int score;
    array<int, MAX_PLANET_COUNT> myUnits;
    array<int, MAX_PLANET_COUNT> myTolerance;
    array<int, MAX_PLANET_COUNT> otherUnits;
    array<int, MAX_PLANET_COUNT> otherTolerance;
    array<int, MAX_PLANET_COUNT> canAssign;

    GameState() {}
    GameState(const GameState& state) {
        myUnits        = state.myUnits;
        myTolerance    = state.myTolerance;
        otherUnits     = state.otherUnits;
        otherTolerance = state.otherTolerance;
        canAssign      = state.canAssign;
    }
    bool operator<(const GameState& state) const {
        return score < state.score;
    }
    bool operator>(const GameState& state) const {
        return score > state.score;
    }
};

// n未満の自然数をランダムに生成する
int randInt(int n) {
    static random_device rnd;
    static mt19937 mt(rnd());
    return mt();
}

void inputGameConstants() {
    cin >> planetCount >> edgeCount; cin.ignore();

    for (int i = 0; i < edgeCount; i++) {
        int planetA, planetB;
        cin >> planetA >> planetB; cin.ignore();

        G[planetA].push_back(planetB);
        G[planetB].push_back(planetA);
    }
}

GameState inputPlanets() {
    GameState state;
    for (int i = 0; i < planetCount; i++) {
        cin >> state.myUnits[i] >> state.myTolerance[i]
            >> state.otherUnits[i] >> state.otherTolerance[i] >> state.canAssign[i]; cin.ignore();
    }
    return state;
}

GameState advanceGameState(GameState& state, Move& move) {
    GameState nextState(state);
    if (move.type == NORMAL) {
        nextState.myUnits[move.target]++;
    } else if (move.target >= 0) {  // UNIT SPREAD
        nextState.myUnits[move.target] -= 5;
        for (int neighbor : G[move.target]) {
            nextState.myUnits[neighbor]++;
        }
    }
    return nextState;
}

array<int, MAX_PLANET_COUNT> evaluatePlanets(const GameState& state) {
    array<int, MAX_PLANET_COUNT> scores;    // 0で初期化される
    scores.fill(0);

    for (int id = 0; id < planetCount; id++) {
        // 1. 均衡している頂点からとっていきたい
        scores[id] -= 3 * abs(state.myUnits[id] - state.otherUnits[id]);
        // 2. myTolerance > otherToleranceなら嬉しい
        scores[id] += state.myTolerance[id] - state.otherTolerance[id];
        // 3. 頂点を割り振れないのなら無視したい
        scores[id] -= state.canAssign[id] ? 0 : INF; 

        // 追加したい要素
        // * 周りの味方の数と敵の数
        // * 簡単な敵の攻撃パターンの予測

        for (int neighbor : G[id]) {
            int advantage = state.myUnits[neighbor] - state.myUnits[neighbor];
            if (advantage > 0) scores[id] += 1;
            else if (advantage < 0) scores[id] -= 1;
        }
    }
    return scores;
}

int evaluateGameState(const GameState& state) {
    int score = 0;
    for (int id = 0; id < planetCount; id++) {
        int advantage = state.myUnits[id] - state.otherUnits[id];
    if (advantage <= 0) continue;

        int friendCount = 0,
            enemyCount  = 0;
        for (int neighbor : G[id]) {
            int nAdvantage = state.myUnits[neighbor] - state.otherUnits[neighbor];
            if (nAdvantage > 0) friendCount++;
            else if (nAdvantage < 0) enemyCount++;
        }

        if (friendCount > enemyCount) advantage++;
        else if (friendCount < enemyCount) advantage--;

        score += 20 * (advantage > 0) - enemyCount;
    }
    return score;
}

pair<Move, int> suggestUnitSpreadPlanet(GameState& state) {
    static Move none(UNIT_SPREAD, -1);

    auto evaluateMove = [&](Move& m) {
        return evaluateGameState(advanceGameState(state, m));
    };

    vector<pair<Move, int>> moves = { make_pair(none, evaluateMove(none)) };

    // どの頂点をunit spreadさせるか
    for (int id = 0; id < planetCount; id++) {
        if (!state.canAssign[id] or state.myUnits[id] < 5) continue;
        Move move(UNIT_SPREAD, id);
        moves.push_back(make_pair(move, evaluateMove(move)));
    }
    // score降順になるようにソート
    sort(moves.begin(), moves.end(), [](const pair<Move, int>& mi1, const pair<Move, int>& mi2) {
        return mi1.second > mi2.second;
    });
    return moves[0];
}

pair<Strategy, int> suggestStrategy1(GameState& state) {
    GameState curState = state;
    Strategy strategy;

    for (int i = 0; i < 5; i++) {
        auto scores = evaluatePlanets(curState);
        // 貪欲に一番評価値が高い頂点を選ぶ
        int target = -1;
        for (int id = 0; id < planetCount; id++) {
            if (target < 0 or scores[id] > scores[target]) target = id;
            // cerr << "scores[" << id << "] " << scores[id] << endl;
        }
        Move move(NORMAL, target);
        
        curState = advanceGameState(state, move);
        strategy.push_back(move);
    }
    auto recommended = suggestUnitSpreadPlanet(curState);
    strategy.push_back(recommended.first);

    return make_pair(strategy, recommended.second);
}

pair<Strategy, int> suggestStrategy2(GameState& state) {
    auto concentrateAndUnitSpread = [&](int target) {
        GameState curState = state;
        Strategy strategy;
        for (int i = 0; i < 5; i++) {
            Move move(NORMAL, target);
            curState = advanceGameState(curState, move);
            strategy.push_back(move);
        }
        auto recommended = suggestUnitSpreadPlanet(curState);
        strategy.push_back(recommended.first);

        return make_pair(strategy, recommended.second);

        if (target == 3) {
            cerr << "target: " << target << " score: " << evaluateGameState(curState) << endl;
            for (int id = 0; id < planetCount; id++) {
                cerr << "id: " << id << " my: " << curState.myUnits[id] << " other: " << curState.otherUnits[id] << endl;
            }
        }
    };

    Strategy strategy;
    int score = -INF;
    for (int id = 0; id < planetCount; id++) {
        if (!state.canAssign[id]) continue;
        auto choice = concentrateAndUnitSpread(id);

        if (choice.second > score) {
            strategy = choice.first;
            score = choice.second;
        } else if (choice.second == score and randInt(2) > 0) {
            strategy = choice.first;
            score = choice.second;
        }
    }
    return make_pair(strategy, score);
}

Strategy developStrategy(GameState& state) {
    // 戦略1) 評価値の高いところに貪欲に5つ詰め込んでいく。 => unit spreadした方がいいのならする。
    // 戦略2) unit spreadありきの戦略
    auto choice1 = suggestStrategy1(state);
    auto choice2 = suggestStrategy2(state);

    if (choice1.second >= choice2.second) {
        cerr << "Strategy1 [score] " << choice1.second << endl;
        return choice1.first;
    } else {
        cerr << "Strategy2 [score] " << choice2.second << endl;
        return choice2.first;
    }
}

int main() {
    inputGameConstants();

    while (true) {
        auto game = inputPlanets();
        auto moves = developStrategy(game);

        for (auto& move : moves) {
            cout << move.output << endl;
        }
    }

    return 0;
}