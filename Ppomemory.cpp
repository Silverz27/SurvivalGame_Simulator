#include "PPOMemory.h"
#include <algorithm>
#include <numeric>
#include <random>
#include <cmath>

// ── GAE (Generalized Advantage Estimation) ───────────────────────────────────
void PPOMemory::computeAdvantages(float last_value)
{
    int N = static_cast<int>(buffer.size());
    advantages.resize(N);
    returns_.resize(N);

    float gae = 0.f;
    float next_val = last_value;

    for (int t = N - 1; t >= 0; --t) {
        const auto& tr = buffer[t];
        float mask = tr.done ? 0.f : 1.f;
        float delta = tr.reward + GAMMA * next_val * mask - tr.value;
        gae = delta + GAMMA * LAMBDA * mask * gae;
        advantages[t] = gae;
        returns_[t] = gae + tr.value;
        next_val = tr.value;
    }

    // Normalise advantages → stabilises PPO
    float mean = 0.f, var = 0.f;
    for (float a : advantages) mean += a;
    mean /= N;
    for (float a : advantages) var += (a - mean) * (a - mean);
    var /= N;
    float std = std::sqrt(var + 1e-8f);
    for (float& a : advantages) a = (a - mean) / std;
}

// ── Mini-batch creation ───────────────────────────────────────────────────────
std::vector<PPOMemory::Batch> PPOMemory::makeBatches(int state_dim) const
{
    int N = static_cast<int>(buffer.size());

    // Shuffle indices
    std::vector<int> idx(N);
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), std::mt19937{ std::random_device{}() });

    std::vector<Batch> batches;

    for (int start = 0; start < N; start += MINI_BATCH) {
        int end = std::min(start + MINI_BATCH, N);
        int B = end - start;

        // Pre-allocate raw tensors
        auto states_t = torch::zeros({ B, state_dim });
        auto actions_t = torch::zeros({ B }, torch::kInt64);
        auto logprobs_t = torch::zeros({ B });
        auto advs_t = torch::zeros({ B });
        auto returns_t = torch::zeros({ B });

        auto s_acc = states_t.accessor<float, 2>();
        auto a_acc = actions_t.accessor<int64_t, 1>();
        auto lp_acc = logprobs_t.accessor<float, 1>();
        auto adv_acc = advs_t.accessor<float, 1>();
        auto ret_acc = returns_t.accessor<float, 1>();

        for (int i = 0; i < B; ++i) {
            int id = idx[start + i];
            const auto& tr = buffer[id];
            for (int j = 0; j < state_dim; ++j)
                s_acc[i][j] = tr.state[j];
            a_acc[i] = tr.action;
            lp_acc[i] = tr.log_prob;
            adv_acc[i] = advantages[id];
            ret_acc[i] = returns_[id];
        }

        batches.push_back({ states_t, actions_t, logprobs_t, advs_t, returns_t });
    }
    return batches;
}