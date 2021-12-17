//=============================================================================
//
// メッシュ壁の処理 [meshwall.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "meshwall.h"
#include "mesh.h"
#include "Texture.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_FILENAME	L"data/texture/wall000.jpg"	// テクスチャファイル名
#define	MAX_MESHWALL		(32)						// 壁の総数

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static ID3D11ShaderResourceView*	g_pTexture;	// テクスチャ読み込み場所
static MESH			g_meshWall[MAX_MESHWALL];	// メッシュ壁ワーク
static int			g_nNumMeshWall = 0;			// メッシュ壁の数

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitMeshWall(void)
{
	ID3D11Device* pDevice = GetDevice();

	// テクスチャの読み込み
	HRESULT hr = CreateTextureFromFile(pDevice,				// デバイス ポインタ
									   TEXTURE_FILENAME,	// ファイル名
									   &g_pTexture);		// 読み込むメモリ

	SetMeshWall(XMFLOAT3(  0.0f, 30.0f, -100.0f), XMFLOAT3( 0.0f,   0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1, 1, XMFLOAT2(30.0f, 30.0f));// 手前
	SetMeshWall(XMFLOAT3( 15.0f, 30.0f,  -85.0f), XMFLOAT3( 0.0f, -90.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1, 1, XMFLOAT2(30.0f, 30.0f));// 右
	SetMeshWall(XMFLOAT3(-15.0f, 30.0f,  -85.0f), XMFLOAT3( 0.0f,  90.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1, 1, XMFLOAT2(30.0f, 30.0f));// 左
	SetMeshWall(XMFLOAT3(  0.0f, 60.0f, -100.0f), XMFLOAT3( 90.0f,  0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1, 1, XMFLOAT2(30.0f, 30.0f));// 上
	SetMeshWall(XMFLOAT3(  0.0f, 30.0f, -100.0f), XMFLOAT3(-90.0f,  0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 1, 1, XMFLOAT2(30.0f, 30.0f));// 下
	
	return hr;
}

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT SetMeshWall(XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT4 col,
	int nNumBlockX, int nNumBlockY, XMFLOAT2 sizeBlock)
{
	ID3D11Device* pDevice = GetDevice();
	MESH* pMesh;

	if (g_nNumMeshWall >= MAX_MESHWALL) {
		return E_FAIL;
	}

	pMesh = &g_meshWall[g_nNumMeshWall];
	g_nNumMeshWall++;

	// テクスチャを設定
	XMStoreFloat4x4(&pMesh->mtxTexture, XMMatrixIdentity());
	pMesh->pTexture = g_pTexture;

	// ポリゴン表示位置の中心座標、角度を設定
	pMesh->pos = pos;
	pMesh->rot = rot;

	// 頂点数の設定
	pMesh->nNumVertex = (nNumBlockX + 1) * (nNumBlockY + 1);

	// インデックス数の設定
	pMesh->nNumIndex = (nNumBlockX + 1) * 2 * nNumBlockY + (nNumBlockY - 1) * 2;

	// オブジェクトの頂点バッファを生成
	VERTEX_3D* pVertexWk = new VERTEX_3D[pMesh->nNumVertex];

	// オブジェクトのインデックスバッファを生成
	int* pIndexWk = new int[pMesh->nNumIndex];

	// 頂点バッファの中身を埋める
	VERTEX_3D* pVtx = pVertexWk;
#if 0
	const float texSizeX = 1.0f / nNumBlockX;
	const float texSizeZ = 1.0f / nNumBlockY;
#else
	const float texSizeX = 1.0f;
	const float texSizeZ = 1.0f;
#endif
	for (int nCntVtxY = 0; nCntVtxY < nNumBlockY + 1; ++nCntVtxY) {
		for (int nCntVtxX = 0; nCntVtxX < nNumBlockX + 1; ++nCntVtxX) {
			// 頂点座標の設定
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].vtx.x = -(nNumBlockX / 2.0f) * sizeBlock.x + nCntVtxX * sizeBlock.x;
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].vtx.y = nNumBlockY * sizeBlock.y - nCntVtxY * sizeBlock.y;
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].vtx.z = 0.0f;

			// 法線の設定
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);

			// 反射光の設定
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].diffuse = col;

			// テクスチャ座標の設定
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].tex.x = texSizeX * nCntVtxX;
			pVtx[nCntVtxY * (nNumBlockX + 1) + nCntVtxX].tex.y = texSizeZ * nCntVtxY;
		}
	}

	// インデックスバッファの中身を埋める
	int* pIdx = pIndexWk;

	int nCntIdx = 0;
	for (int nCntVtxY = 0; nCntVtxY < nNumBlockY; ++nCntVtxY) {
		if (nCntVtxY > 0) {
			// 縮退ポリゴンのためのダブりの設定
			pIdx[nCntIdx] = (nCntVtxY + 1) * (nNumBlockX + 1);
			++nCntIdx;
		}

		for (int nCntVtxX = 0; nCntVtxX < nNumBlockX + 1; ++nCntVtxX) {
			pIdx[nCntIdx] = (nCntVtxY + 1) * (nNumBlockX + 1) + nCntVtxX;
			++nCntIdx;
			pIdx[nCntIdx] = nCntVtxY * (nNumBlockX + 1) + nCntVtxX;
			++nCntIdx;
		}

		if (nCntVtxY < nNumBlockY - 1) {
			// 縮退ポリゴンのためのダブりの設定
			pIdx[nCntIdx] = nCntVtxY * (nNumBlockX + 1) + nNumBlockX;
			++nCntIdx;
		}
	}

	// 頂点バッファ/インデックス バッファ生成
	MakeMeshVertex(pDevice, pMesh, pVertexWk, pIndexWk);

	// 一時配列解放
	delete[] pIndexWk;
	delete[] pVertexWk;

	return S_OK;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitMeshWall(void)
{
	MESH* pMesh;

	// 全メッシュ開放
	for (int i = 0; i < g_nNumMeshWall; ++i) {
		pMesh = &g_meshWall[i];
		pMesh->pTexture = nullptr;
		ReleaseMesh(pMesh);
	}
	g_nNumMeshWall = 0;

	// テクスチャの開放
	SAFE_RELEASE(g_pTexture);
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdateMeshWall(void)
{
	for (int i = 0; i < g_nNumMeshWall; ++i) {
		// メッシュ更新
		UpdateMesh(&g_meshWall[i]);
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawMeshWall(EDrawPart dp)
{
	ID3D11DeviceContext* pDC = GetDeviceContext();

	MESH* pWall = g_meshWall;
	for (int i = 0; i < g_nNumMeshWall; ++i, ++pWall) {
		// メッシュ描画
		switch (dp) {
		case DRAWPART_OPAQUE:		// 不透明のみ
			if (pWall->fAlpha < 1.0f)
				continue;
			SetBlendState(BS_NONE);
			SetZWrite(false);
			break;
		case DRAWPART_TRANSLUCENT:	// 半透明のみ
			if (pWall->fAlpha >= 1.0f)
				continue;
			SetBlendState(BS_ALPHABLEND);
			SetZBuffer(true);
			break;
		default:
			if (pWall->fAlpha < 1.0f) {
				SetBlendState(BS_NONE);
				SetZWrite(false);
			} else {
				SetBlendState(BS_ALPHABLEND);
				SetZBuffer(true);
			}
			break;
		}
		DrawMesh(pDC, pWall);
	}
	SetBlendState(BS_NONE);
	SetZBuffer(true);
}
