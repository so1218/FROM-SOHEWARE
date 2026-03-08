#include "pch.h"
#include "AnimationData.h"
#include "TimeManager.h"
#include "BufferManager.h"
#include "MathUtils.h"

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
			return Math::Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
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
		// 対象のJointのAnimationがあれば、値の適用を行う
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end())
		{
			const NodeAnimation& nodeAnimation = (*it).second;

			joint.transform.translation_ = CalculateValue(nodeAnimation.translate.keyframes, animationTime);
			joint.transform.rotationQuaternion_ = CalculateValue(nodeAnimation.rotate.keyframes, animationTime);
			joint.transform.scale_ = CalculateValue(nodeAnimation.scale.keyframes, animationTime);
		}
	}
}

void UpdateSkeleton(Skeleton& skeleton)
{
	// 全てのJointを更新。親が若いので通常ループで処理可能になっている
	for (Joint& joint : skeleton.joints)
	{
		joint.localMatrix = Matrix4x4::MakeAffine(joint.transform.scale_, joint.transform.rotationQuaternion_, joint.transform.translation_);
		if (joint.parent)
		{
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton.joints[*(joint.parent)].skeletonSpaceMatrix;
		}
		else
		{
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

SkinCluster CreateSkinCluster(
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const Skeleton& skeleton,
	const ModelData& modelData,
	SRVManager* srvManager)
{
	// palette用のResourceを確保
	SkinCluster skinCluster;
	skinCluster.paletteResource = BufferManager::CreateBufferResource(
		device.Get(),
		sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size() };

	// palette用のSRVを作成。structuredBufferでアクセスできるようにする
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc = {};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = static_cast<UINT>(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);

	// CreateSRVで作成し、インデックスをもらう
	skinCluster.paletteSrvIndex = srvManager->CreateSRV(
		skinCluster.paletteResource.Get(),
		paletteSrvDesc
	);

	// InverseBindPoseMatrixの初期化

	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(),
		skinCluster.inverseBindPoseMatrices.end(), []() { return Matrix4x4::MakeIdentity(); });

	// Influence (ウェイト情報) の作成

	// メッシュの数だけInfluence格納場所を確保
	skinCluster.meshInfluences.resize(modelData.meshes.size());

	// メッシュごとに処理
	for (size_t i = 0; i < modelData.meshes.size(); ++i)
	{
		const auto& mesh = modelData.meshes[i];            // 現在のメッシュデータ
		auto& influenceInfo = skinCluster.meshInfluences[i]; // 現在のメッシュ用Influence構造体

		// Resource確保 (このメッシュの頂点数分)
		influenceInfo.influenceResource = BufferManager::CreateBufferResource(
			device.Get(),
			sizeof(VertexInfluence) * mesh.vertices.size());

		VertexInfluence* mappedInfluence = nullptr;
		influenceInfo.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
		std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * mesh.vertices.size()); // 0埋め

		influenceInfo.mappedInfluence = { mappedInfluence, mesh.vertices.size() };

		// VBV作成
		influenceInfo.influenceBufferView.BufferLocation = influenceInfo.influenceResource->GetGPUVirtualAddress();
		influenceInfo.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * mesh.vertices.size());
		influenceInfo.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

		// SkinCluster情報の解析と書き込み
		for (const auto& jointWeight : mesh.skinClusterData)
		{
			auto it = skeleton.jointMap.find(jointWeight.first);
			if (it == skeleton.jointMap.end())
			{
				continue; // Skeletonに含まれていないJointは無視
			}

			// InverseBindPoseMatrixのセット
			skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;

			// 頂点ウェイトのセット
			for (const auto& vertexWeight : jointWeight.second.vertexWeights)
			{
				// メッシュ内のローカルな頂点インデックスでアクセス
				auto& currentInfluence = influenceInfo.mappedInfluence[vertexWeight.vertexIndex];

				for (uint32_t index = 0; index < kNumMaxInfluence; ++index)
				{
					if (currentInfluence.weights[index] == 0.0f) // 空きを探す
					{
						currentInfluence.weights[index] = vertexWeight.weight;
						currentInfluence.jointIndices[index] = static_cast<int32_t>((*it).second);
						break;
					}
				}
			}
		}

		// ウェイトの正規化処理 (このメッシュに対して行う)
		for (auto& influence : influenceInfo.mappedInfluence)
		{
			float totalWeight = 0.0f;
			for (float w : influence.weights)
			{
				totalWeight += w;
			}

			if (totalWeight > 0.0f)
			{
				for (float& w : influence.weights)
				{
					w /= totalWeight;
				}
			}
		}
	}

	return skinCluster;
}

void UpdateSkinCluster(SkinCluster& skinCluster, const Skeleton& skeleton)
{
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
	{
		assert(jointIndex < skinCluster.inverseBindPoseMatrices.size());
		skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Matrix4x4::Inverse(skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix).Transpose();
	}
}