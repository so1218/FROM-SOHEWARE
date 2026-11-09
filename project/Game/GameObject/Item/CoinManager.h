#pragma once
#include "GameObject.h"
#include "Model.h"
#include "PropertyBinder.h"

namespace FS { class Terrain; }
class Player;

class CoinManager : public FS::GameObject
{
public:
    CoinManager(FS::Engine* engine, Player* player, const FS::Terrain* terrain);
    ~CoinManager() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void DebugGui() override;

    void SpawnBurst(const FS::Vector3& position);

private:
    enum class State
    {
        Waiting,     
        Flying,     
        Resting, 
        Attracted,  
    };

    struct Coin
    {
        std::unique_ptr<FS::Model> model;
        State state = State::Waiting;
        FS::Vector3 position;
        FS::Vector3 velocity;
        float spawnDelay = 0.0f;
        float elapsedTime = 0.0f;  
        float spinAngle = 0.0f;
        float attractSpeed = 0.0f;  
        int burstId = 0;           
        bool isCollected = false;
    };

    struct Config
    {
        int coinsPerBurst = 10;
        int valuePerCoin = 3;
        float spawnDuration = 0.2f;    
        float ejectSpeed = 6.0f;       
        float spreadSpeed = 3.0f;      
        float gravity = 25.0f;
        float bounce = 0.4f;           
        float restSpeed = 1.5f;        
        float spinSpeed = 3.0f;        
        float tiltAngle = 0.35f;       // 面が光を受けるよう、少し傾けて回す (ラジアン)
        float magnetDelay = 0.3f;      
        float magnetRadius = 4.0f;
        float magnetAcceleration = 60.0f;
        float magnetMaxSpeed = 30.0f;
        float collectRadius = 0.8f;
        float lifeTime = 30.0f;
        float blinkTime = 3.0f;        
        FS::Vector3 scale = { 0.35f, 0.06f, 0.35f };

        // 散らばった 1 回分ごとに点光源を 1 つ置く
        FS::Vector4 lightColor = { 1.0f, 0.8f, 0.3f, 1.0f };
        float lightIntensity = 5.0f;
        float lightRadius = 6.0f;
        float lightVolumetric = 1.0f;
        float lightHeight = 0.5f;
        int maxLitBursts = 3;         
    };

    // プレイヤーの当たり判定の箱
    struct PlayerArea
    {
        FS::Vector3 center;
        FS::Vector3 halfSize;
    };

    void UpdateCoin(Coin& coin, float deltaTime, const PlayerArea& playerArea);
    bool IsInsidePlayer(const FS::Vector3& position, const PlayerArea& playerArea) const;
    void UpdateFlying(Coin& coin, float deltaTime);
    void Collect(Coin& coin);
    void SubmitBurstLights(const FS::Vector3& playerCenter);
    bool IsHiddenByBlink(const Coin& coin) const;

    FS::Engine* engine_ = nullptr;
    Player* player_ = nullptr;
    const FS::Terrain* terrain_ = nullptr;

    // マテリアルの共有元
    std::unique_ptr<FS::Model> sourceModel_;
    std::unique_ptr<FS::PropertyBinder> binder_;
    Config config_;

    std::vector<Coin> coins_;
    int nextBurstId_ = 0;
    float elapsedTime_ = 0.0f;
    std::mt19937 random_;
};
