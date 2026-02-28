#pragma once

class GraphicsDevice;
class CommandManager;
class PSOManager;
class RootSignatureManager;
class TextureLoader;
class SRVManager;
class LightManager;
class GlobalConstants;
class MaterialManager;
class PostEffectManager; 

struct RenderEnvironment
{
    GraphicsDevice* device = nullptr;
    CommandManager* commandManager = nullptr;
    PSOManager* psoManager = nullptr;
    RootSignatureManager* rootSignatureManager = nullptr;
    TextureLoader* textureLoader = nullptr;
    SRVManager* srvManager = nullptr;
    LightManager* lightManager = nullptr;
    GlobalConstants* globalConstants = nullptr;
    MaterialManager* materialManager = nullptr;
    PostEffectManager* postEffectManager = nullptr;
};
