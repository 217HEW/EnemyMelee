//=============================================================================
//
// 2D処理 [billboard.cpp]
//
//=============================================================================
#include "billboard.h"
#include "Camera.h"
//#include "shadow.h"
#include "Texture.h"
#include "mesh.h"
#include "Light.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_BILLBOARD	L"data/texture/wall00.jpg"	// 読み込むテクスチャファイル名
#define	BILLBOARD_WIDTH		(50.0f)					    // 幅
#define	BILLBOARD_HEIGHT	(80.0f)						// 高さ

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT MakeVertexBillboard(ID3D11Device* pDevice);
void SetVertexBillboard(int idxBillboard, float width, float height);
void SetColorBillboard(int idxBillboard, XMFLOAT4 col);

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static MESH				g_mesh;				// メッシュ情報
static MATERIAL			g_material;			// マテリアル
static TBillboard		g_billboard[MAX_BILLBOARD];	// ビルボードワーク

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitBillboard(void)
{
	ID3D11Device* pDevice = GetDevice();

	// 頂点情報の作成
	MakeVertexBillboard(pDevice);

	// テクスチャの読み込み
	CreateTextureFromFile(pDevice,				// デバイスへのポインタ
		TEXTURE_BILLBOARD,			// ファイルの名前
		&g_mesh.pTexture);	// 読み込むメモリ
	XMStoreFloat4x4(&g_mesh.mtxTexture, XMMatrixIdentity());

	g_material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	g_material.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	g_material.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	g_material.Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	g_material.Power = 0.0f;
	g_mesh.pMaterial = &g_material;

	for (int cntBillboard = 0; cntBillboard < MAX_BILLBOARD; ++cntBillboard) {
		g_billboard[cntBillboard].pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		g_billboard[cntBillboard].col = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		g_billboard[cntBillboard].width = BILLBOARD_WIDTH;
		g_billboard[cntBillboard].height = BILLBOARD_HEIGHT;
		g_billboard[cntBillboard].use = false;
	}

		
	return S_OK;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitBillboard(void)
{
	for (int cntBillboard = 0; cntBillboard < MAX_BILLBOARD; ++cntBillboard) {
		if (g_billboard[cntBillboard].use) {
			//ReleaseShadow(g_billboard[cntBillboard].idxShadow);
			g_billboard[cntBillboard].idxShadow = -1;
			g_billboard[cntBillboard].use = false;
		}
	}
	// メッシュの開放
	ReleaseMesh(&g_mesh);
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdateBillboard(void)
{
	for (int cntBillboard = 0; cntBillboard < MAX_BILLBOARD; ++cntBillboard) {
		// 未使用ならスキップ
		if (!g_billboard[cntBillboard].use) {
			continue;
		}
		// 影の位置設定
		//MoveShadow(g_billboard[cntBillboard].idxShadow, XMFLOAT3(g_billboard[cntBillboard].pos.x, 0.1f, g_billboard[cntBillboard].pos.z));
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawBillboard(void)
{
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();
	XMMATRIX mtxWorld, mtxScale, mtxTranslate;

	CLight::Get()->SetDisable();	// 光源無効
	SetBlendState(BS_ALPHABLEND);	// αブレンディング有効

	// ビューマトリックスを取得
	XMFLOAT4X4& mtxView = CCamera::Get()->GetViewMatrix();

	for (int cntBillboard = 0; cntBillboard < MAX_BILLBOARD; ++cntBillboard) {
		// 未使用ならスキップ
		if (!g_billboard[cntBillboard].use) {
			continue;
		}
		// ワールド マトリックス初期化
		mtxWorld = XMMatrixIdentity();
		XMStoreFloat4x4(&g_mesh.mtxWorld, mtxWorld);

		// 回転を反映
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

		// スケールを反映 (回転より先に反映)
		mtxScale = XMMatrixScaling(g_billboard[cntBillboard].width, g_billboard[cntBillboard].height, 1.0f);
		mtxWorld = XMMatrixMultiply(mtxScale, mtxWorld);

		// 移動を反映
		mtxTranslate = XMMatrixTranslation(g_billboard[cntBillboard].pos.x, g_billboard[cntBillboard].pos.y, g_billboard[cntBillboard].pos.z);
		mtxWorld = XMMatrixMultiply(mtxWorld, mtxTranslate);

		// ワールドマトリックスの設定
		XMStoreFloat4x4(&g_mesh.mtxWorld, mtxWorld);

		// 色の設定
		g_material.Diffuse = g_billboard[cntBillboard].col;

		// ポリゴンの描画
		DrawMesh(pDeviceContext, &g_mesh);
	}

	//指定した場所に描画
	SetBillboard(XMFLOAT3(100.0f, 0.0f, 0.0f), 30.0f, 130.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	SetBillboard(XMFLOAT3(-100.0f, 0.0f, 0.0f), 30.0f, 130.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	SetBillboard(XMFLOAT3(0.0f, 120.0f, 30.0f), 230.0f, 30.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

	SetBlendState(BS_NONE);		// αブレンディング無効
	CLight::Get()->SetEnable();	// 光源有効
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
HRESULT MakeVertexBillboard(ID3D11Device* pDevice)
{
	// 一時的な頂点配列を生成
	g_mesh.nNumVertex = 4;
	VERTEX_3D* pVertexWk = new VERTEX_3D[g_mesh.nNumVertex];

	// 頂点配列の中身を埋める
	VERTEX_3D* pVtx = pVertexWk;

	// 頂点座標の設定
	pVtx[0].vtx = XMFLOAT3(-1.0f / 2.0f, 0.0f, 0.0f);
	pVtx[1].vtx = XMFLOAT3(-1.0f / 2.0f, 1.0f, 0.0f);
	pVtx[2].vtx = XMFLOAT3(1.0f / 2.0f, 0.0f, 0.0f);
	pVtx[3].vtx = XMFLOAT3(1.0f / 2.0f, 1.0f, 0.0f);

	// 法線の設定
	pVtx[0].nor = XMFLOAT3(0.0f, 0.0f, 1.0f);
	pVtx[1].nor = XMFLOAT3(0.0f, 0.0f, 1.0f);
	pVtx[2].nor = XMFLOAT3(0.0f, 0.0f, 1.0f);
	pVtx[3].nor = XMFLOAT3(0.0f, 0.0f, 1.0f);

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

	g_mesh.nNumIndex = 4;
	int* pIndexWk = new int[g_mesh.nNumIndex];
	pIndexWk[0] = 0;
	pIndexWk[1] = 1;
	pIndexWk[2] = 2;
	pIndexWk[3] = 3;

	HRESULT hr = MakeMeshVertex(pDevice, &g_mesh, pVertexWk, pIndexWk);

	delete[] pIndexWk;
	delete[] pVertexWk;

	return hr;
}

//=============================================================================
// 頂点座標の設定
//=============================================================================
void SetVertexBillboard(int idxBillboard, float width, float height)
{
	if (idxBillboard >= 0 && idxBillboard < MAX_BILLBOARD) {
		g_billboard[idxBillboard].width = width;
		g_billboard[idxBillboard].height = height;
	}
}

//=============================================================================
// 頂点カラーの設定
//=============================================================================
void SetColorBillboard(int idxBillboard, XMFLOAT4 col)
{
	if (idxBillboard >= 0 && idxBillboard < MAX_BILLBOARD) {
		g_billboard[idxBillboard].col = col;
	}
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
int SetBillboard(XMFLOAT3 pos, float width, float height, XMFLOAT4 col)
{
	int idxBillboard = -1;

	for (int cntBillboard = 0; cntBillboard < MAX_BILLBOARD; ++cntBillboard) {
		// 使用中ならスキップ
		if (g_billboard[cntBillboard].use) {
			continue;
		}
		g_billboard[cntBillboard].use = true;
		g_billboard[cntBillboard].pos = pos;

		// 頂点座標の設定
		SetVertexBillboard(cntBillboard, width, height);

		// 頂点カラーの設定
		SetColorBillboard(cntBillboard, col);

		// 影の設定
		//g_billboard[cntBillboard].idxShadow = CreateShadow(g_billboard[cntBillboard].pos, g_billboard[cntBillboard].width * 0.5f);

		idxBillboard = cntBillboard;
		break;
	}

	return idxBillboard;
}
