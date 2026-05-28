#pragma once
#include <torch/torch.h>

// ─────────────────────────────────────────────────────────────────────────────
//  ActorCritic
//  Shared backbone → Actor head (policy) + Critic head (value)
//
//  Forward returns:
//    .first  = action probabilities  (softmax, shape [batch, ACTION_DIM])
//    .second = state value estimate  (shape [batch, 1])
// ─────────────────────────────────────────────────────────────────────────────
struct ActorCriticImpl : torch::nn::Module
{
    // Hyper-params (set once at construction)
    const int STATE_DIM;
    const int ACTION_DIM;
    const int HIDDEN = 128;

    // Layers
    torch::nn::Linear fc1{ nullptr }, fc2{ nullptr };
    torch::nn::Linear actor_head{ nullptr };
    torch::nn::Linear critic_head{ nullptr };

    ActorCriticImpl(int state_dim, int action_dim)
        : STATE_DIM(state_dim), ACTION_DIM(action_dim)
    {
        fc1 = register_module("fc1", torch::nn::Linear(state_dim, HIDDEN));
        fc2 = register_module("fc2", torch::nn::Linear(HIDDEN, HIDDEN));
        actor_head = register_module("actor", torch::nn::Linear(HIDDEN, action_dim));
        critic_head = register_module("critic", torch::nn::Linear(HIDDEN, 1));

        // Orthogonal init – stabilises PPO early training
        torch::nn::init::orthogonal_(fc1->weight);
        torch::nn::init::orthogonal_(fc2->weight);
        torch::nn::init::orthogonal_(actor_head->weight, 1.0); // small for actor
        torch::nn::init::orthogonal_(critic_head->weight, 1.0);
    }

    std::pair<torch::Tensor, torch::Tensor> forward(torch::Tensor x)
    {
        x = torch::relu(fc1->forward(x));
        x = torch::relu(fc2->forward(x));
        auto probs = torch::softmax(actor_head->forward(x),  /*dim=*/-1);
        auto value = critic_head->forward(x);
        return { probs, value };
    }
};
TORCH_MODULE(ActorCritic);  // wraps impl in shared_ptr, exposes as ActorCritic