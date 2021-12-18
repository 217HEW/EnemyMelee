//=============================================================================
//
// 壁処理 [Wall.h]
//
//=============================================================================
#pragma once

#include "main.h"

#define MAX_WALL (256)

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct TWall {
	XMFLOAT3	m_pos;		// 現在の位置
	XMFLOAT3	m_rot;		// 現在の向き
	XMFLOAT3    m_size;		// 現在のサイズ
	XMFLOAT4X4	m_mtxWorld;	// ワールドマトリックス

	int			m_nLife;	// 壁の耐久置
	bool		use;		// 使用しているか
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT InitWall(void);
void UninitWall(void);
void UpdateWall(void);
void DrawWall(void);

int SetWall(XMFLOAT3 pos);

bool CollisionWall(XMFLOAT3 Apos, XMFLOAT3 Asize, XMFLOAT3 Bpos, XMFLOAT3 Bsize);



TWall* GetWall();


XMFLOAT3 GetPosWall(int i);
XMFLOAT3 GetSizeWall(int i);


XMFLOAT3 GetWallPos();
XMFLOAT3 GetWallSize();


