#pragma once
#include "Quaternion.h"
#include "Structures.h"
#include "WorldTransform.h"
#include "SRVAllocator.h"

#include <vector>    
#include <map>       
#include <string>
#include <optional>
#include <array>

static uint32_t constexpr kNumMaxInfluence = 4; 

template<typename tValue>
struct Keyframe
{
    float time;
    tValue value;
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe <Quaternion>;

template<typename tValue>
struct AnimationCurve
{
    std::vector< Keyframe <tValue>> keyframes;
};

struct NodeAnimation
{
    AnimationCurve<Vector3> translate;
    AnimationCurve<Quaternion> rotate;
    AnimationCurve<Vector3> scale;
};

struct Animation
{
    float duration; // アニメーション全体の尺(単位は秒)
    // NodeAnimationの集合。Node名で引けるようにしておく
    std::map<std::string, NodeAnimation> nodeAnimations;
    // rootNodeの名前
    std::string rootNodeName;
};

struct AnimatedModelData
{
    ModelData modelData; 
    Animation animation; 

    float animationTime = 0.0f;
    Matrix4x4 localMatrix;  // 現在のアニメーション変換行列

};

struct Joint
{
    WorldTransform transform;
    Matrix4x4 localMatrix;
    Matrix4x4 skeletonSpaceMatrix; // skeletonSpaceでの変換行列
    std::string name;
	std::vector<int32_t> children; // 子Jointのインデックスのリスト。いなければ空
    int32_t index; // 自身のインデックス
    std::optional<int32_t> parent; // 親Jointのインデックス。いなければnull
};

struct Skeleton
{
    int32_t root; // RootJointのIndex
    std::map<std::string, int32_t> jointMap; // Joint名とIndexとの辞書
    std::vector<Joint> joints; // 所属しているジョイント
};

struct VertexInfluence
{
    std::array<float, kNumMaxInfluence> weights;
	std::array<int32_t, kNumMaxInfluence> jointIndices; // JointのIndex
};

struct WellForGPU
{
    Matrix4x4 skeletonSpaceMatrix; // 位置用
	Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線用 
};

struct SkinCluster
{
    std::vector<Matrix4x4> inverseBindPoseMatrices;
    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView;
    std::span<VertexInfluence> mappedInfluence;
    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    std::span<WellForGPU> mappedPalette;
    std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> paletteSrvHandle;
};

Skeleton CreateSkeleton(const Node& rootNode);

inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
{
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
}

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);
void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);
void UpdateSkeleton(Skeleton& skeleton);
SkinCluster CreateSkinCluster(
    const Microsoft::WRL::ComPtr<ID3D12Device>& device,
    const Skeleton& skeleton,
    const ModelData& modelData,
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize,
    SRVAllocator* srvAllocator);
void UpdateSkinCluster(SkinCluster& skinCluster, const Skeleton& skeleton);