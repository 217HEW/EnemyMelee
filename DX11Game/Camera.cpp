//=============================================================================
//
// �J���� �N���X [Camera.cpp]
// Author : HIROHIKO HAMAYA
//
//=============================================================================
#include "Camera.h"
#include "input.h"
#include "debugproc.h"
#include "player.h"

//*****************************************************************************
// �O���[�o���ϐ�
//*****************************************************************************
namespace {
	const float CAM_POS_P_X = 0.0f;					// �J�����̎��_�����ʒu(X���W)
	const float CAM_POS_P_Y = 100.0f;				// �J�����̎��_�����ʒu(Y���W)
	const float CAM_POS_P_Z = -200.0f;				// �J�����̎��_�����ʒu(Z���W)
	const float CAM_POS_R_X = 0.0f;					// �J�����̒����_�����ʒu(X���W)
	const float CAM_POS_R_Y = 50.0f;				// �J�����̒����_�����ʒu(Y���W)
	const float CAM_POS_R_Z = 0.0f;					// �J�����̒����_�����ʒu(Z���W)
	const float VIEW_ANGLE = 45.0f;					// �r���[���ʂ̎���p
	const float VIEW_ASPECT = (float)SCREEN_WIDTH / SCREEN_HEIGHT;	// �r���[���ʂ̃A�X�y�N�g��
	const float VIEW_NEAR_Z = 10.0f;				// �r���[���ʂ�NearZ�l
	const float VIEW_FAR_Z = 5000.0f;				// �r���[���ʂ�FarZ�l
	const float VALUE_MOVE_CAMERA = 2.0f;			// �J�����̈ړ���
	const float VALUE_ROTATE_CAMERA = 1.8f;			// �J�����̉�]��
	const float RATE_ROTATE_CAMERA = 0.20f;			// �J�����̒����_�ւ̕␳�W��

	const float INTERVAL_CAMERA_R = 12.5f;			// ���f���̎����̐�܂ł̋���
	const float RATE_CHASE_CAMERA_P = 0.35f;		// �J�����̎��_�ւ̕␳�W��
	const float RATE_CHASE_CAMERA_R = 0.20f;		// �J�����̒����_�ւ̕␳�W��

	const float CHASE_HEIGHT_P = 100.0f;			// �ǐՎ��̎��_�̍���
	const float CHASE_HEIGHT_R = 10.0f;				// �ǐՎ��̒����_�̍���

	CCamera g_camera;								// �J���� �C���X�^���X
}

CCamera* CCamera::m_pCamera = &g_camera;			// ���݂̃J����

// �R���X�g���N�^
CCamera::CCamera()
{
	Init();
}

// ������
void CCamera::Init()
{
	m_vPos = XMFLOAT3(CAM_POS_P_X, CAM_POS_P_Y, CAM_POS_P_Z);	// ���_
	m_vTarget = XMFLOAT3(CAM_POS_R_X, CAM_POS_R_Y, CAM_POS_R_Z);// �����_
	m_vUp = XMFLOAT3(0.0f, 1.0f, 0.0f);							// ����x�N�g��
	m_vSrcPos = m_vPos;
	m_vDestPos = m_vPos;
	m_vDestTarget = m_vTarget;
	m_vVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_vAngle = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_vDestAngle = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_vDestTargetAngle = XMFLOAT3(0.0f, 0.0f, 0.0f);

	m_fAspectRatio = VIEW_ASPECT;		// �c����
	m_fFovY = VIEW_ANGLE;				// ����p(�P��:Degree)
	m_fNearZ = VIEW_NEAR_Z;				// �O���N���b�v����
	m_fFarZ = VIEW_FAR_Z;				// ����N���b�v����

	float dx, dz;
	dx = m_vPos.x - m_vTarget.x;
	dz = m_vPos.z - m_vTarget.z;
	m_fLengthInterval = sqrtf(dx * dx + dz * dz);

	CalcWorldMatrix();
}

