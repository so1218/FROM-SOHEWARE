#include "GlobalVariables.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

#include <fstream>
#include <iostream>
#include <windows.h>

GlobalVariables* GlobalVariables::GetInstance()
{
	// 関数内static変数は、最初の呼び出し時にのみ初期化され、スレッドセーフが保証される
	static GlobalVariables instance;
	return &instance;
}

void GlobalVariables::Update()
{
	if (!ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_MenuBar))
	{
		ImGui::End();
		return;
	}

	// メニューバー
	if (ImGui::BeginMenuBar())
	{
		// 感度設定メニュー
		if (ImGui::BeginMenu("ドラッグの感度設定"))
		{
			ImGui::DragFloat("感度", &dragSensitivity_, 0.01f, 0.001f, 10.0f, "%.3f");
			ImGui::EndMenu();
		}

		// トップレベルグループをメニューとして表示
		for (auto& [groupName, group] : datas_)
		{
			if (ImGui::BeginMenu(groupName.c_str()))
			{
				DrawGroupRecursive({ groupName }, group);
				ImGui::EndMenu();
			}
		}

		ImGui::EndMenuBar();
	}

	ImGui::End();
}

void GlobalVariables::DrawGroupRecursive(const std::vector<std::string>& groupPath, Group& group)
{
	// groupPathの末尾が現在のグループ名
	const std::string& groupName = groupPath.back();

	if (ImGui::TreeNode(groupName.c_str()))
	{
		// items を表示
		for (auto& [itemName, value] : group.items)
		{
			std::string label = itemName;

			if (value.type() == typeid(int32_t)) {
				int32_t* ptr = std::any_cast<int32_t>(&value);
				ImGui::DragInt(label.c_str(), ptr, (float)dragSensitivity_);
			}
			else if (value.type() == typeid(float)) {
				float* ptr = std::any_cast<float>(&value);
				ImGui::DragFloat(label.c_str(), ptr, dragSensitivity_);
			}
			else if (value.type() == typeid(bool)) {
				bool* ptr = std::any_cast<bool>(&value);
				ImGui::Checkbox(label.c_str(), ptr);
			}
			else if (value.type() == typeid(Vector2)) {
				Vector2* ptr = std::any_cast<Vector2>(&value);
				ImGui::DragFloat2(label.c_str(), reinterpret_cast<float*>(ptr), dragSensitivity_);
			}
			else if (value.type() == typeid(Vector3)) {
				Vector3* ptr = std::any_cast<Vector3>(&value);
				ImGui::DragFloat3(label.c_str(), reinterpret_cast<float*>(ptr), dragSensitivity_);
			}
			else if (value.type() == typeid(Vector4)) {
				Vector4* ptr = std::any_cast<Vector4>(&value);
				ImGui::ColorEdit4(label.c_str(), reinterpret_cast<float*>(ptr));
			}
		}

		ImGui::Spacing();

		// サブグループを再帰的に描画
		for (auto& [subGroupName, subGroup] : group.subGroups)
		{
			// 次の階層パスを作成して再帰呼び出し
			std::vector<std::string> nextGroupPath = groupPath;
			nextGroupPath.push_back(subGroupName);
			DrawGroupRecursive(nextGroupPath, subGroup);
		}

		// セーブボタン
		// 「Save [現在のグループ名]」ボタン -> このグループ階層だけを保存
		if (ImGui::Button(("Save [" + groupName + "]").c_str()))
		{
			// 新しい階層パスで保存するSaveFileを呼び出す
			SaveFile(groupPath);
			std::string message = std::format("Updated group '{}' in {}.json", groupName, groupPath[0]);
			MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		}

		ImGui::TreePop();
	}
}

void GlobalVariables::CreateGroup(const std::vector<std::string>& groupPath)
{
	if (groupPath.empty()) return;

	Group* current = &datas_[groupPath[0]];
	for (size_t i = 1; i < groupPath.size(); ++i)
	{
		current = &current->subGroups[groupPath[i]];
	}
}

int32_t GlobalVariables::GetIntValue(const std::vector<std::string>& groupPath, const std::string& key) const
{
	const Group* group = FindGroup(groupPath);
	assert(group != nullptr);

	auto itItem = group->items.find(key);
	assert(itItem != group->items.end());

	// std::anyからint32_tにキャスト
	try
	{
		return std::any_cast<int32_t>(itItem->second);
	}
	catch (const std::bad_any_cast&)
	{
		assert(false && "Item type is not int32_t");
		return 0; // 失敗時のデフォルト
	}
}

