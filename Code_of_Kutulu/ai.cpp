#include <iostream>
#include <vector>
#include <queue>
#include <tuple>
#include <cstring>

struct Entity {
    std::string entityType;
    int id;
    int x;
    int y;
    int param0;
    int param1;
    int param2;

    static Entity input() {
        Entity e;
        std::cin >> e.entityType >> e.id >> e.x >> e.y >> e.param0 >> e.param1 >> e.param2; std::cin.ignore();
        return e;
    }
};

constexpr int inf = (1 << 28);

int width;
int height;
std::vector<std::string> map;

int dx[] = {1, 0, -1, 0},
    dy[] = {0, 1, 0, -1};
int score[30][30];

int nearestWanderer(std::vector<Entity>& entities, int cx, int cy) {
    int dist[30][30];
    memset(dist, -1, sizeof(dist));

    using T = std::tuple<int, int, int>;

    std::queue<T> Q;
    Q.push(T(cx, cy, 0));
    dist[cx][cy] = 0;

    while (!Q.empty()) {
        int x, y, d;
        std::tie(x, y, d) = Q.front(); Q.pop();

        for (auto& e : entities) {
            if (e.entityType == "WANDERER" and e.param1 == 1 and e.x == x and e.y == y) {
                return d;
            }
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i],
                    ny = y + dy[i];
                if (map[nx][ny] != '#' and dist[nx][ny] < 0) {
                    Q.push(T(nx, ny, d + 1));
                    dist[nx][ny] = d + 1;
                }
            }
        }
    }
    return inf;
}

std::string think(std::vector<Entity>& entities) {
    Entity me = entities[0];

    int bestX = me.x,
        bestY = me.y;

    for (int i = 0; i < 4; i++) {
        int nx = me.x + dx[i],
            ny = me.y + dy[i];
        
        // std::cerr << nx << ", " << ny << " score is " << score[nx][ny] << std::endl;
        if (map[nx][ny] == '#') continue;
        
        if (nearestWanderer(entities, nx, ny) > nearestWanderer(entities, bestX, bestY)) {
            bestX = nx;
            bestY = ny;
        }
    }
    // std::cerr << me.x << ", " << me.y << " score is " << score[me.x][me.y] << std::endl;

    return "MOVE " + std::to_string(bestX) + " " + std::to_string(bestY);
}

int main() {
    std::cin >> width; std::cin.ignore();
    std::cin >> height; std::cin.ignore();

    map.resize(width, "");
    for (int y = 0; y < height; y++) {
        std::string row;
        std::getline(std::cin, row);

        for (int x = 0; x < width; x++) {
            map[x].push_back(row[x]);
        }
    }

    int sanityLossLonely; // how much sanity you lose every turn when alone, always 3 until wood 1
    int sanityLossGroup; // how much sanity you lose every turn when near another player, always 1 until wood 1
    int wandererSpawnTime; // how many turns the wanderer take to spawn, always 3 until wood 1
    int wandererLifeTime; // how many turns the wanderer is on map after spawning, always 40 until wood 1
    std::cin >> sanityLossLonely >> sanityLossGroup >> wandererSpawnTime >> wandererLifeTime; std::cin.ignore();

    // game loop
    while (true) {
        int entityCount; // the first given entity corresponds to your explorer
        std::cin >> entityCount; std::cin.ignore();

        std::vector<Entity> entities(entityCount);

        for (int i = 0; i < entityCount; i++) {
            entities[i] = Entity::input();
        }

        std::cout << think(entities) << std::endl;
    }

    return 0;
}
