#include "AnimationData.h"
#include "TimeManager.h"

#include <assimp/Importer.hpp>  
#include <assimp/scene.h>    
#include <cassert>

Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time)
{
	assert(!keyframes.empty()); // キーが無いものは返す値がわからないのでダメ
	if (keyframes.size() == 1 || time <= keyframes[0].time)// キーが1つか、時刻がキーフレーム前なら最初の値とする
	{
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;
		// indexとnextIndexの2つのkeyframeを取得して範囲内に時刻があるかを判定
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
		{
			// 範囲内を保管する
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	// ここまできた場合は一番後の時刻よりも後ろなので最後の値を返すことにする
	return (*keyframes.rbegin()).value;
}

Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time)
{
	assert(!keyframes.empty());
	if (keyframes.size() == 1 || time <= keyframes[0].time)
	{
		return keyframes[0].value;
	}

	for (size_t index = 0; index < keyframes.size() - 1; ++index)
	{
		size_t nextIndex = index + 1;
		if (keyframes[index].time <= time && time <= keyframes[nextIndex].time)
		{
			float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
			return Quaternion::Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
		}
	}

	return keyframes.back().value;
}


int32_t CreateJoint(const Node& node, std::optional<int32_t> parentIndex, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = Matrix4x4::MakeIdentity();
	joint.transform = node.transform;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parentIndex;
	joints.push_back(joint);
	// 子のJointのインデックスを取得
	for (const Node& childNode : node.children)
	{
		int32_t childIndex = CreateJoint(childNode, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}
	return joint.index;
}


Skeleton CreateSkeleton(const Node& rootNode)
{
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	// 名前とindexのマッピングを行いアクセスしやすくする
	for (const Joint& joint : skeleton.joints)
	{
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	return skeleton;
}

void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime)
{
	for (Joint& joint : skeleton.joints)
	{
		// 対象のJointのAnimationがあれば、値の適用を行う。下記のif文はC++17から可能になった初期化付きif文
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end())
		{
			const NodeAnimation& nodeAnimation = (*it).second;
		
			joint.transform.translation_ = CalculateValue(nodeAnimation.translate.keyframes, animationTime);
			joint.transform.rotationQuaternion_ = CalculateValue(nodeAnimation.rotate.keyframes, animationTime);
			joint.transform.scale_ = CalculateValue(nodeAnimation.scale.keyframes, animationTime);
		}
	}
}

void Update(Skeleton& skeleton)
{
	// 全てのJointを更新。親が若いので通常ループで処理可能になっている
	for (Joint& joint : skeleton.joints)
	{
		joint.localMatrix = Matrix4x4::MakeAffine(joint.transform.scale_, joint.transform.rotationQuaternion_, joint.transform.translation_);
		if (joint.parent)
		{
			joint.skeletonSpaceMatrix =  joint.localMatrix * skeleton.joints[*(joint.parent)].skeletonSpaceMatrix;
		}
		else
		{
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}