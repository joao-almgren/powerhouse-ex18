
/**************************************************************************
 *	D3D Ex. 18
 *
 *  Heightmap rendering
 *
 **************************************************************************/



/**************************************************************************
 *	WINDOWS INCLUDES
 **************************************************************************/

#include <windows.h>
#pragma comment(lib, "winmm.lib")
#include <stdio.h>



/**************************************************************************
 *	D3D INCLUDES
 **************************************************************************/

#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")



/**************************************************************************
 *	CAMERA & USERINPUT
 **************************************************************************/

#include "UserInput.h"
#include "Camera.h"



/**************************************************************************
 *	USEFUL DEFINES
 **************************************************************************/

// set to TRUE for windowed mode, FALSE for fullscreen mode
#define WINDOWED_MODE false

const int MAPSIZE = 256;			// heightmap size
const float MAPSCALE = 8;			// landscape scale
const float MAPHEIGHT = 150;		// height scale


/**************************************************************************
 *	CUSTOM FLEXIBLE VERTEX FORMAT
 **************************************************************************/


#define D3DFVF_SCAPEVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX3)

struct SCAPEVERTEX
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	D3DCOLOR color;
	float u, v;
	float u2, v2;
	float u3, v3;
};



/**************************************************************************
 *	GLOBAL D3D INTERFACE VARIABLES
 **************************************************************************/

IDirect3D9 * g_pD3D = NULL;							// object handle
IDirect3DDevice9 * g_pd3dDevice = NULL;				// device handle

ID3DXFont * g_pFont = NULL;							// fonts

IDirect3DVertexBuffer9 * g_pVB = NULL;				// vertex buffer
IDirect3DIndexBuffer9 * g_pIB = NULL;				// index buffer

IDirect3DTexture9 * g_pTexture[4] = {NULL, NULL, NULL, NULL};	// texture handle

UserInput g_ui;													// direct input interface
Camera g_cam(D3DXVECTOR3(0, 0, 0), 0, D3DX_PI * 1.15f, 0);		// the camera

__int32 g_nVertices = 0;		// number of vertices
__int32 g_nIndices = 0;			// number if indices

float * g_height = NULL;	// heightmap

float g_offsY = 0;			// camera height over ground



/**************************************************************************
 *	USEFUL HELPER FUNCTION
 **************************************************************************/

inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }



/**************************************************************************
 *	RELEASE D3D INTERFACES
 **************************************************************************/

void destroy()
{
	if ( g_height )
		delete [] g_height;

	for (int x = 0; x < 4; x++)
		if (g_pTexture[x])
			g_pTexture[x]->Release();

	if (g_pIB)
		g_pIB->Release();

	if (g_pVB)
		g_pVB->Release();

	if ( g_pFont )
		g_pFont->Release();

	if (g_pd3dDevice)
		g_pd3dDevice->Release();

	if (g_pD3D)
		g_pD3D->Release();
}



/**************************************************************************
 *	LOAD HEIGHTMAP
 **************************************************************************/

