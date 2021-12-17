//=============================================================================
//
// 弾 [bullet.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "bullet.h"
#include "Camera.h"
#include "shadow.h"
#include "Texture.h"
#include "mesh.h"
#include "Light.h"
#include "explosion.h"
#include "smoke.h"
//#include "enemy.h"
#include "player.h"
#include "effect.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_FILENAME	L"data/texture/bullet000.png"	// テクスチャ ファイル名

#define M_DIFFUSE			XMFLOAT4(1.0f,1.0f,1.0f,1.0f)
#define M_SPECULAR			XMFLOAT4(0.0f,0.0f,0.0f,1.0f)
#define M_POWER				(1.0f)
#define M_AMBIENT			XMFLOAT4(0.0f,0.0f,0.0f,1.0f)
#define M_EMISSIVE			XMFLOAT4(0.0f,0.0f,0.0f,1.0f)

#define MAX_BULLET			100

#define BULLET_SPEED		7.5f
#define EBULLET_SPEED		5.5f
#define BULLET_RADIUS		5.0f
#define BULLET_STRENGTH		1.0f

#define MIN_FIELD_X			-640.0f
#define MAX_FIELD_X			640.0f
#define MIN_FIELD_Z			-640.0f
#define MAX_FIELD_Z			640.0f

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct TBullet {
	XMFLOAT3	pos;		// 位置
	XMFLOAT3	vel;		// 速度
	int			nShadow;	// 影ID
	bool		use;		// 使用しているかどうか
	EBulletType	type;		// 種類
};

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static MESH			g_mesh;					// メッシュ情報
static MATERIAL		g_material;				// マテリアル
static TBullet		g_bullet[MAX_BULLET];	// 弾情報

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
static HRESULT MakeVertexBullet(ID3D11Device* pDevice);