float GlobalVariables::GetFloatValue(const std::vector<std::string>& groupPath, const std::string& key) const
{
	const Group* group = FindGroup(groupPath);
	assert(group != nullptr);

	auto itItem = group->items.find(key);
	assert(itItem != group->items.end());

	try
	{
		return std::any_cast<float>(itItem->second);
	}
	catch (const std::bad_any_cast&)
	{
		assert(false && "Item type is not float");
		return 0.f;
	}
}

bool GlobalVariables::GetBoolValue(const std::vector<std::string>& groupPath, const std::string& key) const
{
	const Group* group = FindGroup(groupPath);
	assert(group != nullptr);

	auto itItem = group->items.find(key);
	assert(itItem != group->items.end());

	try
	{
		return std::any_cast<bool>(itItem->second);
	}
	catch (const std::bad_any_cast&)
	{
		assert(false && "Item type is not bool");
		return false;
	}
}

Vector2 GlobalVariables::GetVector2Value(const std::vector<std::string>& groupPath, const std::string& key) const
{
	const Group* group = FindGroup(groupPath);
	assert(group != nullptr);

	auto itItem = group->items.find(key);
	assert(itItem != group->items.end());

	try
	{
		return std::any_cast<Vector2>(itItem->second);
	}
	catch (const std::bad_any_cast&)
	{
		assert(false && "Item type is not Vector2");
		return Vector2{};
	}
}


Vector3 GlobalVariables::GetVector3Value(const std::vector<std::string>& groupPath, const std::string& key) const
{
	const Group* group = FindGroup(groupPath);
	assert(group != nullptr);

	auto itItem = group->items.find(key);
	assert(itItem != group->items.end());

	try
	{
		return std::any_cast<Vector3>(itItem->second);
	}
	catch (const std::bad_any_cast&)
	{
		assert(false && "Item type is not Vector3");
		return Vector3{};
	}
}


Vector4 GlobalVariables::GetVector4Value(const std::vector<std::string>& groupPath, const std::string& key) const
{
	const Group* group = FindGroup(groupPath);
	assert(group != nullptr);

	auto itItem = group->items.find(key);
	assert(itItem != group->items.end());

	try
	{
		return std::any_cast<Vector4>(itItem->second);
	}
	catch (const std::bad_any_cast&)
	{
		assert(false && "Item type is not Vector4");
		return Vector4{};
	}
}

const GlobalVariables::Group* GlobalVariables::FindGroup(const std::vector<std::string>& groupPath) const
{
	if (groupPath.empty()) return nullptr;

	auto it = datas_.find(groupPath[0]);
	if (it == datas_.end()) return nullptr;

	const Group* current = &it->second;
	for (size_t i = 1; i < groupPath.size(); ++i)
	{
		auto itSub = current->subGroups.find(groupPath[i]);
		if (itSub == current->subGroups.end()) return nullptr;
		current = &itSub->second;
	}
	return current;
}

void GlobalVariables::SetValue(
	const std::vector<std::string>& groupPath,
	const std::string& key, int32_t value)
{
	Group& group = FindOrCreateGroup(groupPath);
	group.items[key] = value;
}

void GlobalVariables::SetValue(
	const std::vector<std::string>& groupPath,
	const std::string& key, float value)
{
	Group& group = FindOrCreateGroup(groupPath);
	group.items[key] = value;
}

void GlobalVariables::SetValue(const std::vector<std::string>& groupPath, const std::string& key, bool value)
{
	Group& group = FindOrCreateGroup(groupPath);
	group.items[key] = value;
}

void GlobalVariables::SetValue(const std::vector<std::string>& groupPath, const std::string& key, const Vector2& value)
{
	Group& group = FindOrCreateGroup(groupPath);
	group.items[key] = value;
}

void GlobalVariables::SetValue(
	const std::vector<std::string>& groupPath,
	const std::string& key, const Vector3& value)
{
	Group& group = FindOrCreateGroup(groupPath);
	group.items[key] = value;
}

void GlobalVariables::SetValue(const std::vector<std::string>& groupPath, const std::string& key, const Vector4& value)
{
	Group& group = FindOrCreateGroup(groupPath);
	group.items[key] = value;
}

GlobalVariables::Group& GlobalVariables::FindOrCreateGroup(const std::vector<std::string>& groupPath)
{
	assert(!groupPath.empty());
	Group* current = &datas_[groupPath[0]];
	for (size_t i = 1; i < groupPath.size(); ++i)
	{
		current = &current->subGroups[groupPath[i]];
	}
	return *current;
}

