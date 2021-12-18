//=============================================================================
//
// メイン処理 [main.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "AssimpModel.h"
#include "main.h"
#include "input.h"
#include "polygon.h"
#include "debugproc.h"
#include "mesh.h"
#include "meshfield.h"
#include "player.h"
#include "shadow.h"
#include "bg.h"
#include "bullet.h"
#include "explosion.h"
#include "effect.h"
#include "smoke.h"
#include "meshwall.h"
#include "polyline.h"
#include "billboard.h"
#include "Wall.h"
#include "EnemyMelee.h"
//#include "pause.h"

//-------- ライブラリのリンク
#pragma comment(lib, "winmm")
#pragma comment(lib, "imm32")
#pragma comment(lib, "d3d11")

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define CLASS_NAME		_T("AppClass")			// ウインドウのクラス名
#define WINDOW_NAME		_T("ポリライン処理")	// ウインドウのキャプション名

#define MAX_POLYLINE	(20)					// ポリライン数
#define THICKNESS		(10.0f)					// 線の太さ

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int OnCreate(HWND hWnd, LPCREATESTRUCT lpcs);
HRESULT Init(HWND hWnd, BOOL bWindow);
void Uninit(void);
void Update(void);
void Draw(void);

//*****************************************************************************
// グローバル変数:
//*****************************************************************************
HWND						g_hWnd;					// メイン ウィンドウ ハンドル
HINSTANCE					g_hInst;				// インスタンス ハンドル

ID3D11Device*				g_pDevice;				// デバイス
ID3D11DeviceContext*		g_pDeviceContext;		// デバイス コンテキスト
IDXGISwapChain*				g_pSwapChain;			// スワップチェーン
ID3D11RenderTargetView*		g_pRenderTargetView;	// フレームバッファ
ID3D11Texture2D*			g_pDepthStencilTexture;	// Zバッファ用メモリ
ID3D11DepthStencilView*		g_pDepthStencilView;	// Zバッファ
UINT						g_uSyncInterval = 0;	// 垂直同期 (0:無, 1:有)
ID3D11RasterizerState*		g_pRs[MAX_CULLMODE];	// ラスタライザ ステート
ID3D11BlendState*			g_pBlendState[MAX_BLENDSTATE];// ブレンド ステート
ID3D11DepthStencilState*	g_pDSS[2];				// Z/ステンシル ステート

int							g_nCountFPS;			// FPSカウンタ
static bool g_bPause;								//一時停止中

TPolyline					g_polyline[MAX_POLYLINE];	// ポリライン情報

//=============================================================================
// メイン関数
//=============================================================================
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);	// 未使用宣言
	UNREFERENCED_PARAMETER(lpCmdLine);		// 未使用宣言

	DWORD dwExecLastTime;
	DWORD dwFPSLastTime;
	DWORD dwCurrentTime;
	DWORD dwFrameCount;

	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0,
		0,
		hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINFRM)),
		LoadCursor(nullptr, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		nullptr,
		CLASS_NAME,
		nullptr
	};
	MSG msg;
	
	// COM初期化
	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
		MessageBox(NULL, _T("COMの初期化に失敗しました。"), _T("error"), MB_OK);
		return -1;
	}

	// インスタンス ハンドル保存
	g_hInst = hInstance;

	// ウィンドウクラスの登録
	RegisterClassEx(&wcex);

	// クライアント領域サイズからウィンドウ サイズ算出
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION
		| WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX;
	DWORD dwExStyle = 0;
	RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);

	// ウィンドウの作成
	g_hWnd = CreateWindowEx(dwExStyle,
		CLASS_NAME,
		WINDOW_NAME,
		dwStyle,
		CW_USEDEFAULT,		// ウィンドウの左座標
		CW_USEDEFAULT,		// ウィンドウの上座標
		rc.right - rc.left,	// ウィンドウ横幅
		rc.bottom - rc.top,	// ウィンドウ縦幅
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	// フレームカウント初期化
	timeBeginPeriod(1);				// 分解能を設定
	dwExecLastTime = dwFPSLastTime = timeGetTime();
	dwCurrentTime = dwFrameCount = 0;

	// ウインドウの表示(初期化処理の後に呼ばないと駄目)
	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);
	
	// DirectXの初期化(ウィンドウを作成してから行う)
	if (FAILED(Init(g_hWnd, true))) {
		return -1;
	}

	// メッセージループ
	for (;;) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				// PostQuitMessage()が呼ばれたらループ終了
				break;
			} else {
				// メッセージの翻訳とディスパッチ
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else {
			dwCurrentTime = timeGetTime();
			if ((dwCurrentTime - dwFPSLastTime) >= 500) {	// 0.5秒ごとに実行
				g_nCountFPS = dwFrameCount * 1000 / (dwCurrentTime - dwFPSLastTime);
				dwFPSLastTime = dwCurrentTime;
				dwFrameCount = 0;
			}
			if ((dwCurrentTime - dwExecLastTime) >= (1000 / 60)) {
				dwExecLastTime = dwCurrentTime;
				// 更新処理
				Update();
			}
			// 描画処理
			Draw();
			dwFrameCount++;
		}
	}

	// タイマ設定を元に戻す
	timeEndPeriod(1);

	// 終了処理
	Uninit();

	// ウィンドウクラスの登録を解除
	UnregisterClass(CLASS_NAME, g_hInst);

	// COM終了処理
	CoUninitialize();

	return (int)msg.wParam;
}

