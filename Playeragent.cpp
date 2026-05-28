#include "PlayerAgent.h"
#include <iostream>
#include <cmath>

// ── Constructor ───────────────────────────────────────────────────────────────
PlayerAgent::PlayerAgent()
    : net(GameStateForAI::DIM, 8)
    , optimizer(net->parameters(), torch::optim::AdamOptions(LR))
{
    net->train();
}

// ── selectAction ─────────────────────────────────────────────────────────────
//  1. Build state tensor
//  2. Forward pass → probabilities + value
//  3. Sample action from categorical distribution
//  4. Cache transition data (reward comes later via observe())
int PlayerAgent::selectAction(const GameStateForAI& state)
{
    lastState = state;
    auto sv = state.toVector();

    torch::NoGradGuard no_grad;
    auto state_t = torch::tensor(sv).unsqueeze(0); // [1, STATE_DIM]
    auto [probs, value] = net->forward(state_t);

    // Sample from categorical
    auto action_t = torch::multinomial(probs, 1);
    int  action = action_t.item<int>();
    float log_prob = torch::log(probs[0][action] + 1e-8f).item<float>();

    lastAction = action;
    lastLogProb = log_prob;
    lastValue = value[0][0].item<float>();

    return action;
}

// ── observe ───────────────────────────────────────────────────────────────────
//  Called after the game applies the action and computes the reward.
//  Pushes the complete transition to memory; triggers PPO when buffer full.
void PlayerAgent::observe(float reward, bool done, const GameStateForAI& nextState)
{
    ++totalSteps;
    episodeRet += reward;

    Transition tr;
    tr.state = lastState.toVector();
    tr.action = lastAction;
    tr.log_prob = lastLogProb;
    tr.reward = reward;
    tr.value = lastValue;
    tr.done = done;
    reward = std::clamp(reward, -10.0f, 10.0f);
    memory.push(tr);

    if (done) {
        // Exponential moving average of episode return
        avgReward = 0.95f * avgReward + 0.05f * episodeRet;
        episodeRet = 0.f;
        ++episode;
    }

    if (memory.ready()) {
        // Bootstrap value for the last state
        float bootstrap = 0.f;
        if (!done) {
            torch::NoGradGuard ng;
            auto sv = nextState.toVector();
            auto state_t = torch::tensor(sv).unsqueeze(0);
            auto [_, v] = net->forward(state_t);
            bootstrap = v[0][0].item<float>();
        }
        memory.computeAdvantages(bootstrap);
        ppoUpdate();
        memory.clear();
    }
}

// ── ppoUpdate ─────────────────────────────────────────────────────────────────
void PlayerAgent::ppoUpdate()
{
    net->train();
    float totalLoss = 0.f;
    int   updates = 0;

    for (int epoch = 0; epoch < PPOMemory::EPOCHS; ++epoch) {
        auto batches = memory.makeBatches(GameStateForAI::DIM);
        for (auto& batch : batches) {
            // Forward pass
            auto [probs, values] = net->forward(batch.states);  // [B,8], [B,1]

            // Log probs of chosen actions
            auto log_probs = torch::log(
                probs.gather(1, batch.actions.unsqueeze(1)).squeeze(1) + 1e-8f
            );

            // ── PPO Clip loss ──────────────────────────────────────────────
            auto ratio = torch::exp(log_probs - batch.log_probs);
            auto clip = torch::clamp(ratio, 1.f - CLIP_EPS, 1.f + CLIP_EPS);
            auto policy_loss = -torch::min(ratio * batch.advantages,
                clip * batch.advantages).mean();

            // ── Value loss (MSE, clipped) ──────────────────────────────────
            auto value_loss = torch::mse_loss(values.squeeze(1), batch.returns);

            // ── Entropy bonus (encourages exploration) ─────────────────────
            auto entropy = -(probs * torch::log(probs + 1e-8f)).sum(-1).mean();

            auto loss = policy_loss
                + VALUE_COEF * value_loss
                - ENTROPY_COEF * entropy;

            optimizer.zero_grad();
            loss.backward();
            torch::nn::utils::clip_grad_norm_(net->parameters(), MAX_GRAD);
            optimizer.step();

            totalLoss += loss.item<float>();
            ++updates;
        }
    }

    lastLoss = updates > 0 ? totalLoss / updates : 0.f;
    std::cout << "[PPO] Episode=" << episode
        << "  Steps=" << totalSteps
        << "  AvgRet=" << avgReward
        << "  Loss=" << lastLoss << "\n";
}

// ── Save / Load ───────────────────────────────────────────────────────────────
void PlayerAgent::saveWeights(const std::string& path) const
{
    torch::save(net, path);
    std::cout << "[Agent] Saved weights → " << path << "\n";
}

void PlayerAgent::loadWeights(const std::string& path)
{
    torch::load(net, path);
    std::cout << "[Agent] Loaded weights ← " << path << "\n";
}