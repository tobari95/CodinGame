#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <utility>
#include <string>
#include <cstring>
#include <assert.h>
using namespace std;

const int INF = (1 << 28);

namespace Random
{
    static random_device rnd;
    static mt19937 mt(rnd());

    // n未満の非負整数をランダムに返す
    int randInt(int n)
    {
        return mt() % n;
    }

    // [a, b)に属する整数をランダムに返す
    int randInt(int a, int b)
    {
        int width = b - a;
        return randInt(width) + a;
    }
};

namespace Util
{
    vector<int> makePermutation(int n)
    {
        vector<int> perm(n);
        for (int i = 0; i < n; i++) perm[i] = i;
        return perm;
    }

    template <class T>
    vector<T> concat(const vector<T>& v1, const vector<T>& v2)
    {
        vector<T> vec(v1.begin(), v1.end());
        for (auto elt : v2)
        {
            vec.push_back(elt);
        }
        return vec;
    }
};

namespace GC
{
    // location
    static const int MY_HAND = 0;
    static const int MY_SIDE = 1;
    static const int OP_SIDE = -1;

    // move
    static const string names[] = { "PASS", "PICK", "SUMMON", "ATTACK", "USE" };
    static const int PASS = 0;
    static const int PICK = 1;
    static const int SUMMON = 2;
    static const int ATTACK = 3;
    static const int USE = 4;

    // card
    static const int CREATURE = 0;
    static const int GREEN_ITEM = 1;
    static const int RED_ITEM = 2;
    static const int BLUE_ITEM = 3;
};

struct Card
{
    int number;
    int id;
    int location;   // 0: 自分の手札, 1: 自分の場, -1: 相手の場
    int type;
    int cost;
    int attack;
    int defense;
    string abilities;   // ex) ---G--
    int myHealthChange;
    int opponentHealthChange;
    int cardDraw;
    bool canUse;

    Card() {}
    Card(int number, int id, int location, int type, int cost, int attack,
         int defense, string abilities, int myHealthChange, int opponentHealthChange,
         int cardDraw) :
        number{number}, id{id}, location{location}, type{type}, cost{cost}, attack{attack},
        defense{defense}, abilities{abilities}, myHealthChange{myHealthChange},
        opponentHealthChange{opponentHealthChange}, cardDraw{cardDraw}, canUse{canUse}
    {
        if (type == GC::CREATURE) canUse = (location == GC::MY_SIDE);
        else canUse = (type == GC::GREEN_ITEM);
    }
    Card(const Card& card) : number{card.number}, id{card.id}, location{card.location},
                             type{card.type}, cost{card.cost}, attack{card.attack}, defense{card.defense},
                             abilities{card.abilities}, myHealthChange{card.myHealthChange},
                             opponentHealthChange{card.opponentHealthChange}, cardDraw{card.cardDraw},
                             canUse{card.canUse} {}
    
    static Card input()
    {
        int number, id, location, type, cost, attack, defense,
            myHealthChange, opponentHealthChange, cardDraw;
        string abilities;

        cin >> number >> id >> location >> type >> cost >> attack >> defense >>
               abilities >> myHealthChange >> opponentHealthChange >> cardDraw; cin.ignore();

        return Card(number, id, location, type, cost, attack, defense,
                    abilities, myHealthChange, opponentHealthChange, cardDraw);
    }
};

bool hasBreakthroughAbility(const Card& card)
{
    return card.abilities[0] == 'B';
}

void addBreakthroughAbility(Card& card)
{
    card.abilities[0] = 'B';
}

bool hasChargeAbility(const Card& card)
{
    return card.abilities[1] == 'C';
}

void addChargeAbility(Card& card)
{
    card.abilities[1] = 'C';
}

bool hasDrainAbility(const Card& card)
{
    return card.abilities[2] == 'D';
}

void addDrainAbility(Card& card)
{
    card.abilities[2] = 'D';
}

bool hasGuardAbility(const Card& card)
{
    return card.abilities[3] == 'G';
}

void addGuardAbility(Card& card)
{
    card.abilities[3] = 'G';
}

