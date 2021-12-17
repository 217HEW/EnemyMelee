//**************************************************************
//
//	EnemyMelee.h
//	�v���C���[��Ǐ]����ߐړG
//
//--------------------------------------------------------------
//	����ҁF�Ό����l
//--------------------------------------------------------------
//	�J������
//	2021/12/15 �v���C���[��Ǐ]���铮���̎���
//
//**************************************************************

//**************************************************************
// �C���N���[�h��
//**************************************************************
#include "EnemyMelee.h"
#include "AssimpModel.h"
#include "debugproc.h"
#include "collision.h"
#include "main.h"
#include "player.h"

//**************************************************************
// �\���̒�`
//**************************************************************
struct TEnemy {
	XMFLOAT3	m_pos;		// ���݂̈ʒu
	XMFLOAT3	m_rot;		// ���݂̌���
	XMFLOAT3	m_rotDest;	// �ړI�̌���
	XMFLOAT3	m_move;		// �ړ���
	bool		m_use;		// �g�p���Ă邩�ۂ�	ON:�g�p��

	XMFLOAT4X4	m_mtxWorld;	// ���[���h�}�g���b�N�X
};

//**************************************************************
// �}�N����`
//**************************************************************
#define MODEL_ENEMY			"data/model/helicopter000.fbx"

#define	VALUE_MOVE_ENEMY		(1.0f)		// �ړ����x
#define MAX_ENEMYMELEE			(10)		// �G�@�ő吔

#define	VALUE_ROTATE_ENEMY	(7.0f)		// ��]���x
#define	RATE_ROTATE_ENEMY	(0.20f)		// ��]�����W��

enum
{
	WAIT,
	MOVE,

	MAX
};


//**************************************************************
// �O���[�o���ϐ�
//**************************************************************
static CAssimpModel	g_model;			// ���f��
static TEnemy		g_EMelee[MAX_ENEMYMELEE];	// �G�@���

//**************************************************************
// ����������
//**************************************************************
HRESULT InitEnemyMelee(void)
{
	HRESULT hr = S_OK;
	ID3D11Device* pDevice = GetDevice();
	ID3D11DeviceContext* pDeviceContext = GetDeviceContext();

	// ���f���f�[�^�̓ǂݍ���
	if (!g_model.Load(pDevice, pDeviceContext, MODEL_ENEMY))
	{
		MessageBoxA(GetMainWnd(), "���f���f�[�^�ǂݍ��݃G���[", "InitEnemy", MB_OK);
		return E_FAIL;
	}

	for (int i = 0; i < MAX_ENEMYMELEE; ++i)
	{// ���������������m������΂����Ɂ�
		g_EMelee[i].m_pos = (XMFLOAT3(0.0f, 0.0f, 0.0f));
		g_EMelee[i].m_move = (XMFLOAT3(0.0f, 0.0f, 0.0f));
		g_EMelee[i].m_rot = (XMFLOAT3(3.0f, 0.0f, 0.0f));
		g_EMelee[i].m_rotDest = g_EMelee[i].m_rot;
		g_EMelee[i].m_use = false;
	}

	//for (int i = 0; i < MAX_ENEMYMELEE; ++i)
	//{
	//	// �ʒu�E��]�E�X�P�[���̏����ݒ�
	//	g_EMelee[i].m_pos = XMFLOAT3(rand() % 620 - 310.0f, 20.0f, rand() % 620 - 310.0f);
	//	g_EMelee[i].m_rot = XMFLOAT3(0.0f, rand() % 360 - 180.0f, 0.0f);
	//	g_EMelee[i].m_rotDest = g_EMelee[i].m_rot;
	//	g_EMelee[i].m_move = XMFLOAT3(-SinDeg(g_EMelee[i].m_rot.y) * VALUE_MOVE_ENEMY,0.0f,
	//		-CosDeg(g_EMelee[i].m_rot.y) * VALUE_MOVE_ENEMY);
	//}

	
	return hr;
}

//**************************************************************
// �I������
//**************************************************************
void UninitEnemyMelee(void)
{
	// ���f���̉��
	g_model.Release();
}

