# 変更履歴

版ごとの変更点。ゲーム側で対応が必要な変更は「対応が必要」に書きます。

## 未リリース

### 追加
- 保存するビネットとは別に、ゲームの状態 (隠れているなど) を伝えるビネットを重ねる `PostEffectManager::SetOverlayVignette` を追加。強さ・色と、暗くなり始める距離・一番暗くなる距離を渡す。毎フレーム渡す値で、保存はしない。渡さなければ見た目は変わらない
- 草を生やす密度のしきい値 `GrassGenerationData::densityThreshold` を追加。密度マップがこの値以上の所に草を生やす。今までは 0.5 で固定だった

### 対応が必要
- `GrassGenerationData` を自分で作って `GrassSystem::Generate` に渡しているゲームは、`densityThreshold` を入れる。今までと同じにするなら 0.5。入れないと 0 になり、密度マップが黒い所にも草が生える

## v1.1.0 (2026-11-13)

### 追加
- 空と太陽の光を出す `Environment` をエンジンに追加 (`#include "Environment.h"`)。シーンに置かないと光がなく、モデルが真っ黒に映る
- テンプレートの `MainScene` に `Environment` を置き、`Game/GameObject` フォルダとそのインクルードディレクトリを追加した (これから始めるゲームのみ)

### 変更
- ドキュメントを、テンプレートから始めたゲームだけで動く内容に直した。チュートリアルで使っていたゲーム側のファイル・素材 (`Environment.h`・`floatingDamage` など) を使わないようにし、シーンの切り替えに `SceneManager.h` が必要なこと、パーティクルのプリセットの作り方を書いた
- 「コードの決まり」を「コードを書くときに必要なこと」にして、ビルドや動作に関係のある内容だけにした
- 完成版のフォルダを作る処理を、Visual Studio の外でのビルド (GitHub Actions などの自動ビルド) では止めないようにした。`Cache/Shaders` がないときは警告を出して作らない。作りたくないときは `/p:CreateGamePackage=false` を渡す

## v1.0.1 (2026-11-09)

### 変更
- 完成版のフォルダ (`Package`) を、Release をビルドするたびに必ず作り直すようにした。コードを変えていないと作り直されないことがあった

## v1.0.0 (2026-11-09)

最初の版です

### はじめ方
- `Template/` のテンプレートから始める。手順は `Docs/start/new-project.md`
- エンジンの更新は `UpdateEngine.ps1`。使い方は `README.md`
- 完成版のまとめ方は `Docs/guides/packaging.md`
