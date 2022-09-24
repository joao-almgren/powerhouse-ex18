
/**************************************************************************
 *	D3D Ex. 13b
 *
 *  Camera class
 *
 **************************************************************************/

#pragma once

#include <math.h>
#include <d3dx9.h>


class Camera
{
	public:
		Camera(D3DXVECTOR3 pos, float pitch, float yaw, float roll);

		void rotate(float dPitch, float dYaw, float dRoll);
		void moveUp(float scale);
		void moveDown(float scale);
		void moveLeft(float scale);
		void moveRight(float scale);
		void moveForward(float scale);
		void moveBackward(float scale);

		void setView(IDirect3DDevice9 * pDevice);
		void setViewOrientation(IDirect3DDevice9 * pDevice, D3DXVECTOR3 pos);
		void setViewBehind(IDirect3DDevice9 * pDevice, float scale);

// public for easy access
		float pitch, yaw, roll;
		D3DXVECTOR3 pos;
		D3DXVECTOR3 dir, right, up;
};

Camera::Camera(D3DXVECTOR3 pos, float pitch, float yaw, float roll)
{
	this->pitch = pitch;
	this->yaw = yaw;
	this->roll = roll;
	this->pos = pos;

	rotate(0, 0, 0);
}

void Camera::rotate(float dPitch, float dYaw, float dRoll)
{
	this->pitch += dPitch;
	this->yaw += dYaw;
	this->roll += dRoll;

	const float HALF_PI = 0.99f * (3.1415f / 2.0f);
    if (pitch > HALF_PI)
		pitch = HALF_PI;
    if (pitch < -HALF_PI)
		pitch = -HALF_PI;


	D3DXMATRIX matY, matX, matZ, matRot;
	D3DXMatrixRotationY( &matY, yaw );
	D3DXMatrixRotationX( &matX, pitch );
	D3DXMatrixRotationZ( &matZ, roll );
	matRot = matY * matX * matZ;
	dir = D3DXVECTOR3( matRot._13, matRot._23, matRot._33 );


	D3DXMATRIX view;
	D3DXMatrixLookAtLH( &view, &pos, &(pos + dir), &D3DXVECTOR3( 0, 1, 0 ) );


	right = D3DXVECTOR3( view._11, view._21, view._31 );
	up = D3DXVECTOR3( view._12, view._22, view._32 );
	dir = D3DXVECTOR3( view._13, view._23, view._33 );
}

void Camera::moveLeft(float scale = 1.0f)
{
	pos -= scale * right;
}

void Camera::moveRight(float scale = 1.0f)
{
	pos += scale * right;
}

void Camera::moveForward(float scale = 1.0f)
{
	pos += scale * dir;
}

void Camera::moveBackward(float scale = 1.0f)
{
	pos -= scale * dir;
}

void Camera::moveUp(float scale = 1.0f)
{
	pos += scale * up;
}

void Camera::moveDown(float scale = 1.0f)
{
	pos -= scale * up;
}

void Camera::setView(IDirect3DDevice9 * pDevice)
{
	D3DXMATRIX view;
	D3DXMatrixLookAtLH( &view, &pos, &(pos + dir), &up );

	pDevice->SetTransform( D3DTS_VIEW, &view );
}

void Camera::setViewOrientation(IDirect3DDevice9 * pDevice, D3DXVECTOR3 pos = D3DXVECTOR3(0, 0, 0))
{
	D3DXMATRIX view;
	D3DXMatrixLookAtLH( &view, &pos, &(pos + dir), &up );

	pDevice->SetTransform( D3DTS_VIEW, &view );
}

void Camera::setViewBehind(IDirect3DDevice9 * pDevice, float scale = 10.0f)
{
	D3DXMATRIX view;
	D3DXMatrixLookAtLH( &view, &pos, &(pos + dir), &up );

	right = D3DXVECTOR3( view._11, view._21, view._31 );
	up = D3DXVECTOR3( view._12, view._22, view._32 );
	dir = D3DXVECTOR3( view._13, view._23, view._33 );

	D3DXVECTOR3 behind = pos - (scale * dir);
	D3DXMatrixLookAtLH( &view, &behind, &pos, &up );

	pDevice->SetTransform( D3DTS_VIEW, &view );
}

