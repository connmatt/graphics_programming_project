// Matthew Connolly
// CST 325
// Final Project
// December 9, 2017



#include "ground.h"
//	defines
#define MAX_LOADSTRING 1000
#define TIMER1 111
//	global variables
HINSTANCE hInst;											//	program number = instance
TCHAR szTitle[MAX_LOADSTRING];								//	name in window title
TCHAR szWindowClass[MAX_LOADSTRING];						//	class name of window
HWND hMain = NULL;											//	number of windows = handle window = hwnd
static char MainWin[] = "MainWin";							//	class name
HBRUSH  hWinCol = CreateSolidBrush(RGB(180, 180, 180));		//	a color
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
XMFLOAT3 pos[500];
int billboardcount = 500;

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;

// device context
ID3D11Device*           g_pd3dDevice = NULL;			// is for initialization and loading things (pictures, models, ...) <- InitDevice()
ID3D11DeviceContext*    g_pImmediateContext = NULL;		// is for render your models w/ pics on the screen					<- Render()
														// page flipping:
IDXGISwapChain*         g_pSwapChain = NULL;
//screen <- thats our render target
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
//how a vertex looks like
ID3D11InputLayout*      g_pVertexLayout = NULL;
//our model (array of vertices on the GPU MEM)
ID3D11Buffer*           g_pVertexBuffer = NULL; //our rectangle
ID3D11Buffer*           g_pVertexBufferPlane = NULL; //our plane
ID3D11Buffer*           g_pVertexBufferSky = NULL; //our plane
int						g_vertices_plane; // plane vertices
int						g_vertices_sky; //vertices for sky
ID3D11Buffer*           g_pVertexBufferWater = NULL; //our terrain
int						water_vertex_count = 0; //vertex count for water
ID3D11Buffer*           g_pVertexBufferTerrain = NULL; //our terrain
int						terrain_vertex_count = 0; // vertex count for grass




//exchange of data, e.g. sending mouse coordinates to the GPU
ID3D11Buffer*			g_pConstantBuffer11 = NULL;

ID3D11Buffer*			g_pConstantMatrixBuffer = NULL; // second constant buffer for the heightmaps

// function on the GPU what to do with the model exactly
ID3D11VertexShader*     g_pVertexShader = NULL;
ID3D11VertexShader*     g_pVertexShaderHeight = NULL;
ID3D11VertexShader*     g_pVertexShaderWaterHeight = NULL;
ID3D11PixelShader*      g_pPixelShader = NULL;
ID3D11PixelShader*      g_pPixelShaderSky = NULL;
ID3D11PixelShader*		g_pPixelShaderCloud = NULL;
ID3D11PixelShader*      g_pPixelShaderHeight = NULL;
ID3D11PixelShader*      g_pPixelShaderWaterHeight = NULL;

//depth stencli buffer
ID3D11Texture2D*                    g_pDepthStencil = NULL;
ID3D11DepthStencilView*             g_pDepthStencilView = NULL;

//for transparency:
ID3D11BlendState*					g_BlendState;

//Texture
ID3D11ShaderResourceView*           g_Texture = NULL; // SKYBOX
ID3D11ShaderResourceView*           g_TexPlane = NULL; //PLANE
ID3D11ShaderResourceView*           g_TextureCloud = NULL; //CLOUD
ID3D11ShaderResourceView*           g_HeightMap = NULL; //HEIGHTMAP GRASS
ID3D11ShaderResourceView*           g_HeightMap2 = NULL; //HEIGHTMAP Water
ID3D11ShaderResourceView*           g_HeightMapWave1 = NULL; // WAVE1
ID3D11ShaderResourceView*           g_HeightMapWave2 = NULL; //WAVE2
ID3D11ShaderResourceView*           g_grass = NULL; //Grass texture

//Texture Sampler
ID3D11SamplerState*                 g_Sampler = NULL;

//rasterizer states
ID3D11RasterizerState				*rs_CW, *rs_CCW, *rs_NO, *rs_Wire;

//depth state
ID3D11DepthStencilState				*ds_on, *ds_off;

//	structures we need later

XMMATRIX g_world;//model: per object position and rotation and scaling of the object
XMMATRIX g_view;//camera: position and rotation of the camera
XMMATRIX g_projection;//perspective: angle of view, near plane / far plane

//Create Cam object
camera cam;


struct VS_CONSTANT_BUFFER
	{
	float some_variable_a;
	float some_variable_b;
	float some_variable_c;
	float some_variable_d;
	float div_tex_x;	//dividing of the texture coordinates in x
	float div_tex_y;	//dividing of the texture coordinates in x
	float slice_x;		//which if the 4x4 images
	float slice_y;		//which if the 4x4 images
	
	XMMATRIX world;//model: per object position and rotation and scaling of the object
	XMMATRIX view;//camera: position and rotation of the camera
	XMMATRIX projection;//perspective: angle of view, near plane / far plane
	XMFLOAT4 campos;//position of the camera for various effects
	

	};	
		//note: we can only copy chunks of 16 byte to the GPU
