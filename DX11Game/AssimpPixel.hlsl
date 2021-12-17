// Assimp用ピクセルシェーダ (AssimpPixel.hlsl)

// グローバル
cbuffer global : register(b0) {
	matrix	g_mtxWVP;			// ワールド×ビュー×射影行列
	matrix	g_mtxWorld;			// ワールド行列
	matrix	g_mtxTexture;		// テクスチャ行列
	float4	g_vCameraPos;		// 視点座標(ワールド空間)
	float4	g_vLightDir;		// 光源方向(ワールド空間)
	float4	g_vLightAmbient;	// 環境光
	float4	g_vLightDiffuse;	// 拡散光
	float4	g_vLightSpecular;	// 鏡面反射光
};

// マテリアル
cbuffer global2 : register(b1) {
	float4	g_Ambient;			// 環境色
	float4	g_Diffuse;			// 拡散色+アルファ
	float4	g_Specular;			// 鏡面反射色+強度
	float4	g_Emissive;			// 発光色
	float4	g_Flags;			// 拡散色テクスチャ有無, 発光色テクスチャ有無, 透過テクスチャ有無
};

Texture2D    g_texture			: register(t0);	// テクスチャ
Texture2D    g_texEmissive		: register(t1);	// 発光テクスチャ
Texture2D    g_texTransparent	: register(t2);	// 透過テクスチャ
Texture2D    g_texSpecular		: register(t3);	// 鏡面反射テクスチャ
SamplerState g_sampler			: register(s0);	// サンプラ

// パラメータ
struct VS_OUTPUT {
	float4	Pos			: SV_Position;
	float2	Tex			: TEXCOORD0;
	float3	Normal		: TEXCOORD1;
	float3	PosForPS	: TEXCOORD2;
};

//
// ピクセルシェーダ
//
float4 main(VS_OUTPUT input) : SV_Target0
{
	float3 Diff = g_Diffuse.rgb;
	float Alpha = g_Diffuse.a;
	if (g_Flags.x != 0.0f) {		// テクスチャ有無
		float4 TexDiff = g_texture.Sample(g_sampler, input.Tex);
		Diff *= TexDiff.rgb;
		Alpha *= TexDiff.a;
	}
	if (g_Flags.z != 0.0f) {		// テクスチャ有無
		float4 TexTran = g_texTransparent.Sample(g_sampler, input.Tex);
		Alpha *= (TexTran.r * TexTran.g * TexTran.b * TexTran.a);
	}
	if (Alpha <= 0.0f) discard;		// 完全透明なら描画しない
	float3 Spec = g_Specular.rgb;
	if (g_Flags.w != 0.0f) {		// テクスチャ有無
		float4 TexSpec = g_texSpecular.Sample(g_sampler, input.Tex);
		Spec *= TexSpec.rgb;
	}

	if (g_vLightDir.x != 0.0f || g_vLightDir.y != 0.0f || g_vLightDir.z != 0.0f) {
		float3 L = normalize(-g_vLightDir.xyz);					// 光源へのベクトル
		float3 N = normalize(input.Normal);						// 法線ベクトル
		float3 V = normalize(g_vCameraPos.xyz - input.PosForPS);// 視点へのベクトル
		float3 H = normalize(L + V);							// ハーフベクトル
		Diff = g_vLightAmbient.rgb * g_Ambient.rgb +
			g_vLightDiffuse.rgb * Diff * saturate(dot(L, N));	// 拡散色 + 環境色
		Spec = Spec * g_vLightSpecular.rgb *
			pow(saturate(dot(N, H)), g_Specular.a);				// 鏡面反射色
		Diff += Spec;
	}

	float3 Emis = g_Emissive.rgb;
	if (g_Flags.y != 0.0f) {		// テクスチャ有無
		float4 TexEmis = g_texEmissive.Sample(g_sampler, input.Tex);
		Emis *= (TexEmis.rgb * TexEmis.a);
	}
	Diff += Emis;

	return float4(Diff, Alpha);
}
