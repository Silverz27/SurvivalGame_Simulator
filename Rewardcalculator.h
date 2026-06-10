#pragma once
#include <SFML/Graphics.hpp>

// ─────────────────────────────────────────────────────────────────────────────
//  RewardCalculator
//
//  Stateless helper – Game.cpp calls compute() once per step and passes the
//  result to PlayerAgent::observe().
//
//  Design principles:
//   • Dense rewards (small signal every frame) so the agent gets frequent
//     feedback – sparse reward (only on death/kill) = too slow to learn.
//   • Shaped to guide 3 behaviours:
//       1. Find food when hungry
//       2. Kill enemies weaker than self
//       3. Flee enemies stronger than self
// ─────────────────────────────────────────────────────────────────────────────

struct RewardEvent {
    bool ate_food = false;
    bool killed_enemy = false;
    float killed_power = 0.f;   // power of the enemy that was killed
    bool took_damage = false;
    float damage_taken = 0.f;
    bool died = false;
    bool attacked_stronger = false; // attacked enemy with power > self

    // Per-frame delta stats (pass current − previous values)
    float hunger_delta = 0.f;  // positive = hunger went up (ate), negative = starving
    float dist_to_food_delta = 0.f; // negative = moved closer to food (good)

    bool is_fleeing_correctly = false;  // moved away from danger enemy
    bool is_chasing_correctly = false;  // moved toward weak enemy

    sf::Vector2f player_pos = { 960.f, 540.f };
    bool moved = false;

};

class RewardCalculator
{
public:
    float compute(const RewardEvent& ev) const
    {
        float r = 0.f;

        // ── Survival reward (tiny positive each step to bias toward staying alive)
        r += 0.01f;

        // ── Food
        if (ev.ate_food)         r += 8.0f;
        // Shaping: reward getting closer to food when hungry (hunger_ratio < 0.5)
        /*r += -0.02f * ev.dist_to_food_delta;*/ // closer = negative delta = positive r

        float maxDist = 1101.f; // sqrt(960^2 + 540^2)
        float normDist = std::sqrt(
            (ev.player_pos.x - 960.f) * (ev.player_pos.x - 960.f) +
            (ev.player_pos.y - 540.f) * (ev.player_pos.y - 540.f)
        ) / maxDist;
        r -= 0.1f * normDist; // tối đa -0.1/frame thay vì -2.2 lần trước

        // ── Combat
        if (ev.killed_enemy)     r += 5.0f;
        if (ev.attacked_stronger)r -= 3.0f;

        // ── Damage / death
        if (ev.took_damage)      r -= ev.damage_taken * 0.1f;
        if (ev.died)             r -= 10.0f;

        // ── Behavioural shaping
        if (ev.is_fleeing_correctly)  r += 0.03f;
        if (ev.is_chasing_correctly)  r += 0.03f;

        // ── Starvation penalty (hunger draining toward 0)
        if (ev.hunger_delta < 0.f) r += ev.hunger_delta * 0.02f; // small negative

        return r;
    }
};