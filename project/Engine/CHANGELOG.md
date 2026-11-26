# 変更履歴

版ごとの変更点。ゲーム側で対応が必要な変更は「対応が必要」に書きます。

## 未リリース

### 追加
- `ParticleSystem::ReleaseAttractionTarget` を追加。粒は `ParticleEmitter::SetAttractionTarget` で渡した `WorldTransform` を指したまま飛ぶので、その持ち主が先に消えると、消えたメモリを読んで落ちることがあった。持ち主を消す前 (デストラクタなど) に呼ぶと、そこへ向かっている粒を消す
- `RendererManager::SetSpriteDrawEnabled` を追加。false の間はスプライトと文字を描かない。「全体のデバッグ情報・設定」の「スプライト」で切り替えられる
- `EnvironmentManager::SetTimeFlowEnabled` を追加。false の間は時刻を自動で進めないので、ゲーム側で `SetTimeOfDay` を呼んで時刻を決められる
- `EnvironmentManager::RequestWeatherChange` をゲームから呼べるようにした (今まで private)。すぐに天候を変える `SetWeatherImmediately` も追加
- 夕方の移り変わりの時刻 (夕焼けになり始める・日が沈む・夜になりきる) を、環境の設定 `DuskStartHour` / `SunsetHour` / `NightHour` で変えられるようにした。標準は 17 / 18 / 19 時で、今までと同じ見た目になる。太陽の向き・時間帯の色・霧はこの時刻に合わせて動き、時計の時刻 (`GetTimeOfDay`) はそのまま。見た目の時刻は `GetVisualTimeOfDay` で取れる
- シーンの途中で画面演出を重ねる仕組みを追加。放射ブラー・色収差・画面の縁のビネット・ピクセル化・砂嵐の値を `ScreenEffectValues` で渡す。シーンの設定で切っている効果にもかけられ、重なったときは効果ごとに強いほうを使う。シーンを作り直すと全部消える。「ポストエフェクト設定」の「一時的に重ねる演出の確認」で試せる
  - `PostEffectManager::PlayScreenEffect`: `ScreenEffectRequest` で決めた長さ (フェードイン・フェードアウト付き) だけかけ、終わると自動で消える。長さを 0 以下にすると `StopScreenEffect` を呼ぶまでかけ続ける。`pulseFrequency` / `pulseDepth` / `pulseRandomness` で強弱を繰り返したり震わせたりできる。ポーズやヒットストップ中も進めるか (`useUnscaledTime`) を選べる
  - `PostEffectManager::AddFrameScreenEffect`: そのフレームだけ重ねる。毎フレーム呼んでいる間だけかかるので、値の動きをゲームが自由に計算できる
- `PropertyBinder::Draw` で出した項目を右クリックすると、「最後に保存した値に戻す」「初期値に戻す」が選べるようにした。エディターを起動し直さなくても戻せる。押し間違えないよう、戻した先の値をメニューに出し、今と同じ値のときは押せない。戻したあとは「戻す前の値に戻す」で取り消せる。戻してもファイルには書かない。ゲーム側の変更は要らない
- 「パラメータの保存」ウィンドウに、まとめて最後に保存した値に戻すボタンを追加。一番上の「すべてを最後に保存した値に戻す」で全部、グループごとの「セーブ」の横のボタンでそのグループだけを戻す。押すと戻る項目と値の一覧を出し、確かめてから戻す (最初は「やめる」が選ばれている)。戻したあとは「まとめて戻したのを取り消す」で取り消せる。ファイルには書かない
- `GlobalVariables` に、型を問わずに値を読み書きする `GetAnyValue` / `SetAnyValue` と、ファイルに保存されている値だけを読む `LoadSavedValue` / `LoadSavedGroup` を追加

### 変更
- パッドとキーボードのどちらで遊んでいるかを、キーやマウスのボタンを押し続けているかではなく、押した瞬間で判定するようにした。押されたままと読めるキーがあると、パッドから手を離すたびにキーボードの表示へ戻っていた
- v1.2.0 で CHANGELOG の中身が消えていたのを元に戻した。v1.2.0 で変わった内容は下の「v1.2.0」に書いてある
- 画面演出 (ビネット・色収差・ピクセル化・放射ブラー・砂嵐) の設定を、GPU に渡す定数バッファから `ScreenEffectSettings` に分けた。GPU に渡す値は、設定と重ねている演出から描く直前に毎フレーム作る。演出のために変えた値が保存されることも、Map したままの遅いメモリを ImGui が毎フレーム読むこともなくなった。保存のキーは今までと同じ
- エディター画面のフォントに、和文の記号 (「」『』、。〜)・一般の句読点 (…)・矢印 (→)・図形 (◆)・その他の記号 (★) の文字を足した。今までは「」などが ? で表示されていた
- 確認のポップアップ (モーダル) を出している間に後ろへかぶせる色を、ImGui の初期設定の薄い白から、半透明の黒に変えた。後ろの画面が白くぼやけず、ポップアップに目が行く
- `SetOverlayVignette` の値も描く直前に合わせるようにした。`PlayScreenEffect` のビネットと重なったときは、強いほうを使う

