//=============================================================================
//
// ポリライン処理 [polyline.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "polyline.h"
#include "Shader.h"
#include "Texture.h"
#include "Light.h"
#include "Camera.h"
#include "mesh.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_POLYLINE	L"data/texture/polyline001.png"	// テクスチャファイル名

#define M_DIFFUSE			XMFLOAT4(1,1,1,1)
#define M_AMBIENT			XMFLOAT4(0,0,0,1)
#define M_SPECULAR			XMFLOAT4(0,0,0,1)
#define M_EMISSIVE			XMFLOAT4(0,0,0,1)
#define M_POWER				(1.0f)

//*****************************************************************************
// 構造体定義
//*****************************************************************************
// シェーダに渡す値
struct SHADER_GLOBAL {
	XMMATRIX	mWVP;		// ワールド×ビュー×射影行列(転置行列)
	XMMATRIX	mW;			// ワールド行列(転置行列)
	XMMATRIX	mTex;		// テクスチャ行列(転置行列)
};
struct SHADER_GLOBAL2 {
	XMVECTOR	vEye;		// 視点座標(ワールド空間)
	// 光源
	XMVECTOR	vLightDir;	// 光源方向
	XMVECTOR	vLa;		// 光源色(アンビエント)
	XMVECTOR	vLd;		// 光源色(ディフューズ)
	XMVECTOR	vLs;		// 光源色(スペキュラ)
	// マテリアル
	XMVECTOR	vAmbient;	// アンビエント色(+テクスチャ有無)
	XMVECTOR	vDiffuse;	// ディフューズ色
	XMVECTOR	vSpecular;	// スペキュラ色(+スペキュラ強度)
	XMVECTOR	vEmissive;	// エミッシブ色
};

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static ID3D11Buffer*				g_pConstantBuffer[2];	// 定数バッファ
static ID3D11SamplerState*			g_pSamplerState;		// テクスチャ サンプラ
static ID3D11VertexShader*			g_pVertexShader;		// 頂点シェーダ
static ID3D11InputLayout*			g_pInputLayout;			// 頂点フォーマット
static ID3D11PixelShader*			g_pPixelShader;			// ピクセルシェーダ

static MATERIAL						g_material;				// マテリアル

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
static HRESULT UpdatePolylineVertex(ID3D11DeviceContext* pDC, TPolyline* pPolyline);

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitPolyline(void)
{
	ID3D11Device* pDevice = GetDevice();
	HRESULT hr;

	// シェーダ初期化
	static const D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	hr = LoadShader("Vertex", "Pixel", &g_pVertexShader, &g_pInputLayout, &g_pPixelShader, layout, _countof(layout));
	if (FAILED(hr)) {
		return hr;
	}

	// 定数バッファ生成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SHADER_GLOBAL);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = pDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer[0]);
	if (FAILED(hr)) return hr;
	bd.ByteWidth = sizeof(SHADER_GLOBAL2);
	hr = pDevice->CreateBuffer(&bd, nullptr, &g_pConstantBuffer[1]);
	if (FAILED(hr)) return hr;

	// テクスチャ サンプラ生成
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hr = pDevice->CreateSamplerState(&sd, &g_pSamplerState);
	if (FAILED(hr)) {
		return hr;
	}

	// マテリアルの初期設定
	g_material.Diffuse = M_DIFFUSE;
	g_material.Ambient = M_AMBIENT;
	g_material.Specular = M_SPECULAR;
	g_material.Power = M_POWER;
	g_material.Emissive = M_EMISSIVE;

	return hr;
}

//=============================================================================
// 終了処理
//=============================================================================
void UninitPolyline(void)
{
	// テクスチャ サンプラの開放
	SAFE_RELEASE(g_pSamplerState);
	// 定数バッファの解放
	for (int i = 0; i < _countof(g_pConstantBuffer); ++i) {
		SAFE_RELEASE(g_pConstantBuffer[i]);
	}
	// ピクセルシェーダ解放
	SAFE_RELEASE(g_pPixelShader);
	// 頂点フォーマット解放
	SAFE_RELEASE(g_pInputLayout);
	// 頂点シェーダ解放
	SAFE_RELEASE(g_pVertexShader);
}

