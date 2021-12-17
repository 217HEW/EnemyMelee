//=============================================================================
//
// エフェクト処理 [effect.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "effect.h"
#include "Texture.h"
#include "mesh.h"
#include "input.h"
#include "Camera.h"
#include "Light.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_EFFECT			L"data/TEXTURE/effect000.jpg"	// 読み込むテクスチャファイル名
#define	EFFECT_SIZE_X			(50.0f)							// ビルボードの幅
#define	EFFECT_SIZE_Y			(50.0f)							// ビルボードの高さ
#define	VALUE_MOVE_BULLET		(2.0f)							// 移動速度

#define	MAX_EFFECT				(4096)							// エフェクト最大数

#define MATERIAL_DIFFUSE		XMFLOAT4(1,1,1,1)
#define MATERIAL_AMBIENT		XMFLOAT4(0,0,0,1)
#define MATERIAL_SPECULAR		XMFLOAT4(0,0,0,1)
#define MATERIAL_EMISSIVE		XMFLOAT4(0,0,0,1)
#define MATERIAL_POWER			(1.0f)

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct TEffect {
	XMFLOAT3 pos;			// 位置
	XMFLOAT3 move;			// 移動量
	XMFLOAT4 col;			// 色
	XMFLOAT2 size;			// 大きさ
	int nTimer;				// タイマー
	float fDecAlpha;		// 減衰値
	bool use;				// 使用しているかどうか
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT MakeVertexEffect(ID3D11Device* pDevice);
void SetVertexEffect(int nIdxEffect, XMFLOAT2 size);
void SetColorEffect(int nIdxEffect, XMFLOAT4 col);

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static MESH				g_mesh;
static MATERIAL			g_material;
static TEffect			g_effect[MAX_EFFECT];	// エフェクト情報

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitEffect(void)
{
	ID3D11Device* pDevice = GetDevice();

	// 頂点情報の作成
	MakeVertexEffect(pDevice);

	// テクスチャの読み込み
	CreateTextureFromFile(pDevice,				// デバイスへのポインタ
						  TEXTURE_EFFECT,		// ファイルの名前
						  &g_mesh.pTexture);	// 読み込むメモリー
	XMStoreFloat4x4(&g_mesh.mtxTexture, XMMatrixIdentity());

	// マテリアルの設定
	g_material.Diffuse = MATERIAL_DIFFUSE;
	g_material.Ambient = MATERIAL_AMBIENT;
	g_material.Specular = MATERIAL_SPECULAR;
	g_material.Emissive = MATERIAL_EMISSIVE;
	g_material.Power = MATERIAL_POWER;
	g_mesh.pMaterial = &g_material;

	for (int nCntEffect = 0; nCntEffect < MAX_EFFECT; ++nCntEffect) {
		g_effect[nCntEffect].pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		g_effect[nCntEffect].move = XMFLOAT3(1.0f, 1.0f, 1.0f);
		g_effect[nCntEffect].size = XMFLOAT2(EFFECT_SIZE_X, EFFECT_SIZE_Y);
		g_effect[nCntEffect].nTimer = 0;
		g_effect[nCntEffect].use = false;
	}

	return S_OK;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitEffect(void)
{
	// メッシュの開放
	ReleaseMesh(&g_mesh);
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdateEffect(void)
{
	for (int nCntEffect = 0; nCntEffect < MAX_EFFECT; ++nCntEffect) {
		if (g_effect[nCntEffect].use) {
			g_effect[nCntEffect].pos.x += g_effect[nCntEffect].move.x;
			g_effect[nCntEffect].pos.z += g_effect[nCntEffect].move.z;

			g_effect[nCntEffect].col.w -= g_effect[nCntEffect].fDecAlpha;
			if (g_effect[nCntEffect].col.w <= 0.0f) {
				g_effect[nCntEffect].col.w = 0.0f;
			}
			SetColorEffect(nCntEffect, g_effect[nCntEffect].col);

			--g_effect[nCntEffect].nTimer;
			if (g_effect[nCntEffect].nTimer <= 0) {
				g_effect[nCntEffect].use = false;
			}
		}
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawEffect(void)
{
	ID3D11DeviceContext* pDC = GetDeviceContext();
	XMMATRIX mtxWorld, mtxScale, mtxTranslate;

	CLight::Get()->SetDisable();
	SetBlendState(BS_ADDITIVE);		// 加算合成
	SetZWrite(false);	// 半透明描画はZバッファを更新しない(Zチェックは行う)

	// ビューマトリックスを取得
	XMFLOAT4X4& mtxView = CCamera::Get()->GetViewMatrix();

	for (int nCntEffect = 0; nCntEffect < MAX_EFFECT; ++nCntEffect) {
		if (g_effect[nCntEffect].use) {
			// ワールドマトリックスの初期化
			mtxWorld = XMMatrixIdentity();
			XMStoreFloat4x4(&g_mesh.mtxWorld, mtxWorld);

			g_mesh.mtxWorld._11 = mtxView._11;
			g_mesh.mtxWorld._12 = mtxView._21;
			g_mesh.mtxWorld._13 = mtxView._31;
			g_mesh.mtxWorld._21 = mtxView._12;
			g_mesh.mtxWorld._22 = mtxView._22;
			g_mesh.mtxWorld._23 = mtxView._32;
			g_mesh.mtxWorld._31 = mtxView._13;
			g_mesh.mtxWorld._32 = mtxView._23;
			g_mesh.mtxWorld._33 = mtxView._33;

			mtxWorld = XMLoadFloat4x4(&g_mesh.mtxWorld);

			// スケールを反映
			mtxScale = XMMatrixScaling(g_effect[nCntEffect].size.x, g_effect[nCntEffect].size.y, 1.0f);
			mtxWorld = XMMatrixMultiply(mtxScale, mtxWorld);

			// 移動を反映
			mtxTranslate = XMMatrixTranslation(g_effect[nCntEffect].pos.x, g_effect[nCntEffect].pos.y, g_effect[nCntEffect].pos.z);
			mtxWorld = XMMatrixMultiply(mtxWorld, mtxTranslate);

			// ワールドマトリックスの設定
			XMStoreFloat4x4(&g_mesh.mtxWorld, mtxWorld);

			// カラーの設定
			g_material.Diffuse = g_effect[nCntEffect].col;

			// メッシュの描画
			DrawMesh(pDC, &g_mesh);
		}
	}

	SetZWrite(true);
	SetBlendState(BS_NONE);
	CLight::Get()->SetEnable();
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
HRESULT MakeVertexEffect(ID3D11Device* pDevice)
{
	// オブジェクトの頂点配列を生成
	g_mesh.nNumVertex = 4;
	VERTEX_3D* pVertexWk = new VERTEX_3D[g_mesh.nNumVertex];

	// 頂点配列の中身を埋める
	VERTEX_3D* pVtx = pVertexWk;

	// 頂点座標の設定
	pVtx[0].vtx = XMFLOAT3(-1.0f / 2, -1.0f / 2, 0.0f);
	pVtx[1].vtx = XMFLOAT3(-1.0f / 2,  1.0f / 2, 0.0f);
	pVtx[2].vtx = XMFLOAT3( 1.0f / 2, -1.0f / 2, 0.0f);
	pVtx[3].vtx = XMFLOAT3( 1.0f / 2,  1.0f / 2, 0.0f);

	// 法線の設定
	pVtx[0].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVtx[1].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVtx[2].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVtx[3].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);

	// 反射光の設定
	pVtx[0].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVtx[1].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVtx[2].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVtx[3].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// テクスチャ座標の設定
	pVtx[0].tex = XMFLOAT2(0.0f, 1.0f);
	pVtx[1].tex = XMFLOAT2(0.0f, 0.0f);
	pVtx[2].tex = XMFLOAT2(1.0f, 1.0f);
	pVtx[3].tex = XMFLOAT2(1.0f, 0.0f);

	// インデックス配列を生成
	g_mesh.nNumIndex = 4;
	int* pIndexWk = new int[g_mesh.nNumIndex];

	// インデックス配列の中身を埋める
	pIndexWk[0] = 0;
	pIndexWk[1] = 1;
	pIndexWk[2] = 2;
	pIndexWk[3] = 3;

	// 頂点バッファ生成
	HRESULT hr = MakeMeshVertex(pDevice, &g_mesh, pVertexWk, pIndexWk);

	// 一時配列の解放
	delete[] pIndexWk;
	delete[] pVertexWk;

	return hr;
}

//=============================================================================
// 頂点座標の設定
//=============================================================================
void SetVertexEffect(int nIdxEffect, XMFLOAT2 size)
{
	g_effect[nIdxEffect].size = size;
}

//=============================================================================
// 頂点カラーの設定
//=============================================================================
void SetColorEffect(int nIdxEffect, XMFLOAT4 col)
{
	g_effect[nIdxEffect].col = col;
}

//=============================================================================
// エフェクトの設定
//=============================================================================
int SetEffect(XMFLOAT3 pos, XMFLOAT3 move, XMFLOAT4 col, XMFLOAT2 size, int nTimer)
{
	int nIdxEffect = -1;

	for (int nCntEffect = 0; nCntEffect < MAX_EFFECT; ++nCntEffect) {
		if (!g_effect[nCntEffect].use) {
			g_effect[nCntEffect].pos = pos;
			g_effect[nCntEffect].move = move;
			g_effect[nCntEffect].nTimer = nTimer;
			g_effect[nCntEffect].fDecAlpha = col.w / nTimer;
			g_effect[nCntEffect].use = true;

			// 頂点座標の設定
			SetVertexEffect(nCntEffect, size);

			// 頂点カラーの設定
			SetColorEffect(nCntEffect, col);

			nIdxEffect = nCntEffect;

			break;
		}
	}

	return nIdxEffect;
}