VS_CONSTANT_BUFFER VsConstData;		//gloab object of this structure

struct CONSTANT_MATRIX_BUFFER
{
	XMMATRIX World;
	XMMATRIX View;
	XMMATRIX Projection;
	XMFLOAT4 offsets;

	XMFLOAT4 lightPos; //<-- always use float4 instead of float3
	XMFLOAT4 camPos; //<-- float3 will bite you in the butt!
};

CONSTANT_MATRIX_BUFFER MatrixBuffer;

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------

HRESULT InitDevice()
	{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(hMain, &rc);	//getting the windows size into a RECT structure
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
		{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
		};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
		{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hMain;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
		}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;



	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	// Compile the vertex shader
	ID3DBlob* pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VShader", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
		{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	//*****************************************************************
	//for height mapping
	//*****************************************************************

	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VShaderHeight", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}
	//*****************************************************************
	// Create the vertex shader
	//*****************************************************************

	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShaderHeight);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}
	//*****************************************************************
	//Water Height map
	//*****************************************************************

	pVSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "VShaderWaterHeight", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, &g_pVertexShaderWaterHeight);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}
	

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
		{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &g_pVertexLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
		{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;
	
	//*****************************************************************
	//SKYBOX
	//*****************************************************************
	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSsky", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
		{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
		}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderSky);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	
	//******************************************************************************************************************************************************************
	
	//*****************************************************************
	//PSTERRAIN
	//*****************************************************************

	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSterrain", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderHeight);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	//******************************************************************************************************************************************************************

	//*****************************************************************
	//PSWATER
	//*****************************************************************

	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PSWater", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderWaterHeight);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	//**************************************************************************************************************************************************************************

	//*****************************************************************
	//CLOUDS Billboards
	//*****************************************************************

	pPSBlob = NULL;
	hr = CompileShaderFromFile(L"shader.fx", "PScloud", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(NULL,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, &g_pPixelShaderCloud);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA InitData;

	// Create vertex buffer, the billboard, skybox
	SimpleVertex sbv[36];
	SimpleVertex vertices[6];
	float texWidth = (1.0 / 6.0);

	//*****************************************************************
	//SKYBOX VERTICIES
	//*****************************************************************

	//top
	sbv[0] =
	{
		XMFLOAT3(-1, 1, 1),
		XMFLOAT2(3 * texWidth, 1)
	};
	sbv[1] =
	{
		XMFLOAT3(-1, 1, -1),
		XMFLOAT2(2 * texWidth, 1)
	};
	sbv[2] =
	{
		XMFLOAT3(1, 1, 1),
		XMFLOAT2(3 * texWidth, 0)
	};

	sbv[3] =
	{
		XMFLOAT3(1, 1, 1),
		XMFLOAT2(3 * texWidth, 0)
	};
	sbv[4] =
	{
		XMFLOAT3(-1, 1, -1),
		XMFLOAT2(2 * texWidth, 1)
	};
	sbv[5] =
	{
		XMFLOAT3(1, 1, -1),
		XMFLOAT2(2 * texWidth, 0)
	};
	//bottom
	sbv[6] =
	{
		XMFLOAT3(-1, -1, -1),
		XMFLOAT2(3 * texWidth, 0)
	};
	sbv[7] =
	{
		XMFLOAT3(-1, -1, 1),
		XMFLOAT2(4 * texWidth, 0)
	};
	sbv[8] =
	{
		XMFLOAT3(1, -1, -1),
		XMFLOAT2(3 * texWidth, 1)
	};

	sbv[9] =
	{
		XMFLOAT3(1, -1, -1),
		XMFLOAT2(3 * texWidth, 1)
	};
	sbv[10] =
	{
		XMFLOAT3(-1, -1, 1),
		XMFLOAT2(4 * texWidth, 0)
	};
	sbv[11] =
	{
		XMFLOAT3(1, -1, 1),
		XMFLOAT2(4 * texWidth, 1)
	};
	//left
	sbv[12] =
	{
		XMFLOAT3(-1, -1, -1),
		XMFLOAT2(4 * texWidth, 1)
	};
	sbv[13] =
	{
		XMFLOAT3(-1, 1, -1),
		XMFLOAT2(4 * texWidth, 0)
	};
	sbv[14] =
	{
		XMFLOAT3(-1, -1, 1),
		XMFLOAT2(5 * texWidth, 1)
	};

	sbv[15] =
	{
		XMFLOAT3(-1, -1, 1),
		XMFLOAT2(5 * texWidth, 1)
	};
	sbv[16] =
	{
		XMFLOAT3(-1, 1, -1),
		XMFLOAT2(4 * texWidth, 0)
	};
	sbv[17] =
	{
		XMFLOAT3(-1, 1, 1),
		XMFLOAT2(5 * texWidth, 0)
	};

	//forward
	sbv[18] =
	{
		XMFLOAT3(-1, -1, 1),
		XMFLOAT2(0, 1)
	};
	sbv[19] =
	{
		XMFLOAT3(-1, 1, 1),
		XMFLOAT2(0, 0)
	};
	sbv[20] =
	{
		XMFLOAT3(1, -1, 1),
		XMFLOAT2(texWidth, 1)
	};

	sbv[21] =
	{
		XMFLOAT3(1, -1, 1),
		XMFLOAT2(texWidth, 1)
	};
	sbv[22] =
	{
		XMFLOAT3(-1, 1, 1),
		XMFLOAT2(0, 0)
	};
	sbv[23] =
	{
		XMFLOAT3(1, 1, 1),
		XMFLOAT2(texWidth, 0)
	};
	//right
	sbv[24] =
	{
		XMFLOAT3(1, -1, 1),
		XMFLOAT2(5 * texWidth, 1)
	};
	sbv[25] =
	{
		XMFLOAT3(1, 1, 1),
		XMFLOAT2(5 * texWidth, 0)
	};
	sbv[26] =
	{
		XMFLOAT3(1, -1, -1),
		XMFLOAT2(1, 1)
	};

	sbv[27] =
	{
		XMFLOAT3(1, -1, -1),
		XMFLOAT2(1, 1)
	};
	sbv[28] =
	{
		XMFLOAT3(1, 1, 1),
		XMFLOAT2(5 * texWidth, 0)
	};
	sbv[29] =
	{
		XMFLOAT3(1, 1, -1),
		XMFLOAT2(1, 0)
	};
	//back
	sbv[30] =
	{
		XMFLOAT3(1, -1, -1),
		XMFLOAT2(texWidth, 1)
	};
	sbv[31] =
	{
		XMFLOAT3(1, 1, -1),
		XMFLOAT2(texWidth, 0)
	};
	sbv[32] =
	{
		XMFLOAT3(-1, -1, -1),
		XMFLOAT2(2 * texWidth, 1)
	};

	sbv[33] =
	{
		XMFLOAT3(-1, -1, -1),
		XMFLOAT2(2 * texWidth, 1)
	};
	sbv[34] =
	{
		XMFLOAT3(1, 1, -1),
		XMFLOAT2(texWidth, 0)
	};
	sbv[35] =
	{
		XMFLOAT3(-1, 1, -1),
		XMFLOAT2(2 * texWidth, 0)
	};

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 36;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = sbv;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferSky);
	if (FAILED(hr))
		return hr;

	//*****************************************************************
	//BILLBOARD VERTICES
	//*****************************************************************

	vertices[0].Pos = XMFLOAT3(-1, 1, 1);	//left top
	vertices[1].Pos = XMFLOAT3(1, -1, 1);	//right bottom
	vertices[2].Pos = XMFLOAT3(-1, -1, 1); //left bottom
	vertices[0].Tex = XMFLOAT2(0.0f, 0.0f);
	vertices[1].Tex = XMFLOAT2(1.0f, 1.0f);
	vertices[2].Tex = XMFLOAT2(0.0f, 1.0f);

	vertices[3].Pos = XMFLOAT3(-1, 1, 1);	//left top
	vertices[4].Pos = XMFLOAT3(1, 1, 1);	//right top
	vertices[5].Pos = XMFLOAT3(1, -1, 1);	//right bottom
	vertices[3].Tex = XMFLOAT2(0.0f, 0.0f);			//left top
	vertices[4].Tex = XMFLOAT2(1.0f, 0.0f);			//right top
	vertices[5].Tex = XMFLOAT2(1.0f, 1.0f);			//right bottom	


	

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
	if (FAILED(hr))
		return hr;
	
	//*****************************************************************
	// Create vertex buffer, terrain
	//*****************************************************************

	terrain_vertex_count = 500 * 500 * 6;
	SimpleVertex *Tvertices = new SimpleVertex[terrain_vertex_count];//dynamic only because of limitations of static arrays
	// 500 x 500
	// 1920 x 1080
	int count = 0;
	float ft = 1.0 / 500;
	for (int zz = 0; zz<500; zz++)
		for (int xx = 0; xx < 500; xx++)
		{
			Tvertices[count + 0].Pos = XMFLOAT3(0, 0, 1) + XMFLOAT3(xx, 0, zz);		//left top
			Tvertices[count + 1].Pos = XMFLOAT3(1, 0, 0) + XMFLOAT3(xx, 0, zz);		//right bottom
			Tvertices[count + 2].Pos = XMFLOAT3(0, 0, 0) + XMFLOAT3(xx, 0, zz);		//left bottom
			Tvertices[count + 0].Tex = XMFLOAT2(0.0f, ft) + XMFLOAT2(ft*xx, ft*zz);
			Tvertices[count + 1].Tex = XMFLOAT2(ft, 0.0f) + XMFLOAT2(ft*xx, ft*zz);
			Tvertices[count + 2].Tex = XMFLOAT2(0.0f, 0.0f) + XMFLOAT2(ft*xx, ft*zz);

			Tvertices[count + 3].Pos = XMFLOAT3(0, 0, 1) + XMFLOAT3(xx, 0, zz);		//left top
			Tvertices[count + 4].Pos = XMFLOAT3(1, 0, 1) + XMFLOAT3(xx, 0, zz);		//right top
			Tvertices[count + 5].Pos = XMFLOAT3(1, 0, 0) + XMFLOAT3(xx, 0, zz);		//right bottom
			Tvertices[count + 3].Tex = XMFLOAT2(0.0f, ft) + XMFLOAT2(ft*xx, ft*zz);			//left top
			Tvertices[count + 4].Tex = XMFLOAT2(ft, ft) + XMFLOAT2(ft*xx, ft*zz);			//right top
			Tvertices[count + 5].Tex = XMFLOAT2(ft, 0.0f) + XMFLOAT2(ft*xx, ft*zz);			//right bottom	
			count = count + 6;
		}

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * terrain_vertex_count;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = Tvertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferTerrain);
	if (FAILED(hr))
		return hr;

	delete[] Tvertices;

	//*****************************************************************
	// Create vertex buffer, WATER
	//*****************************************************************

	water_vertex_count = 500 * 500 * 6;
	SimpleVertex *Wvertices = new SimpleVertex[water_vertex_count];//dynamic only because of limitations of static arrays

	count = 0;
	ft = 1.0 / 500;
	for (int zz = 0; zz<500; zz++)
		for (int xx = 0; xx < 500; xx++)
		{
			Wvertices[count + 0].Pos = XMFLOAT3(0, 0, 1) + XMFLOAT3(xx, 0, zz);		//left top
			Wvertices[count + 1].Pos = XMFLOAT3(1, 0, 0) + XMFLOAT3(xx, 0, zz);		//right bottom
			Wvertices[count + 2].Pos = XMFLOAT3(0, 0, 0) + XMFLOAT3(xx, 0, zz);		//left bottom
			Wvertices[count + 0].Tex = XMFLOAT2(0.0f, ft) + XMFLOAT2(ft*xx, ft*zz);
			Wvertices[count + 1].Tex = XMFLOAT2(ft, 0.0f) + XMFLOAT2(ft*xx, ft*zz);
			Wvertices[count + 2].Tex = XMFLOAT2(0.0f, 0.0f) + XMFLOAT2(ft*xx, ft*zz);

			Wvertices[count + 3].Pos = XMFLOAT3(0, 0, 1) + XMFLOAT3(xx, 0, zz);		//left top
			Wvertices[count + 4].Pos = XMFLOAT3(1, 0, 1) + XMFLOAT3(xx, 0, zz);		//right top
			Wvertices[count + 5].Pos = XMFLOAT3(1, 0, 0) + XMFLOAT3(xx, 0, zz);		//right bottom
			Wvertices[count + 3].Tex = XMFLOAT2(0.0f, ft) + XMFLOAT2(ft*xx, ft*zz);			//left top
			Wvertices[count + 4].Tex = XMFLOAT2(ft, ft) + XMFLOAT2(ft*xx, ft*zz);			//right top
			Wvertices[count + 5].Tex = XMFLOAT2(ft, 0.0f) + XMFLOAT2(ft*xx, ft*zz);			//right bottom	
			count = count + 6;
		}

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * water_vertex_count;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = Wvertices;
	hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBufferWater);
	if (FAILED(hr))
		return hr;

	delete[] Wvertices;


	// Supply the vertex shader constant data.
	MatrixBuffer.World = XMMatrixIdentity();
	MatrixBuffer.View = XMMatrixIdentity();
	MatrixBuffer.Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (float)height, 0.001, 1000);
	MatrixBuffer.offsets = XMFLOAT4(0, 0, 0, 0);

	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDESC;
	ZeroMemory(&cbDESC, sizeof(cbDESC));
	cbDESC.ByteWidth = sizeof(CONSTANT_MATRIX_BUFFER);
	cbDESC.Usage = D3D11_USAGE_DEFAULT;
	cbDESC.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDESC.CPUAccessFlags = 0;
	cbDESC.MiscFlags = 0;
	cbDESC.StructureByteStride = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &MatrixBuffer;
	// Create the buffer.
	hr = g_pd3dDevice->CreateBuffer(&cbDESC, &InitData, &g_pConstantMatrixBuffer);
	if (FAILED(hr))
		return hr;

	 //Supply the vertex shader constant data.

	VsConstData.some_variable_a = 0;
	VsConstData.some_variable_b = 0;
	VsConstData.some_variable_c = 1;
	VsConstData.some_variable_d = 1;
	VsConstData.div_tex_x = 1;
	VsConstData.div_tex_y = 1;
	VsConstData.slice_x = 0;
	VsConstData.slice_y = 0;

	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.ByteWidth = sizeof(VS_CONSTANT_BUFFER);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = &VsConstData;
	// Create the buffer.
	hr = g_pd3dDevice->CreateBuffer(&cbDesc, &InitData, &g_pConstantBuffer11);
	if (FAILED(hr))
		return hr;


	//Init texture
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"cube.jpg", NULL, NULL, &g_Texture, NULL);
	if (FAILED(hr))
		return hr;

	//Init texture PLANE
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"FA18.png", NULL, NULL, &g_TexPlane, NULL);
	if (FAILED(hr))
		return hr;

	//TEXTURE FOR CLOUD
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"Cloud.png", NULL, NULL, &g_TextureCloud, NULL);
	if (FAILED(hr))
		return hr;

	//HEIGHTMAP TEX
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"hmap2.png", NULL, NULL, &g_HeightMap, NULL);
	if (FAILED(hr))
		return hr;

	//GRASS TEX
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"grass.jpg", NULL, NULL, &g_grass, NULL);
	if (FAILED(hr))
		return hr;

	//Wave 1 TEX
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"wave1.jpg", NULL, NULL, &g_HeightMapWave1, NULL);
	if (FAILED(hr))
		return hr;

	//Wave 2 TEX
	hr = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice, L"wave2.jpg", NULL, NULL, &g_HeightMapWave2, NULL);
	if (FAILED(hr))
		return hr;





	//SAmpler init
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_Sampler);
	if (FAILED(hr))
		return hr;

	//blendstate init

	//blendstate:
	D3D11_BLEND_DESC blendStateDesc;
	ZeroMemory(&blendStateDesc, sizeof(D3D11_BLEND_DESC));
	blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	g_pd3dDevice->CreateBlendState(&blendStateDesc, &g_BlendState);

	//matrices

	//world
	g_world = XMMatrixIdentity();

	//view:
	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);//camera position
	XMVECTOR At = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);//look at
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);// normal vector on at vector (always up)
	g_view = XMMatrixLookAtLH(Eye, At, Up);

	//perspective:
	// Initialize the projection matrix
	g_projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 10000.0f);

	//load the plane:
	LoadOBJ("FA18.obj", g_pd3dDevice, &g_pVertexBufferPlane, &g_vertices_plane);

	//depth buffer:

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_R32_TYPELESS;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;


	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	//rasterizer states:
	//setting the rasterizer:
	D3D11_RASTERIZER_DESC			RS_CW, RS_Wire;

	RS_CW.AntialiasedLineEnable = FALSE;
	RS_CW.CullMode = D3D11_CULL_BACK;
	RS_CW.DepthBias = 0;
	RS_CW.DepthBiasClamp = 0.0f;
	RS_CW.DepthClipEnable = true;
	RS_CW.FillMode = D3D11_FILL_SOLID;
	RS_CW.FrontCounterClockwise = false;
	RS_CW.MultisampleEnable = FALSE;
	RS_CW.ScissorEnable = false;
	RS_CW.SlopeScaledDepthBias = 0.0f;
	//rasterizer state clockwise triangles
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CW);
	//rasterizer state counterclockwise triangles
	RS_CW.CullMode = D3D11_CULL_FRONT;
	g_pd3dDevice->CreateRasterizerState(&RS_CW, &rs_CCW);
	RS_Wire = RS_CW;
	RS_Wire.CullMode = D3D11_CULL_NONE;
	//rasterizer state seeing both sides of the triangle
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_NO);
	//rasterizer state wirefrime
	RS_Wire.FillMode = D3D11_FILL_WIREFRAME;
	g_pd3dDevice->CreateRasterizerState(&RS_Wire, &rs_Wire);

	//init depth stats:
	//create the depth stencil states for turning the depth buffer on and of:
	D3D11_DEPTH_STENCIL_DESC		DS_ON, DS_OFF;
	DS_ON.DepthEnable = true;
	DS_ON.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DS_ON.DepthFunc = D3D11_COMPARISON_LESS;
	// Stencil test parameters
	DS_ON.StencilEnable = true;
	DS_ON.StencilReadMask = 0xFF;
	DS_ON.StencilWriteMask = 0xFF;
	// Stencil operations if pixel is front-facing
	DS_ON.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	DS_ON.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Stencil operations if pixel is back-facing
	DS_ON.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	DS_ON.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DS_ON.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	// Create depth stencil state
	DS_OFF = DS_ON;
	DS_OFF.DepthEnable = false;
	g_pd3dDevice->CreateDepthStencilState(&DS_ON, &ds_on);
	g_pd3dDevice->CreateDepthStencilState(&DS_OFF, &ds_off);
	
	//Billboards spacing and placements
	for (int i = 0; i < billboardcount; i++)
	{
		pos[i] = XMFLOAT3(2 + (rand() % 1000 + ((rand() % 10 / 10.0) / 10.0)), rand() % 100 + ((rand() % 10 / 10.0) / 10.0), 5 + rand() % 1000 + ((rand() % 10 / 10.0) / 10.0));
	}

	return S_OK;
	}

