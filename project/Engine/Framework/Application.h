#pragma once
#include "SceneManager.h"

namespace FS
{

class Engine;
struct ProjectConfig;

class Application
{
public:
    explicit Application(const ProjectConfig& config);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();

protected:
    Engine* GetEngine() const { return engine_.get(); }
    SceneManager& GetSceneManager() { return sceneManager_; }

private:
    void Update();
    void DebugGui();
    void Draw();

    std::unique_ptr<Engine> engine_;
    SceneManager sceneManager_;
};

}