//=============================================================================
// 更新処理
//=============================================================================
void UpdatePolyline(TPolyline* pPolyline)
{
	if (!pPolyline) return;

	int nVertex = (int)pPolyline->pointList.size();
	pPolyline->nNumVertex = nVertex * 2 + 2;
	if (pPolyline->nNumVertex > MAX_POLYLINE_VERTEX) {
		nVertex = (MAX_POLYLINE_VERTEX - 2) / 2;
		pPolyline->nNumVertex = MAX_POLYLINE_VERTEX;
	}
	if (nVertex < 1) return;

	VERTEX_3D* pVtx = pPolyline->vertex;
	// 点からカメラへの単位ベクトルを求める
	XMFLOAT3 z = CCamera::Get()->GetPos();
	XMFLOAT3 w = pPolyline->pointList[0];
	z.x -= w.x;
	z.y -= w.y;
	z.z -= w.z;
	// 点の接線ベクトルを求める
	XMFLOAT3 t;
	if (nVertex > 1) {
		t = pPolyline->pointList[1];
		w = pPolyline->pointList[0];
		t.x -= w.x;
		t.y -= w.y;
		t.z -= w.z;
	} else {
		t = CCamera::Get()->GetUpVector();
	}
	// ストリップ頂点を求める
	XMFLOAT3 cross;
	XMVECTOR vCross;
	if (pPolyline->bRound) {
		vCross = XMVector3Cross(XMLoadFloat3(&t), XMLoadFloat3(&z));
		vCross = XMVector3Cross(XMLoadFloat3(&z), vCross);
		XMStoreFloat3(&cross, XMVector3Normalize(vCross));
		pVtx->vtx = pPolyline->pointList[0];
		pVtx->vtx.x -= cross.x * pPolyline->fWidth;
		pVtx->vtx.y -= cross.y * pPolyline->fWidth;
		pVtx->vtx.z -= cross.z * pPolyline->fWidth;
	} else {
		pVtx->vtx = pPolyline->pointList[0];
	}
	pVtx->diffuse = pPolyline->cDiffuse;
	pVtx->tex.x = 0.0f;
	pVtx->tex.y = 0.0f;
	pVtx->nor = XMFLOAT3(0.0f, 0.0f, 0.0f);
	++pVtx;
	for (int i = 0; i < nVertex; ++i, pVtx += 2) {
		// 点からカメラへの単位ベクトルを求める
		z = CCamera::Get()->GetPos();
		w = pPolyline->pointList[i];
		z.x -= w.x;
		z.y -= w.y;
		z.z -= w.z;
		// 点の接線ベクトルを求める
		if (i == 0) {
			if (nVertex > 1) {
				t = pPolyline->pointList[i + 1];
				w = pPolyline->pointList[i];
				t.x -= w.x;
				t.y -= w.y;
				t.z -= w.z;
			} else {
				t = CCamera::Get()->GetUpVector();
			}
		} else if (i == nVertex - 1) {
			t = pPolyline->pointList[i];
			w = pPolyline->pointList[i - 1];
			t.x -= w.x;
			t.y -= w.y;
			t.z -= w.z;
		} else {
			t = pPolyline->pointList[i + 1];
			w = pPolyline->pointList[i];
			t.x -= w.x;
			t.y -= w.y;
			t.z -= w.z;
		}
		// ストリップ頂点を求める
		vCross = XMVector3Cross(XMLoadFloat3(&t), XMLoadFloat3(&z));
		XMStoreFloat3(&cross, XMVector3Normalize(vCross));
		pVtx[0].vtx = pPolyline->pointList[i];
		pVtx[0].vtx.x += cross.x * pPolyline->fWidth;
		pVtx[0].vtx.y += cross.y * pPolyline->fWidth;
		pVtx[0].vtx.z += cross.z * pPolyline->fWidth;
		pVtx[0].diffuse = pPolyline->cDiffuse;
		pVtx[0].tex.x = 0.5f + i * 0.5f / nVertex;
		pVtx[0].tex.y = i * 0.5f / nVertex;
		pVtx[0].nor = XMFLOAT3(0.0f, 0.0f, 0.0f);
		pVtx[1].vtx = pPolyline->pointList[i];
		pVtx[1].vtx.x -= cross.x * pPolyline->fWidth;
		pVtx[1].vtx.y -= cross.y * pPolyline->fWidth;
		pVtx[1].vtx.z -= cross.z * pPolyline->fWidth;
		pVtx[1].diffuse = pPolyline->cDiffuse;
		pVtx[1].tex.x = i * 0.5f / nVertex;
		pVtx[1].tex.y = 0.5f + i * 0.5f / nVertex;
		pVtx[1].nor = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}
	// 点からカメラへの単位ベクトルを求める
	z = CCamera::Get()->GetPos();
	w = pPolyline->pointList[nVertex - 1];
	z.x -= w.x;
	z.y -= w.y;
	z.z -= w.z;
	// 点の接線ベクトルを求める
	if (nVertex > 1) {
		t = pPolyline->pointList[nVertex - 1];
		w = pPolyline->pointList[nVertex - 2];
		t.x -= w.x;
		t.y -= w.y;
		t.z -= w.z;
	} else {
		t = CCamera::Get()->GetUpVector();
	}
	// ストリップ頂点を求める
	if (pPolyline->bRound) {
		vCross = XMVector3Cross(XMLoadFloat3(&t), XMLoadFloat3(&z));
		vCross = XMVector3Cross(XMLoadFloat3(&z), vCross);
		XMStoreFloat3(&cross, XMVector3Normalize(vCross));
		pVtx->vtx = pPolyline->pointList[nVertex - 1];
		pVtx->vtx.x += cross.x * pPolyline->fWidth;
		pVtx->vtx.y += cross.y * pPolyline->fWidth;
		pVtx->vtx.z += cross.z * pPolyline->fWidth;
	} else {
		pVtx->vtx = pPolyline->pointList[nVertex - 1];
	}
	pVtx->diffuse = pPolyline->cDiffuse;
	pVtx->tex.x = 1.0f;
	pVtx->tex.y = 1.0f;
	pVtx->nor = XMFLOAT3(0.0f, 0.0f, 0.0f);

	// 頂点バッファ更新
	UpdatePolylineVertex(GetDeviceContext(), pPolyline);
}

