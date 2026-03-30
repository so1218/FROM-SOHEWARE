#pragma once

namespace FE
{

class Engine;

enum class TextureType
{
    Albedo,
    Normal,
    Toon,
    Noise,
    CubeMap,
    Particle,
    UI,
    LUT,
    Unknown
};

struct TextureHandleData
{
    std::string name;       // 検索キー
    std::string fullPath;   // フルパス
    TextureType type;       // タイプ
    uint32_t handle;        // SRVインデックス
};

class TextureManager
{
public:
    static TextureManager& GetInstance()
    {
        static TextureManager instance;
        return instance;
    }

    void LoadAllTextures(Engine* engine, const std::string& rootDirectory = "Assets/Textures");
    uint32_t Get(const std::string& name);
    const TextureHandleData* GetMetaData(const std::string& name);
    const std::vector<TextureHandleData>& GetAllTextures() const { return textures_; }

    // 指定タイプのテクスチャ名前一覧を取得する関数
    std::vector<std::string> GetTextureNamesByType(TextureType type) const;

private:
    TextureManager() = default;
    ~TextureManager() = default;

    TextureType DetectTypeFromPath(const std::filesystem::path& path);

    std::vector<TextureHandleData> textures_;
    std::unordered_map<std::string, size_t> nameToIndex_;
    uint32_t errorHandle_ = 0;
};

}