//=============================================================================
// 初期化
//=============================================================================
HRESULT InitBullet(void)
{
	ID3D11Device* pDevice = GetDevice();
	HRESULT hr = S_OK;

	// 頂点情報の作成
	hr = MakeVertexBullet(pDevice);
	if (FAILED(hr)) {
		return hr;
	}

	// マテリアルの設定
	g_material.Diffuse = M_DIFFUSE;
	g_material.Ambient = M_AMBIENT;
	g_material.Specular = M_SPECULAR;
	g_material.Power = M_POWER;
	g_material.Emissive = M_EMISSIVE;
	g_mesh.pMaterial = &g_material;

	// テクスチャの読み込み
	hr = CreateTextureFromFile(pDevice, TEXTURE_FILENAME, &g_mesh.pTexture);
	if (FAILED(hr)) {
		return hr;
	}
	XMStoreFloat4x4(&g_mesh.mtxTexture, XMMatrixIdentity());

	// ワールド マトリックス初期化
	XMStoreFloat4x4(&g_mesh.mtxWorld, XMMatrixIdentity());

	// 弾情報初期化
	for (int i = 0; i < MAX_BULLET; ++i) {
		g_bullet[i].pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		g_bullet[i].vel = XMFLOAT3(0.0f, 0.0f, 0.0f);
		g_bullet[i].use = false;
		g_bullet[i].nShadow = -1;
		g_bullet[i].type = BULLETTYPE_PLAYER;
	}

	return hr;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitBullet(void)
{
	TBullet* pBullet = g_bullet;
	for (int i = 0; i < MAX_BULLET; ++i, ++pBullet) {
		if (pBullet->use) {
			pBullet->use = false;
			ReleaseShadow(pBullet->nShadow);
			pBullet->nShadow = -1;
		}
	}
	ReleaseMesh(&g_mesh);
}

//=============================================================================
// 更新
//=============================================================================
void UpdateBullet(void)
{
	TBullet* pBullet = g_bullet;
	for (int i = 0; i < MAX_BULLET; ++i, ++pBullet) {
		// 未使用ならスキップ
		if (!pBullet->use) {
			continue;
		}
		// 位置を更新
		pBullet->pos.x += pBullet->vel.x;
		pBullet->pos.y += pBullet->vel.y;
		pBullet->pos.z += pBullet->vel.z;
		// 範囲チェック
		if (pBullet->pos.x < MIN_FIELD_X || pBullet->pos.x > MAX_FIELD_X ||
			pBullet->pos.z < MIN_FIELD_Z || pBullet->pos.z > MAX_FIELD_Z) {
			pBullet->use = false;
			// 丸影解放
			ReleaseShadow(pBullet->nShadow);
			pBullet->nShadow = -1;
			// 爆発開始
			pBullet->pos.x -= pBullet->vel.x;
			pBullet->pos.y -= pBullet->vel.y;
			pBullet->pos.z -= pBullet->vel.z;
			int nExp = StartExplosion(pBullet->pos, XMFLOAT2(20.0f, 20.0f));
			if (pBullet->type == BULLETTYPE_ENEMY) {
				SetExplosionColor(nExp, XMFLOAT4(1.0f, 0.7f, 0.7f, 1.0f));
			}
			continue;
		}
		if (pBullet->type == BULLETTYPE_PLAYER) {
			// 敵との当たり判定
			//int nEnemy = CollisionEnemy(pBullet->pos, BULLET_RADIUS, BULLET_STRENGTH);
			//if (nEnemy >= 0) {
			//	pBullet->use = false;
			//	// 丸影解放
			//	ReleaseShadow(pBullet->nShadow);
			//	pBullet->nShadow = -1;
			//	continue;
			//}
		} else if (pBullet->type == BULLETTYPE_ENEMY) {
			// プレイヤーとの当たり判定
			if (CollisionPlayer(pBullet->pos, BULLET_RADIUS, BULLET_STRENGTH)) {
				pBullet->use = false;
				// 丸影解放
				ReleaseShadow(pBullet->nShadow);
				pBullet->nShadow = -1;
				continue;
			}
		}
		// 丸影移動
		MoveShadow(pBullet->nShadow, pBullet->pos);
		// エフェクトの設定
		if (pBullet->type == BULLETTYPE_PLAYER) {
			// 煙生成
			SetSmoke(pBullet->pos, XMFLOAT2(8.0f, 8.0f));
		} else {
			SetEffect(pBullet->pos, XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT4(0.85f, 0.05f, 0.25f, 0.50f), XMFLOAT2(16.0f, 16.0f), 30);
			SetEffect(pBullet->pos, XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT4(0.65f, 0.85f, 0.05f, 0.30f), XMFLOAT2(12.0f, 12.0f), 30);
			SetEffect(pBullet->pos, XMFLOAT3(0.0f, 0.0f, 0.0f),
				XMFLOAT4(0.45f, 0.45f, 0.05f, 0.20f), XMFLOAT2(6.0f, 6.0f), 30);
		}
	}
}

//=============================================================================
// 描画
//=============================================================================
void DrawBullet(void)
{
	ID3D11DeviceContext* pDC = GetDeviceContext();

	CLight::Get()->SetDisable();	// 光源無効
	SetBlendState(BS_ALPHABLEND);	// αブレンディング有効
	XMFLOAT4X4& mView = CCamera::Get()->GetViewMatrix();
	TBullet* pBullet = g_bullet;
	for (int i = 0; i < MAX_BULLET; ++i, ++pBullet) {
		// 未使用ならスキップ
		if (!pBullet->use) {
			continue;
		}
		// ビュー行列の回転成分の転置行列を設定
		g_mesh.mtxWorld._11 = mView._11;
		g_mesh.mtxWorld._12 = mView._21;
		g_mesh.mtxWorld._13 = mView._31;
		g_mesh.mtxWorld._14 = 0.0f;
		g_mesh.mtxWorld._21 = mView._12;
		g_mesh.mtxWorld._22 = mView._22;
		g_mesh.mtxWorld._23 = mView._32;
		g_mesh.mtxWorld._24 = 0.0f;
		g_mesh.mtxWorld._31 = mView._13;
		g_mesh.mtxWorld._32 = mView._23;
		g_mesh.mtxWorld._33 = mView._33;
		g_mesh.mtxWorld._34 = 0.0f;
		// 位置を反映
		g_mesh.mtxWorld._41 = pBullet->pos.x;
		g_mesh.mtxWorld._42 = pBullet->pos.y;
		g_mesh.mtxWorld._43 = pBullet->pos.z;
		g_mesh.mtxWorld._44 = 1.0f;
		// 描画
		DrawMesh(pDC, &g_mesh);
	}
	SetBlendState(BS_NONE);		// αブレンディング無効
	CLight::Get()->SetEnable();	// 光源有効
}

//=============================================================================
// 発射
//=============================================================================
int FireBullet(XMFLOAT3 pos, XMFLOAT3 dir, EBulletType type)
{
	TBullet* pBullet = g_bullet;
	for (int i = 0; i < MAX_BULLET; ++i, ++pBullet) {
		// 使用中ならスキップ
		if (pBullet->use) {
			continue;
		}
		pBullet->pos = pos;
		// 方向ベクトルを正規化
		XMStoreFloat3(&dir, XMVector3Normalize(XMLoadFloat3(&dir)));
		if (type == BULLETTYPE_PLAYER) {
			pBullet->vel.x = dir.x * BULLET_SPEED;
			pBullet->vel.y = dir.y * BULLET_SPEED;
			pBullet->vel.z = dir.z * BULLET_SPEED;
		} else {
			pBullet->vel.x = dir.x * EBULLET_SPEED;
			pBullet->vel.y = dir.y * EBULLET_SPEED;
			pBullet->vel.z = dir.z * EBULLET_SPEED;
		}
		pBullet->nShadow = CreateShadow(pos, BULLET_RADIUS);
		XMFLOAT4X4 mtxWorld;
		XMStoreFloat4x4(&mtxWorld,
			XMMatrixTranslation(pBullet->pos.x, pBullet->pos.y, pBullet->pos.z));
		pBullet->use = true;
		pBullet->type = type;
		return i;	// 発射できた(0～:弾番号)
	}
	return -1;		// 発射できなかった
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
HRESULT MakeVertexBullet(ID3D11Device* pDevice)
{
	// 一時的な頂点配列を生成
	g_mesh.nNumVertex = 4;
	VERTEX_3D* pVertexWk = new VERTEX_3D[g_mesh.nNumVertex];

	// 頂点配列の中身を埋める
	pVertexWk[0].vtx = XMFLOAT3(-BULLET_RADIUS,  BULLET_RADIUS, 0.0f);
	pVertexWk[1].vtx = XMFLOAT3( BULLET_RADIUS,  BULLET_RADIUS, 0.0f);
	pVertexWk[2].vtx = XMFLOAT3(-BULLET_RADIUS, -BULLET_RADIUS, 0.0f);
	pVertexWk[3].vtx = XMFLOAT3( BULLET_RADIUS, -BULLET_RADIUS, 0.0f);
	pVertexWk[0].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[1].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[2].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[3].diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	pVertexWk[0].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVertexWk[1].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVertexWk[2].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVertexWk[3].nor = XMFLOAT3(0.0f, 0.0f, -1.0f);
	pVertexWk[0].tex = XMFLOAT2(0.0f, 0.0f);
	pVertexWk[1].tex = XMFLOAT2(1.0f, 0.0f);
	pVertexWk[2].tex = XMFLOAT2(0.0f, 1.0f);
	pVertexWk[3].tex = XMFLOAT2(1.0f, 1.0f);

	// 一時的なインデックス配列を生成
	g_mesh.nNumIndex = 4;
	int* pIndexWk = new int[4];

	// インデックス配列の中身を埋める
	pIndexWk[0] = 0;
	pIndexWk[1] = 1;
	pIndexWk[2] = 2;
	pIndexWk[3] = 3;

	// 頂点バッファ/インデックス バッファ生成
	HRESULT hr = MakeMeshVertex(pDevice, &g_mesh, pVertexWk, pIndexWk);

	// 一時的な頂点配列/インデックス配列を解放
	delete[] pIndexWk;
	delete[] pVertexWk;
	return hr;
}