bool hasLethalAbility(const Card& card)
{
    return card.abilities[4] == 'L';
}

void addLethalAbility(Card& card)
{
    card.abilities[4] = 'L';
}

bool hasWardAbility(const Card& card)
{
    return card.abilities[5] == 'W';
}

void addWardAbility(Card& card)
{
    card.abilities[5] = 'W';
}

void removeWardAbility(Card& card)
{
    card.abilities[5] = '-';
}

struct Move
{
    int type;
    int target1;
    int target2;

    Move() {}
    Move(int type, int target1 = -1, int target2 = -1) :
        type{type}, target1{target1}, target2{target2} {}

    string to_string()
    {
        string res = GC::names[type];

        if (type == GC::PASS)
        {
            return res;
        }
        else if (type == GC::PICK or type == GC::SUMMON)
        {
            return res + " " + std::to_string(target1);
        }
        else
        {
            return res + " " + std::to_string(target1) + " " + std::to_string(target2);
        }
    }
};

using Strategy = vector<Move>;

void output(Strategy& s)
{
    cout << s[0].to_string();

    for (int i = 1; i < s.size(); i++)
    {
        cout << "; " << s[i].to_string();
    }
    cout << endl;
}

bool isDead(const Card& creature)
{
    return creature.defense <= 0;
}

bool isAlive(const Card& creature)
{
    return creature.defense > 0;
}

bool isAttacker(const Card& card)
{
    return card.location == GC::MY_SIDE and card.canUse and isAlive(card);
}

bool isEnemy(const Card& card)
{
    return card.location == GC::OP_SIDE and isAlive(card);
}

bool isReserve(const Card& card)
{
    return card.location == GC::MY_SIDE and card.type == GC::CREATURE and card.canUse;
}

bool isObstacle(const Card& card)
{
    return isEnemy(card) and hasGuardAbility(card);
}

//////////////////////////////////////// 戦略部分 //////////////////////////////////////// 

struct GameNode
{
    int id;
    int parent;
    int myHealth;
    int myMana;
    int myDeck; // 無視
    int myRune; // 無視
    int opHealth;
    int opMana;
    int opDeck; // 無視
    int opRune; // 無視
    int opHand;   // 無視
    vector<Card> cards;
    int score;
    Move move;

    GameNode() {}
    GameNode(int myHealth, int myMana, int myDeck, int myRune,
             int opHealth, int opMana, int opDeck, int opRune,
             int opponentHand, vector<Card>& cards, int score = -INF) :
             id{-1}, parent{-1},
             myHealth{myHealth}, myMana{myMana}, myDeck{myDeck}, myRune{myRune},
             opHealth{opHealth}, opMana{opMana}, opDeck{opDeck}, opRune{opRune},
             opHand{opponentHand}, cards{cards}, score{score} {}
    GameNode(const GameNode& node) :
             id{-1}, parent{node.id},
             myHealth{node.myHealth}, myMana{node.myMana}, myDeck{node.myDeck}, myRune{node.myRune},
             opHealth{node.opHealth}, opMana{node.opMana}, opDeck{node.opDeck}, opRune{node.opRune},
             opHand{node.opHand}, cards{node.cards}, score{node.score}, move{node.move} {}

    static GameNode input()
    {
        int myHealth, myMana, myDeck, myRune,
            opHealth, opMana, opDeck, opRune;
        cin >> myHealth >> myMana >> myDeck >> myRune; cin.ignore();
        cin >> opHealth >> opMana >> opDeck >> opRune; cin.ignore();

        int opponentHand;
        cin >> opponentHand; cin.ignore();
        
        int cardCount;
        cin >> cardCount; cin.ignore();

        vector<Card> cards(cardCount);
        for (int i = 0; i < cardCount; i++)
        {
            cards[i] = Card::input();
        }
        return GameNode(myHealth, myMana, myDeck, myRune,
                        opHealth, opMana, opDeck, opRune, opponentHand, cards);
    }

