//=============================================================================
//
// 一時停止処理 [pause.cpp]
// Author : AKIRA TANAKA
//
//=============================================================================
#include "pause.h"
#include "input.h"
//#include "fade.h"
#include "polygon.h"
#include "Texture.h"
//#include "Sound.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	NUM_PAUSE_MENU		(3)			// ポーズメニュー数
#define	PAUSE_MENU_WIDTH	(320.0f)	// ポーズメニュー幅
#define	PAUSE_MENU_HEIGHT	(60.0f)		// ポーズメニュー高さ
#define	PAUSE_MENU_POS_X	(0.0f)		// ポーズメニュー位置(X座標)
#define	PAUSE_MENU_POS_Y	(PAUSE_MENU_INTERVAL)	// ポーズメニュー位置(Y座標)
#define	PAUSE_MENU_INTERVAL	(100.0f)	// ポーズメニュー間隔
#define	PLATE_WIDTH			(360.0f)	// プレートの幅
#define	PLATE_HEIGHT		(340.0f)	// プレートの幅
#define	PLATE_POS_X			(0.0f)		// プレートの位置(X座標)
#define	PLATE_POS_Y			(0.0f)		// プレートの位置(Y座標)

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static ID3D11ShaderResourceView*	g_pTextures[3] = { nullptr };	// テクスチャへのポインタ

static PAUSE_MENU g_nSelectMenu = PAUSE_MENU_CONTINUE;		// 選択中のメニューNo.
static float g_fCurve = 0.0f;
static float g_fCol = 0.0f;

static LPCWSTR c_aFileNamePauseMenu[NUM_PAUSE_MENU] =
{
	L"data/texture/pause000.png",	// コンティニュー
	L"data/texture/pause001.png",	// リトライ
	L"data/texture/pause002.png",	// クイット
};

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitPause(void)
{
	ID3D11Device* pDevice = GetDevice();
	HRESULT hr = S_OK;

	for (int nCntPauseMenu = 0; nCntPauseMenu < NUM_PAUSE_MENU; ++nCntPauseMenu) {
		// テクスチャの読み込み
		hr = CreateTextureFromFile(pDevice,									// デバイスへのポインタ
			c_aFileNamePauseMenu[nCntPauseMenu],	// ファイルの名前
			&g_pTextures[nCntPauseMenu]);			// 読み込むメモリー
	}

	g_nSelectMenu = PAUSE_MENU_CONTINUE;
	g_fCurve = 0.0f;

	// 効果音初期化
	//g_pSE_Select = CreateSound(SE_SELECT_PATH);

	return hr;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitPause(void)
{
	// テクスチャの開放
	for (int nCntPauseMenu = 0; nCntPauseMenu < NUM_PAUSE_MENU; ++nCntPauseMenu) {
		SAFE_RELEASE(g_pTextures[nCntPauseMenu]);
	}
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdatePause(void)
{
	if (GetKeyRepeat(VK_W) || GetKeyRepeat(VK_UP)) {
		//CSound::Play(SE_SELECT);
		g_nSelectMenu = (PAUSE_MENU)((g_nSelectMenu + NUM_PAUSE_MENU - 1) % NUM_PAUSE_MENU);
		SetPauseMenu();
	}
	else if (GetKeyRepeat(VK_S) || GetKeyRepeat(VK_DOWN)) {
		//CSound::Play(SE_SELECT);
		g_nSelectMenu = (PAUSE_MENU)((g_nSelectMenu + 1) % NUM_PAUSE_MENU);
		SetPauseMenu();
	}

	g_fCurve += XM_PI * 0.01f;
	if (g_fCurve > XM_PI) {
		g_fCurve -= XM_2PI;
	}

	// 反射光の設定
	g_fCol = cosf(g_fCurve) * 0.2f + 0.8f;
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawPause(void)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();

	SetPolygonTexture(nullptr);
	SetPolygonSize(PLATE_WIDTH, PLATE_HEIGHT);
	SetPolygonPos(PLATE_POS_X, PLATE_POS_Y);
	SetPolygonFrameSize(1.0f, 1.0f);
	SetPolygonUV(0.0f, 0.0f);
	SetPolygonColor(g_fCol, g_fCol, g_fCol);
	SetPolygonAlpha(1.0f);
	DrawPolygon(pDeviceContext);

	SetPolygonSize(PAUSE_MENU_WIDTH, PAUSE_MENU_HEIGHT);
	for (int nCntPauseMenu = 0; nCntPauseMenu < NUM_PAUSE_MENU; ++nCntPauseMenu) {
		SetPolygonPos(PAUSE_MENU_POS_X, PAUSE_MENU_POS_Y - nCntPauseMenu * PAUSE_MENU_INTERVAL);

		if (nCntPauseMenu == g_nSelectMenu) {
			SetPolygonColor(1.0f, 1.0f, 1.0f);
		}
		else {
			SetPolygonColor(0.3f, 0.3f, 0.3f);
		}
		// テクスチャの設定
		SetPolygonTexture(g_pTextures[nCntPauseMenu]);
		// ポリゴンの描画
		DrawPolygon(pDeviceContext);
	}

	SetPolygonColor(1.0f, 1.0f, 1.0f);
}

//=============================================================================
// ポーズメニューの設定
//=============================================================================
void SetPauseMenu(void)
{
	g_fCurve = 0.0f;
}

//=============================================================================
// ポーズメニューの取得
//=============================================================================
PAUSE_MENU GetPauseMenu(void)
{
	return g_nSelectMenu;
}

//=============================================================================
// ポーズメニューのリセット
//=============================================================================
void ResetPauseMenu(void)
{
	g_nSelectMenu = PAUSE_MENU_CONTINUE;
	//CSound::Play(SE_SELECT);
	SetPauseMenu();
}
