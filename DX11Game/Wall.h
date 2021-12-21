//=============================================================================
//
// �Ǐ��� [Wall.h]
//
//=============================================================================
#pragma once

#include "main.h"

#define MAX_WALL (256)

//*****************************************************************************
// �\���̒�`
//*****************************************************************************
struct TWall {
	XMFLOAT3	m_pos;		// ���݂̈ʒu
	XMFLOAT3	m_rot;		// ���݂̌���
	XMFLOAT3    m_size;		// ���݂̃T�C�Y
	XMFLOAT4X4	m_mtxWorld;	// ���[���h�}�g���b�N�X

	int			m_nLife;	// �ǂ̑ϋv�u
	bool		use;		// �g�p���Ă��邩
};

//*****************************************************************************
// �v���g�^�C�v�錾
//*****************************************************************************
HRESULT InitWall(void);
void UninitWall(void);
void UpdateWall(void);
void DrawWall(void);

int SetWall(XMFLOAT3 pos);

bool CollisionWall(XMFLOAT3 Apos, XMFLOAT3 Asize, XMFLOAT3 Bpos, XMFLOAT3 Bsize);


TWall* GetWall();


