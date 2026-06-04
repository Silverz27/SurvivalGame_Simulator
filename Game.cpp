#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm>
#include <limits>

using namespace sf;

static constexpr float WORLD_W = 1920.f;
static constexpr float WORLD_H = 1080.f;

// ── Helpers ───────────────────────────────────────────────────────────────────
static float dist(Vector2f a, Vector2f b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
static Vector2f unitDir(Vector2f from, Vector2f to) {
    Vector2f d = to - from;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    return len > 0.f ? d / len : Vector2f(0.f, 0.f);
}

// ── Constructor ───────────────────────────────────────────────────────────────
Game::Game()
    : window(VideoMode({ static_cast<unsigned>(WORLD_W),
                         static_cast<unsigned>(WORLD_H) }), "Survival Game – AI Mode")
{
    window.setFramerateLimit(60);
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    bool loaded = font.openFromFile("arial.ttf");
    if (!loaded) font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    if (!loaded) font.openFromFile("/System/Library/Fonts/Helvetica.ttc");
    if (!loaded) font.openFromFile("C:/Windows/Fonts/arial.ttf");

    resetEpisode();

    if (AI_MODE)
        agent = std::make_unique<PlayerAgent>();
}

// ── resetEpisode ─────────────────────────────────────────────────────────────
void Game::resetEpisode()
{
    player = std::make_unique<Player>(Vector2f(WORLD_W / 2.f, WORLD_H / 2.f));
    enemies.clear();
    foods.clear();
    spawnEnemiesSafe();
    spawnFood(FOOD_COUNT);
    prevHunger = 100.f;
    prevFoodDist = 1.f;
    foodRespawnTimer = 0.f;
    enemyRespawnTimer = 0.f;
    episodeStep = 0; // <--- THÊM DÒNG NÀY VÀO ĐÂY (Đưa tuổi thọ về 0 khi đầu thai)
}

// ── Main loop ─────────────────────────────────────────────────────────────────
void Game::run()
{
    Clock clock;
    while (window.isOpen()) {
        float dt = std::min(clock.restart().asSeconds(), 0.05f);
        processEvents();
        update(dt);
        render();
    }
}

// ── Events ────────────────────────────────────────────────────────────────────
void Game::processEvents()
{
    while (const auto event = window.pollEvent()) {
        if (event->is<Event::Closed>()) window.close();
        if (const auto* k = event->getIf<Event::KeyPressed>()) {
            if (k->code == Keyboard::Key::Escape) window.close();
            if (k->code == Keyboard::Key::F5 && agent)
                agent->saveWeights("player_weights.pt");
            if (k->code == Keyboard::Key::F9 && agent)
                agent->loadWeights("player_weights.pt");
        }
    }
}

// ── buildState ────────────────────────────────────────────────────────────────
GameStateForAI Game::buildState()
{
    GameStateForAI s{};
    const float VIEW = PlayerAgent::VIEW_RADIUS;
    Vector2f pPos = player->getPosition();
    float selfPower = player->getPower();

    s.hp_ratio = player->getHP() / Player::MAX_HP;
    s.hunger_ratio = player->getHunger() / Player::MAX_HUNGER;
    s.power = selfPower;

    // ── Nearest food ─────────────────────────────────────────────────────────
    Vector2f nearestFoodPos;
    float bestFoodDist = VIEW;
    for (const auto& f : foods) {
        if (f.eaten) continue;
        float d = dist(pPos, f.getPosition());
        if (d < bestFoodDist) {
            bestFoodDist = d;
            nearestFoodPos = f.getPosition();
        }
    }
    if (bestFoodDist < VIEW) {
        float dx = nearestFoodPos.x - pPos.x;
        float dy = nearestFoodPos.y - pPos.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.f) { dx /= len; dy /= len; }
        s.food_in_view = true;
        s.food_dir_x = dx;
        s.food_dir_y = dy;
        s.food_dist = bestFoodDist / VIEW;
        lastKnownFoodDir.x = dx;
        lastKnownFoodDir.y = dy;
        s.last_known_food_dir_x = dx;
        s.last_known_food_dir_y = dy;
    }
    else {
        s.food_in_view = false;
        s.food_dist = 1.f;
        s.food_dir_x = 0.f;
        s.food_dir_y = 0.f;
        s.last_known_food_dir_x = lastKnownFoodDir.x;
        s.last_known_food_dir_y = lastKnownFoodDir.y;
    }

    // ── Nearest enemy + most dangerous ───────────────────────────────────────
    float nearestDist = VIEW;
    float dangerDist = VIEW;
    float dangerPower = 0.f;

    for (const auto& e : enemies) {
        if (e.isDead()) continue;
        float d = dist(pPos, e.getPosition());
        if (d > VIEW) continue;

        float ratio = (selfPower > 0.f) ? e.getPower() / selfPower : 99.f;
        auto  dir = unitDir(pPos, e.getPosition());

        if (d < nearestDist) {
            nearestDist = d;
            s.near_enemy_dist = d / VIEW;
            s.near_enemy_dir_x = dir.x;
            s.near_enemy_dir_y = dir.y;
            s.near_enemy_power_ratio = ratio;
        }
        if (e.getPower() > dangerPower && d < VIEW) {
            dangerPower = e.getPower();
            dangerDist = d;
            s.danger_dist = d / VIEW;
            s.danger_dir_x = dir.x;
            s.danger_dir_y = dir.y;
            s.danger_power_ratio = ratio;
        }
    }

    if (nearestDist >= VIEW) {
        s.near_enemy_dist = 1.f;
        s.near_enemy_power_ratio = 0.f;
    }
    if (dangerDist >= VIEW) {
        s.danger_dist = 1.f;
        s.danger_power_ratio = 0.f;
    }

    // ── Wall sensors – non-linear (sqrt) so the signal spikes sharply near walls
    // sensor = sqrt( clamp(gap / WALL_VIEW, 0, 1) )
    // → reads ~1.0 far away, drops steeply inside ~2×PLAYER_R of the wall.
    {
        constexpr float WALL_VIEW = 200.f;
        constexpr float R = Player::RADIUS;

        auto wallSensor = [&](float rawDist) -> float {
            float gap = std::max(rawDist - R, 0.001f);   // body-edge to wall
            float norm = std::min(gap / WALL_VIEW, 1.f);  // 0 = touching, 1 = far
            return std::sqrt(norm);                        // non-linear: large gradient when close
            };

        s.dist_left = wallSensor(pPos.x);
        s.dist_right = wallSensor(WORLD_W - pPos.x);
        s.dist_top = wallSensor(pPos.y);
        s.dist_bottom = wallSensor(WORLD_H - pPos.y);
    }

    return s;
}