    // 計算を軽くするため、cardsのインデックスを引数にする
    bool summon(int cardIndex)
    {
        Card& creature = cards[cardIndex];
        if (myMana < creature.cost) return false;

        int summonedNum = 0;    // 場に出ているCreatureの数
        for (auto& card : cards) if (card.location == GC::MY_SIDE) summonedNum++;

        if (summonedNum >= 6) return false;

        myMana = max(0, myMana - creature.cost);
        myHealth += creature.myHealthChange;
        opHealth = max(0, opHealth - creature.opponentHealthChange);

        // assert(creature.opponentHealthChange <= 0);
        cards[cardIndex].location = GC::MY_SIDE;

        if (hasChargeAbility(creature))
        {
            cards[cardIndex].canUse = true;
        }
        // 手札については無視
        return true;
    }

    bool existsObstacle()
    {
        for (Card& card : cards)
        {
            if (card.location == GC::OP_SIDE and hasGuardAbility(card) and isAlive(card))
            {
                return true;
            }
        }
        return false;
    }

    bool useGreenItem(int cardIndex1, int cardIndex2)
    {
        Card &item = cards[cardIndex1],
             &cr = cards[cardIndex2];
        if (myMana < item.cost or isDead(cr)) return false;

        cards[cardIndex2].attack += item.attack;
        cards[cardIndex2].defense += item.defense;
        for (int i = 0; i < 6; i++)
        {
            if (item.abilities[i] != '-') cr.abilities[i] = item.abilities[i];
        }
        return true;
    }

    bool attack(int cardIndex1, int cardIndex2)
    {
        if (cardIndex2 < 0) // direct attack
        {
            if (existsObstacle()) return false;
            opHealth -= cards[cardIndex1].attack;
            cards[cardIndex1].canUse = false;
            return true;
        }
        // Wardは無視
        Card &cr1 = cards[cardIndex1],
             &cr2 = cards[cardIndex2];
        if (!isEnemy(cr2) or !isAttacker(cr1)) return false;

        cr1.defense -= cr2.attack;
        cr1.canUse = false;

        if (hasWardAbility(cr2))
        {
            removeWardAbility(cr2);
            return true;
        }

        int damage = min(cr1.attack, cr2.defense);

        cr2.defense -= cr1.attack;

        if (damage > 0 and hasDrainAbility(cr1))
        {
            myHealth += damage;
        }
        if (damage > 0 and hasLethalAbility(cr1))
        {
            cr2.defense = 0;
        }
        return true;
    }

    int getID(int idx)
    {
        return cards[idx].id;
    }

    int findReserve(int start = 0)
    {
        int idx = -1;
        for (int i = start; i < cards.size(); i++)
        {
            if (cards[i].location == GC::MY_HAND and cards[i].type == GC::CREATURE)
            {
                idx = i;
                break;
            }
        }
        return idx;
    }

    int findAttacker(int start = 0)
    {
        int idx = -1;
        for (int i = start; i < cards.size(); i++)
        {
            if (isAttacker(cards[i]))
            {
                idx = i;
                break;
            }
        }
        return idx;
    }

    int findObstacle(int start = 0)
    {
        int idx = -1;
        for (int i = start; i < cards.size(); i++)
        {
            if (isObstacle(cards[i]))
            {
                idx = i;
                break;
            }
        }
        return idx;
    }

    int findEnemy(int start = 0)
    {
        int idx = -1;
        for (int i = start; i < cards.size(); i++)
        {
            if (isEnemy(cards[i]))
            {
                idx = i;
                break;
            }
        }
        return idx;
    }

    int findGreenItem(int start = 0)
    {
        int idx = -1;
        for (int i = start; i < cards.size(); i++)
        {
            if (cards[i].type == GC::GREEN_ITEM and cards[i].canUse)
            {
                idx = i;
                break;
            }
        }
        return idx;
    }

    bool operator<(const GameNode& node) const
    {
        return score < node.score;
    }
};

//                 0, 1, 2, 3, 4, 5, 6, 7+
int idealDeck[] = {1, 4, 7, 6, 5, 3, 2, 2};
// int idealDeck[] = {INF, INF, INF, INF, INF, INF, INF, INF};
int deck[8] = {};
int creatureNum = 0;

