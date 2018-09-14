#include <iostream>
#include <map>
#include <algorithm>
#include <vector>
#include <array>
#include <queue>
#include <functional>
#include <assert.h>
#include <random>
using namespace std;

const int MAX_WIDTH  = 24;
const int MAX_HEIGHT = 20;

const int INF = (1 << 28);

// entityを表す構造体
struct Entity {
    string entityType;
    int id;
    int x;
    int y;
    int param0;
    int param1;
    int param2;

    Entity() {}
    Entity(string entityType, int id, int x, int y, int param0, int param1, int param2):
        entityType(entityType), id(id), x(x), y(y), param0(param0), param1(param1), param2(param2) {}
};

// ビットマスク関連
using BitMask = int;

const BitMask BLANK    = 0;
const BitMask EXPLORER = (1 << 0);
const BitMask WANDERER = (1 << 1);
const BitMask SLASHER  = (1 << 2);
const BitMask WALL     = (1 << 3);
const BitMask SPAWN    = (1 << 4);

// EFFECT_PLAN, EFFECT_LIGHT, EFFECT_SHELTER, EFFECT_YELLは無視

using Map = array<array<BitMask, MAX_HEIGHT>, MAX_WIDTH>;

Map buildMap(vector<Entity>& entities) {
    Map map;
    // mazeとentitiesからmapを作り上げる
    return map;
}

// ゲームの状態を表す構造体
// [ルール]
// 引数で渡すときは参照渡し、返り値は実体で扱うものとする。
// 値として扱うときは変数名としてnode、参照として扱うときはnowやnext、nなどとする。
struct Node {
    Map map;
    Node *parent;
    string output;
    vector<Entity> entities;
    int planningDuration;
    int lightingDuration;
    int score;

    Node() {}
    Node(vector<Entity>& entities, int planningDuration, int lightingDuration):
            entities(entities), planningDuration(planningDuration), lightingDuration(lightingDuration) {
        map    = buildMap(entities);
        parent = nullptr;
        score  = 0;
    }
    Node(int entityCount) {
        entities.resize(entityCount);
    }
    Node(Node* n) {
        map              = n->map;
        parent           = n;
        entities         = n->entities;
        planningDuration = n->planningDuration;
        lightingDuration = n->lightingDuration;
        score            = n->score;
    }

    bool operator<(const Node& node) const {
        return score < node.score;
    }
    bool operator>(const Node& node) const {
        return score > node.score;
    }
};

// ゲームについて不変の情報
int width, height;
int sanityLossLonely, sanityLossGroup, wandererSpawnTime, wandererLifeTime;
BitMask maze[MAX_WIDTH][MAX_HEIGHT];

// プレイヤーのExplorerについての情報
int myX, myY, myID, mySanity;
int planningDuration, lightingDuration;

//
// 評価関数周り
//
struct Evaluator {
    Node *n;
    int score[MAX_WIDTH][MAX_HEIGHT];

    Evaluator(Node *n) : n(n) {}

    int evaluate() {
        return 0;
    }
    void setNode(Node *n) {
        this->n = n;
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                score[x][y] = 0;
            }
        }
    }
};

//
// シミュレータ周り
//
struct Simulator {
    Node *n;

    Simulator(Node *n) : n(n) {}

    Node simulateRandom() {
    }
    void setNode(Node *n) {
        this->n = n;
    }
};

//
//  ユーティリティ
//
bool isInside(int x, int y) {
    return 0 <= x and x < width and 0 <= y and y < height;
}

int calcDistance(Entity& e1, Entity& e2) {
    return abs(e1.x - e2.x) + abs(e1.y - e2.y);
}

int calcDistance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

void updateState() {
    planningDuration = max(0, planningDuration - 1);
    lightingDuration = max(0, lightingDuration - 1);
}

void updateState(Node* n) {
    n->planningDuration = max(0, n->planningDuration - 1);
    n->lightingDuration = max(0, n->lightingDuration - 1);
}

bool hasExplorer(Map& map, int x, int y) {
    return map[x][y] & EXPLORER;
}

bool hasWanderer(Map& map, int x, int y) {
    return map[x][y] & WANDERER;
}

bool hasSlasher(Map& map, int x, int y) {
    return map[x][y] & SLASHER;
}

bool hasWall(Map& map, int x, int y) {
    return map[x][y] & WALL;
}

bool hasSpawn(Map& map, int x, int y) {
    return map[x][y] & SPAWN;
}

int randInt() {
    static random_device rnd;
    static mt19937 mt(rnd());
    return mt();
}

//
// ゲームのルール部分
//

void inputGameConstant() {
    cin >> width; cin.ignore();

    cin >> height; cin.ignore();

    for (int i = 0; i < height; i++) {
        string line;
        getline(cin, line);

        map<char, int> m = {{'.', BLANK}, {'w', WALL}, {'#', SPAWN}};

        for (int j = 0; j < width; j++) {
            maze[j][i] |= m[line[j]];
        }
    }

    cin >> sanityLossLonely >> sanityLossGroup >> wandererSpawnTime >> wandererLifeTime; cin.ignore();
}

vector<Entity> inputEntities() {
    int entityCount;
    cin >> entityCount; cin.ignore();

    vector<Entity> entities;

    for (int i = 0; i < entityCount; i++) {
        string entityType;
        int id;
        int x;
        int y;
        int param0;
        int param1;
        int param2;

        cin >> entityType >> id >> x >> y >> param0 >> param1 >> param2; cin.ignore();

        entities.emplace_back(entityType, id, x, y, param0, param1, param2);

        if (i == 0) {
            myID     = id;
            myX      = x;
            myY      = y;
            mySanity = param0;
        }
    }
    return entities;
}

void execNode(Node *n) {
    cout << n->output << endl;

    if (n->output == "PLAN") {
        planningDuration = 5;
    } else if (n->output == "LIGHT") {
        lightingDuration = 3;
    }
    updateState();
}

//
// アルゴリズム部分
//
const int SEARCH_DEPTH = 100;
const int BEAM_WIDTH   = 20;

// 参考:
Node beamSearch(Node *now) {
    priority_queue<Node> nexts;
    nexts.push(*now);

    // nextsに突っ込まれたデータをすべて保存するものが必要
    static Node history[SEARCH_DEPTH * BEAM_WIDTH];

    for (int depth = 0; depth < SEARCH_DEPTH; depth++) {
        priority_queue<Node> temp;  // BEAM_WIDTH個しか要素が入らない

        for (int i = 0; i < BEAM_WIDTH and !nexts.empty(); i++) {
            Node node = nexts.top(); nexts.pop();
            temp.push(node);
        }

        // nextsにはtempからの遷移先がすべて入る
        nexts = priority_queue<Node>();

        while (!temp.empty()) {
            Node node = temp.top(); temp.pop();

            // 遷移先
            node.output = "WAIT";
            node.parent = now;
            nexts.push(node);
        }
    }
    Node node = nexts.top(),
         *cur = &node;

    while (cur->parent != now) {
        cur = cur->parent;
    }
    return *cur;
}

int main() {
    inputGameConstant();

    while (true) {
        auto entities = inputEntities();

        Node node = Node(entities, planningDuration, lightingDuration),
             ans  = beamSearch(&node);
        execNode(&ans);

        cerr << randInt() << endl;
    }


    return 0;
}