//--------------------------------------------------------------------------------------
// Render function
//--------------------------------------------------------------------------------------
	float angle = 0;
	bool a_key = false;
	bool s_key = false;
	float angleSpeed = 0.01;

	XMFLOAT2 camPos = XMFLOAT2(0, 0); //<-- camera position in x-z plane. note that z is y here
	float camSpeed = 0.001;

void Render()
	{

	static float x1 = 0;
	static float x2 = 0;
	static float y1 = 0;
	static float y2 = 0;

	x1 += 0.0000005;
	x2 -= 0.0000005;
	y1 -= 0.0000005;
	y2 += 0.0000005;


	if (a_key && !s_key)
	{
		angle -= angleSpeed;
	}
	else if (s_key && !a_key)
	{
		angle += angleSpeed;
	}

	{
		XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
		XMMATRIX rotation = XMMatrixRotationY(angle);
		forward = XMVector3Transform(forward, rotation);
		forward = camSpeed * XMVector3Normalize(forward);
		camPos.x += XMVectorGetX(forward);
		camPos.y += XMVectorGetZ(forward);

		MatrixBuffer.camPos.x = camPos.x;
		MatrixBuffer.camPos.y = camPos.y;
	}

	
	// Clear the back buffer 
	float ClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // red,green,blue,alpha
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	float blendFactor[] = { 0, 0, 0, 0 };
	UINT sampleMask = 0xffffffff;
	g_pImmediateContext->OMSetBlendState(g_BlendState, blendFactor, sampleMask);


	// Set stuff for all objects
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_Sampler);
	g_pImmediateContext->VSSetSamplers(0, 1, &g_Sampler);
	VsConstData.campos.x = cam.pos.x;
	VsConstData.campos.y = cam.pos.y;
	VsConstData.campos.z = cam.pos.z;
	VsConstData.campos.w = 1;


	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderSky, NULL, 0);

	MatrixBuffer.World = XMMatrixTranslation(-250, -10, -250);
	MatrixBuffer.View = XMMatrixRotationY(-angle);
	g_pImmediateContext->UpdateSubresource(g_pConstantMatrixBuffer, 0, 0, &MatrixBuffer, 0, 0);
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pConstantMatrixBuffer);
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pConstantMatrixBuffer);

	// Set vertex buffer, setting the model
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;



	g_pImmediateContext->RSSetState(rs_CW);

	XMMATRIX Mplane;

	//change the world matrix

	XMMATRIX V = g_view;
	V = XMMatrixRotationY(-angle);
	VsConstData.projection = g_projection;
	

	//			************			render the skysphere:				********************

	XMMATRIX Tv = XMMatrixTranslation(cam.pos.x, cam.pos.y, -cam.pos.z);
	XMMATRIX Rx = XMMatrixRotationX(XM_PIDIV2);
	XMMATRIX S = XMMatrixScaling(0.1, 0.1, 0.1);
	XMMATRIX Msky = Tv; //from left to right
	VsConstData.world = Msky;
	VsConstData.view = V;
	VsConstData.projection = g_projection;
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferSky, &stride, &offset);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_Texture);

	g_pImmediateContext->RSSetState(rs_CW);//to see it from the inside
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);//no depth writing
	g_pImmediateContext->Draw(36, 0);		//render sky
	g_pImmediateContext->RSSetState(rs_CW);//reset to default
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);//reset to default

	//********************************************************************

	// Render billboard
	
	XMMATRIX Vc = V;
	Vc._41 = 0;
	Vc._42 = 0;
	Vc._43 = 0;
	XMVECTOR f;
	Vc = XMMatrixInverse(&f, Vc);
	g_pImmediateContext->OMSetDepthStencilState(ds_off, 1);//no depth writing
	for (int i = 0; i < billboardcount; i++)
	{
		Tv = XMMatrixTranslation(pos[i].x, pos[i].y, pos[i].z);
		S = XMMatrixScaling(5.0, 5.0, 5.0);
		Mplane = S*Vc*Tv; //from left to right

		VsConstData.view = XMMatrixRotationY(-angle);
		VsConstData.world = Mplane;
		VsConstData.projection = g_projection;

		g_pImmediateContext->PSSetShaderResources(0, 1, &g_TextureCloud);
		g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
		g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
		g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);//setting it enable for the PixelShader
		g_pImmediateContext->PSSetShader(g_pPixelShaderCloud, NULL, 0);
		
		g_pImmediateContext->Draw(6, 0);		//render rectangle
	}
	g_pImmediateContext->OMSetDepthStencilState(ds_on, 1);//no depth writing

	//**********************************************************WATER TERRAIN*****************************************

	MatrixBuffer.World = XMMatrixTranslation(-250, -22, -250);;
	MatrixBuffer.View = XMMatrixRotationY(-angle);
	MatrixBuffer.Projection = g_projection;
	MatrixBuffer.offsets.x = x1;
	MatrixBuffer.offsets.y = x2;
	g_pImmediateContext->UpdateSubresource(g_pConstantMatrixBuffer, 0, 0, &MatrixBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pConstantMatrixBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pConstantMatrixBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferTerrain, &stride, &offset);

	g_pImmediateContext->VSSetShader(g_pVertexShaderWaterHeight, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderWaterHeight, NULL, 0);

	g_pImmediateContext->PSSetShaderResources(1, 1, &g_HeightMapWave1);
	g_pImmediateContext->VSSetShaderResources(1, 1, &g_HeightMapWave1);
	g_pImmediateContext->PSSetShaderResources(2, 1, &g_HeightMapWave2);
	g_pImmediateContext->VSSetShaderResources(2, 1, &g_HeightMapWave2);
	g_pImmediateContext->Draw(water_vertex_count, 0);		//render terrain