// ── Update ────────────────────────────────────────────────────────────────────
void Game::update(float dt)
{
    RewardEvent ev{};
    bool done = false;
    episodeStep++; // <--- THÊM DÒNG NÀY VÀO ĐÂY (Cứ mỗi khung hình trôi qua là thọ thêm 1 tuổi)

    // ── AI or manual input ────────────────────────────────────────────────────
    if (AI_MODE && agent && !player->isDead()) {
        GameStateForAI state = buildState();

        ev.hunger_delta = player->getHunger() - prevHunger;
        ev.dist_to_food_delta = state.food_dist - prevFoodDist;
        prevHunger = player->getHunger();
        prevFoodDist = state.food_dist;

        bool nearDanger = state.danger_power_ratio > 1.0f && state.danger_dist < 0.6f;
        bool nearWeak = state.near_enemy_power_ratio < 1.0f && state.near_enemy_dist < 0.6f;

        if (actionRepeatCounter <= 0) {
            currentAction = agent->selectAction(state);
            actionRepeatCounter = ACTION_REPEAT;
        }
        else {
            actionRepeatCounter--;
        }

        Vector2f dir = actionToDir(currentAction);
        ev.player_pos = player->getPosition();
        ev.moved = (dir.x != 0.f || dir.y != 0.f);
        player->move(dir * Player::SPEED * dt);

        // Did agent move away from danger?
        if (nearDanger) {
            Vector2f dangerDir(state.danger_dir_x, state.danger_dir_y);
            float dot = dir.x * dangerDir.x + dir.y * dangerDir.y;
            ev.is_fleeing_correctly = (dot < -0.3f);
        }
        // Did agent move toward weak enemy?
        if (nearWeak) {
            Vector2f weakDir(state.near_enemy_dir_x, state.near_enemy_dir_y);
            float dot = dir.x * weakDir.x + dir.y * weakDir.y;
            ev.is_chasing_correctly = (dot > 0.5f);
        }
    }
    else if (!AI_MODE) {
        player->handleInput(dt);
    }

    player->update(dt);

    for (auto& e : enemies)
        e.update(dt, player->getPosition());

    checkCollisions(ev);

    done = player->isDead();
    ev.died = done;

    // ── Observe (AI step) ─────────────────────────────────────────────────────
    if (AI_MODE && agent) {
        GameStateForAI nextState = buildState();
        float reward = rewardCalc.compute(ev);
        // ==========================================
        // THUẬT TOÁN CHỐNG KẸT GÓC/VIỀN (BẢN CHUẨN V-SHAPE CŨ)
        // ==========================================
        Vector2f pos = player->getPosition();
        float margin = 80.f; // Khoảng cách 1 ô lưới sát biên

        float penalty = -0.05f;       // Phạt cực nhẹ để AI không bị trầm cảm
        float escape_reward = 0.5f;   // Thưởng vừa nếu lách khỏi viền đơn
        float super_escape = 2.0f;    // SIÊU THƯỞNG nếu biết đi chéo thoát góc chữ V!

        // Định nghĩa 8 hướng Action (từ 0 đến 7)
        bool moving_left = (currentAction == 5 || currentAction == 6 || currentAction == 7);
        bool moving_right = (currentAction == 1 || currentAction == 2 || currentAction == 3);
        bool moving_up = (currentAction == 0 || currentAction == 1 || currentAction == 7);
        bool moving_down = (currentAction == 3 || currentAction == 4 || currentAction == 5);

        // Quét xem AI có đang chạm lề không
        bool in_left_zone = (pos.x < margin);
        bool in_right_zone = (pos.x > WORLD_W - margin);
        bool in_top_zone = (pos.y < margin);
        bool in_bottom_zone = (pos.y > WORLD_H - margin);

        // 🏠 XỬ LÝ KẸT GÓC CHỮ V (Nhân đôi phạt nếu đâm bừa, thưởng siêu to nếu đi chéo)
        if (in_left_zone && in_top_zone) {
            if (currentAction == 3) reward += super_escape; // Thoát góc Trên-Trái: Đi Xuống-Phải
            else if (moving_left || moving_up) reward += penalty * 2;
        }
        else if (in_right_zone && in_top_zone) {
            if (currentAction == 5) reward += super_escape; // Thoát góc Trên-Phải: Đi Xuống-Trái
            else if (moving_right || moving_up) reward += penalty * 2;
        }
        else if (in_left_zone && in_bottom_zone) {
            if (currentAction == 1) reward += super_escape; // Thoát góc Dưới-Trái: Đi Lên-Phải
            else if (moving_left || moving_down) reward += penalty * 2;
        }
        else if (in_right_zone && in_bottom_zone) {
            if (currentAction == 7) reward += super_escape; // Thoát góc Dưới-Phải: Đi Lên-Trái
            else if (moving_right || moving_down) reward += penalty * 2;
        }
        // 🚧 XỬ LÝ KẸT VIỀN ĐƠN 
        else {
            if (in_left_zone) {
                if (moving_left) reward += penalty;
                else if (moving_right) reward += escape_reward;
            }
            else if (in_right_zone) {
                if (moving_right) reward += penalty;
                else if (moving_left) reward += escape_reward;
            }

            if (in_top_zone) {
                if (moving_up) reward += penalty;
                else if (moving_down) reward += escape_reward;
            }
            else if (in_bottom_zone) {
                if (moving_down) reward += penalty;
                else if (moving_up) reward += escape_reward;
            }
        }
        // ==========================================

        agent->observe(reward, done, nextState);
    }

    // ── Auto-reset on death ───────────────────────────────────────────────────
    if (done) resetEpisode();

    // ── Respawn food / enemies ────────────────────────────────────────────────
    foodRespawnTimer += dt;
    if (foodRespawnTimer >= FOOD_RESPAWN_INTERVAL) {
        foodRespawnTimer = 0.f;
        int alive = std::count_if(foods.begin(), foods.end(),
            [](const Food& f) { return !f.eaten; });
        if (alive < FOOD_COUNT) spawnFood(FOOD_COUNT - alive);
    }
    enemyRespawnTimer += dt;
    if (enemyRespawnTimer >= ENEMY_RESPAWN_INTERVAL) {
        enemyRespawnTimer = 0.f;
        int alive = std::count_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return !e.isDead(); });
        if (alive < ENEMY_COUNT) spawnEnemies(ENEMY_COUNT - alive);
    }

    enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
        [](const Enemy& e) { return e.isDead(); }), enemies.end());
    foods.erase(std::remove_if(foods.begin(), foods.end(),
        [](const Food& f) { return f.eaten; }), foods.end());
}