### 対応が必要
- `PropertyBinder` をコピー・移動できないようにした。まとめて元に戻すために、作られている `PropertyBinder` のアドレスを覚えておくため。今までどおり `std::unique_ptr` などで持つ
- `PostEffectManager::GetPostEffectData` を消した。GPU に渡す値は毎フレーム作り直すので、書き換えても次のフレームで戻る。ずっと変えるなら `GetScreenEffectSettings` (保存される値)、一時的にかけるなら `PlayScreenEffect` を使う

## v1.2.0 (2026-11-23)

### 追加

#### 草の延焼
- 草の延焼を追加。草を生やす範囲を覆う燃え具合の地図 (1024×1024) を GPU で毎フレーム進め、隣の炎から熱を受けて燃え移る。風下ほど燃え広がりやすく、雨で濡れていると燃え広がりにくい。密度マップで草が無い所は燃えない。火をつけた所から燃え広がる距離 (`maxSpreadDistance`) を決められ、風下へは遠く、風上へは近くまで広がる。`RendererManager::SubmitGrassIgnition` で火をつけ、`ResetGrassFire` で燃え跡を消す。燃え方と見た目は `GetGrassFireSettings` で変える
- 草は燃えるにつれて枯れ、先端から炭になって縮む。地面は焦げて黒くなる
- `GrassFireConstants::rainSpreadDistanceRate` を追加。地面が濡れきったときに、燃え広がる距離に掛ける割合 (初期値 0.35)。燃え移るたびに判定するので、燃えている途中で降り出した雨にも効く。今まで使っていなかった `charPadding` の場所を使うので、定数の並びは変わらない
- 燃え具合の地図を 256×256 に縮めて CPU へ読み戻すようにした。炎の強さはマスの中の最大値、燃え進み具合はマスの中の平均値で縮める。1 フレーム遅れる
  - `RendererManager::GetGrassFireReadback`: 炎の強さの地図。ゲームが燃えている場所にライトや火の粉を置ける
  - `RendererManager::SampleGrassFire`: その場所の炎の強さ (0 ~ 1)。燃えている所にいるキャラクターにダメージを与えるのに使う
  - `RendererManager::SampleGrassBurnt`: その場所の燃え進み具合 (0 は燃えていない、1 は燃え尽きた)。焼けた草むらに隠れられないようにするのに使う

#### 環境・空
- 晴れのときの Volumetric Fog を、昼と夜で別に決められるようにした。空の色と同じく 5 ~ 7 時で夜から昼へ、17 ~ 19 時で昼から夜へ滑らかに移る。雨と雷雨は今までどおり天候ごとの霧を使い、天候の移り変わりに合わせて混ぜる。設定は環境の「時間帯」タブの「晴れのときの霧」。はじめは今までの晴れの霧と同じ値になる
- 霧の設定に、太陽と月の光が霧を照らす強さ (`FogProfile::sunScattering`) を足した。ディレクショナルライトの `volumetricScatteringIntensity` は毎フレームこの値で決まる
- 夜空に星を追加。空を立方体の 6 面のマス目に分け、マスごとの乱数で星を置くか・位置・明るさ・色 (青白から橙) を決める。星ごとに速さと位相をずらして瞬き、地平線に近いほど強く瞬く。太陽が沈むほど見え、雲の後ろと地平線の近くでは薄れる。時間とともに空と一緒に回る。設定は環境の「空」タブ
- `EnvironmentManager::LoadSettings` を追加。時間・天候・霧などの設定を、渡したグループから読み直す。始まりの天気 (`StartWeather`) も保存でき、読み直すとその天気から始まる