//***********************************************************GRASS TERRAIN*******************************************
															
	MatrixBuffer.World = XMMatrixTranslation(-250, -20, -250);
	MatrixBuffer.View = XMMatrixRotationY(-angle);
	g_pImmediateContext->UpdateSubresource(g_pConstantMatrixBuffer, 0, 0, &MatrixBuffer, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pConstantMatrixBuffer);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(1, 1, &g_pConstantMatrixBuffer);					//setting it enable for the PixelShader
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferTerrain, &stride, &offset);

	g_pImmediateContext->VSSetShader(g_pVertexShaderHeight, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShaderHeight, NULL, 0);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_grass);
	g_pImmediateContext->PSSetShaderResources(1, 1, &g_HeightMap);
	g_pImmediateContext->VSSetShaderResources(1, 1, &g_HeightMap);
	g_pImmediateContext->Draw(terrain_vertex_count, 0);		//render terrain


	

	// *****************************************************************************Render Plane:


	VsConstData.world = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0, -2, 15) * XMMatrixRotationY(angle);

	g_pImmediateContext->PSSetShaderResources(0, 1, &g_TexPlane);
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer11, 0, 0, &VsConstData, 0, 0);	//copying it freshly into the GPU buffer from VsConstData
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer11);					//setting it enable for the VertexShader
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer11);
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBufferPlane, &stride, &offset);//setting it enable for the PixelShader
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, 0);
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, 0);
	g_pImmediateContext->Draw(g_vertices_plane, 0);		//render plane

	// LAST OBJECT TO RENDER BEFORE SWAPCHAIN

	g_pSwapChain->Present(1, 0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);	//message loop function (containing all switch-case statements