//=============================================================================
// 描画処理
//=============================================================================
void DrawPolyline(TPolyline* pPolyline)
{
	if (!pPolyline) return;
	if (pPolyline->nNumVertex < 4) return;

	CLight::Get()->SetDisable();	// 光源無効
	SetCullMode(CULLMODE_NONE);		// カリング無し
	SetBlendState(pPolyline->bs);	// 半透明合成
	SetZWrite(false);				// Z更新無効

	// シェーダ設定
	ID3D11DeviceContext* pDC = GetDeviceContext();
	pDC->VSSetShader(g_pVertexShader, nullptr, 0);
	pDC->PSSetShader(g_pPixelShader, nullptr, 0);
	pDC->IASetInputLayout(g_pInputLayout);

	// 頂点バッファをセット
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	pDC->IASetVertexBuffers(0, 1, &pPolyline->pVertexBuffer, &stride, &offset);

	pDC->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

	pDC->PSSetSamplers(0, 1, &g_pSamplerState);
	pDC->PSSetShaderResources(0, 1, &pPolyline->pTexture);

	SHADER_GLOBAL cb;
	XMMATRIX mtxWorld = XMLoadFloat4x4(&pPolyline->mtxWorld);
	CCamera* pCamera = CCamera::Get();
	cb.mWVP = XMMatrixTranspose(mtxWorld * XMLoadFloat4x4(&pCamera->GetViewMatrix()) * XMLoadFloat4x4(&pCamera->GetProjMatrix()));
	cb.mW = XMMatrixTranspose(mtxWorld);
	cb.mTex = XMMatrixTranspose(XMLoadFloat4x4(&pPolyline->mtxTexture));
	pDC->UpdateSubresource(g_pConstantBuffer[0], 0, nullptr, &cb, 0, 0);
	pDC->VSSetConstantBuffers(0, 1, &g_pConstantBuffer[0]);
	SHADER_GLOBAL2 cb2;
	cb2.vEye = XMLoadFloat3(&pCamera->GetPos());
	CLight* pLight = CLight::Get();
	cb2.vLightDir = XMLoadFloat3(&pLight->GetDir());
	cb2.vLa = XMLoadFloat4(&pLight->GetAmbient());
	cb2.vLd = XMLoadFloat4(&pLight->GetDiffuse());
	cb2.vLs = XMLoadFloat4(&pLight->GetSpecular());
	cb2.vDiffuse = XMLoadFloat4(&g_material.Diffuse);
	cb2.vAmbient = XMVectorSet(g_material.Ambient.x, g_material.Ambient.y, g_material.Ambient.z, (pPolyline->pTexture != nullptr) ? 1.f : 0.f);
	cb2.vSpecular = XMVectorSet(g_material.Specular.x, g_material.Specular.y, g_material.Specular.z, g_material.Power);
	cb2.vEmissive = XMLoadFloat4(&g_material.Emissive);
	pDC->UpdateSubresource(g_pConstantBuffer[1], 0, nullptr, &cb2, 0, 0);
	pDC->PSSetConstantBuffers(1, 1, &g_pConstantBuffer[1]);

	// プリミティブ形状をセット
	pDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴンの描画
 	pDC->Draw(pPolyline->nNumVertex, 0);

	SetZWrite(true);			// Z更新有効
	SetBlendState(BS_NONE);		// αブレンド無効
	SetCullMode(CULLMODE_CCW);	// カリングあり (背面カリング)
	CLight::Get()->SetEnable();	// 光源有効
}

