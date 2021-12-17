//=============================================================================
//
// 丸影 [shadow.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "shadow.h"
#include "mesh.h"
#include "Texture.h"
#include "Light.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_FILENAME	L"data/texture/shadow000.jpg"	// テクスチャファイル名

#define M_DIFFUSE			XMFLOAT4(1.0f,1.0f,1.0f,1.0f)
#define M_SPECULAR			XMFLOAT4(0.0f,0.0f,0.0f,1.0f)
#define M_POWER				(50.0f)
#define M_AMBIENT			XMFLOAT4(0.0f,0.0f,0.0f,1.0f)
#define M_EMISSIVE			XMFLOAT4(0.0f,0.0f,0.0f,1.0f)

#define MAX_SHADOW			(5000)

//*****************************************************************************
// 構造体
//*****************************************************************************
typedef struct {
	int			stat;	// 状態
	XMFLOAT3	pos;	// 位置
	float		radius;	// 半径
} SHADOW;

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static ID3D11ShaderResourceView*	g_pTexture;				// テクスチャ
static MESH							g_mesh;					// 構造体
static MATERIAL						g_material;				// マテリアル
static SHADOW						g_shadow[MAX_SHADOW];	// 丸影情報

// 初期化
HRESULT InitShadow(void)
{
	ID3D11Device* pDevice = GetDevice();
	HRESULT hr = S_OK;

	// マテリアルの初期設定
	g_material.Diffuse = M_DIFFUSE;
	g_material.Ambient = M_AMBIENT;
	g_material.Specular = M_SPECULAR;
	g_material.Power = M_POWER;
	g_material.Emissive = M_EMISSIVE;
	g_mesh.pMaterial = &g_material;

	// テクスチャの読み込み
	hr = CreateTextureFromFile(pDevice, TEXTURE_FILENAME, &g_mesh.pTexture);
	if (FAILED(hr))
		return hr;
	XMStoreFloat4x4(&g_mesh.mtxTexture, XMMatrixIdentity());

	// ワールドマトリックス初期化
	XMStoreFloat4x4(&g_mesh.mtxWorld, XMMatrixIdentity());

	// 頂点情報の作成
	g_mesh.nNumVertex = 4;
	VERTEX_3D* pVertexWk = new VERTEX_3D[g_mesh.nNumVertex];
	pVertexWk[0].vtx = XMFLOAT3(-0.5f, 0.0f,  0.5f);
	pVertexWk[1].vtx = XMFLOAT3( 0.5f, 0.0f,  0.5f);
	pVertexWk[2].vtx = XMFLOAT3(-0.5f, 0.0f, -0.5f);
	pVertexWk[3].vtx = XMFLOAT3( 0.5f, 0.0f, -0.5f);
	pVertexWk[0].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[1].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[2].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[3].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[0].nor = XMFLOAT3(0.0f, 1.0f, 0.0f);
	pVertexWk[1].nor = XMFLOAT3(0.0f, 1.0f, 0.0f);
	pVertexWk[2].nor = XMFLOAT3(0.0f, 1.0f, 0.0f);
	pVertexWk[3].nor = XMFLOAT3(0.0f, 1.0f, 0.0f);
	pVertexWk[0].tex = XMFLOAT2(0.0f, 0.0f);
	pVertexWk[1].tex = XMFLOAT2(1.0f, 0.0f);
	pVertexWk[2].tex = XMFLOAT2(0.0f, 1.0f);
	pVertexWk[3].tex = XMFLOAT2(1.0f, 1.0f);
	g_mesh.nNumIndex = 4;
	int* pIndexWk = new int[4];
	pIndexWk[0] = 0;
	pIndexWk[1] = 1;
	pIndexWk[2] = 2;
	pIndexWk[3] = 3;
	hr = MakeMeshVertex(pDevice, &g_mesh, pVertexWk, pIndexWk);
	delete[] pIndexWk;
	delete[] pVertexWk;

	return hr;
}

// 終了処理
void UninitShadow(void)
{
	ReleaseMesh(&g_mesh);
}

// 更新
void UpdateShadow(void)
{
	// 何もしない
}

// 描画
void DrawShadow(void)
{
	ID3D11DeviceContext* pDC = GetDeviceContext();
	CLight::Get()->SetDisable();
	SetBlendState(BS_SUBTRACTION);	// 減算合成
	SetZWrite(false);	// 半透明描画はZバッファを更新しない(Zチェックは行う)
	SHADOW* pShadow = g_shadow;
	for (int i = 0; i < MAX_SHADOW; ++i, ++pShadow) {
		if (!pShadow->stat) continue;
		float ratio = pShadow->pos.y / 320.0f;
		if (ratio < 0.0f) ratio = 0.0f;
		if (ratio > 1.0f) ratio = 1.0f;
		// サイズを反映
		g_mesh.mtxWorld._11 =
		g_mesh.mtxWorld._33 = pShadow->radius * 2 * (1.0f - ratio);
		// 位置を反映
		g_mesh.mtxWorld._41 = pShadow->pos.x;
		g_mesh.mtxWorld._42 = 1.0f;	// Zファイティング回避のため0.0fより上に
		g_mesh.mtxWorld._43 = pShadow->pos.z;
		// 不透明度を反映
		g_material.Diffuse.w = 0.5f - ratio;
		if (g_material.Diffuse.w < 0.0f)
			g_material.Diffuse.w = 0.0f;
		// 描画
		DrawMesh(pDC, &g_mesh);
	}
	SetZWrite(true);
	SetBlendState(BS_NONE);
	CLight::Get()->SetEnable();
}

// 生成
int CreateShadow(XMFLOAT3 pos, float radius)
{
	SHADOW* pShadow = g_shadow;
	for (int i = 0; i < MAX_SHADOW; ++i, ++pShadow) {
		if (pShadow->stat) continue;
		pShadow->pos = pos;
		pShadow->radius = radius;
		pShadow->stat = 1;
		return i;
	}
	return -1;
}

// 移動
void MoveShadow(int nShadow, XMFLOAT3 pos)
{
	if (nShadow < 0 || nShadow >= MAX_SHADOW)
		return;
	g_shadow[nShadow].pos = pos;
}

// 解放
void ReleaseShadow(int nShadow)
{
	if (nShadow < 0 || nShadow >= MAX_SHADOW)
		return;
	g_shadow[nShadow].stat = 0;
}
