# 完成版を作る

提出や配布のために、ゲームを Visual Studio がない PC でも動く形のフォルダにまとめる手順です。

## 手順

1. **Development** でビルドして一度起動し、使うシーンを一通り表示してから閉じる
2. ビルド構成を **Release** にしてビルドする
3. 出力ウィンドウに「配布用のフォルダを作った」と出たら完了。表示された場所 (`generated/outputs/Release/Package/`) のフォルダを、そのまま zip にして渡す
4. 渡す前に、`Package` の中の exe をダブルクリックして起動を確かめる

## なぜ Development で一度起動するのか

Release はシェーダーをその場でコンパイルせず、`Cache/Shaders/` にある結果だけを使います。シェーダーの結果は Development で起動したときに作られるので、Release の前に一度起動しておく必要があります。Debug で起動して作られるのは最適化していない別の結果 (`Cache/ShadersDebug/`) なので、Release では使われません。テクスチャの変換結果 (`Cache/Textures/`・`Cache/EngineTextures/`) も同じように使うので、配布先で起動したときに変換で待たされずに済みます。

`Cache/Shaders/` がないまま Release をビルドすると、「Cache\Shaders がないので、配布用のフォルダを作れない」というエラーになります。Development で一度起動してから、もう一度 Release をビルドしてください。

## フォルダの中身

| もの | 内容 |
| :--- | :--- |
| `ゲーム名.exe` と `.dll` | ゲーム本体と、シェーダー・PIX の dll |
| `Engine/Assets/` | エンジンが使うテクスチャ・モデル・フォント・描画の設定 |
| `Assets/` | ゲームのモデル・テクスチャ・音・保存したパラメータ |
| `Cache/` | シェーダー・テクスチャ・フォントの変換結果 |

ソースコード・シェーダーのソース (`Engine/Shaders/`)・外部ライブラリは入りません。

## つまずきやすい点

- **シェーダーを変えたのに反映されない。** Development で起動し直してから Release をビルドする
- **exe を `Package` の外に出すと起動しない。** exe は、自分と同じ場所にある `Engine/`・`Assets/`・`Cache/` を読む。フォルダごと渡す