//=============================================================================
// 頂点バッファの作成
//=============================================================================
HRESULT CreatePolyline(TPolyline* pPolyline, float fWidth, bool bRound, XMFLOAT4 vColor, EBlendState bs)
{
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	ClearPolylinePoint(pPolyline);
	UpdatePolyline(pPolyline);
	initData.pSysMem = pPolyline->vertex;
	vbd.ByteWidth = sizeof(VERTEX_3D) * MAX_POLYLINE_VERTEX;
	ID3D11Device* pDevice = GetDevice();
	HRESULT hr = pDevice->CreateBuffer(&vbd, &initData, &pPolyline->pVertexBuffer);
	if (SUCCEEDED(hr)) {
		hr = CreateTextureFromFile(pDevice, TEXTURE_POLYLINE, &pPolyline->pTexture);
	}
	XMStoreFloat4x4(&pPolyline->mtxTexture, XMMatrixIdentity());

	XMStoreFloat4x4(&pPolyline->mtxWorld, XMMatrixIdentity());
	pPolyline->fWidth = fWidth;
	pPolyline->bRound = bRound;
	pPolyline->cDiffuse = vColor;
	pPolyline->bs = bs;
	return hr;
}

//=============================================================================
// ポリライン解放
//=============================================================================
void ReleasePolyline(TPolyline* pPolyline)
{
	if (!pPolyline) return;
	// テクスチャ解放
	SAFE_RELEASE(pPolyline->pTexture);
	// 頂点クリア
	ClearPolylinePoint(pPolyline);
	// 頂点バッファ解放
	SAFE_RELEASE(pPolyline->pVertexBuffer);
}

//=============================================================================
// 頂点バッファ更新
//=============================================================================
HRESULT UpdatePolylineVertex(ID3D11DeviceContext* pDC, TPolyline* pPolyline)
{
	// 頂点データの範囲をロックし、頂点バッファへのポインタを取得
	D3D11_MAPPED_SUBRESOURCE msr;
	HRESULT hr = pDC->Map(pPolyline->pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	if (SUCCEEDED(hr)) {
		// 頂点データを上書き
		rsize_t len = sizeof(VERTEX_3D) * pPolyline->nNumVertex;
		memcpy_s(msr.pData, len, pPolyline->vertex, len);
		// 頂点データをアンロックする
		pDC->Unmap(pPolyline->pVertexBuffer, 0);
	}
	return hr;
}

//=============================================================================
// 全頂点削除
//=============================================================================
void ClearPolylinePoint(TPolyline* pPolyline)
{
	if (pPolyline)
		pPolyline->pointList.clear();
}

//=============================================================================
// 頂点追加
//=============================================================================
void AddPolylinePoint(TPolyline* pPolyline, DirectX::XMFLOAT3 pos)
{
	if (pPolyline)
		pPolyline->pointList.push_back(pos);
}
