//=============================================================================
//
// 煙処理 [smoke.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "smoke.h"
#include "Texture.h"
#include "mesh.h"
#include "shadow.h"
#include "Camera.h"
#include "Light.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_SMOKE		L"data/texture/smoke000.png"	// 読み込むテクスチャファイル名
#define	SMOKE_SIZE_X		(50.0f)							// ビルボードの幅
#define	SMOKE_SIZE_Y		(50.0f)							// ビルボードの高さ

#define	MAX_SMOKE			(512)							// ビルボード最大数

#define MATERIAL_DIFFUSE		XMFLOAT4(1.0f,1.0f,1.0f,1.0f)
#define MATERIAL_AMBIENT		XMFLOAT4(0,0,0,1)
#define MATERIAL_SPECULAR		XMFLOAT4(0,0,0,1)
#define MATERIAL_EMISSIVE		XMFLOAT4(0,0,0,1)
#define MATERIAL_POWER			(1.0f)

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct TSmoke {
	XMFLOAT4X4	mtxWorld;	// ワールドマトリックス
	XMFLOAT3	pos;		// 位置
	XMFLOAT3	vel;		// 速度
	XMFLOAT4	col;		// 色
	XMFLOAT2	size;		// 大きさ
	float		fAlpha;		// アルファ値
	bool		use;		// 使用しているかどうか
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT MakeVertexSmoke(ID3D11Device* pDevice);
void SetVertexSmoke(int nIdxBullet, XMFLOAT2 size);
void SetColorSmoke(int nIdxSmoke, XMFLOAT4 col);

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static MESH			g_mesh;
static MATERIAL		g_material;
static TSmoke		g_smoke[MAX_SMOKE];	// 煙ワーク

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitSmoke(void)
{
	HRESULT hr = S_OK;
	ID3D11Device* pDevice = GetDevice();

	ZeroMemory(&g_mesh, sizeof(MESH));

	// 頂点情報の作成
	MakeVertexSmoke(pDevice);

	// テクスチャの読み込み
	hr = CreateTextureFromFile(pDevice,				// デバイスへのポインタ
							   TEXTURE_SMOKE,		// ファイル名
							   &g_mesh.pTexture);	// 読み込むメモリ
	XMStoreFloat4x4(&g_mesh.mtxTexture, XMMatrixIdentity());

	// マテリアルの設定
	g_material.Diffuse = MATERIAL_DIFFUSE;
	g_material.Ambient = MATERIAL_AMBIENT;
	g_material.Specular = MATERIAL_SPECULAR;
	g_material.Emissive = MATERIAL_EMISSIVE;
	g_material.Power = MATERIAL_POWER;
	g_mesh.pMaterial = &g_material;

	TSmoke* pSmoke = g_smoke;
	for (int i = 0; i < MAX_SMOKE; ++i, ++pSmoke) {
		pSmoke->pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		pSmoke->col = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		pSmoke->size = XMFLOAT2(SMOKE_SIZE_X, SMOKE_SIZE_Y);
		pSmoke->fAlpha = 1.0f;
		pSmoke->use = false;
	}

	return S_OK;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitSmoke(void)
{
	// メッシュの開放
	ReleaseMesh(&g_mesh);
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdateSmoke(void)
{
	TSmoke* pSmoke = g_smoke;
	for (int i = 0; i < MAX_SMOKE; ++i, ++pSmoke) {
		if (pSmoke->use) {
			// 移動
			pSmoke->pos.x += pSmoke->vel.x;
			pSmoke->pos.y += pSmoke->vel.y;
			pSmoke->pos.z += pSmoke->vel.z;

			// 頂点座標の設定
			pSmoke->size.x += 0.1f;
			pSmoke->size.y += 0.1f;
			SetVertexSmoke(i, pSmoke->size);

			// 頂点カラーの設定
			pSmoke->fAlpha -= 0.002f;
			if (pSmoke->fAlpha < 0.0f) {
				pSmoke->fAlpha = 0.0f;
				pSmoke->use = false;
				continue;
			}
			SetColorSmoke(i, XMFLOAT4(1.0f, 1.0f, 1.0f, pSmoke->fAlpha));
		}
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawSmoke(void)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	XMMATRIX mtxWorld, mtxScale, mtxTranslate;

	// ビューマトリックスを取得
	XMFLOAT4X4& mtxView = CCamera::Get()->GetViewMatrix();

	CLight::Get()->SetDisable();
	//SetBlendState(BS_SUBTRACTION);	// 減算合成
	SetBlendState(BS_ALPHABLEND);		// 加算合成
	SetZWrite(false);	// 半透明描画はZバッファを更新しない(Zチェックは行う)
	TSmoke* pSmoke = g_smoke;
	for (int i = 0; i < MAX_SMOKE; ++i, ++pSmoke) {
		if (pSmoke->use) {
			// ワールドマトリックスの初期化
			mtxWorld = XMMatrixIdentity();
			XMStoreFloat4x4(&pSmoke->mtxWorld, mtxWorld);

			pSmoke->mtxWorld._11 = mtxView._11;
			pSmoke->mtxWorld._12 = mtxView._21;
			pSmoke->mtxWorld._13 = mtxView._31;
			pSmoke->mtxWorld._21 = mtxView._12;
			pSmoke->mtxWorld._22 = mtxView._22;
			pSmoke->mtxWorld._23 = mtxView._32;
			pSmoke->mtxWorld._31 = mtxView._13;
			pSmoke->mtxWorld._32 = mtxView._23;
			pSmoke->mtxWorld._33 = mtxView._33;

			mtxWorld = XMLoadFloat4x4(&pSmoke->mtxWorld);

			// スケールを反映
			mtxScale = XMMatrixScaling(pSmoke->size.x, pSmoke->size.y, 1.0f);
			mtxWorld = XMMatrixMultiply(mtxScale, mtxWorld);

			// 移動を反映
			mtxTranslate = XMMatrixTranslation(pSmoke->pos.x, pSmoke->pos.y, pSmoke->pos.z);
			mtxWorld = XMMatrixMultiply(mtxWorld, mtxTranslate);

			// ワールドマトリックスの設定
			XMStoreFloat4x4(&pSmoke->mtxWorld, mtxWorld);
			g_mesh.mtxWorld = pSmoke->mtxWorld;

			// カラーの設定
			g_material.Diffuse = pSmoke->col;

			// メッシュの描画
			DrawMesh(pDeviceContext, &g_mesh);
		}
	}
	SetZWrite(true);
	SetBlendState(BS_NONE);
	CLight::Get()->SetEnable();
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
HRESULT MakeVertexSmoke(ID3D11Device* pDevice)
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
void SetVertexSmoke(int nIdxSmoke, XMFLOAT2 size)
{
	if (nIdxSmoke >= 0 && nIdxSmoke < MAX_SMOKE) {
		TSmoke* pSmoke = &g_smoke[nIdxSmoke];
		pSmoke->size = size;
	}
}

//=============================================================================
// 頂点カラーの設定
//=============================================================================
void SetColorSmoke(int nIdxSmoke, XMFLOAT4 col)
{
	if (nIdxSmoke >= 0 && nIdxSmoke < MAX_SMOKE) {
		g_smoke[nIdxSmoke].col = col;
	}
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
int SetSmoke(XMFLOAT3 pos, XMFLOAT2 size)
{
	int nIdxSmoke = -1;

	TSmoke* pSmoke = g_smoke;
	for (int i = 0; i < MAX_SMOKE; ++i, ++pSmoke) {
		if (!pSmoke->use) {
			pSmoke->pos = pos;
			XMMATRIX mRot = XMMatrixRotationY(XMConvertToRadians((float)(rand() % 360)));
			XMVECTOR vAxis = XMVector3TransformNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), mRot);
			XMStoreFloat3(&pSmoke->vel, XMVector3TransformNormal(XMVectorSet(0.0f, 0.5f, 0.0f, 0.0f), XMMatrixRotationAxis(vAxis, XMConvertToRadians(10))));
			pSmoke->col = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			pSmoke->fAlpha = 0.2f;
			pSmoke->use = true;

			// 頂点座標の設定
			SetVertexSmoke(i, size);

			nIdxSmoke = i;
			break;
		}
	}

	return nIdxSmoke;
}
