#pragma once
#include <variant>
#include <map>
#include <string>
#include <json.hpp>

#include "Vector.h"

using json = nlohmann::json;

// ゲームの設定値やパラメータをグループごとに管理するクラス
class GlobalVariables
{
public:
	// 項目
	struct Item
	{
		// 項目の値
		std::variant<int32_t, float, bool, Vector2, Vector3, Vector4> value;
	};

	// グループ
	struct Group
	{
		std::unordered_map<std::string, std::any> items;
		std::unordered_map<std::string, Group> subGroups;
	};


	// シングルトンのインスタンスを取得
	static GlobalVariables* GetInstance();

	// グループの作成
	void CreateGroup(const std::vector<std::string>& groupPath);

	// 値の取得
	int32_t GetIntValue(const std::vector<std::string>& groupPath, const std::string& key) const;
	float GetFloatValue(const std::vector<std::string>& groupPath, const std::string& key) const;
	bool GetBoolValue(const std::vector<std::string>& groupPath, const std::string& key) const;
	Vector2 GetVector2Value(const std::vector<std::string>& groupPath, const std::string& key) const;
	Vector3 GetVector3Value(const std::vector<std::string>& groupPath, const std::string& key) const;
	Vector4 GetVector4Value(const std::vector<std::string>& groupPath, const std::string& key) const;

	// グループ階層をたどる関数
	const Group* FindGroup(const std::vector<std::string>& groupPath) const;

	// 値のセット(int)
	void SetValue(const std::vector<std::string>& groupPath, const std::string& key, int32_t value);
	// 値のセット(float)
	void SetValue(const std::vector<std::string>& groupPath, const std::string& key, float value);
	// 値のセット(bool)
	void SetValue(const std::vector<std::string>& groupPath, const std::string& key, bool value);
	// 値のセット(Vector2)
	void SetValue(const std::vector<std::string>& groupPath, const std::string& key, const Vector2& value);
	// 値のセット(Vector3)
	void SetValue(const std::vector<std::string>& groupPath, const std::string& key, const Vector3& value);
	// 値のセット(Vector4)
	void SetValue(const std::vector<std::string>& groupPath, const std::string& key, const Vector4& value);

	// ネスとしたグループに対して値をセットさせる関数
	Group& FindOrCreateGroup(const std::vector<std::string>& groupPath);

	// 項目の追加(int)
	void AddItem(const std::vector<std::string>& groupPath, const std::string& key, int32_t value);
	// 項目の追加(float)
	void AddItem(const std::vector<std::string>& groupPath, const std::string& key, float value);
	// 項目の追加(bool)
	void AddItem(const std::vector<std::string>& groupPath, const std::string& key, bool value);
	// 項目の追加(Vector2)
	void AddItem(const std::vector<std::string>& groupPath, const std::string& key, const Vector2& value);
	// 項目の追加(Vector3)
	void AddItem(const std::vector<std::string>& groupPath, const std::string& key, const Vector3& value);
	// 項目の追加(Vector4)
	void AddItem(const std::vector<std::string>& groupPath, const std::string& key, const Vector4& value);

	// 毎フレーム処理
	void Update();

	// 再帰描画関数
	void DrawGroupRecursive(const std::vector<std::string>& groupPath, Group& group);

	// ファイルに書き出し
	// 階層パスを受け取り、JSONファイルの一部だけを更新する
	void SaveFile(const std::vector<std::string>& groupPath);
	void SaveAllFiles();

	// GroupをJSON に変換する再帰関数を作る
	json GroupToJson(const Group& group);

	// ディレクトリの全ファイル読み込み
	void LoadFiles();

	// ファイルから読み込む
	void LoadFile(const std::string& groupName);

	// 再帰的にグループを読み込む関数
	void LoadGroupRecursive(const std::vector<std::string>& groupPath, const json& jGroup);

private:
	// コンストラクタをprivateにし、外部からの直接生成を禁止
	GlobalVariables() = default;
	// デストラクタをprivateにし、外部からの直接削除を禁止
	~GlobalVariables() = default;

	// コピーコンストラクタを禁止
	GlobalVariables(const GlobalVariables&) = delete;
	// コピー代入演算子を禁止
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	// 全データ
	std::map<std::string, Group> datas_;

	// グローバル変数の保存先ファイルパス
	const std::string kDirectoryPath_ = "Resources/json/GlobalVariables/";

	// ドラッグの感度
	int dragSensitivityInt_ = 1;
	float dragSensitivity_ = 0.1f;
	float dragSensitivityVector3_ = 0.1f;

	// 画面表示用のステータスメッセージ
	std::string statusMessage_;
};