// �X�V
void CCamera::Update()
{
	if (GetKeyPress(VK_C)) {
		// �E����
		m_vDestAngle.y -= VALUE_ROTATE_CAMERA;
		if (m_vDestAngle.y < -180.0f) {
			m_vDestAngle.y += 360.0f;
		}
	}
	if (GetKeyPress(VK_Z)) {
		// ������
		m_vDestAngle.y += VALUE_ROTATE_CAMERA;
		if (m_vDestAngle.y >= 180.0f) {
			m_vDestAngle.y -= 360.0f;
		}
	}

	// �ړI�̊p�x�܂ł̍���
	float fDiffRotY = m_vDestAngle.y - m_vAngle.y;
	if (fDiffRotY >= 180.0f) {
		fDiffRotY -= 360.0f;
	}
	if (fDiffRotY < -180.0f) {
		fDiffRotY += 360.0f;
	}

	// �ړI�̊p�x�܂Ŋ�����������
	m_vAngle.y += fDiffRotY * RATE_ROTATE_CAMERA;
	if (m_vAngle.y >= 180.0f) {
		m_vAngle.y -= 360.0f;
	}
	if (m_vAngle.y < -180.0f) {
		m_vAngle.y += 360.0f;
	}
	m_vSrcPos.x = -SinDeg(m_vAngle.y) * m_fLengthInterval;
	m_vSrcPos.z = -CosDeg(m_vAngle.y) * m_fLengthInterval;

	// �ǐՃJ����
	XMFLOAT3& vModelPos = GetPlayerPos();	// ���@���W
	// ���_���W�ړ�����Z�o
	m_vDestPos.x = m_vSrcPos.x + vModelPos.x;
	m_vDestPos.y = m_vSrcPos.y + vModelPos.y;
	m_vDestPos.z = m_vSrcPos.z + vModelPos.z;
	// �����_���W�ړ�����Z�o
	m_vDestTarget.x = CAM_POS_R_X + vModelPos.x;
	m_vDestTarget.y = CAM_POS_R_Y + vModelPos.y;
	m_vDestTarget.z = CAM_POS_R_Z + vModelPos.z;
	// ���_�����X�Ɉړ���ɋ߂Â���
	m_vPos.x = m_vPos.x * 0.9f + m_vDestPos.x * 0.1f;
	m_vPos.y = m_vPos.y * 0.9f + m_vDestPos.y * 0.1f;
	m_vPos.z = m_vPos.z * 0.9f + m_vDestPos.z * 0.1f;
	// �����_�����X�Ɉړ���ɋ߂Â���
	m_vTarget.x = m_vTarget.x * 0.9f + m_vDestTarget.x * 0.1f;
	m_vTarget.y = m_vTarget.y * 0.9f + m_vDestTarget.y * 0.1f;
	m_vTarget.z = m_vTarget.z * 0.9f + m_vDestTarget.z * 0.1f;

	if (GetKeyTrigger(VK_HOME)) {
		// ���Z�b�g
		m_vPos = XMFLOAT3(CAM_POS_P_X, CAM_POS_P_Y, CAM_POS_P_Z);	// ���_
		m_vTarget = XMFLOAT3(CAM_POS_R_X, CAM_POS_R_Y, CAM_POS_R_Z);// �����_
		m_vSrcPos = m_vPos;
		m_vDestPos = m_vPos;
		m_vDestTarget = m_vTarget;
		m_vAngle = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_vDestAngle = m_vAngle;
	}

	// �}�g���b�N�X�X�V
	UpdateMatrix();

	//PrintDebugProc("[��� �� : (%f, %f, %f)]\n", m_vPos.x, m_vPos.y, m_vPos.z);
	//PrintDebugProc("[������ : (%f, %f, %f)]\n", m_vTarget.x, m_vTarget.y, m_vTarget.z);
	//PrintDebugProc("[��� ѷ : (%f)]\n", m_vAngle.y);
	//PrintDebugProc("\n");

	//PrintDebugProc("*** ��� ��� ***\n");
	//PrintDebugProc("���� �ݶ� : Z\n");
	//PrintDebugProc("з�  �ݶ� : C\n");
	//PrintDebugProc("\n");
}

// �r���[/�v���W�F�N�V���� �}�g���b�N�X�X�V
void CCamera::UpdateMatrix()
{
	XMStoreFloat4x4(&m_mtxView, XMMatrixLookAtLH(
		XMLoadFloat3(&m_vPos), XMLoadFloat3(&m_vTarget), XMLoadFloat3(&m_vUp)));
	XMStoreFloat4x4(&m_mtxProj, XMMatrixPerspectiveFovLH(
		XMConvertToRadians(m_fFovY), m_fAspectRatio, m_fNearZ, m_fFarZ));
}

// ���[���h �}�g���b�N�X�ݒ�
void CCamera::SetWorldMatrix(XMFLOAT4X4& mtxWorld)
{
	m_mtxWorld = mtxWorld;
	m_vPos = XMFLOAT3(mtxWorld._41, mtxWorld._42, mtxWorld._43);
	m_vTarget = XMFLOAT3(mtxWorld._41 + mtxWorld._31, mtxWorld._42 + mtxWorld._32, mtxWorld._43 + mtxWorld._33);
	m_vUp = XMFLOAT3(mtxWorld._21, mtxWorld._22, mtxWorld._23);
}

// ���_/�����_/����x�N�g�����烏�[���h �}�g���b�N�X�Z�o
XMFLOAT4X4& CCamera::CalcWorldMatrix()
{
	XMVECTOR vecZ = XMVectorSet(m_vTarget.x - m_vPos.x, m_vTarget.y - m_vPos.y, m_vTarget.z - m_vPos.z, 0.0f);
	XMFLOAT3 vZ;
	XMStoreFloat3(&vZ, XMVector3Normalize(vecZ));
	XMVECTOR vecY = XMLoadFloat3(&m_vUp);
	XMVECTOR vecX = XMVector3Normalize(XMVector3Cross(vecY, vecZ));
	XMFLOAT3 vX;
	XMStoreFloat3(&vX, vecX);
	vecY = XMVector3Normalize(XMVector3Cross(vecZ, vecX));
	XMFLOAT3 vY;
	XMStoreFloat3(&vY, vecY);

	m_mtxWorld._11 = vX.x;
	m_mtxWorld._12 = vX.y;
	m_mtxWorld._13 = vX.z;
	m_mtxWorld._14 = 0.0f;
	m_mtxWorld._21 = vY.x;
	m_mtxWorld._22 = vY.y;
	m_mtxWorld._23 = vY.z;
	m_mtxWorld._24 = 0.0f;
	m_mtxWorld._31 = vZ.x;
	m_mtxWorld._32 = vZ.y;
	m_mtxWorld._33 = vZ.z;
	m_mtxWorld._34 = 0.0f;
	m_mtxWorld._41 = m_vPos.x;
	m_mtxWorld._42 = m_vPos.y;
	m_mtxWorld._43 = m_vPos.z;
	m_mtxWorld._44 = 1.0f;

	return m_mtxWorld;
}