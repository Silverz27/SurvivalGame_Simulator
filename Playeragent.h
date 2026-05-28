#pragma once
#include <torch/torch.h>
#include <vector>
#include <string>
#include <SFML/Graphics.hpp>
#include "NeuralNet.h"
#include "PPOMemory.h"

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerAgent
//
//  Owns the ActorCritic network + PPO training loop.
//  Game code calls:
//    int  act(state)        → chooses action, stores transition internally
//    void observe(r, done)  → records reward; triggers PPO update when ready
//    void save/load(path)   → persist weights
//
//  Actions (8 discrete directions):
//    0=N  1=NE  2=E  3=SE  4=S  5=SW  6=W  7=NW
// ─────────────────────────────────────────────────────────────────────────────

struct GameStateForAI {
    // ── Self ──────────────────────────────────────────────────────────────────
    float hp_ratio;        // hp / MAX_HP
    float hunger_ratio;    // hunger / MAX_HUNGER
    float power;           // raw power (will be normalised inside agent)

    // ── Nearest food (within VIEW_RADIUS) ─────────────────────────────────────
    float food_dist;       // normalised 0-1  (1 = not found / edge of view)
    float food_dir_x;      // unit vector components (-1..1)
    float food_dir_y;

    // ── Nearest enemy (any power) ─────────────────────────────────────────────
    float near_enemy_dist;
    float near_enemy_dir_x;
    float near_enemy_dir_y;
    float near_enemy_power_ratio; // enemy_power / self_power  (>1 = dangerous)

    // ── Most dangerous enemy in view ──────────────────────────────────────────
    float danger_dist;
    float danger_dir_x;
    float danger_dir_y;
    float danger_power_ratio;

    float last_known_food_dir_x = 0.f;
    float last_known_food_dir_y = 0.f;
    bool  food_in_view = false;


    std::vector<float> toVector() const {
        return {
            hp_ratio, hunger_ratio, power / 100.f,
            food_dist, food_dir_x, food_dir_y,
            food_in_view ? 1.f : 0.f,           
            last_known_food_dir_x,               
            last_known_food_dir_y,
            near_enemy_dist, near_enemy_dir_x, near_enemy_dir_y, near_enemy_power_ratio,
            danger_dist,     danger_dir_x,     danger_dir_y,     danger_power_ratio
        };
    }
    static constexpr int DIM = 17;
};

// Direction vectors for each action index
inline sf::Vector2f actionToDir(int action) {
    static const float D = 0.7071f; // 1/sqrt(2)
    static const sf::Vector2f dirs[8] = {
        {  0, -1}, {  D, -D}, {  1,  0}, {  D,  D},
        {  0,  1}, { -D,  D}, { -1,  0}, { -D, -D}
    };
    return dirs[action];
}

class PlayerAgent
{
public:
    // ── PPO hyper-params ──────────────────────────────────────────────────────
    static constexpr float LR = 1e-4f;
    static constexpr float CLIP_EPS = 0.2f;
    static constexpr float VALUE_COEF = 0.25f;
    static constexpr float ENTROPY_COEF = 0.01f;
    static constexpr float MAX_GRAD = 0.5f;
    static constexpr float VIEW_RADIUS = 300.f;

    PlayerAgent();

    // Main interface called each game step
    int  selectAction(const GameStateForAI& state);   // returns action 0-7
    void observe(float reward, bool done, const GameStateForAI& nextState);

    // Stats for HUD display
    int   getEpisode()      const { return episode; }
    float getAvgReward()    const { return avgReward; }
    float getLastLoss()     const { return lastLoss; }
    int   getTotalSteps()   const { return totalSteps; }

    void saveWeights(const std::string& path) const;
    void loadWeights(const std::string& path);

private:
    ActorCritic            net;
    torch::optim::Adam     optimizer;
    PPOMemory              memory;

    // Last transition (filled by selectAction, completed by observe)
    GameStateForAI         lastState;
    int                    lastAction = 0;
    float                  lastLogProb = 0.f;
    float                  lastValue = 0.f;

    // Stats
    int   episode = 0;
    int   totalSteps = 0;
    float episodeRet = 0.f;
    float avgReward = 0.f;
    float lastLoss = 0.f;

    void ppoUpdate();
};