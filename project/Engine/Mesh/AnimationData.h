#pragma once
#include "Quaternion.h"
#include "Structures.h"
#include "WorldTransform.h"

#include <vector>    
#include <map>       
#include <string>
#include <optional>

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
    // NOdeAnimationの集合。Node名で引けるようにしておく
    std::map<std::string, NodeAnimation> nodeAnimations;
};

struct AnimatedModelData
{
    ModelData modelData; 
    Animation animation; 

    float animationTime = 0.0f;
    Matrix4x4 localMatrix;  // 現在のアニメーション変換行列

    // rootNodeの名前もここで管理
    std::string rootNodeName;
};

struct Node
{
    WorldTransform transform;
	Matrix4x4 localMatrix; 
    std::string name;
	std::vector<Node> children;
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

//Skeleton CreateSkeleton(const Node& rootNode);

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


