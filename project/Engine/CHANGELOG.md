# 変更履歴

版ごとの変更点。ゲーム側で対応が必要な変更は「対応が必要」に書きます。

## 未リリース

### 追加
- 保存するビネットとは別に、ゲームの状態 (隠れているなど) を伝えるビネットを重ねる `PostEffectManager::SetOverlayVignette` を追加。強さ・色と、暗くなり始める距離・一番暗くなる距離を渡す。毎フレーム渡す値で、保存はしない。渡さなければ見た目は変わらない
- 草を生やす密度のしきい値 `GrassGenerationData::densityThreshold` を追加。密度マップがこの値以上の所に草を生やす。今までは 0.5 で固定だった
- 歩ける地形の外に見せる遠景の地形を描けるようにした。`Terrain::LoadFromHeightRatios` で CPU で作った高さから地形を作り、`SetDrawAsVista(true)` で遠景として描く。遠景は影を落とさず、草や植物の生成にも使わない。`ShareMaterialFrom` で歩ける地形と質感・模様の位置をそろえ、`SetHiddenArea` で歩ける地形の下にあるチャンクを省く
- `TerrainSettings` に、質感のテクスチャの UV を決める `texcoordOrigin` と `texcoordScale` を追加。今までの地形の見た目は変わらない
- 歩ける範囲の外で濃くなる霧を追加。`PostEffectManager::SetFogBoundary` で範囲を毎フレーム渡すと、境界の少し内側から霧が濃くなり、低い所ほど濃く溜まる。Volumetric Fog が届く所は光や影を受ける霧として、その先は合成のときに式で求めて、同じ光の当て方でつなげる。設定はポストエフェクトの Volumetric Fog の「歩ける範囲の外の霧」。渡さないシーンや Volumetric Fog が無効のときは何も変わらない
- `AnimationModel::ShareMaterialsFrom` を追加。`Model` と同じく、ほかの AnimationModel とマテリアルを共有する。同じモデルの敵を何体も出すときに、1 つを ImGui で調整すれば全員に反映できる
- 草の延焼を追加。草を生やす範囲を覆う燃え具合の地図を GPU で毎フレーム進め、隣の炎から熱を受けて燃え移る。風下ほど燃え広がりやすく、雨で濡れていると燃え広がりにくい。密度マップで草が無い所は燃えない。火をつけた所から燃え広がる距離 (`maxSpreadDistance`) を決められ、風下へは遠く、風上へは近くまで広がる。`RendererManager::SubmitGrassIgnition` で火をつけ、`ResetGrassFire` で燃え跡を消す。燃え方と見た目は `GetGrassFireSettings` で変える
- 燃え具合の地図を小さく縮めて CPU へ読み戻すようにした (`RendererManager::GetGrassFireReadback`)。ゲームが燃えている場所にライトや火の粉を置ける
- 草は燃えるにつれて枯れ、先端から炭になって縮む。地面は焦げて黒くなる

### 変更
- 草と地形のルートシグネチャに、燃え具合の地図 (草: t13 と b9、地形: t11 と b11) を足した。`GrassRenderer::Draw` と `TerrainRenderer::Draw` は地図と定数を受け取る
- 木を描ける数の上限を 2000 から 8000 に増やした。木 1 本は幹と葉で 2 つ数える

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