int evaluateCreature(const Card& creature)
{
    double score = 1.5 * creature.cost - (creature.attack + creature.defense) / 2;
    if (hasGuardAbility(creature)) score -= creature.defense;
    else score -= creature.attack;

    if (hasChargeAbility(creature)) score -= 5 + creature.attack;
    score -= creature.cardDraw * 3;

    if (hasLethalAbility(creature)) score -= 20;
    if (hasWardAbility(creature)) score -= 5;

    return score;
}

Strategy draft(GameNode& node)
{
    int reserve = 0,
        target = -1;
    Card targetCreature;

    for (int i = 0; i < 3; i++)
    {
        Card& card = node.cards[i];
        int cost = min(card.cost, 7);

        if (card.number == 80 or card.number == 81)
        {
            return Strategy({ Move(GC::PICK, i) });
        }

        if (card.type == GC::GREEN_ITEM)
        {
            reserve = i;
            /*
            if (creatureNum >= 26)
            {
                return Strategy({ Move(GC::PICK, i) });
            }
            */
        }
        if (card.type != GC::CREATURE) continue;

        if (deck[cost] >= idealDeck[cost])
        {
            reserve = i;
        }
        else if (target < 0 or evaluateCreature(targetCreature) > evaluateCreature(card))
        {
            targetCreature = card;
            target = i;
        }
    }
    if (target < 0) target = reserve;
    if (node.cards[target].type == GC::CREATURE) creatureNum++;
    deck[min(7, node.cards[target].cost)]++;

    return Strategy{ Move(GC::PICK, target) };
}

Strategy targetObstacles(GameNode& node)
{
    Strategy strategy;

    // Guard持ちを攻撃
    int obstacleI = 0;
    while ((obstacleI = node.findObstacle()) != -1)
    {
        Card& obstacle = node.cards[obstacleI];

        int idx = 0;
        vector<int> attackerIndexes;
        while ((idx = node.findAttacker(idx)) != -1)
        {
            attackerIndexes.push_back(idx);
            idx++;
        }

        int bestAttackerI = -1, bestScore = (1 << 28);
        for (int attackerI : attackerIndexes)
        {
            Card& attacker = node.cards[attackerI];
            if (attacker.attack == 0)
            {
                attacker.canUse = false;
                continue;
            }
            int score = -(20 * (hasLethalAbility(attacker) or attacker.attack >= obstacle.defense)
                            - (attacker.attack + attacker.defense));
            
            if (score < bestScore)
            {
                bestScore = score;
                bestAttackerI = attackerI;
            }
        }

        if (bestAttackerI < 0) break;

        if (node.attack(bestAttackerI, obstacleI))
        {
            strategy.emplace_back(GC::ATTACK, node.getID(bestAttackerI), node.getID(obstacleI));
        }
    }
    return strategy;
}

int findBestAttacker(GameNode& node, int target)
{
    if (target < 0) return node.findAttacker();

    Card& targetCreature = node.cards[target];

    int bestAttackerI = -1,
        bestScore = -INF;
    for (int i = 0; i < node.cards.size(); i++)
    {
        Card& card = node.cards[i];
        if (!isAttacker(card)) continue;

        int score = -(card.attack + card.defense) / 2;
        if (hasWardAbility(targetCreature)) score = - card.attack * card.attack * 5;
        else if (hasLethalAbility(card) or card.attack >= targetCreature.attack) score += 10;

        if (score > bestScore)
        {
            bestAttackerI = i;
            bestScore = score;
        }
    }
    return bestAttackerI;
}

// 攻撃力が高いのに体力が少ないやつ
int findTarget(GameNode& node)
{
    int target = -1,
        bestScore = -INF;
    for (int i = 0; i < node.cards.size(); i++)
    {
        Card& card = node.cards[i];
        int score = (card.attack - card.defense) + hasDrainAbility(card) * 3;

        if (isEnemy(card) and bestScore < score)
        {
            target = i;
            bestScore = score;
        }
    }
    return target;
}