bool loadMap(const char * fname)
{
	// allocate and read heightmap from file
	g_nVertices = MAPSIZE * MAPSIZE;
	g_height = new float[g_nVertices];

	FILE * f = fopen(fname, "rb");
	if (!f)
		return false;

	for (int i = 0; i < g_nVertices; i++)
	{
		float height;
		fread(&height, sizeof (float), 1, f);

		float noise = (rand() % 1000) / 800.0f; // offset height by some noise
		g_height[i] = 150 * height + noise;
	}

	if (ferror(f))
		return false;

	fclose(f);


	// allocate vertex array
	SCAPEVERTEX * vertices = new SCAPEVERTEX[g_nVertices];

	// fill the vertex array
	for (__int32 y = 0; y < MAPSIZE; y++)
		for (__int32 x = 0; x < MAPSIZE; x++)
		{
			// set vertex position
			vertices[x + y * MAPSIZE].position.x = 8 * (float)(x - (MAPSIZE / 2));
			vertices[x + y * MAPSIZE].position.z = 8 * (float)(y - (MAPSIZE / 2));
			vertices[x + y * MAPSIZE].position.y = g_height[x + y * MAPSIZE];

			// initialize vertex normal
			vertices[x + y * MAPSIZE].normal = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

			// set vertex colour to a gradient
			BYTE c = (BYTE)(g_height[x + y * MAPSIZE] * 64 / 150) + 195;
			vertices[x + y * MAPSIZE].color = D3DCOLOR_XRGB(c, c, c);

			// set the u,v texture coordinate pairs
			vertices[x + y * MAPSIZE].u = (float)x / (MAPSIZE);
			vertices[x + y * MAPSIZE].v = (float)y / (MAPSIZE);

			vertices[x + y * MAPSIZE].u2 = (float)x / (MAPSIZE / 10);
			vertices[x + y * MAPSIZE].v2 = (float)y / (MAPSIZE / 10);

			vertices[x + y * MAPSIZE].u3 = (float)x / (MAPSIZE / 128);
			vertices[x + y * MAPSIZE].v3 = (float)y / (MAPSIZE / 128);
		}


	// hack; calculate face normals and drop straight into vertex normals
	for (__int32 z = 0; z < (MAPSIZE - 1); z++ )
		for (__int32 x = 0; x < (MAPSIZE - 1); x++)
		{
			D3DXVECTOR3 p(vertices[(x + 0) + (z + 0) * MAPSIZE].position),
						q(vertices[(x + 1) + (z + 0) * MAPSIZE].position),
						r(vertices[(x + 0) + (z + 1) * MAPSIZE].position);

			D3DXVECTOR3 u(p - q), v(p - r), n;

			D3DXVec3Cross(&n, &v, &u);

			D3DXVec3Normalize(&vertices[x + z * MAPSIZE].normal, &n);
		}

	// create and fill vertex buffer
	HRESULT hr = g_pd3dDevice->CreateVertexBuffer(sizeof(SCAPEVERTEX) * g_nVertices, 0, D3DFVF_SCAPEVERTEX, D3DPOOL_DEFAULT, &g_pVB, NULL);

	if (FAILED(hr))
	{
		delete [] vertices;
		return false;
	}

	VOID * pVertices;
	hr = g_pVB->Lock(0, sizeof(SCAPEVERTEX) * g_nVertices, (void **) &pVertices, 0);

	if (FAILED(hr))
	{
		delete [] vertices;
		return false;
	}

	memcpy(pVertices, vertices, sizeof(SCAPEVERTEX) * g_nVertices);
	g_pVB->Unlock();

	delete [] vertices;


	// allocate and fill index array
	const int nCellCols = MAPSIZE - 1;
	const int nCellRows = MAPSIZE - 1;
	const int nVerticesPerCell = 2 * 3;
	g_nIndices = nCellCols * nCellRows * nVerticesPerCell;
	DWORD * indices = new DWORD[g_nIndices];

	int baseIndex = 0;
	for (y = 0; y < nCellRows; y++)
		for (__int32 x = 0; x < nCellCols; x++)
		{
			// triangle 1
			indices[baseIndex + 0] =     (x) +     (y) * MAPSIZE;
			indices[baseIndex + 1] = (x + 1) +     (y) * MAPSIZE;
			indices[baseIndex + 2] =     (x) + (y + 1) * MAPSIZE;

			// triangle 2
			indices[baseIndex + 3] =     (x) + (y + 1) * MAPSIZE;
			indices[baseIndex + 4] = (x + 1) +     (y) * MAPSIZE;
			indices[baseIndex + 5] = (x + 1) + (y + 1) * MAPSIZE;

			baseIndex += 6; // next cell
		}

	// create and fill index buffer
	hr = g_pd3dDevice->CreateIndexBuffer(sizeof(DWORD) * g_nIndices, D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &g_pIB, NULL);

	if (FAILED(hr))
	{
		delete [] indices;
		return false;
	}

	VOID * pIndices;
	hr = g_pIB->Lock(0, sizeof(DWORD) * g_nIndices, (void **) &pIndices, 0);

	if (FAILED(hr))
	{	
		delete [] indices;
		return false;
	}

	memcpy(pIndices, indices, sizeof(DWORD) * g_nIndices);
	g_pIB->Unlock();

	delete [] indices;

	return true;
}



/**************************************************************************
 *	FIND HEIGHT IN HEIGHTMAP
 **************************************************************************/