// ── Render ────────────────────────────────────────────────────────────────────
void Game::render()
{
    window.clear(Color(28, 32, 38));

    // Grid
    RectangleShape line;
    line.setFillColor(Color(45, 50, 58));
    for (int x = 0; x < (int)WORLD_W; x += 80) {
        line.setSize({ 1.f, WORLD_H }); line.setPosition({ float(x), 0 }); window.draw(line);
    }
    for (int y = 0; y < (int)WORLD_H; y += 80) {
        line.setSize({ WORLD_W, 1.f }); line.setPosition({ 0, float(y) }); window.draw(line);
    }

    // View radius ring
    if (AI_MODE) {
        CircleShape viewRing(PlayerAgent::VIEW_RADIUS);
        viewRing.setOrigin({ PlayerAgent::VIEW_RADIUS, PlayerAgent::VIEW_RADIUS });
        viewRing.setPosition(player->getPosition());
        viewRing.setFillColor(Color::Transparent);
        viewRing.setOutlineColor(Color(255, 255, 100, 40));
        viewRing.setOutlineThickness(1.f);
        window.draw(viewRing);
    }

    for (auto& f : foods)   window.draw(f.shape);
    for (auto& e : enemies) e.draw(window, font);
    player->draw(window);

    drawUI();
    if (AI_MODE && agent) drawAIOverlay();

    window.display();
}

