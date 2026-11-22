# 変更履歴

版ごとの変更点。ゲーム側で対応が必要な変更は「対応が必要」に書きます。

## 未リリース

### 追加
- 保存するビネットとは別に、ゲームの状態 (隠れているなど) を伝えるビネットを重ねる `PostEffectManager::SetOverlayVignette` を追加。強さ・色と、暗くなり始める距離・一番暗くなる距離を渡す。毎フレーム渡す値で、保存はしない。渡さなければ見た目は変わらない
- 草を生やす密度のしきい値 `GrassGenerationData::densityThreshold` を追加。密度マップがこの値以上の所に草を生やす。今までは 0.5 で固定だった
- 歩ける地形の外に見せる遠景の地形を描けるようにした。`Terrain::LoadFromHeightRatios` で CPU で作った高さから地形を作り、`SetDrawAsVista(true)` で遠景として描く。遠景は影を落とさず、草や植物の生成にも使わない。`ShareMaterialFrom` で歩ける地形と質感・模様の位置をそろえ、`SetHiddenArea` で歩ける地形の下にあるチャンクを省く
- `TerrainSettings` に、質感のテクスチャの UV を決める `texcoordOrigin` と `texcoordScale` を追加。今までの地形の見た目は変わらない
- `AnimationModel::ShareMaterialsFrom` を追加。`Model` と同じく、ほかの AnimationModel とマテリアルを共有する。同じモデルの敵を何体も出すときに、1 つを ImGui で調整すれば全員に反映できる
- 影を落とす距離を、カメラの描画距離と分けた (`LightManager` の「影の範囲」、初期値 120m)。カスケードはこの距離までを、手前ほど細かく割り振る (初期値 0.75)。今まではカメラの描画距離 (2000m) いっぱいに影の地図を広げていたため、手前でも 1 ピクセルが 36cm ほどになり、葉の影の形が出ていなかった。影は距離の手前 1 割でだんだん消える
- `AudioPlayer::SetVolume` を追加。鳴っている音の大きさを後から変える。3D の音では、距離による減衰がこれに掛かる。ループの音をゆっくり大きくしたり消したりするのに使う
- シーンを作り直せるようにした。`SceneManager::RegisterScene` にシーンを作る関数を渡して登録すると、`RequestSceneReload` でフェードアウトしてから今のシーンを捨てて作り直し、フェードインで見せる。古いシーンは、そのフレームの描画が終わった次のフレームの始めに捨てる。ステージを読み込み直すときに使う
- GlobalVariables が `Assets/Data/GlobalVariables/` の中のフォルダも読むようにした。`Stages/Stage1/Ground.json` は `"Stages/Stage1/Ground"` というグループになり、保存も同じ場所へ戻る
- ステージごとに設定を分けられるよう、保存するグループの名前を渡せるようにした。`BaseScene` (カメラ・ポストエフェクト・ライト)、`Environment` (時間・天候・霧と空)、`Skydome`。渡さなければ今までと同じ名前を使う
- `EnvironmentManager::LoadSettings` を追加。時間・天候・霧などの設定を、渡したグループから読み直す。始まりの天気 (`StartWeather`) も保存でき、読み直すとその天気から始まる
- 晴れのときの Volumetric Fog を、昼と夜で別に決められるようにした。空の色と同じく 5 ~ 7 時で夜から昼へ、17 ~ 19 時で昼から夜へ滑らかに移る。雨と雷雨は今までどおり天候ごとの霧を使い、天候の移り変わりに合わせて混ぜる。設定は環境の「時間帯」タブの「晴れのときの霧」。はじめは今までの晴れの霧と同じ値になる
- 霧の設定に、太陽と月の光が霧を照らす強さ (`FogProfile::sunScattering`) を足した。ディレクショナルライトの `volumetricScatteringIntensity` は毎フレームこの値で決まる
- 夜空に星を追加。空を立方体の 6 面のマス目に分け、マスごとの乱数で星を置くか・位置・明るさ・色 (青白から橙) を決める。星ごとに速さと位相をずらして瞬き、地平線に近いほど強く瞬く。太陽が沈むほど見え、雲の後ろと地平線の近くでは薄れる。時間とともに空と一緒に回る。設定は環境の「空」タブ
- 草の延焼を追加。草を生やす範囲を覆う燃え具合の地図を GPU で毎フレーム進め、隣の炎から熱を受けて燃え移る。風下ほど燃え広がりやすく、雨で濡れていると燃え広がりにくい。密度マップで草が無い所は燃えない。火をつけた所から燃え広がる距離 (`maxSpreadDistance`) を決められ、風下へは遠く、風上へは近くまで広がる。`RendererManager::SubmitGrassIgnition` で火をつけ、`ResetGrassFire` で燃え跡を消す。燃え方と見た目は `GetGrassFireSettings` で変える
- 燃え具合の地図を小さく縮めて CPU へ読み戻すようにした (`RendererManager::GetGrassFireReadback`)。ゲームが燃えている場所にライトや火の粉を置ける
- 草は燃えるにつれて枯れ、先端から炭になって縮む。地面は焦げて黒くなる

### 変更
- ステレオの音を `Play3D` で鳴らすと落ちていたのを直した。X3DAudio にチャンネルごとの向きを渡していなかったため。2ch 以上の音は 1 点から鳴る音として扱う。振り分けの表も、出力が 5.1ch などのときに足りるよう大きくした
- 草と地形のルートシグネチャに、燃え具合の地図 (草: t13 と b9、地形: t11 と b11) を足した。`GrassRenderer::Draw` と `TerrainRenderer::Draw` は地図と定数を受け取る
- 木を描ける数の上限を 2000 から 8000 に増やした。木 1 本は幹と葉で 2 つ数える

### 対応が必要
- `EnvironmentManager::Initialize` は設定を読まなくなった。`Environment` を作ると、そのときに `LoadSettings` で読む。`Environment` を作らずに `EnvironmentManager` の設定を使うゲームは、自分で `LoadSettings("Environment")` を呼ぶ
- `WeatherVisualParams` の霧の値は `fog` (`FogProfile`) にまとめた。`visual.extinction` などを読んでいるゲームは `visual.fog.extinction` に直す
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