float getMapheight(float x, float z)
{
	// transform to heightmap space
	x = (MAPSIZE / 2) + (x / MAPSCALE);
	z = (MAPSIZE / 2) + (z / MAPSCALE);

	int col = (int)x;
	int row = (int)z;

	if (col < 0 || col >= MAPSIZE || row < 0 || row >= MAPSIZE)
		return 0;

	float dx = x - col;
	float dz = z - row;

	float p = g_height[      col +       row * MAPSIZE];
	float q = g_height[(col + 1) +       row * MAPSIZE];
	float r = g_height[      col + (row + 1) * MAPSIZE];
	float t = g_height[(col + 1) + (row + 1) * MAPSIZE];

	float h;
	if (dz < 1 - dx) // upper triangle
	{
		float uy = q - p;
		float vy = r - p;

		h = p + (dx * uy) + (dz * vy);
	}
	else
	{
		float uy = r - t;
		float vy = q - t;

		h = t + ((1 - dx) * uy) + ((1 - dz) * vy);
	}

	return h;
}



/**************************************************************************
 *	CREATE AND INITIALIZE D3D INTERFACES
 **************************************************************************/

bool create( HWND hwnd, bool mode = WINDOWED_MODE )
{
	HRESULT hr;

	// create the D3D object

	g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );

	if (NULL == g_pD3D)
	{
		MessageBox( hwnd, "Version mismatch!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}


	// set up D3D screen parameters

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof d3dpp );

	switch ( mode )
	{
		case true:
			d3dpp.Windowed = TRUE;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;	// overwrite the buffer
			d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;	// current display format
			d3dpp.EnableAutoDepthStencil = TRUE;
			d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
			break;
		case false:
			d3dpp.Windowed = FALSE;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3dpp.hDeviceWindow = hwnd;
			d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;	// 24 bit colour, 8 bit alpha
			d3dpp.BackBufferWidth = 1024;
			d3dpp.BackBufferHeight = 768;
			d3dpp.EnableAutoDepthStencil = TRUE;
			d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
			break;
	}


	// create D3D device

	hr = g_pD3D->CreateDevice(
		D3DADAPTER_DEFAULT,								// default adapter
		D3DDEVTYPE_HAL,									// hardware acceleration
		hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,			// hardware acceleration
		&d3dpp,
		&g_pd3dDevice );

	if ( FAILED( hr ) )
	{
		MessageBox( hwnd, "Couldn't create device!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}


	// create a font

/*	hr = D3DXCreateFont(
		g_pd3dDevice,
		12,								// height
		0,								// default width
		FW_BOLD,						// weight (FW_NORMAL for normal text)
		1,								// no more mip levels
		FALSE,							// italic
		DEFAULT_CHARSET,				// char set
		OUT_DEFAULT_PRECIS,				// precision
		DEFAULT_QUALITY,				// quality
		DEFAULT_PITCH | FF_DONTCARE,	// pitch and family
		"System",						// face name
		&g_pFont);

	if (FAILED(hr))
	{
		destroy();
		MessageBox( hwnd, "Couldn't create font!", "Fatal Error", MB_ICONERROR | MB_OK );
		return false;
	}
*/
	
	// load heightmap

	if (!loadMap("output.r32"))
	{
		MessageBox( hwnd, "Couldn't load heightmap!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}


	// load textures

	if ( FAILED ( D3DXCreateTextureFromFile(g_pd3dDevice, "grass.dds", &g_pTexture[0]) ) )
	{
		MessageBox( hwnd, "Couldn't load texture 0!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}

	if ( FAILED ( D3DXCreateTextureFromFile(g_pd3dDevice, "rock.dds", &g_pTexture[1]) ) )
	{
		MessageBox( hwnd, "Couldn't load texture 1!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}

	if ( FAILED ( D3DXCreateTextureFromFile(g_pd3dDevice, "alpha.dds", &g_pTexture[2]) ) )
	{
		MessageBox( hwnd, "Couldn't load texture 2!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}

	if ( FAILED ( D3DXCreateTextureFromFile(g_pd3dDevice, "detail.dds", &g_pTexture[3]) ) )
	{
		MessageBox( hwnd, "Couldn't load texture 3!", "Fatal Error", MB_ICONERROR | MB_OK );
		destroy();
		return false;
	}



	// set render states

	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	// enable fog

	g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_FOGCOLOR, D3DCOLOR_XRGB(192, 255, 255));
	g_pd3dDevice->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_EXP2);
	g_pd3dDevice->SetRenderState(D3DRS_FOGDENSITY, FtoDW(0.0010f));


	// set a light

	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof D3DLIGHT9);
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0);

	D3DXVECTOR3 vecDir = D3DXVECTOR3( 0.0f, -1.0f, -1.0f);
	D3DXVec3Normalize((D3DXVECTOR3 *)&light.Direction, &vecDir);

	g_pd3dDevice->SetLight(0, &light);
	g_pd3dDevice->LightEnable(0, TRUE);


	// set projection and world matrix

	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 320.0f / 240.0f, 1.0f, 4096.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

	D3DXMATRIX matWorld;
	D3DXMatrixIdentity( &matWorld );
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

	return true;
}



/**************************************************************************
 *	RENDER TO BACKBUFFER
 **************************************************************************/

void draw()
{
/*	static double fps = 0;

	// fps counter
	{
		static unsigned int frames = 0;
		frames++;

		static DWORD start = 0, stop;

		if (frames % 25 == 0) // get start time
			start = timeGetTime();
		else if (frames % 25 == 24) // get stop time and calculate fps
		{
			stop = timeGetTime();
			fps = 24.0 * 1000.0 / (double)(stop - start);
		}
	}
*/

	// update camera
	{
		// get input
		g_ui.updateMouse();
		g_ui.updateKeyboard();

		// update view orientation
		POINT currMouse = {g_ui.mouseState.lX, g_ui.mouseState.lY};
		g_cam.rotate((float)-currMouse.y / 256.0f, (float)-currMouse.x / 256.0f, 0);


		// jump and fall
		static float speedY = 0;	// fall speed

		// create a speed upwards
		if (g_ui.keyState[DIK_SPACE] && 0 == speedY)
			speedY = 0.5f;

		// add speed to height offset
		g_offsY += speedY;

		// apply gravity
		if (g_offsY > 3.5f)		// if above the ground
			speedY -= 0.02f;	// ...fall...
		else					// else stop falling
		{
			speedY = 0;
			g_offsY = 3.5f;
		}


		// run around
		static float speedX = 0;	// run speed
		static float speedZ = 0;	// strafe speed

		// accelerate
		if (g_ui.keyState[DIK_D] || g_ui.keyState[DIK_RIGHT])
			speedX += 0.05f;
		if (g_ui.keyState[DIK_A] || g_ui.keyState[DIK_LEFT])
			speedX -= 0.05f;
		if (g_ui.keyState[DIK_W] || g_ui.keyState[DIK_UP])
			speedZ += 0.05f;
		if (g_ui.keyState[DIK_S] || g_ui.keyState[DIK_DOWN])
			speedZ -= 0.05f;

		// clamp speed
		if (speedX > 0.35)
			speedX = 0.35f;
		else if (speedX < -0.35)
			speedX = -0.35f;

		if (speedZ > 0.35)
			speedZ = 0.35f;
		else if (speedZ < -0.35)
			speedZ = -0.35f;

		// add friction
		if (0 == speedY) // no air friction
		{
			if (speedX > 0.015f)
				speedX -= 0.015f;
			else if (speedX < -0.015f)
				speedX += 0.015f;
			else
				speedX = 0.0f;

			if (speedZ > 0.015f)
				speedZ -= 0.015f;
			else if (speedZ < -0.015f)
				speedZ += 0.015f;
			else
				speedZ = 0.0f;
		}

		// move view point
		g_cam.moveRight(speedX);
		g_cam.moveForward(speedZ);


		// keep position inside map
		if (g_cam.pos.x < -(MAPSIZE / 2) * MAPSCALE)
			g_cam.pos.x = -(MAPSIZE / 2) * MAPSCALE;
		else if (g_cam.pos.x > ((MAPSIZE - 1) / 2) * MAPSCALE)
			g_cam.pos.x = ((MAPSIZE - 1) / 2) * MAPSCALE;

		if (g_cam.pos.z < -(MAPSIZE / 2) * MAPSCALE)
			g_cam.pos.z = -(MAPSIZE / 2) * MAPSCALE;
		else if (g_cam.pos.z > ((MAPSIZE - 1) / 2) * MAPSCALE)
			g_cam.pos.z = ((MAPSIZE - 1) / 2) * MAPSCALE;


		// set view point height
		g_cam.pos.y = getMapheight(g_cam.pos.x, g_cam.pos.z) + g_offsY;
	}


	// set up view matrix
	g_cam.setView(g_pd3dDevice);


	// clear
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(128, 196, 255), 1.0f, 0);

	// begin scene
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// LANDSCAPE
		{
			// base texture grass
			g_pd3dDevice->SetTexture(0, g_pTexture[0]);
			g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 2);
			g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
			g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 1);
			// modulate by vertex colour
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE4X);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

			// alpha texture
			g_pd3dDevice->SetTexture(1, g_pTexture[2]);
			g_pd3dDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
			g_pd3dDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			g_pd3dDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);

			// base texture rock
			g_pd3dDevice->SetTexture(2, g_pTexture[1]);
			g_pd3dDevice->SetSamplerState(2, D3DSAMP_MAXANISOTROPY, 2);
			g_pd3dDevice->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
			g_pd3dDevice->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
			g_pd3dDevice->SetTextureStageState(2, D3DTSS_TEXCOORDINDEX, 1);
			g_pd3dDevice->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_BLENDCURRENTALPHA);

			// detail texture
			g_pd3dDevice->SetTexture(3, g_pTexture[3]);
			g_pd3dDevice->SetSamplerState(3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetSamplerState(3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			g_pd3dDevice->SetSamplerState(3, D3DSAMP_MIPMAPLODBIAS, FtoDW(-2.0f));
			g_pd3dDevice->SetSamplerState(3, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
			g_pd3dDevice->SetTextureStageState(3, D3DTSS_TEXCOORDINDEX, 2);
			g_pd3dDevice->SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_ADDSIGNED);

			// draw it!
			g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof SCAPEVERTEX);
			g_pd3dDevice->SetFVF(D3DFVF_SCAPEVERTEX);
			g_pd3dDevice->SetIndices(g_pIB);
			g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, g_nVertices, 0, g_nIndices / 3);
		}

		// print fps
/*		{
			char szText[20];
			sprintf(szText, "FPS: %.2f", fps);
			RECT rScreen = {10, 10, 1024, 768};
			g_pFont->DrawText(NULL, szText, -1, &rScreen, DT_LEFT | DT_TOP, D3DCOLOR_XRGB(0, 0, 0));
		}
*/

		g_pd3dDevice->EndScene();
	}

	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}