// ── Collisions ────────────────────────────────────────────────────────────────
void Game::checkCollisions(RewardEvent& ev)
{
    Vector2f pPos = player->getPosition();

    for (auto& f : foods) {
        if (f.eaten) continue;
        if (dist(pPos, f.getPosition()) < Player::RADIUS + Food::RADIUS) {
            f.eaten = true;
            player->eatFood(Food::NUTRITION);
            player->gainPower(2.f);
            ev.ate_food = true;
        }
    }

    for (auto& e : enemies) {
        if (e.isDead()) continue;
        if (dist(pPos, e.getPosition()) < Player::RADIUS + Enemy::RADIUS) {
            if (player->getPower() >= e.getPower()) {
                ev.killed_enemy = true;
                ev.killed_power = e.getPower();
                player->gainPower(e.getPower() * 0.3f);
                e.kill();
            }
            else {
                ev.took_damage = true;
                ev.damage_taken += e.getPower() * 0.05f;
                ev.attacked_stronger = true;
                player->takeDamage(e.getPower() * 0.05f);
            }
        }
    }
}

// ── HUD ───────────────────────────────────────────────────────────────────────
void Game::drawUI()
{
    auto label = [&](const std::string& t, float x, float y, Color c, unsigned sz = 18) {
        Text tx(font, t, sz);
        tx.setFillColor(c);
        tx.setPosition({ x, y });
        window.draw(tx);
        };

    label("HP:     " + std::to_string((int)player->getHP()), 20, 20, Color(220, 80, 80));
    label("Hunger: " + std::to_string((int)player->getHunger()), 20, 48, Color(240, 180, 40));
    label("Power:  " + std::to_string((int)player->getPower()), 20, 76, Color(80, 200, 120));
    label("Enemies:" + std::to_string(enemies.size()), 20, 110, Color(180, 180, 180));
    label("F5=Save  F9=Load  ESC=Quit", 20, WORLD_H - 30, Color(100, 100, 100), 15);
}