int WINAPI wWinMain(				//	the main function in a window program. program starts here
	HINSTANCE hInstance,			//	here the program gets its own number
	HINSTANCE hPrevInstance,		//	in case this program is called from within another program
	LPTSTR    lpCmdLine,
	int       nCmdShow)
	{

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	hInst = hInstance;												//						save in global variable for further use
	MSG msg;

	// Globale Zeichenfolgen initialisieren
	LoadString(hInstance, 103, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, 104, szWindowClass, MAX_LOADSTRING);
	//register Window													<<<<<<<<<<			STEP ONE: REGISTER WINDOW						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	WNDCLASSEX wcex;												//						=> Filling out struct WNDCLASSEX
	BOOL Result = TRUE;
	wcex.cbSize = sizeof(WNDCLASSEX);								//						size of this struct (don't know why
	wcex.style = CS_HREDRAW | CS_VREDRAW;							//						?
	wcex.lpfnWndProc = (WNDPROC)WndProc;							//						The corresponding Proc File -> Message loop switch-case file
	wcex.cbClsExtra = 0;											//
	wcex.cbWndExtra = 0;											//
	wcex.hInstance = hInstance;										//						The number of the program
	wcex.hIcon = LoadIcon(hInstance, NULL);							//
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);						//
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);				//						Background color
	wcex.lpszMenuName = NULL;										//
	wcex.lpszClassName = L"TutorialWindowClass";									//						Name of the window (must the the same as later when opening the window)
	wcex.hIconSm = LoadIcon(wcex.hInstance, NULL);					//
	Result = (RegisterClassEx(&wcex) != 0);							//						Register this struct in the OS

						//Rectangle size look here											//													STEP TWO: OPENING THE WINDOW with x,y position and xlen, ylen !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	RECT rc = { 0, 0, 1920, 1080 };//640,480 ... 1280,720
	hMain = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 2: Rendering a Triangle",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
		NULL);
	if (hMain == 0)	return 0;

	ShowWindow(hMain, nCmdShow);
	UpdateWindow(hMain);


	if (FAILED(InitDevice()))
		{
		return 0;
		}

	//													STEP THREE: Going into the infinity message loop							  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// Main message loop
	msg = { 0 };
	while (WM_QUIT != msg.message)
		{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		else
			{
			Render();
			}
		}

	return (int)msg.wParam;
	}
