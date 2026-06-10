#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "Player.h"
#include "Enemy.h"
#include "Food.h"
#include "PlayerAgent.h"
#include "RewardCalculator.h"

// ── Mode toggle ───────────────────────────────────────────────────────────────
//  AI_MODE  = true  → PlayerAgent drives the player (AI learning)
//  AI_MODE  = false → WASD manual control (for testing)
static constexpr bool AI_MODE = true;

class Game {
public:
    Game();
    void run();

private:
    sf::RenderWindow window;
    sf::Font         font;

    std::unique_ptr<Player>      player;
    std::unique_ptr<PlayerAgent> agent;
    RewardCalculator             rewardCalc;

    std::vector<Enemy> enemies;
    std::vector<Food>  foods;

    // Respawn
    float foodRespawnTimer = 0.f;
    float enemyRespawnTimer = 0.f;
    static constexpr float FOOD_RESPAWN_INTERVAL = 3.f;
    static constexpr float ENEMY_RESPAWN_INTERVAL = 8.f;
    static constexpr int   FOOD_COUNT = 25;
    static constexpr int   ENEMY_COUNT = 10;
    int   ENEMY_POWER_BUFF = 0;

    // Per-step AI bookkeeping
    GameStateForAI buildState();
    RewardEvent    lastRewardEvent;
    float          prevHunger = 100.f;
    float          prevFoodDist = 1.f;
    int            currentAction = 0;

    int actionRepeatCounter = 0;
    // Thêm biến này vào để đếm tuổi thọ của mỗi lần sống:
    int episodeStep = 0;


    static constexpr int ACTION_REPEAT = 5;

    // Episode auto-reset
    void resetEpisode();

    void processEvents();
    void update(float dt);
    void render();

    void spawnFood(int count);
    void spawnEnemies(int count);
    void spawnEnemiesSafe();
    void checkCollisions(RewardEvent& ev);
    void drawUI();
    void drawAIOverlay();   // shows neural net stats, reward curve

    sf::Vector2f randomPos(float margin = 40.f) const;

    sf::Vector2f lastKnownFoodDir = { 0.0f, 0.0f };
};