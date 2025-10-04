#include "GlobalVariables.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

#include <fstream>
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

	if (!ImGui::BeginMenuBar())
	{
		ImGui::End();
		return;
	}

	if (ImGui::BeginMenu("ドラッグの感度設定"))
	{
		ImGui::DragFloat("Sensitivity", &dragSensitivity_, 0.01f, 0.001f, 10.0f, "%.3f");
		ImGui::EndMenu();
	}

	// 各グループについて
	for (auto itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup)
	{
		const std::string& groupName = itGroup->first;
		Group& group = itGroup->second;

		if (!ImGui::BeginMenu(groupName.c_str()))
			continue;

		for (auto itItem = group.items.begin(); itItem != group.items.end(); ++itItem)
		{
			const std::string& itemName = itItem->first;
			Item& item = itItem->second;

			if (std::holds_alternative<int32_t>(item.value))
			{
				int32_t* ptr = std::get_if<int32_t>(&item.value);
				ImGui::DragInt(itemName.c_str(), ptr, (float)dragSensitivity_);
			}
			else if (std::holds_alternative<float>(item.value))
			{
				float* ptr = std::get_if<float>(&item.value);
				ImGui::DragFloat(itemName.c_str(), ptr, dragSensitivity_);
			}
			else if (std::holds_alternative<bool>(item.value)) 
			{
				bool* ptr = std::get_if<bool>(&item.value);
				ImGui::Checkbox(itemName.c_str(), ptr);
			}
			else if (std::holds_alternative<Vector2>(item.value)) 
			{
				Vector2* ptr = std::get_if<Vector2>(&item.value);
				ImGui::DragFloat2(itemName.c_str(), reinterpret_cast<float*>(ptr), dragSensitivity_);
			}
			else if (std::holds_alternative<Vector3>(item.value))
			{
				Vector3* ptr = std::get_if<Vector3>(&item.value);
				ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), dragSensitivity_);
			}
			else if (std::holds_alternative<Vector4>(item.value)) 
			{
				Vector4* ptr = std::get_if<Vector4>(&item.value);
				ImGui::ColorEdit4(itemName.c_str(), reinterpret_cast<float*>(ptr));
			}
		}

		ImGui::Text("\n");

		if (ImGui::Button("Save"))
		{
			SaveFile(groupName);
			std::string message = std::format("{}.json saved", groupName);
			MessageBoxA(nullptr, message.c_str(), "GlobalVariables", 0);
		}

		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
	ImGui::End();
}
void GlobalVariables::CreateGroup(const std::string& groupName)
{
	// 指定名のオブジェクトが無ければ追加する
	datas_[groupName];
}

int32_t GlobalVariables::GetIntValue(const std::string& groupName, const std::string& key) const
{
	// グループが存在するかチェック
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());

	const Group& group = itGroup->second;

	// 項目が存在するかチェック
	auto itItem = group.items.find(key);
	assert(itItem != group.items.end());

	const Item& item = itItem->second;

	// 型がint32_tかチェック
	assert(std::holds_alternative<int32_t>(item.value));

	// 値を取得して返す
	return std::get<int32_t>(item.value);
}

float GlobalVariables::GetFloatValue(const std::string& groupName, const std::string& key) const
{
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());

	const Group& group = itGroup->second;

	auto itItem = group.items.find(key);
	assert(itItem != group.items.end());

	const Item& item = itItem->second;

	assert(std::holds_alternative<float>(item.value));

	return std::get<float>(item.value);
}

bool GlobalVariables::GetBoolValue(const std::string& groupName, const std::string& key) const {
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());
	const Group& group = itGroup->second;
	auto itItem = group.items.find(key);
	assert(itItem != group.items.end());
	const Item& item = itItem->second;
	assert(std::holds_alternative<bool>(item.value));
	return std::get<bool>(item.value);
}

Vector2 GlobalVariables::GetVector2Value(const std::string& groupName, const std::string& key) const
{
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());

	const Group& group = itGroup->second;

	auto itItem = group.items.find(key);
	assert(itItem != group.items.end());

	const Item& item = itItem->second;

	assert(std::holds_alternative<Vector2>(item.value));

	return std::get<Vector2>(item.value);
}


