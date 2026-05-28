#pragma once
#include <torch/torch.h>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  PPOMemory
//  Stores one rollout of N steps, then computes GAE advantages and
//  returns mini-batches for the PPO update.
// ─────────────────────────────────────────────────────────────────────────────
struct Transition {
    std::vector<float> state;
    int                action;
    float              log_prob;   // log π_old(a|s)
    float              reward;
    float              value;      // V(s) from critic at collection time
    bool               done;
};

class PPOMemory
{
public:
    // ── Config ────────────────────────────────────────────────────────────────
    static constexpr int   ROLLOUT_LEN = 512;   // steps before each update
    static constexpr int   MINI_BATCH = 64;
    static constexpr int   EPOCHS = 4;
    static constexpr float GAMMA = 0.99f;
    static constexpr float LAMBDA = 0.95f;  // GAE lambda

    void push(const Transition& t) { buffer.push_back(t); }
    bool ready()  const { return static_cast<int>(buffer.size()) >= ROLLOUT_LEN; }
    void clear() { buffer.clear(); }
    int  size()   const { return static_cast<int>(buffer.size()); }

    // Call after rollout is full.
    // last_value: V(s_T) – bootstrap for non-terminal last state.
    void computeAdvantages(float last_value);

    // Returns tensors for one mini-batch (randomly sampled).
    struct Batch {
        torch::Tensor states;      // [B, STATE_DIM]
        torch::Tensor actions;     // [B]  int64
        torch::Tensor log_probs;   // [B]
        torch::Tensor advantages;  // [B]
        torch::Tensor returns;     // [B]  = advantage + value
    };
    std::vector<Batch> makeBatches(int state_dim) const;

private:
    std::vector<Transition> buffer;
    std::vector<float>      advantages;
    std::vector<float>      returns_;
};