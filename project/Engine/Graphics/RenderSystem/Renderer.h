#pragma once

//class Renderer
//{
//public:
//    void Initialize(GraphicsDevice* device, CommandManager* commandManager, /* ... */);
//    void Finalize();
//
//    void BeginFrame();
//    void ExecuteDrawCommands(); // 実際に描画命令を積む
//    void EndFrame();
//
//    void ResetDrawCounters();
//
//    // 描画リクエストを受け付けるメソッド (旧Drawメソッド)
//    void SubmitTriangle(WorldTransform& worldTransform, uint32_t color, ...);
//    void SubmitSphere(WorldTransform& worldTransform, Camera& camera, ...);
//    void SubmitModel(WorldTransform& worldTransform, Camera& camera, ModelData& modelData, ...);
//    // ...
//
//private:
//    // ここにEngineから移動してきたメンバー変数を配置する
//    GraphicsDevice* device_ = nullptr;
//    CommandManager* commandManager_ = nullptr;
//    // ...
//
//    uint32_t indexSphere_ = 0;
//    std::vector<RenderData> spheres_;
//    // ...
//};