//=============================================================================
//
// 弾 [bullet.h]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#pragma once

#include "main.h"

//**************************************
// 種類
//**************************************
enum EBulletType {
	BULLETTYPE_PLAYER = 0,		// プレイヤーの弾
	BULLETTYPE_ENEMY,			// 敵の弾
	BULLETTYPE_MAX
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT InitBullet(void);
void UninitBullet(void);
void UpdateBullet(void);
void DrawBullet(void);

int FireBullet(XMFLOAT3 pos, XMFLOAT3 dir, EBulletType type);
