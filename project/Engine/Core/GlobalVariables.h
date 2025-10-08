#pragma once
#include <variant>
#include <map>
#include <string>
#include <json.hpp>

#include "Vector.h"

using json = nlohmann::json;

/// <summary>
/// ゲームの設定値やパラメータをグループごとに管理するクラス
/// </summary>
class GlobalVariables 
{
public:
	// シングルトンのインスタンスを取得
	static GlobalVariables* GetInstance();

	/// <summary>
	/// グループの作成
	/// </summary>
	/// <param name="groupName">グループ名</param>
	void CreateGroup(const std::string& groupName);

	// 値の取得
	int32_t GetIntValue(const std::string& groupName, const std::string& key) const;
	float GetFloatValue(const std::string& groupName, const std::string& key) const;
	bool GetBoolValue(const std::string& groupName, const std::string& key) const;
	Vector2 GetVector2Value(const std::string& groupName, const std::string& key) const;
	Vector3 GetVector3Value(const std::string& groupName, const std::string& key) const;
	Vector4 GetVector4Value(const std::string& groupName, const std::string& key) const;

	// 値のセット(int)
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);
	// 値のセット(float)
	void SetValue(const std::string& groupName, const std::string& key, float value);
	// 値のセット(bool)
	void SetValue(const std::string& groupName, const std::string& key, bool value);
	// 値のセット(Vector2)
	void SetValue(const std::string& groupName, const std::string& key, const Vector2& value);
	// 値のセット(Vector3)
	void SetValue(const std::string& groupName, const std::string& key, const Vector3& value);
	// 値のセット(Vector4)
	void SetValue(const std::string& groupName, const std::string& key, const Vector4& value);

	// 項目の追加(int)
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);
	// 項目の追加(float)
	void AddItem(const std::string& groupName, const std::string& key, float value);
	// 項目の追加(bool)
	void AddItem(const std::string& groupName, const std::string& key, bool value);
	// 項目の追加(Vector2)
	void AddItem(const std::string& groupName, const std::string& key, const Vector2& value);
	// 項目の追加(Vector3)
	void AddItem(const std::string& groupName, const std::string& key, const Vector3& value);
	// 項目の追加(Vector4)
	void AddItem(const std::string& groupName, const std::string& key, const Vector4& value);

	/// <summary>
	/// 毎フレーム処理
	/// </summary>
	void Update();

	/// <summary>
	/// ファイルに書き出し
	/// </summary>
	/// <param name="groupName">グループ</param>
	void SaveFile(const std::string& groupName);

	/// <summary>
	/// ディレクトリの全ファイル読み込み
	/// </summary>
	void LoadFiles();

	/// <summary>
	/// ファイルから読み込む
	/// </summary>
	/// <param name="groupName">グループ</param>
	void LoadFile(const std::string& groupName);

private:
	// コンストラクタをprivateにし、外部からの直接生成を禁止
	GlobalVariables() = default;
	// デストラクタをprivateにし、外部からの直接削除を禁止
	~GlobalVariables() = default;

	// コピーコンストラクタを禁止
	GlobalVariables(const GlobalVariables&) = delete;
	// コピー代入演算子を禁止
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	// 項目
	struct Item 
	{
		// 項目の値
		std::variant<int32_t, float, bool, Vector2, Vector3, Vector4> value;
	};

	// グループ
	struct Group 
	{
		std::map<std::string, Item> items;
	};

	// 全データ
	std::map<std::string, Group> datas_;

	// グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath_ = "Resources/GlobalVariables/";

	// ドラッグの感度
	float dragSensitivity_ = 0.1f;
};
