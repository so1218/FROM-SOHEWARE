#pragma once

#include "Engine.h"

enum class MapChipType
{
	kBlank, // 空白
	kBlock, // ブロック
};

struct MapChipData
{
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField
{
public:
	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	// ゲッター
	static inline uint32_t GetNumBlockVirtical() { return kNumBlockVirtical; }
	static inline uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal; }

	// 1ブロックのサイズ
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;

	MapChipData mapChipData_;

	struct IndexSet
	{
		uint32_t xIndex;
		uint32_t yIndex;
	};
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position);

	// 範囲矩形
	struct Rect
	{
		float left;
		float right;
		float bottom;
		float top;
	};
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

private:
	// ブロックの個数
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;
};