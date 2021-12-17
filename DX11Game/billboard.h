//=============================================================================
//
// 2D処理 [Billboard.h]
//
//=============================================================================
#pragma once

#include "main.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	MAX_BILLBOARD				(256)	// 最大数

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct TBillboard {
	XMFLOAT3	pos;		// 位置
	XMFLOAT4	col;		// 色
	float		width;		// 幅
	float		height;		// 高さ
	int			idxShadow;	// 影ID
	bool		use;		// 使用しているかどうか
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT InitBillboard(void);
void UninitBillboard(void);
void UpdateBillboard(void);
void DrawBillboard(void);

int SetBillboard(XMFLOAT3 pos, float width, float height, XMFLOAT4 col);
