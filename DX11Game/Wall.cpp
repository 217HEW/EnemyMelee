//=============================================================================
//
// 壁処理 [Wall.cpp]
//
//=============================================================================
#include "Wall.h"
#include "main.h"
#include "AssimpModel.h"
#include "debugproc.h"
#include "collision.h"
#include "player.h"
#include "explosion.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MODEL_WALL			"data/model/wall002.X"

#define MAX_LIFE			(2)			// 壁耐久置
#define MAX_WALL			(256)		// 壁最大数


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
// グローバル変数
//*****************************************************************************
static CAssimpModel	g_model;			// モデル
static TWall		g_wall[MAX_WALL];	// 壁情報

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitWall(void)
{
	HRESULT hr = S_OK;
	ID3D11Device* pDevice = GetDevice();
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();

	for (int i = 0; i < MAX_WALL; ++i)
	{
		g_wall[i].m_size = XMFLOAT3(38.0f, 38.0f, 38.0f);
		//g_wall->m_pos = XMFLOAT3(0.0f, 50.0f, 150.0f);
		g_wall[i].m_nLife = MAX_LIFE;
		g_wall[i].use = false;

	}

	// モデルデータの読み込み
	if (!g_model.Load(pDevice, pDeviceContext, MODEL_WALL)) {
		MessageBoxA(GetMainWnd(), "モデルデータ読み込みエラー", "InitWall", MB_OK);
		return E_FAIL;
	}

	return hr;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitWall(void)
{
	// モデルの解放
	g_model.Release();
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdateWall(void)
{

	XMMATRIX mtxWorld, mtxRot, mtxTranslate;

	for (int i = 0; i < MAX_WALL; ++i)
	{
		// ワールドマトリックスの初期化
		mtxWorld = XMMatrixIdentity();

		// 回転を反映
		mtxRot = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(g_wall[i].m_rot.x),
			XMConvertToRadians(g_wall[i].m_rot.y),
			XMConvertToRadians(g_wall[i].m_rot.z));
		mtxWorld = XMMatrixMultiply(mtxWorld, mtxRot);

		// 移動を反映
		mtxTranslate = XMMatrixTranslation(
			g_wall[i].m_pos.x,
			g_wall[i].m_pos.y,
			g_wall[i].m_pos.z);
		mtxWorld = XMMatrixMultiply(mtxWorld, mtxTranslate);

		// ワールドマトリックス設定
		XMStoreFloat4x4(&g_wall[i].m_mtxWorld, mtxWorld);

	}

	for (int i = 0; i < MAX_WALL; ++i)
	{
		if (!g_wall[i].use)
		{// 未使用なら次へ
			continue;
		}

		// 壁とプレイヤーが衝突していたら
		if (CollisionWall(g_wall[i].m_pos, g_wall[i].m_size, GetPlayerPos(), GetPlayerSize()))
		{
			StartExplosion(g_wall[i].m_pos, XMFLOAT2(80.0f, 80.0f));
			g_wall[i].m_nLife--;

			if (g_wall[i].m_nLife <= 0)
			{
				g_wall[i].use = false;
			}
		}
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawWall(void)
{
	ID3D11DeviceContext* pDC = GetDeviceContext();


	// 不透明部分を描画
	for (int i = 0; i < MAX_WALL; ++i)
	{
		if (!g_wall[i].use)
		{
			continue;
		}
		g_model.Draw(pDC, g_wall[i].m_mtxWorld, eOpacityOnly);
	}

	// 半透明部分を描画
	for (int i = 0; i < MAX_WALL; ++i) {
		SetBlendState(BS_ALPHABLEND);	// アルファブレンド有効
		SetZWrite(false);				// Zバッファ更新しない
		g_model.Draw(pDC, g_wall[i].m_mtxWorld, eTransparentOnly);
		SetZWrite(true);				// Zバッファ更新する
		SetBlendState(BS_NONE);			// アルファブレンド無効
	}
}

//===============================================================================
//
//  　　壁配置
//
//===============================================================================
int SetWall(XMFLOAT3 pos)
{
	int Wall = -1;

	for (int cntWall = 0; cntWall < MAX_WALL; ++cntWall) {
		// 使用中ならスキップ
		if (g_wall[cntWall].use) {
			continue;
		}
		g_wall[cntWall].use = true;
		g_wall[cntWall].m_pos = pos;

		Wall = cntWall;
		break;
	}

	return Wall;
}

// 壁位置取得
XMFLOAT3 GetPosWall(int i)
{
	return g_wall[i].m_pos;
}

// 壁サイズ取得
XMFLOAT3 GetSizeWall(int i)
{
	return g_wall[i].m_size;
}


// 壁位置取得
XMFLOAT3 GetWallPos()
{
	return g_wall->m_pos;
}

// 壁サイズ取得
XMFLOAT3 GetWallSize()
{
	return g_wall->m_size;
}

//=============================================================================
// 衝突判定
//=============================================================================
bool CollisionWall(XMFLOAT3 Apos, XMFLOAT3 Asize, XMFLOAT3 Bpos, XMFLOAT3 Bsize)
{
	
	return CollisionAABB(Apos, Asize, Bpos, Bsize);

}