/**************************************************************************
 *	WINDOW MESSAGE HANDLING PROCEDURE
 **************************************************************************/

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
		case WM_KEYDOWN:
			switch ( wParam )
			{
				case VK_ESCAPE:
					PostMessage( hwnd, WM_CLOSE, 0, 0 ); // quit on ESC
					break;
			}
			break;

		case WM_DESTROY:
			destroy(); // release D3D
			PostQuitMessage( 0 );
			break;

		default:
			return DefWindowProc( hwnd, uMsg, wParam, lParam );
	}
	return 0;
}



/**************************************************************************
 *	WinMain
 **************************************************************************/

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	// create window class

	char szWinName[] = "D3D9Ex18";

	WNDCLASSEX wcl = {0};
	wcl.cbSize = sizeof WNDCLASSEX;

	wcl.lpszClassName = szWinName;
	wcl.lpfnWndProc = WindowProc;
	wcl.hInstance = hInstance;

	wcl.hIcon = LoadIcon( 0, IDI_WINLOGO );
	wcl.hIconSm = LoadIcon( 0, IDI_WINLOGO );
	wcl.hCursor = LoadCursor( 0, IDC_ARROW );
	wcl.hbrBackground = (HBRUSH)( COLOR_APPWORKSPACE + 1 );

	if ( !RegisterClassEx(&wcl) )
	{
		MessageBox( HWND_DESKTOP, "Couldn't register window class!", "Fatal Error", MB_ICONERROR | MB_OK );
		return 0;
	}


	// create window

	HWND hwnd = CreateWindowEx(
		WS_EX_TOPMOST, szWinName, szWinName,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
		HWND_DESKTOP, 0, hInstance, 0 );

	if ( !hwnd )
	{
		MessageBox( hwnd, "Couldn't create window!", "Fatal Error", MB_ICONERROR | MB_OK );
		return 0;
	}

	ShowWindow( hwnd, nCmdShow );
	UpdateWindow( hwnd );
	SetFocus( hwnd );


	// initialize DirectInput

	if (!g_ui.initMouse(hwnd) || !g_ui.initKeyboard(hwnd))
	{
		MessageBox( hwnd, "Couldn't initialize DirectInput!", "Fatal Error", MB_ICONERROR | MB_OK );
		return 0;
	}


	ShowCursor(FALSE);



	// initialize D3D

	if ( !create( hwnd, WINDOWED_MODE ) )
	{
		MessageBox( hwnd, "Couldn't initialize D3D!", "Fatal Error", MB_ICONERROR | MB_OK );
		return 0;
	}


	// go into message loop

	MSG msg = { 0 };
	while ( WM_QUIT != msg.message )
	{
		if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
			draw();
	}

	return (int)msg.wParam;
}