void GlobalVariables::AddItem(const std::vector<std::string>& groupPath, const std::string& key, int32_t value)
{
	if (groupPath.empty()) return;

	// 1階層目を取得
	Group* current = &datas_[groupPath[0]];

	// 2階層目以降
	for (size_t i = 1; i < groupPath.size(); ++i) {
		current = &current->subGroups[groupPath[i]];
	}

	// keyが未登録なら追加
	if (current->items.find(key) == current->items.end())
	{
		current->items[key] = value;
	}
}

void GlobalVariables::AddItem(const std::vector<std::string>& groupPath, const std::string& key, float value)
{
	if (groupPath.empty()) return;

	// 1階層目を取得
	Group* current = &datas_[groupPath[0]];

	// 2階層目以降
	for (size_t i = 1; i < groupPath.size(); ++i) {
		current = &current->subGroups[groupPath[i]];
	}

	// keyが未登録なら追加
	if (current->items.find(key) == current->items.end())
	{
		current->items[key] = value;
	}
}

void GlobalVariables::AddItem(const std::vector<std::string>& groupPath, const std::string& key, bool value)
{
	if (groupPath.empty()) return;

	// 1階層目を取得
	Group* current = &datas_[groupPath[0]];

	// 2階層目以降
	for (size_t i = 1; i < groupPath.size(); ++i) {
		current = &current->subGroups[groupPath[i]];
	}

	// keyが未登録なら追加
	if (current->items.find(key) == current->items.end())
	{
		current->items[key] = value;
	}
}

void GlobalVariables::AddItem(const std::vector<std::string>& groupPath, const std::string& key, const Vector2& value)
{
	if (groupPath.empty()) return;

	// 1階層目を取得
	Group* current = &datas_[groupPath[0]];

	// 2階層目以降
	for (size_t i = 1; i < groupPath.size(); ++i) {
		current = &current->subGroups[groupPath[i]];
	}

	// keyが未登録なら追加
	if (current->items.find(key) == current->items.end())
	{
		current->items[key] = value;
	}
}

void GlobalVariables::AddItem(const std::vector<std::string>& groupPath, const std::string& key, const Vector3& value)
{
	if (groupPath.empty()) return;

	// 1階層目を取得
	Group* current = &datas_[groupPath[0]];

	// 2階層目以降
	for (size_t i = 1; i < groupPath.size(); ++i) {
		current = &current->subGroups[groupPath[i]];
	}

	// keyが未登録なら追加
	if (current->items.find(key) == current->items.end())
	{
		current->items[key] = value;
	}
}
void GlobalVariables::AddItem(const std::vector<std::string>& groupPath, const std::string& key, const Vector4& value)
{
	if (groupPath.empty()) return;

	// 1階層目を取得
	Group* current = &datas_[groupPath[0]];

	// 2階層目以降
	for (size_t i = 1; i < groupPath.size(); ++i) {
		current = &current->subGroups[groupPath[i]];
	}

	// keyが未登録なら追加
	if (current->items.find(key) == current->items.end())
	{
		current->items[key] = value;
	}
}

void GlobalVariables::SaveFile(const std::vector<std::string>& groupPath)
{
	if (groupPath.empty()) {
		return; // パスが空なら何もしない
	}

	// ファイル名はパスの先頭要素から決まる (例: {"Player", "Stage1"} -> "Player.json")
	const std::string& topLevelName = groupPath[0];
	std::string filePathStr = kDirectoryPath_ + topLevelName + ".json";
	std::filesystem::path filePath = filePathStr;

	// プログラム内のメモリから更新対象のGroupオブジェクトを見つける
	Group* targetGroup = &datas_[topLevelName];
	for (size_t i = 1; i < groupPath.size(); ++i) {
		auto it = targetGroup->subGroups.find(groupPath[i]);
		if (it == targetGroup->subGroups.end())
		{
			assert(false && "Group path not found in memory.");
			return;
		}
		targetGroup = &it->second;
	}

	// 既存のJSONファイルを読み込む（なければ新規作成の準備）
	json rootJson;
	std::ifstream ifs(filePath);
	if (ifs.is_open()) {
		// ファイルが存在すれば、その内容を読み込む
		ifs >> rootJson;
		ifs.close();
	}

	// JSONデータ内で、更新したい階層まで移動する
	//    (途中の階層がなければ自動的に作られる)
	json* currentJsonNode = &rootJson[topLevelName];
	for (size_t i = 1; i < groupPath.size(); ++i) {
		currentJsonNode = &(*currentJsonNode)[groupPath[i]];
	}

	// メモリ上のGroupをJSONに変換し、対象の階層を丸ごと上書きする
	*currentJsonNode = GroupToJson(*targetGroup);

	// 更新したJSONデータ全体をファイルに書き戻す
	std::ofstream ofs(filePath);
	if (ofs.fail()) {
		MessageBoxA(nullptr, "Failed to open file for saving.", "GlobalVariables", MB_OK);
		assert(false);
		return;
	}
	ofs << std::setw(4) << rootJson << std::endl;
	ofs.close();
}