#### 描画
- 影を落とす距離を、カメラの描画距離と分けた (`LightManager` の「影の範囲」、初期値 300m)。カスケードはこの距離までを、手前ほど細かく割り振る (初期値 0.75)。今まではカメラの描画距離 (2000m) いっぱいに影の地図を広げていたため、手前でも 1 ピクセルが 36cm ほどになり、葉の影の形が出ていなかった。影は距離の手前 1 割でだんだん消える
- 歩ける地形の外に見せる遠景の地形を描けるようにした。`Terrain::LoadFromHeightRatios` で CPU で作った高さから地形を作り、`SetDrawAsVista(true)` で遠景として描く。遠景は影を落とさず、草や植物の生成にも使わない。`ShareMaterialFrom` で歩ける地形と質感・模様の位置をそろえ、`SetHiddenArea` で歩ける地形の下にあるチャンクを省く
- `TerrainSettings` に、質感のテクスチャの UV を決める `texcoordOrigin` と `texcoordScale` を追加。今までの地形の見た目は変わらない
- 草を生やす密度のしきい値 `GrassGenerationData::densityThreshold` を追加。密度マップがこの値以上の所に草を生やす。今までは 0.5 で固定だった
- 保存するビネットとは別に、ゲームの状態 (隠れているなど) を伝えるビネットを重ねる `PostEffectManager::SetOverlayVignette` を追加。強さ・色と、暗くなり始める距離・一番暗くなる距離を渡す。毎フレーム渡す値で、保存はしない。渡さなければ見た目は変わらない
- `AnimationModel::ShareMaterialsFrom` を追加。`Model` と同じく、ほかの AnimationModel とマテリアルを共有する。同じモデルの敵を何体も出すときに、1 つを ImGui で調整すれば全員に反映できる

#### シーン・設定の保存
- シーンを作り直せるようにした。`SceneManager::RegisterScene` にシーンを作る関数を渡して登録すると、`RequestSceneReload` でフェードアウトしてから今のシーンを捨てて作り直し、フェードインで見せる。古いシーンは、そのフレームの描画が終わった次のフレームの始めに捨てる。ステージを読み込み直すときに使う
- GlobalVariables が `Assets/Data/GlobalVariables/` の中のフォルダも読むようにした。`Stages/Stage1/Ground.json` は `"Stages/Stage1/Ground"` というグループになり、保存も同じ場所へ戻る
- ステージごとに設定を分けられるよう、保存するグループの名前を渡せるようにした。`BaseScene` (カメラ・ポストエフェクト・ライト)、`Environment` (時間・天候・霧と空)、`Skydome`。渡さなければ今までと同じ名前を使う

#### 音
- `AudioPlayer::SetVolume` を追加。鳴っている音の大きさを後から変える。3D の音では、距離による減衰がこれに掛かる。ループの音をゆっくり大きくしたり消したりするのに使う
- `AudioPlayer::HasSound` を追加。その名前の音を読み込んでいるかを返す。素材を置いたときだけ鳴らす音に使う

### 変更
- シェーダーの共通のヘッダー (`.hlsli`) を変えたときも、シェーダーをコンパイルし直すようにした。今までは `.hlsl` の時刻だけを見ていたので、`ShaderConstants.hlsli` の定数の並びを変えても古い `.cso` が使われ、CPU から渡す値とずれて描画が壊れることがあった。どれかの `.hlsli` が `.cso` より新しければ、全部のシェーダーを作り直す (最初の起動だけ時間がかかる)
- ステレオの音を `Play3D` で鳴らすと落ちていたのを直した。X3DAudio にチャンネルごとの向きを渡していなかったため。2ch 以上の音は 1 点から鳴る音として扱う。振り分けの表も、出力が 5.1ch などのときに足りるよう大きくした
- 草と地形のルートシグネチャに、燃え具合の地図 (草: t13 と b9、地形: t11 と b11) を足した。`GrassRenderer::Draw` と `TerrainRenderer::Draw` は地図と定数を受け取る
- `RendererManager::SetGrassFireReadback` は、炎の強さの地図に加えて燃え進み具合の地図も受け取るようにした。描画の流れ (`RenderPipeline`) から呼ぶもので、ゲームからは呼ばない
- 木を描ける数の上限を 2000 から 8000 に増やした。木 1 本は幹と葉で 2 つ数える
- 置ける霧 (Volume Fog) の上限を 8 から 32 に増やした (`MAX_FOG_VOLUMES`)。エディタで置いた霧と、`VolumetricFogPass::SubmitFogVolume` でゲームから渡した霧を合わせた数。重さは上限ではなく、そのフレームに実際に渡した霧の数と大きさで決まる

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
