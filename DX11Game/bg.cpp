// 背景 [bg.cpp]
#include "bg.h"
#include "Texture.h"
#include "polygon.h"

// マクロ定義
#define PATH_BGTEXTURE	L"data/texture/sky001.jpg"
#define BG_POS_X		0.0f
#define BG_POS_Y		0.0f
#define BG_WIDTH		SCREEN_WIDTH
#define BG_HEIGHT		SCREEN_HEIGHT

// グローバル変数
static ID3D11ShaderResourceView* g_pTexture;

// 初期化
HRESULT InitBG()
{
	HRESULT hr = S_OK;
	ID3D11Device* pDevice = GetDevice();

	// テクスチャ読込
	hr = CreateTextureFromFile(pDevice, PATH_BGTEXTURE, &g_pTexture);

	return hr;
}

// 終了処理
void UninitBG()
{
	// 背景テクスチャ解放
	SAFE_RELEASE(g_pTexture);
}

// 更新
void UpdateBG()
{
	// 何もしない
}

// 描画
void DrawBG()
{
	ID3D11DeviceContext* pDC = GetDeviceContext();
	SetPolygonSize(BG_WIDTH, BG_HEIGHT);
	SetPolygonPos(BG_POS_X, BG_POS_Y);
	SetPolygonTexture(g_pTexture);
	SetPolygonUV(0.0f, 0.0f);
	DrawPolygon(pDC);
}