json GlobalVariables::GroupToJson(const Group& group)
{
	json j;

	// items を JSON に変換
	for (const auto& [key, value] : group.items)
	{
		if (value.type() == typeid(int32_t)) {
			j[key] = std::any_cast<int32_t>(value);
		}
		else if (value.type() == typeid(float)) {
			j[key] = std::any_cast<float>(value);
		}
		else if (value.type() == typeid(bool)) {
			j[key] = std::any_cast<bool>(value);
		}
		else if (value.type() == typeid(Vector2)) {
			Vector2 v = std::any_cast<Vector2>(value);
			j[key] = { v.x, v.y };
		}
		else if (value.type() == typeid(Vector3)) {
			Vector3 v = std::any_cast<Vector3>(value);
			j[key] = { v.x, v.y, v.z };
		}
		else if (value.type() == typeid(Vector4)) {
			Vector4 v = std::any_cast<Vector4>(value);
			j[key] = { v.x, v.y, v.z, v.w };
		}
		else {
			// 対応してない型は無視 or ログ
			std::cerr << "Unsupported type in GlobalVariables: " << key << std::endl;
		}
	}

	// subGroups を再帰的に JSON に変換
	for (const auto& [subGroupName, subGroup] : group.subGroups)
	{
		j[subGroupName] = GroupToJson(subGroup);
	}

	return j;
}

void GlobalVariables::LoadFiles()
{
	std::filesystem::path dir = kDirectoryPath_;

	if (!std::filesystem::exists(dir))
	{
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (!entry.is_regular_file()) continue;

		const std::filesystem::path& filePath = entry.path();
		if (filePath.extension() != ".json") continue;

		std::string filename = filePath.stem().string(); 

		// ドットがある場合、最初のドット以降はカット
		size_t dotPos = filename.find('.');
		if (dotPos != std::string::npos)
		{
			filename = filename.substr(0, dotPos);  
		}

		LoadFile(filename);
	}
}

void GlobalVariables::LoadFile(const std::string& groupName)
{
	std::string filePath = kDirectoryPath_ + groupName + ".json";
	std::ifstream ifs(filePath);
	if (ifs.fail())
	{
		MessageBoxA(nullptr, "Failed to open data file for write.", "GlobalVariables", 0);
		assert(false);
		return;
	}

	json root;
	ifs >> root;
	ifs.close();

	json::iterator itGroup = root.find(groupName);
	assert(itGroup != root.end());

	// 最上位グループ名から再帰的に読み込み開始
	LoadGroupRecursive({ groupName }, *itGroup);
}

void GlobalVariables::LoadGroupRecursive(const std::vector<std::string>& groupPath, const json& jGroup)
{
	for (auto it = jGroup.begin(); it != jGroup.end(); ++it)
	{
		const std::string& key = it.key();
		const json& value = it.value();

		if (value.is_object())
		{
			// サブグループの場合は再帰的に処理
			std::vector<std::string> nextGroupPath = groupPath;
			nextGroupPath.push_back(key);
			LoadGroupRecursive(nextGroupPath, value);
		}
		else
		{
			// 値の場合は型に応じて SetValue 呼び出し

			if (value.is_number_integer())
			{
				SetValue(groupPath, key, value.get<int32_t>());
			}
			else if (value.is_number_float())
			{
				SetValue(groupPath, key, static_cast<float>(value.get<double>()));
			}
			else if (value.is_boolean())
			{
				SetValue(groupPath, key, value.get<bool>());
			}
			else if (value.is_array())
			{
				if (value.size() == 2)
				{
					SetValue(groupPath, key, Vector2{ value[0], value[1] });
				}
				else if (value.size() == 3)
				{
					SetValue(groupPath, key, Vector3{ value[0], value[1], value[2] });
				}
				else if (value.size() == 4)
				{
					SetValue(groupPath, key, Vector4{ value[0], value[1], value[2], value[3] });
				}
			}
		}
	}
}