//=============================================================================
// プロシージャ
//=============================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:					//----- ウィンドウが生成された
		return OnCreate(hWnd, (LPCREATESTRUCT)lParam);
	case WM_DESTROY:				//----- ウィンドウ破棄指示がきた
		PostQuitMessage(0);				// システムにスレッドの終了を要求
		break;
	case WM_KEYDOWN:				//----- キーボードが押された
		switch (wParam) {
		case VK_ESCAPE:					// [ESC]キーが押された
			PostMessage(hWnd, WM_CLOSE, 0, 0);	// [x]が押されたように振舞う
			return 0;
		}
		break;
	case WM_MENUCHAR:
		return MNC_CLOSE << 16;			// [Alt]+[Enter]時のBEEPを抑止
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//=============================================================================
// WM_CREATEメッセージハンドラ
//=============================================================================
int OnCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
	// クライアント領域サイズをSCREEN_WIDTH×SCREEN_HEIGHTに再設定.
	RECT rcClnt;
	GetClientRect(hWnd, &rcClnt);
	rcClnt.right -= rcClnt.left;
	rcClnt.bottom -= rcClnt.top;
	if (rcClnt.right != SCREEN_WIDTH || rcClnt.bottom != SCREEN_HEIGHT) {
		RECT rcWnd;
		GetWindowRect(hWnd, &rcWnd);
		SIZE sizeWnd;
		sizeWnd.cx = (rcWnd.right - rcWnd.left) - rcClnt.right + SCREEN_WIDTH;
		sizeWnd.cy = (rcWnd.bottom - rcWnd.top) - rcClnt.bottom + SCREEN_HEIGHT;
		SetWindowPos(hWnd, nullptr, 0, 0, sizeWnd.cx, sizeWnd.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	}

	// IME無効化
	ImmAssociateContext(hWnd, nullptr);

	return 0;	// -1を返すとCreateWindow[Ex]が失敗する.
}