// ── AI Overlay (top-right panel) ──────────────────────────────────────────────
void Game::drawAIOverlay()
{
    if (!agent) return;
    float x = WORLD_W - 320.f, y = 20.f;

    auto label = [&](const std::string& t, float dy, Color c = Color(200, 200, 200), unsigned sz = 16) {
        Text tx(font, t, sz);
        tx.setFillColor(c);
        tx.setPosition({ x, y + dy });
        window.draw(tx);
        };

    RectangleShape panel({ 300.f, 140.f });
    panel.setPosition({ x - 8.f, y - 8.f });
    panel.setFillColor(Color(20, 25, 30, 200));
    panel.setOutlineColor(Color(80, 80, 100));
    panel.setOutlineThickness(1.f);
    window.draw(panel);

    label("── Player AI (PPO) ──", 0, Color(140, 180, 255), 16);
    label("Episode : " + std::to_string(agent->getEpisode()), 24, Color(200, 200, 200));
    label("Steps   : " + std::to_string(episodeStep), 46, Color(200, 200, 200));
    label("Avg Ret : " + std::to_string(agent->getAvgReward()).substr(0, 6), 68, Color(100, 240, 140));
    label("Loss    : " + std::to_string(agent->getLastLoss()).substr(0, 6), 90, Color(240, 160, 80));

    static const char* actionNames[8] = { "↑","↗","→","↘","↓","↙","←","↖" };
    label(std::string("Action  : ") + actionNames[currentAction], 112, Color(255, 220, 80));
}

// ── Spawn ─────────────────────────────────────────────────────────────────────
void Game::spawnFood(int count) {
    int cols = 4;
    int rows = 2;
    int totalZones = cols * rows;
    float zoneW = WORLD_W / cols;
    float zoneH = WORLD_H / rows;
    float margin = 40.f;

    // BƯỚC 1: ĐIỀU TRA DÂN SỐ - Đếm xem mỗi vùng đang còn bao nhiêu thức ăn
    std::vector<int> foodPerZone(totalZones, 0);
    for (const auto& f : foods) {
        if (f.eaten) continue;
        Vector2f p = f.getPosition();
        // Ép tọa độ X, Y về số thứ tự Cột và Hàng
        int c = std::min(static_cast<int>(p.x / zoneW), cols - 1);
        int r = std::min(static_cast<int>(p.y / zoneH), rows - 1);
        foodPerZone[r * cols + c]++;
    }

    // BƯỚC 2: CỨU TRỢ CHÍNH XÁC - Vùng nào thiếu nhất thì bù vào vùng đó
    for (int i = 0; i < count; ++i) {
        // Tìm vùng có ít thức ăn nhất hiện tại
        int minZone = 0;
        for (int z = 1; z < totalZones; ++z) {
            if (foodPerZone[z] < foodPerZone[minZone]) {
                minZone = z;
            }
        }

        // Tính tọa độ đất của vùng nghèo nhất (minZone)
        int c = minZone % cols;
        int r = minZone / cols;
        float minX = c * zoneW + margin;
        float maxX = (c + 1) * zoneW - margin;
        float minY = r * zoneH + margin;
        float maxY = (r + 1) * zoneH - margin;

        // Sinh 1 hạt thức ăn vứt vào đúng vùng đó
        float x = minX + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (maxX - minX)));
        float y = minY + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (maxY - minY)));

        foods.emplace_back(Vector2f(x, y));

        // Cộng 1 vào vùng đó để vòng lặp sau (nếu có) nó đi tìm vùng khác
        foodPerZone[minZone]++;
    }
}
void Game::spawnEnemies(int count) {
    for (int i = 0; i < count; ++i) {
        float p = 5.f + float(std::rand() % 26);
        enemies.emplace_back(randomPos(80.f), p);
    }
}

void Game::spawnEnemiesSafe()
{
    const float SAFE_RADIUS = 300.f;
    Vector2f center = { WORLD_W / 2.f, WORLD_H / 2.f };

    int spawned = 0;
    int attempts = 0;
    while (spawned < ENEMY_COUNT && attempts < 1000) {
        ++attempts;
        Vector2f pos = randomPos(80.f);
        float d = std::sqrt(
            (pos.x - center.x) * (pos.x - center.x) +
            (pos.y - center.y) * (pos.y - center.y)
        );
        if (d < SAFE_RADIUS) continue;
        float p = 5.f + float(std::rand() % 50);
        enemies.emplace_back(pos, p);
        ++spawned;
    }
}

Vector2f Game::randomPos(float margin) const {
    float x = margin + float(std::rand() % int(WORLD_W - margin * 2));
    float y = margin + float(std::rand() % int(WORLD_H - margin * 2));
    return { x, y };
}