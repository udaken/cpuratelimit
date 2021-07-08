#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT 1
#define STRICT_TYPED_ITEMIDS
// Windows ヘッダー ファイル
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <pathcch.h>
#include "wil/resource.h"

#if __cpp_designated_initializers
#define DESIGNED_INIT(member) member
#else
#define DESIGNED_INIT(member)
#endif
#define nameof(_a) # _a