Strategy attack(GameNode& node)
{
    Strategy strategy = targetObstacles(node);

    // 自分の攻撃力、相手の攻撃力、自分の守備力を計算
    int myAttack = 0,
        myDefense = 0,
        opAttack = 0;
    for (Card& card : node.cards)
    {
        if (isEnemy(card))
        {
            opAttack += card.attack;
            continue;
        }
        else if (card.location != GC::MY_SIDE) continue;

        if (isAttacker(card)) myAttack += card.attack;
        if (isAlive(card) and hasGuardAbility(card)) myDefense += card.defense;
    }

    // 後で直す
    while (true)
    {
        int obstacleI = node.findObstacle(),
            bestAttackerI = -1,
            target = -1;

        if (obstacleI >= 0)
        {
            bestAttackerI = findBestAttacker(node, obstacleI);
            target = obstacleI;
        }
        else if (node.opHealth < 10 or opAttack - myDefense < 5)
        {
            // 敵をダイレクトアタック
            bestAttackerI = findBestAttacker(node, -1);
            target = -1;
        }
        else
        {
            target = findTarget(node);
            bestAttackerI = findBestAttacker(node, target);
        }
        if (bestAttackerI < 0) break;

        Card& attacker = node.cards[bestAttackerI];

        int curDefense = attacker.defense;
        myAttack -= attacker.attack;

        if (node.attack(bestAttackerI, target))
        {
            int targetID = (target >= 0) ? node.getID(target) : (-1);
            strategy.emplace_back(Move(GC::ATTACK, node.getID(bestAttackerI), targetID));
        }
        else cerr << "ERROR" << endl;

        if (isDead(attacker))
        {
            myDefense -= curDefense;
        }
        if (target >= 0 and isDead(node.cards[target]))
        {
            opAttack -= node.cards[target].attack;
        }
    }

    return strategy;
}

Strategy useGreenItem(GameNode& node)
{
    Strategy strategy;
    int idx = 0;

    while ((idx = node.findGreenItem(idx)) != -1)
    {
        cerr << "greenItem!!!" << endl;

        Card& item = node.cards[idx];
        int attacker = node.findAttacker();

        if (attacker < 0) break;

        if (node.useGreenItem(idx, attacker))
        {
            strategy.emplace_back(Move(GC::USE, node.getID(idx), node.getID(attacker)));
        }

        idx++;
    }
    return strategy;
}

Strategy summon(GameNode& node)
{
    Strategy strategy;
    int idx = 0;

    while ((idx = node.findReserve(idx)) != -1)
    {
        Card& reserve = node.cards[idx];
        if (node.summon(idx))
        {
            strategy.emplace_back(Move(GC::SUMMON, node.getID(idx)));
        }

        idx++;
    }

    return strategy;
}

Strategy battle(GameNode& node)
{
    Strategy strategy;
    // 召喚できるだけ召喚する

    sort(node.cards.begin(), node.cards.end(),
         [&](const Card& c1, const Card& c2)
         {
             return evaluateCreature(c1) < evaluateCreature(c2);
         });

    cerr << "summon start" << endl;
    strategy = Util::concat(strategy, summon(node));
    cerr << "greenItem start" << endl;
    strategy = Util::concat(strategy, useGreenItem(node));
    cerr << "attack start" << endl;
    strategy = Util::concat(strategy, attack(node));
    cerr << "finish" << endl;

    if (strategy.empty())
    {
        strategy.emplace_back(GC::PASS);
    }

    return strategy;
}


// -------------------->8------------------------------------------

GameNode now;
int roundNo = 0;
bool isDraftPhase = true;

bool gameUpdate()
{
    if (roundNo >= 30)
    {
        isDraftPhase = false;
    }

    now = GameNode::input();

    roundNo++;
}


int main()
{
    while (gameUpdate())
    {
        Strategy s;
        if (isDraftPhase)
        {
            s = draft(now);
        }
        else
        {
            s = battle(now);
        }

        output(s);
    }

    return 0;
}