///////////////////////////////////////////////////
void redr_win_full(HWND hwnd, bool erase)
	{
	RECT rt;
	GetClientRect(hwnd, &rt);
	InvalidateRect(hwnd, &rt, erase);
	}

///////////////////////////////////
//		This Function is called every time the Left Mouse Button is down
///////////////////////////////////
void OnLBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is down
///////////////////////////////////
void OnRBD(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{

	}
///////////////////////////////////
//		This Function is called every time a character key is pressed
///////////////////////////////////
void OnChar(HWND hwnd, UINT ch, int cRepeat)
	{

	}
///////////////////////////////////
//		This Function is called every time the Left Mouse Button is up
///////////////////////////////////
void OnLBU(HWND hwnd, int x, int y, UINT keyFlags)
	{
	if (x > 250)
		{
		PostQuitMessage(0);
		}

	}
///////////////////////////////////
//		This Function is called every time the Right Mouse Button is up
///////////////////////////////////
void OnRBU(HWND hwnd, int x, int y, UINT keyFlags)
	{


	}
///////////////////////////////////
//		This Function is called every time the Mouse Moves
///////////////////////////////////


void OnMM(HWND hwnd, int x, int y, UINT keyFlags)
	{

	if ((keyFlags & MK_LBUTTON) == MK_LBUTTON)
		{
		}

	if ((keyFlags & MK_RBUTTON) == MK_RBUTTON)
		{
		}
	}
///////////////////////////////////
//		This Function is called once at the begin of a program
///////////////////////////////////
#define TIMER1 1

BOOL OnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
	{
	hMain = hwnd;
	return TRUE;
	}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
	HWND hwin;

	switch (id)
		{
			default:
				break;
		}

	}
