# FromSoheWare Engine 描画ガイド

本ドキュメントでは、FromSoheWare エンジンにおける各種オブジェクト（3Dモデル、アニメーションモデル、2Dスプライト、ライン）の描画方法と制御について解説します。
エンジンの描画システムは、複雑なレンダリング処理を隠蔽し、最小限の記述で対象を描画できるように設計されています。

---

## 目次
- [1. 3Dモデル (Model) の描画](#1-3dモデル-model-の描画)
- [2. アニメーションモデル (AnimationModel) の描画](#2-アニメーションモデル-animationmodel-の描画)
- [3. 2Dスプライト (Sprite) の描画](#3-2dスプライト-sprite-の描画)
- [4. ライン (Line) の描画](#4-ライン-line-の描画)

---

## 1. 3Dモデル (Model) の描画

静的な3Dモデルの描画には `FS::Model` クラスを使用します。

### 1-1. モデルの生成
オブジェクトの初期化時（コンストラクタなど）に、エンジンポインタと読み込み済みのモデル名を指定して生成します。

```cpp
#include "Model.h"

// メンバ変数として一意なポインタを持たせる
std::unique_ptr<FS::Model> model_;

// コンストラクタや Initialize() 内で生成
model_ = std::make_unique<FS::Model>(engine_, "bullet");