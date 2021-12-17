//=============================================================================
//
// メッシュ地面処理 [meshfield.h]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#pragma once

#include "main.h"

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT InitMeshField(int nNumBlockX, int nNumBlockZ, float fSizeBlockX, float fSizeBlockZ, float fTexSizeX = 1.0f, float fTexSizeZ = 1.0f);
void UninitMeshField(void);
void UpdateMeshField(void);
void DrawMeshField(void);
