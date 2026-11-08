# 衝突判定 (Collision)

球・AABB のコライダー、全コライダーの総当たり判定、レイキャスト、視錐台を扱います。

| 名前 | 種類 | ヘッダー | 説明 (ヘッダーのコメントより) |
| :--- | :--- | :--- | :--- |
| {ref}`FS::Collider <fs-collider>` | class | `Engine/Collision/Collider.h` |  |
| {ref}`FS::RaycastHit <fs-raycasthit>` | struct | `Engine/Collision/CollisionManager.h` |  |
| {ref}`FS::CollisionManager <fs-collisionmanager>` | class | `Engine/Collision/CollisionManager.h` | コライダー同士の衝突判定を管理するクラス |
| {ref}`FS::Plane <fs-plane>` | struct | `Engine/Collision/Frustum.h` | 視界を構成する平面 |
| {ref}`FS::Frustum <fs-frustum>` | struct | `Engine/Collision/Frustum.h` | 視界錐台 |

(fs-collider)=
## FS::Collider

ヘッダー: `Engine/Collision/Collider.h`

```{doxygenclass} FS::Collider
:members:
:protected-members:
:undoc-members:
```

(fs-raycasthit)=
## FS::RaycastHit

ヘッダー: `Engine/Collision/CollisionManager.h`

```{doxygenstruct} FS::RaycastHit
:members:
:protected-members:
:undoc-members:
```

(fs-collisionmanager)=
## FS::CollisionManager

コライダー同士の衝突判定を管理するクラス

ヘッダー: `Engine/Collision/CollisionManager.h`

```{doxygenclass} FS::CollisionManager
:members:
:protected-members:
:undoc-members:
```

(fs-plane)=
## FS::Plane

視界を構成する平面

ヘッダー: `Engine/Collision/Frustum.h`

```{doxygenstruct} FS::Plane
:members:
:protected-members:
:undoc-members:
```

(fs-frustum)=
## FS::Frustum

視界錐台

ヘッダー: `Engine/Collision/Frustum.h`

```{doxygenstruct} FS::Frustum
:members:
:protected-members:
:undoc-members:
```

## 列挙型

### FS::CollisionShapeType

ヘッダー: `Engine/Collision/Collider.h`

```{doxygenenum} FS::CollisionShapeType
```
