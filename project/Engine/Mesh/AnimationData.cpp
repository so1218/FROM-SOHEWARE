#include "AnimationData.h"
#include "TimeManager.h"
#include "BufferManager.h"
#include "BufferManager.h"

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

void UpdateSkeleton(Skeleton& skeleton)
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

SkinCluster CreateSkinCluster(
	const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	const Skeleton& skeleton,
	const ModelData& modelData,
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize,
	SRVAllocator* srvAllocator)
{
	// palette用のResourceを確保
	SkinCluster skinCluster;
	skinCluster.paletteResource = BufferManager::CreateBufferResource(
		device.Get(),
		sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;	
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));	
	skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size() }; // spanを使ってアクセスするようにする
	
	uint32_t srvIndex = srvAllocator->Allocate();
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	cpuHandle.ptr += descriptorSize * srvIndex;
	gpuHandle.ptr += descriptorSize * srvIndex;

	skinCluster.paletteSrvHandle.first = cpuHandle;
	skinCluster.paletteSrvHandle.second = gpuHandle;

	// palette用のSRVを作成。structuredBufferでアクセスできるようにする
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc = {};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.NumElements = static_cast<UINT>(skeleton.joints.size());
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);
	device->CreateShaderResourceView(
		skinCluster.paletteResource.Get(),
		&paletteSrvDesc,
		skinCluster.paletteSrvHandle.first);

	// influence用のResourceを確保。頂点ごとにinfluence情報を追加できるようにする
	skinCluster.influenceResource = BufferManager::CreateBufferResource(
		device.Get(),
		sizeof(VertexInfluence) * modelData.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.vertices.size()); // 0埋め。weightを0にしておく
	skinCluster.mappedInfluence = { mappedInfluence, modelData.vertices.size() }; 

	// influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(),
		skinCluster.inverseBindPoseMatrices.end(), []() { return Matrix4x4::MakeIdentity(); });

	for (const auto& jointWeight : modelData.skinClusterData) // ModelのSkinClusterの情報を解析
	{
		auto it = skeleton.jointMap.find(jointWeight.first); // jointWeight.firstはjoint名なので、Skeletonに対象となるjointが含まれているか判断
		if (it == skeleton.jointMap.end())
		{
			continue; // Skeletonに含まれていないJointは無視
		}

		// (*it).secondにはjointのindexが入っているので、該当のinverseBindPoseMatrixを代入
		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights)
		{
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex]; // 該当のvertexIndexのinfluence情報を参照しておく
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) // 空いているところに入れる 
			{
				if (currentInfluence.weights[index] == 0.0f) // weight==0が空いている状態なので、その場所にweightとjointのindexを代入
				{
					currentInfluence.weights[index] = vertexWeight.weight; 
					currentInfluence.jointIndices[index] = static_cast<int32_t>((*it).second);
					break;
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