//=============================================================================
// バックバッファ生成
//=============================================================================
HRESULT CreateBackBuffer(void)
{
	// レンダーターゲットビュー生成
	ID3D11Texture2D* pBackBuffer = nullptr;
	g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	g_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	SAFE_RELEASE(pBackBuffer);

	// Zバッファ用テクスチャ生成
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = SCREEN_WIDTH;
	td.Height = SCREEN_HEIGHT;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	HRESULT hr = g_pDevice->CreateTexture2D(&td, nullptr, &g_pDepthStencilTexture);
	if (FAILED(hr)) {
		return hr;
	}

	// Zバッファターゲットビュー生成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));
	dsvd.Format = td.Format;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	hr = g_pDevice->CreateDepthStencilView(g_pDepthStencilTexture,
		&dsvd, &g_pDepthStencilView);
	if (FAILED(hr)) {
		return hr;
	}

	// 各ターゲットビューをレンダーターゲットに設定
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// ビューポート設定
	D3D11_VIEWPORT vp;
	vp.Width = (float)SCREEN_WIDTH;
	vp.Height = (float)SCREEN_HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pDeviceContext->RSSetViewports(1, &vp);

	return S_OK;
}

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT Init(HWND hWnd, BOOL bWindow)
{
	HRESULT hr = S_OK;

	// デバイス、スワップチェーンの作成
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(scd));
	scd.BufferCount = 1;
	scd.BufferDesc.Width = SCREEN_WIDTH;
	scd.BufferDesc.Height = SCREEN_HEIGHT;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = bWindow;

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
		nullptr, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &scd,
		&g_pSwapChain, &g_pDevice, nullptr, &g_pDeviceContext);
	if (FAILED(hr)) {
		return hr;
	}

	// バックバッファ生成
	hr = CreateBackBuffer();
	if (FAILED(hr)) {
		return hr;
	}

	// ラスタライズ設定
	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;	// カリング無し(両面描画)
	g_pDevice->CreateRasterizerState(&rd, &g_pRs[0]);
	rd.CullMode = D3D11_CULL_FRONT;	// 前面カリング(裏面描画)
	g_pDevice->CreateRasterizerState(&rd, &g_pRs[1]);
	rd.CullMode = D3D11_CULL_BACK;	// 背面カリング(表面描画)
	g_pDevice->CreateRasterizerState(&rd, &g_pRs[2]);
	g_pDeviceContext->RSSetState(g_pRs[2]);

	// ブレンド ステート生成
	D3D11_BLEND_DESC BlendDesc;
	ZeroMemory(&BlendDesc, sizeof(BlendDesc));
	BlendDesc.AlphaToCoverageEnable = FALSE;
	BlendDesc.IndependentBlendEnable = FALSE;
	BlendDesc.RenderTarget[0].BlendEnable = FALSE;
	BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	g_pDevice->CreateBlendState(&BlendDesc, &g_pBlendState[0]);
	// ブレンド ステート生成 (アルファ ブレンド用)
	//BlendDesc.AlphaToCoverageEnable = TRUE;
	BlendDesc.RenderTarget[0].BlendEnable = TRUE;
	g_pDevice->CreateBlendState(&BlendDesc, &g_pBlendState[1]);
	// ブレンド ステート生成 (加算合成用)
	BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	g_pDevice->CreateBlendState(&BlendDesc, &g_pBlendState[2]);
	// ブレンド ステート生成 (減算合成用)
	BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	g_pDevice->CreateBlendState(&BlendDesc, &g_pBlendState[3]);
	SetBlendState(BS_ALPHABLEND);

	// 深度ステンシルステート生成
	CD3D11_DEFAULT def;
	CD3D11_DEPTH_STENCIL_DESC dsd(def);
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	g_pDevice->CreateDepthStencilState(&dsd, &g_pDSS[0]);
	CD3D11_DEPTH_STENCIL_DESC dsd2(def);
	dsd2.DepthEnable = FALSE;
	g_pDevice->CreateDepthStencilState(&dsd2, &g_pDSS[1]);

	// ポリゴン表示初期化
	hr = InitPolygon(g_pDevice);
	if (FAILED(hr))
		return hr;

	// デバッグ文字列表示初期化
	hr = InitDebugProc();
	if (FAILED(hr))
		return hr;

	// 入力処理初期化
	hr = InitInput();
	if (FAILED(hr))
		return hr;

	// Assimp用シェーダ初期化
	if (!CAssimpModel::InitShader(g_pDevice))
		return E_FAIL;

	// メッシュ初期化
	hr = InitMesh();
	if (FAILED(hr))
		return hr;

	// 丸影初期化
	hr = InitShadow();
	if (FAILED(hr))
		return hr;

	// 自機初期化
	hr = InitPlayer();
	if (FAILED(hr))
		return hr;

	// フィールド初期化
	hr = InitMeshField(16, 16, 80.0f, 80.0f);
	if (FAILED(hr))
		return hr;

	// 背景初期化
	hr = InitBG();
	if (FAILED(hr))
		return hr;

	// ビルボード弾初期化
	hr = InitBullet();
	if (FAILED(hr))
		return hr;

	// 爆発初期化
	hr = InitExplosion();
	if (FAILED(hr))
		return hr;

	// エフェクト初期化
	hr = InitEffect();
	if (FAILED(hr))
		return hr;

	// 煙初期化
	hr = InitSmoke();
	if (FAILED(hr))
		return hr;

	hr = InitBillboard();
	if (FAILED(hr))
		return hr;

	// 壁初期化
	hr = InitMeshWall();
	if (FAILED(hr))
		return hr;
	//					位置								向き						色											サイズ
	SetMeshWall(XMFLOAT3(   0.0f, 0.0f,  640.0f), XMFLOAT3(0.0f,   0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3(-640.0f, 0.0f,    0.0f), XMFLOAT3(0.0f, -90.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3( 640.0f, 0.0f,    0.0f), XMFLOAT3(0.0f,  90.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3(   0.0f, 0.0f, -640.0f), XMFLOAT3(0.0f, 180.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3(   0.0f, 0.0f,  640.0f), XMFLOAT3(0.0f, 180.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 0.2f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3(-640.0f, 0.0f,    0.0f), XMFLOAT3(0.0f,  90.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 0.2f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3( 640.0f, 0.0f,    0.0f), XMFLOAT3(0.0f, -90.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 0.2f), 16, 2, XMFLOAT2(80.0f, 80.0f));
	SetMeshWall(XMFLOAT3(   0.0f, 0.0f, -640.0f), XMFLOAT3(0.0f,   0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 0.2f), 16, 2, XMFLOAT2(80.0f, 80.0f));

	//SetMeshWall(XMFLOAT3(   0.0f, 30.0f, 20.0f), XMFLOAT3(0.0f, 0.0f, 0.0f),XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), 16, 2, XMFLOAT2(4.0f, 40.0f));
	//(XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT4 col,int nNumBlockX, int nNumBlockY, XMFLOAT2 sizeBlock)

	// ボリライン初期化
	hr = InitPolyline();
	if (FAILED(hr))
		return hr;
	XMFLOAT4 vColor[8] = {
		XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
		XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f),
		XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
		XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f),
		XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f),
		XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f),
		XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
	};
	for (int i = 0; i < MAX_POLYLINE; ++i) {
		hr = CreatePolyline(&g_polyline[i], THICKNESS, true, vColor[i % 7 + 1], BS_ADDITIVE);
		if (FAILED(hr)) {
			return hr;
		}
		XMFLOAT3 pos;
		pos.x = rand() % 1240 - 620.0f;
		pos.y = rand() % 140 + 10.0f;
		pos.z = rand() % 1240 - 620.0f;
		AddPolylinePoint(&g_polyline[i], pos);
		pos.x = rand() % 1240 - 620.0f;
		pos.y = rand() % 140 + 10.0f;
		pos.z = rand() % 1240 - 620.0f;
		AddPolylinePoint(&g_polyline[i], pos);
	}

	hr = InitWall();
	SetWall(XMFLOAT3(0.0f, 50.0f, 150.0f));
	/*SetWall(XMFLOAT3(100.0f, 50.0f, 150.0f));
	SetWall(XMFLOAT3(-100.0f, 50.0f, 150.0f));
	SetWall(XMFLOAT3(200.0f, 50.0f, 150.0f));
	SetWall(XMFLOAT3(-200.0f, 50.0f, 150.0f));*/
	
	if (FAILED(hr))
	return hr;

	hr = InitEnemyMelee();
	
	//SetEnemyMelee(XMFLOAT3(rand() % 620 - 310.0f, 20.0f, rand() % 620 - 310.0f));
	
	SetEnemyMelee(XMFLOAT3(0.0f, 40.0f, 0.0f));
	SetEnemyMelee(XMFLOAT3(-200.0f, 40.0f, 0.0f));
	SetEnemyMelee(XMFLOAT3(200.0f, 40.0f, 0.0f));
	
	if (FAILED(hr))
		return hr;
	
	/*hr = InitPause();
	if (FAILED(hr))
	return hr;
	g_bPause = false;*/

}

//=============================================================================
// バックバッファ解放
//=============================================================================
void ReleaseBackBuffer()
{
	if (g_pDeviceContext) {
		g_pDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	}
	SAFE_RELEASE(g_pDepthStencilView);
	SAFE_RELEASE(g_pDepthStencilTexture);
	SAFE_RELEASE(g_pRenderTargetView);
}

//=============================================================================
// 終了処理
//=============================================================================
void Uninit(void)
{
	//ポーズ終了処理
	//UninitPause();

	// 近接敵終了
	UninitEnemyMelee();

	// 壁終了処理
	UninitWall();

	// ポリライン終了処理
	UninitPolyline();

	// 壁終了処理
	UninitMeshWall();

	UninitBillboard();

	// 煙終了処理
	UninitSmoke();

	// エフェクト終了処理
	UninitEffect();

	// 爆発終了処理
	UninitExplosion();

	// ビルボード弾終了処理
	UninitBullet();

	// 背景終了処理
	UninitBG();

	// フィールド終了処理
	UninitMeshField();

	// 自機終了処理
	UninitPlayer();

	// 丸影終了処理
	UninitShadow();

	// メッシュ終了処理
	UninitMesh();

	// Assimp用シェーダ終了処理
	CAssimpModel::UninitShader();

	// 入力処理終了処理
	UninitInput();

	// デバッグ文字列表示終了処理
	UninitDebugProc();

	// ポリゴン表示終了処理
	UninitPolygon();

	// 深度ステンシルステート解放
	for (int i = 0; i < _countof(g_pDSS); ++i) {
		SAFE_RELEASE(g_pDSS[i]);
	}

	// ブレンド ステート解放
	for (int i = 0; i < MAX_BLENDSTATE; ++i) {
		SAFE_RELEASE(g_pBlendState[i]);
	}

	// ラスタライザ ステート解放
	for (int i = 0; i < MAX_CULLMODE; ++i) {
		SAFE_RELEASE(g_pRs[i]);
	}

	// バックバッファ解放
	ReleaseBackBuffer();

	// スワップチェーン解放
	SAFE_RELEASE(g_pSwapChain);

	// デバイス コンテキストの開放
	SAFE_RELEASE(g_pDeviceContext);

	// デバイスの開放
	SAFE_RELEASE(g_pDevice);
}

//=============================================================================
// 更新処理
//=============================================================================
void Update(void)
{
	//一時停止中?
	//if (g_bPause) {
	//	//一時停止更新
	//	UpdatePause();
	//}
	//else
	//{
		// 入力処理更新
		UpdateInput();	// 必ずUpdate関数の先頭で実行.

		// デバッグ文字列表示更新
		UpdateDebugProc();

		// デバッグ文字列設定
		StartDebugProc();
		PrintDebugProc("FPS:%d\n\n", g_nCountFPS);

		// ポリゴン表示更新
		UpdatePolygon();

		// 自機更新
		UpdatePlayer();

		// 背景更新
		UpdateBG();

		// フィールド更新
		UpdateMeshField();

		// 丸影更新
		UpdateShadow();

		// カメラ更新
		CCamera::Get()->Update();

		// ビルボード弾更新
		UpdateBullet();

		// 爆発更新
		UpdateExplosion();

		// エフェクト更新
		UpdateEffect();

		// 煙更新
		UpdateSmoke();

		UpdateBillboard();

		// ポリライン更新
		for (int i = 0; i < MAX_POLYLINE; ++i) {
			UpdatePolyline(&g_polyline[i]);
		}

		// メッシュ壁更新
		UpdateMeshWall();

		// 壁更新
		UpdateWall();

		// 近接敵更新
		UpdateEnemyMelee();

		// ポーズ更新
		//UpdatePause();
	//}
	//一時停止ON/OFF
	//if (GetKeyTrigger(VK_P) || GetKeyTrigger(VK_PAUSE))
	//{
	//	//if (GetFade() == FADE_NONE) {
	//		g_bPause = !g_bPause;
	//		if (g_bPause) {
	//			//CSound::Pause();
	//			//CSound::Play(SE_DECIDE);
	//			ResetPauseMenu();
	//		}
	//		else {
	//			//CSound::Play(SE_CANCEL);
	//			//CSound::Resume();
	//		}
	//	//}
	//}

	//一時停止メニューの選択
	//if (g_bPause && GetFade() == FADE_NONE) {
		//[ENTER]が押された?
		//if (GetKeyTrigger(VK_RETURN)) {
		//	//選択中のメニュー項目により分岐
		//	switch (GetPauseMenu()) {
		//	case PAUSE_MENU_CONTINUE:
		//		g_bPause = false;
		//		//CSound::Play(SE_CANCEL);
		//		//CSound::Resume();
		//		break;
		//	case PAUSE_MENU_RETRY:
		//		//StartFadeOut(SCENE_GAME);
		//		break;
		//	case PAUSE_MENU_QUIT:
		//		//StartFadeOut(SCENE_TITLE);
		//		break;
		//	}
		//}
	//}
}

//=============================================================================
// 描画処理
//=============================================================================
void Draw(void)
{
	// バックバッファ＆Ｚバッファのクリア
	float ClearColor[4] = { 0.117647f, 0.254902f, 0.352941f, 1.0f };
	g_pDeviceContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pDeviceContext->ClearDepthStencilView(g_pDepthStencilView,
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Zバッファ無効(Zチェック無&Z更新無)
	SetZBuffer(false);

	// 背景描画
	DrawBG();

	// Zバッファ有効(Zチェック有&Z更新有)
	SetZBuffer(true);

	// フィールド描画
	DrawMeshField();

	// 壁描画 (不透明部分)
	DrawMeshWall(DRAWPART_OPAQUE);

	// 自機描画
	DrawPlayer();

	// 丸影描画
	DrawShadow();

	// ビルボード弾描画
	DrawBullet();

	// 煙描画
	DrawSmoke();

	// エフェクト描画
	DrawEffect();

	// 爆発描画
	DrawExplosion();

	DrawBillboard();

	// ポリライン描画
	/*for (int i = 0; i < MAX_POLYLINE; ++i) {
		DrawPolyline(&g_polyline[i]);
	}*/

	// 壁描画 (半透明部分)
	DrawMeshWall(DRAWPART_TRANSLUCENT);

	DrawWall();

	DrawEnemyMelee();

	//SetZBuffer(false);
	//DrawPause();
	////一時停止描画
	//if (g_bPause) {
	//	DrawPause();
	//}
	//SetZBuffer(true);
	
	// Zバッファ無効(Zチェック無&Z更新無)
	SetZBuffer(false);

	// デバッグ文字列表示
	SetBlendState(BS_ALPHABLEND);
	DrawDebugProc();
	SetBlendState(BS_NONE);

	// バックバッファとフロントバッファの入れ替え
	g_pSwapChain->Present(g_uSyncInterval, 0);
}

//=============================================================================
// メイン ウィンドウ ハンドル取得
//=============================================================================
HWND GetMainWnd()
{
	return g_hWnd;
}

//=============================================================================
// インスタンス ハンドル取得
//=============================================================================
HINSTANCE GetInstance()
{
	return g_hInst;
}

//=============================================================================
// デバイス取得
//=============================================================================
ID3D11Device* GetDevice()
{
	return g_pDevice;
}

//=============================================================================
// デバイス コンテキスト取得
//=============================================================================
ID3D11DeviceContext* GetDeviceContext()
{
	return g_pDeviceContext;
}

//=============================================================================
// 深度バッファ有効無効制御
//=============================================================================
void SetZBuffer(bool bEnable)
{
	g_pDeviceContext->OMSetDepthStencilState((bEnable) ? nullptr : g_pDSS[1], 0);
}

//=============================================================================
// 深度バッファ更新有効無効制御
//=============================================================================
void SetZWrite(bool bEnable)
{
	g_pDeviceContext->OMSetDepthStencilState((bEnable) ? nullptr : g_pDSS[0], 0);
}

//=============================================================================
// ブレンド ステート設定
//=============================================================================
void SetBlendState(int nBlendState)
{
	if (nBlendState >= 0 && nBlendState < MAX_BLENDSTATE) {
		float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		g_pDeviceContext->OMSetBlendState(g_pBlendState[nBlendState], blendFactor, 0xffffffff);
	}
}

//=============================================================================
// カリング設定
//=============================================================================
void SetCullMode(int nCullMode)
{
	if (nCullMode >= 0 && nCullMode < MAX_CULLMODE) {
		g_pDeviceContext->RSSetState(g_pRs[nCullMode]);
	}
}