//**************************************************************
// �X�V����
//**************************************************************
void UpdateEnemyMelee(void)
{
	XMMATRIX mtxWorld, mtxRot, mtxTranslate;

	//�v���C���[�̍��W�擾
	XMFLOAT3 posPlayer = GetPlayerPos();


	for (int i = 0; i < MAX_ENEMYMELEE; ++i)
	{
		if (!g_EMelee[i].m_use)
		{
			continue;
		}
		
		//��Ƀv���C���[�̕���������
		/*g_EMelee[i].m_rotDest = posPlayer;
		
		g_EMelee[i].m_rot = g_EMelee[i].m_rotDest;

		g_EMelee[i].m_rot = XMFLOAT3(posPlayer.x, posPlayer.y, posPlayer.z);*/

		
		//�G��x���W���v���C���[�����傫��������
		if (g_EMelee[i].m_pos.x >= posPlayer.x)
		{
			g_EMelee[i].m_pos.x += -VALUE_MOVE_ENEMY;
			g_EMelee[i].m_rot = (XMFLOAT3(0.0f, 90.0f, 0.0f));

		}
		//�G��x���W���v���C���[����������������
		if (g_EMelee[i].m_pos.x <= posPlayer.x)
		{
			g_EMelee[i].m_pos.x += VALUE_MOVE_ENEMY;
			g_EMelee[i].m_rot = (XMFLOAT3(0.0f, -90.0f, 0.0f));
	
		}
		//�G��z���W���v���C���[�����傫��������
		if (g_EMelee[i].m_pos.z >= posPlayer.z)
		{
			g_EMelee[i].m_pos.z += -VALUE_MOVE_ENEMY;

		}
		//�G��z���W���v���C���[����������������
		if (g_EMelee[i].m_pos.z <= posPlayer.z)
		{
			g_EMelee[i].m_pos.z += VALUE_MOVE_ENEMY;
		
		}

		// �G���m�̓����蔻��
		if (CollisionSphere(g_EMelee[i].m_pos, 15, g_EMelee[i + 1].m_pos, 15))
		{
			g_EMelee[i + 1].m_pos.x += VALUE_MOVE_ENEMY;
			//= 10 * 10;
		}

		 //�ړI�̊p�x�܂ł̍���
		float fDiffRotY = g_EMelee[i].m_rotDest.y - g_EMelee[i].m_rot.y;
		if (fDiffRotY >= 180.0f) {
			fDiffRotY -= 360.0f;
		}
		if (fDiffRotY < -180.0f) {
			fDiffRotY += 360.0f;
		}

		// �ړI�̊p�x�܂Ŋ�����������
		g_EMelee[i].m_rot.y += fDiffRotY * RATE_ROTATE_ENEMY;
		if (g_EMelee[i].m_rot.y >= 180.0f) {
			g_EMelee[i].m_rot.y -= 360.0f;
		}
		if (g_EMelee[i].m_rot.y < -180.0f) {
			g_EMelee[i].m_rot.y += 360.0f;
		}
		g_EMelee[i].m_move = XMFLOAT3(
			-SinDeg(g_EMelee[i].m_rot.y) * VALUE_MOVE_ENEMY,
			0.0f,
			-CosDeg(g_EMelee[i].m_rot.y) * VALUE_MOVE_ENEMY);


		// ���[���h�}�g���b�N�X�̏�����
		mtxWorld = XMMatrixIdentity();

		// ��]�𔽉f
		mtxRot = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(g_EMelee[i].m_rot.x),
			XMConvertToRadians(g_EMelee[i].m_rot.y),
			XMConvertToRadians(g_EMelee[i].m_rot.z));
		mtxWorld = XMMatrixMultiply(mtxWorld, mtxRot);

		// �ړ��𔽉f
		mtxTranslate = XMMatrixTranslation(
			g_EMelee[i].m_pos.x,
			g_EMelee[i].m_pos.y,
			g_EMelee[i].m_pos.z);
		mtxWorld = XMMatrixMultiply(mtxWorld, mtxTranslate);

		// ���[���h�}�g���b�N�X�ݒ�
		XMStoreFloat4x4(&g_EMelee[i].m_mtxWorld, mtxWorld);
	}
}

//**************************************************************
// �`�揈��
//**************************************************************
void DrawEnemyMelee(void)
{

	ID3D11DeviceContext* pDC = GetDeviceContext();

	// �s����������`��
	for (int i = 0; i < MAX_ENEMYMELEE; ++i) {
		if (!g_EMelee[i].m_use)
		{
			continue;
		}
		g_model.Draw(pDC, g_EMelee[i].m_mtxWorld, eOpacityOnly);
	}

	// ������������`��
	for (int i = 0; i < MAX_ENEMYMELEE; ++i) {
		SetBlendState(BS_ALPHABLEND);	// �A���t�@�u�����h�L��
		SetZWrite(false);				// Z�o�b�t�@�X�V���Ȃ�
		g_model.Draw(pDC, g_EMelee[i].m_mtxWorld, eTransparentOnly);
		SetZWrite(true);				// Z�o�b�t�@�X�V����
		SetBlendState(BS_NONE);			// �A���t�@�u�����h����
	}
}

//*******************************
//
//	�G�̔z�u����
//	
//	����:�z�u���������W x y z
//
//	�߂�l
//		:�g�p���Ă���G�̍ő吔
//
//*******************************
int SetEnemyMelee(XMFLOAT3 pos)
{
	// �߂�l�̏�����
	int EnemyMelee = -1;

	for (int cntEnemyMelee = 0; cntEnemyMelee < MAX_ENEMYMELEE; ++cntEnemyMelee)
	{
		// �g�p���Ȃ�X�L�b�v
		if (g_EMelee[cntEnemyMelee].m_use)
		{
			continue;
		}
		g_EMelee[cntEnemyMelee].m_use = true;	// �g�p��ON
		g_EMelee[cntEnemyMelee].m_pos = pos;	// �w�肵�����W����

		EnemyMelee = cntEnemyMelee;	// �g�p���̓G������
		break;
	}

	return EnemyMelee;
}