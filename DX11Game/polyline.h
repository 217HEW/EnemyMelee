//=============================================================================
//
// ポリライン処理 [polyline.h]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#pragma once

#include <vector>
#include "main.h"

// マクロ定義
#define MAX_POLYLINE_VERTEX		1024			// 最大頂点数

// 構造体定義
struct TPolyline {
	ID3D11Buffer* pVertexBuffer;				// 頂点バッファ インターフェースへのポインタ
	int nNumVertex;								// 総頂点数

	DirectX::XMFLOAT4X4 mtxWorld;				// ワールドマトリックス
	DirectX::XMFLOAT3 pos;						// ポリゴン表示位置の中心座標
	DirectX::XMFLOAT3 rot;						// ポリゴンの回転角

	DirectX::XMFLOAT4X4 mtxTexture;				// テクスチャ マトリックス
	ID3D11ShaderResourceView* pTexture;			// テクスチャ

	std::vector<DirectX::XMFLOAT3> pointList;	// 座標リスト
	float fWidth;								// 線幅
	bool bRound;								// 角丸
	DirectX::XMFLOAT4 cDiffuse;					// 拡散反射色
	EBlendState bs;								// 半透明合成

	VERTEX_3D	vertex[MAX_POLYLINE_VERTEX];	// 頂点配列
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT InitPolyline(void);
void UninitPolyline(void);
void UpdatePolyline(TPolyline* pPolyline);
void DrawPolyline(TPolyline* pPolyline);

HRESULT CreatePolyline(TPolyline* pPolyline, float fWidth = 1.0f, bool bRound = true,
	DirectX::XMFLOAT4 vColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), EBlendState bs = BS_ALPHABLEND);
void ReleasePolyline(TPolyline* pPolyline);
void ClearPolylinePoint(TPolyline* pPolyline);
void AddPolylinePoint(TPolyline* pPolyline, DirectX::XMFLOAT3 pos);