//************************************************************************
void OnTimer(HWND hwnd, UINT id)
	{

	}
//************************************************************************
///////////////////////////////////
//		This Function is called every time the window has to be painted again
///////////////////////////////////


void OnPaint(HWND hwnd)
	{


	}
//****************************************************************************

//*************************************************************************
void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{

	switch (vk)
		{
			case 65://a
				a_key = true;
				break;
			case 68://d
				s_key = true;
				break;
			default:break;

		}
	}

//*************************************************************************
void OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
	{
	switch (vk)
		{

			case 65://a
				a_key = false;
				break;
			case 68://d
				s_key = false;
				break;
			default:
				int i = 2;
				break;

		}

	}


//**************************************************************************
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{

	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
		{



		/*
		#define HANDLE_MSG(hwnd, message, fn)    \
		case (message): return HANDLE_##message((hwnd), (wParam), (lParam), (fn))
		*/

		HANDLE_MSG(hwnd, WM_CHAR, OnChar);			// when a key is pressed and its a character
		HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLBD);	// when pressing the left button
		HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLBU);		// when releasing the left button
		HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMM);		// when moving the mouse inside your window
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);		// called only once when the window is created
													//HANDLE_MSG(hwnd, WM_PAINT, OnPaint);		// drawing stuff
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);	// not used
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKeyDown);	// press a keyboard key
		HANDLE_MSG(hwnd, WM_KEYUP, OnKeyUp);		// release a keyboard key
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);		// timer
			case WM_PAINT:
				hdc = BeginPaint(hMain, &ps);
				EndPaint(hMain, &ps);
				break;
			case WM_ERASEBKGND:
				return (LRESULT)1;
			case WM_DESTROY:
				PostQuitMessage(0);
				break;
			default:
				return DefWindowProc(hwnd, message, wParam, lParam);
		}
	return 0;
	}

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
	{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(szFileName, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
		{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		if (pErrorBlob) pErrorBlob->Release();
		return hr;
		}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
	}