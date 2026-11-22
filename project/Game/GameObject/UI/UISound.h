#pragma once
#include "PropertyBinder.h"

// メニューの操作音
namespace UISound
{

// 大きさはポーズメニューの設定に保存
void BindProperties(FS::PropertyBinder& binder);
void DrawProperties(FS::PropertyBinder& binder);

void PlayConfirm();
void PlayCancel();
void PlayCursor();
void PlayPauseOpen();
void PlayPauseClose();

}