Vector3 GlobalVariables::GetVector3Value(const std::string& groupName, const std::string& key) const
{
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());

	const Group& group = itGroup->second;

	auto itItem = group.items.find(key);
	assert(itItem != group.items.end());

	const Item& item = itItem->second;

	assert(std::holds_alternative<Vector3>(item.value));

	return std::get<Vector3>(item.value);
}


Vector4 GlobalVariables::GetVector4Value(const std::string& groupName, const std::string& key) const
{
	auto itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());

	const Group& group = itGroup->second;

	auto itItem = group.items.find(key);
	assert(itItem != group.items.end());

	const Item& item = itItem->second;

	assert(std::holds_alternative<Vector4>(item.value));

	return std::get<Vector4>(item.value);
}

void GlobalVariables::SetValue(
	const std::string& groupName,
	const std::string& key, int32_t value)
{
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(
	const std::string& groupName,
	const std::string& key, float value)
{
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, bool value)
{
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector2& value)
{
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(
	const std::string& groupName,
	const std::string& key, const Vector3& value)
{
	// グループの参照を取得
	Group& group = datas_[groupName];
	// 新しい項目のデータを設定
	Item newItem{};
	newItem.value = value;
	// 設定した項目をstd::mapに追加
	group.items[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector4& value)
{
	Group& group = datas_[groupName];
	Item newItem{};
	newItem.value = value;
	group.items[key] = newItem;
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, int32_t value)
{
	// グループが存在しなければ作成
	Group& group = datas_[groupName];

	// 項目が未登録なら
	if (group.items.find(key) == group.items.end())
	{
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, float value)
{
	// グループが存在しなければ作成
	Group& group = datas_[groupName];

	// 項目が未登録なら
	if (group.items.find(key) == group.items.end())
	{
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, bool value)
{
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) 
	{
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector2& value) 
{
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end())
	{
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector3& value)
{
	// グループが存在しなければ作成
	Group& group = datas_[groupName];

	// 項目が未登録なら
	if (group.items.find(key) == group.items.end())
	{
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector4& value)
{
	Group& group = datas_[groupName];
	if (group.items.find(key) == group.items.end()) 
	{
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::SaveFile(const std::string& groupName)
{
	// グループ検索
	std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);

	// 未登録チェック
	assert(itGroup != datas_.end());

	json root;
	root = json::object();

	// jsonオブジェクト登録
	root[groupName] = json::object();

	//各項目について
	for (std::map<std::string, Item>::iterator itItem = itGroup->second.items.begin();
		itItem != itGroup->second.items.end(); ++itItem)
	{
		// 項目名を取得
		const std::string& itemName = itItem->first;
		// 項目の参照を取得
		Item& item = itItem->second;

		// int32_t型の値を保持していれば
		if (std::holds_alternative<int32_t>(item.value))
		{
			// int32_t型の値を登録
			root[groupName][itemName] = std::get<int32_t>(item.value);
		}

		// float型の値を保持していれば
		if (std::holds_alternative<float>(item.value))
		{
			// float型の値を登録
			root[groupName][itemName] = std::get<float>(item.value);
		}

		// bool型の値を保持していれば
		if (std::holds_alternative<bool>(item.value))
		{
			// bool型の値を登録
			root[groupName][itemName] = std::get<bool>(item.value);
		}

		// Vector2型の値を保持していれば
		if (std::holds_alternative<Vector2>(item.value))
		{
			// Vector2型のjson配列登録
			Vector2 value = std::get<Vector2>(item.value);
			root[groupName][itemName] = json::array({ value.x,value.y });
		}

		// Vector3型の値を保持していれば
		if (std::holds_alternative<Vector3>(item.value))
		{
			// Vector3型のjson配列登録
			Vector3 value = std::get<Vector3>(item.value);
			root[groupName][itemName] = json::array({ value.x,value.y,value.z });
		}

		// Vector4型の値を保持していれば
		if (std::holds_alternative<Vector4>(item.value))
		{
			// Vector4型のjson配列登録
			Vector4 value = std::get<Vector4>(item.value);
			root[groupName][itemName] = json::array({ value.x,value.y,value.z,value.w });
		}
	}

	// ディレクトリが無ければ作成する
	std::filesystem::path dir(kDirectoryPath_);

	// ディレクトリがなければ作成
	if (!std::filesystem::exists(dir))
	{
		std::filesystem::create_directories(dir);  // 複数階層でも作成可能
	}

	// ファイルパスは path の / operator で結合
	std::filesystem::path filePath = dir / (groupName + ".json");

	// ofstream を開く
	std::ofstream ofs(filePath);

	// ファイルオープン失敗？
	if (ofs.fail())
	{
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlovalVariables", 0);
		assert(0);
		return;
	}

	// ファイルにjson文字列を書き込む(インデント幅4)
	ofs << std::setw(4) << root << std::endl;
	// ファイルを閉じる
	ofs.close();
}

void GlobalVariables::LoadFiles()
{
	// 保存先のディレクトリのパスをローカル変数で宣言する
	std::filesystem::path dir = kDirectoryPath_;

	// ディレクトリがなければスキップする
	if (!std::filesystem::exists(dir))
	{
		return;
	}

	std::filesystem::directory_iterator dir_it(dir);
	for (const std::filesystem::directory_entry& entry : dir_it)
	{
		// ファイルパスを取得
		const std::filesystem::path& filePath = entry.path();

		// ファイル拡張子を取得
		std::string extension = filePath.extension().string();
		// .jsonファイル以外はスキップ
		if (extension.compare(".json") != 0)
		{
			continue;
		}

		// ファイル読み込み
		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName)
{
	// 読み込むJSONファイルのフルパスを合成する
	std::string filePath = kDirectoryPath_ + groupName + ".json";
	// 読み込み用ファイルストリーム
	std::ifstream ifs;
	// ファイルを読み込みように開く
	ifs.open(filePath);

	// ファイルオープン失敗？
	if (ifs.fail())
	{
		std::string message = "Failed open data file for write.";
		MessageBoxA(nullptr, message.c_str(), "GlovalVariables", 0);
		assert(0);
		return;
	}

	json root;

	// json文字列からjsonのデータ構造に展開
	ifs >> root;
	// ファイルを閉じる
	ifs.close();

	// グループを検索
	json::iterator itGroup = root.find(groupName);

	// 未登録チェック
	assert(itGroup != root.end());

	// 各アイテムについて
	for (json::iterator itItem = itGroup->begin(); itItem != itGroup->end(); ++itItem)
	{
		// アイテム名を取得
		const std::string& itemName = itItem.key();

		// int32_t型の値を保持していれば
		if (itItem->is_number_integer())
		{
			// int型の値を登録
			int32_t value = itItem->get<int32_t>();
			SetValue(groupName, itemName, value);
		}
		// float型の値を保持していれば
		else if (itItem->is_number_float())
		{
			// float型の値を登録
			double value = itItem->get<double>();
			SetValue(groupName, itemName, static_cast<float>(value));
		}
		// bool型の値を保持していれば
		else if (itItem->is_boolean())
		{
			bool value = itItem->get<bool>();
			SetValue(groupName, itemName, value);
		}
		// 要素数2の配列であれば
		else if (itItem->is_array() && itItem->size() == 2)
		{
			Vector2 value = { (*itItem)[0], (*itItem)[1] };
			SetValue(groupName, itemName, value);
		}
		// 要素数3の配列であれば
		else if (itItem->is_array() && itItem->size() == 3)
		{
			// float型のjson配列登録
			Vector3 value = { itItem->at(0),itItem->at(1),itItem->at(2) };
			SetValue(groupName, itemName, value);
		}
		// 要素数4の配列であれば
		else if (itItem->is_array() && itItem->size() == 4)
		{
			Vector4 value = { (*itItem)[0], (*itItem)[1], (*itItem)[2], (*itItem)[3] };
			SetValue(groupName, itemName, value);
		}
	}
}