#include <chrono>
#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdint>
#include <cmath>


#include "ConfigParser.h"
#include "dxut.h"
#include "DXUTmisc.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"

#include "d3dx11effect.h"

#include "Terrain.h"
#include "Mesh.h"
#include "GameEffect.h"
#include "SpriteRenderer.h"
#include "ConfigParser.h"
#include "GameObject.h"

#include "debug.h"


// Help macros
#define DEG2RAD( a ) ( (a) * XM_PI / 180.f )

using namespace std;
using namespace DirectX;

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------

// Camera
struct CAMERAPARAMS {
	float   fovy;
	float   aspect;
	float   nearPlane;
	float   farPlane;
}                                       g_cameraParams;
float                                   g_cameraMoveScaler = 1000.f;
float                                   g_cameraRotateScaler = 0.005f;
bool                                    g_cameraMovement = false;
CFirstPersonCamera                      g_camera;               // A first person camera

// User Interface
CDXUTDialogResourceManager              g_dialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                         g_settingsDlg;          // Device settings dialog
CDXUTTextHelper*                        g_txtHelper = NULL;
CDXUTDialog                             g_hud;                  // dialog for standard controls
CDXUTDialog                             g_sampleUI;             // dialog for sample specific controls


bool                                    g_terrainSpinning = false;
XMMATRIX                                g_terrainWorld; // object- to world-space transformation


// Scene information
XMVECTOR                                g_lightDir;
Terrain									g_terrain;

GameEffect								g_gameEffect; // CPU part of Shader
std::unique_ptr<SpriteRenderer>         g_spriteRenderer;
ConfigParser                            g_ConfigParser;

std::map<std::string, std::shared_ptr<Mesh>>    g_meshes;
std::vector<std::shared_ptr<EnemyObject>>       g_enemyPrototypes;
std::vector<MeshObject>                         g_gameObjects;
std::shared_ptr<ParentObject>                   g_cameraObject;
std::shared_ptr<ParentObject>                   g_terrainObject;
std::list<EnemyObject>                          g_enemyObjects;
std::vector<SpriteVertex>                       g_sprites;
std::vector<SpriteVertex>                       g_projectiles;

float                                   g_timeSinceLastEnemy = 5.0f;
bool                                    g_PlasmaGunIsReady = true;
float                                   g_PlasmaGunTimer = 0.0f;
bool                                    g_GatlingGunIsReady = true;
float                                   g_GatlingGunTimer = 0.0f;
//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_TOGGLESPIN          4
#define IDC_RELOAD_SHADERS		101

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *, UINT , const CD3D11EnumDeviceInfo *,
                                       DXGI_FORMAT, bool, void* );
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
                                         const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext );
void CALLBACK OnD3D11DestroyDevice( void* pUserContext );
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
                                 float fElapsedTime, void* pUserContext );

bool compare(SpriteVertex& a, SpriteVertex& b, const CFirstPersonCamera& camera);


void InitApp();
void DeinitApp();
void RenderText();

void ReleaseShader();
HRESULT ReloadShader(ID3D11Device* pd3dDevice);

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // Old Direct3D Documentation:
    // Start > All Programs > Microsoft DirectX SDK (June 2010) > Windows DirectX Graphics Documentation

    // DXUT Documentaion:
    // Start > All Programs > Microsoft DirectX SDK (June 2010) > DirectX Documentation for C++ : The DirectX Software Development Kit > Programming Guide > DXUT
	
    // New Direct3D Documentaion (just for reference, use old documentation to find explanations):
    // http://msdn.microsoft.com/en-us/library/windows/desktop/hh309466%28v=vs.85%29.aspx


    // Initialize COM library for windows imaging components
    /*HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (hr != S_OK)
    {
        MessageBox(0, L"Error calling CoInitializeEx", L"Error", MB_OK | MB_ICONERROR);
        exit(-1);
    }*/


    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D11DeviceAcceptable( IsD3D11DeviceAcceptable );
    DXUTSetCallbackD3D11DeviceCreated( OnD3D11CreateDevice );
    DXUTSetCallbackD3D11SwapChainResized( OnD3D11ResizedSwapChain );
    DXUTSetCallbackD3D11SwapChainReleasing( OnD3D11ReleasingSwapChain );
    DXUTSetCallbackD3D11DeviceDestroyed( OnD3D11DestroyDevice );
    DXUTSetCallbackD3D11FrameRender( OnD3D11FrameRender );
    //DXUTSetIsInGammaCorrectMode(false);

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"TODO: Insert Title Here" ); // You may change the title

    DXUTCreateDevice( D3D_FEATURE_LEVEL_10_0, true, 1280, 720 );

    DXUTMainLoop(); // Enter into the DXUT render loop

    DXUTShutdown();
    DeinitApp();

    return DXUTGetExitCode();
}

//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    HRESULT hr;
    WCHAR path[MAX_PATH];

    // Parse the config file

    V(DXUTFindDXSDKMediaFileCch(path, MAX_PATH, L"game.cfg"));
	char pathA[MAX_PATH];
	size_t size;
	wcstombs_s(&size, pathA, path, MAX_PATH);

    if (!g_ConfigParser.load(pathA))
        MessageBoxA(NULL, "Could not load configfile \"game.cfg\" ", "File not found", MB_ICONERROR | MB_OK);

    // Intialize the user interface

    g_settingsDlg.Init( &g_dialogResourceManager );
    g_hud.Init( &g_dialogResourceManager );
    g_sampleUI.Init( &g_dialogResourceManager );

    g_hud.SetCallback( OnGUIEvent );
    int iY = 30;
    int iYo = 26;
    g_hud.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 0, iY, 170, 22 );
    g_hud.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 0, iY += iYo, 170, 22, VK_F3 );
    g_hud.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 0, iY += iYo, 170, 22, VK_F2 );

	g_hud.AddButton (IDC_RELOAD_SHADERS, L"Reload shaders (F5)", 0, iY += 24, 170, 22, VK_F5);
    
    g_sampleUI.SetCallback( OnGUIEvent ); iY = 10;
    // Not needed for a game
    /*iY += 24;
    g_sampleUI.AddCheckBox( IDC_TOGGLESPIN, L"Toggle Spinning", 0, iY += 24, 125, 22, g_terrainSpinning );*/

    // Create meshes
    for (auto &m : g_ConfigParser.get_Meshes())
        g_meshes.emplace(m.identifier, std::make_shared<Mesh>(m.pathMesh, m.pathDiffuse, m.pathSpecular, m.pathGlow));

    // Create parent game objects
    g_cameraObject = std::make_shared<ParentObject>();
    g_terrainObject = std::make_shared<ParentObject>();
    // Create game objects
    for (auto& o : g_ConfigParser.get_Objects())
    {
        MeshObject new_gameObject;

        new_gameObject.position = XMFLOAT3(o.pos_x, o.pos_y, o.pos_z);
        new_gameObject.rotation = XMFLOAT3(DEG2RAD(o.rot_x), DEG2RAD(o.rot_y), DEG2RAD(o.rot_z));
        new_gameObject.scale = XMFLOAT3(o.scale, o.scale, o.scale);
        
        auto mesh_it = g_meshes.find(o.meshIdentifer);
        if (mesh_it != g_meshes.end())
            new_gameObject.mesh = mesh_it->second;
        else
            std::cerr << "ERROR: Mesh with identifier " << o.meshIdentifer << " could not be found\n";

        if (o.parentIdentifier == "camera")
            new_gameObject.parent = g_cameraObject;
        else if (o.parentIdentifier == "terrain")
            new_gameObject.parent = g_terrainObject;
        
        g_gameObjects.push_back(new_gameObject);
    }

    // Create Enemy Prototypes
    // https://en.wikipedia.org/wiki/Prototype_pattern
    for (auto& e : g_ConfigParser.get_Enemies())
    {
        EnemyObject new_enemy;

        new_enemy.position = XMFLOAT3(e.pos_x, e.pos_y, e.pos_z);
        new_enemy.rotation = XMFLOAT3(DEG2RAD(e.rot_x), DEG2RAD(e.rot_y), DEG2RAD(e.rot_z));
        new_enemy.scale = XMFLOAT3(e.scale * e.size, e.scale * e.size, e.scale * e.size);

        new_enemy.health = e.hp;
        new_enemy.velocity = XMFLOAT3(e.speed, e.speed, e.speed);

        auto mesh_it = g_meshes.find(e.meshIdentifer);
        if (mesh_it != g_meshes.end())
            new_enemy.mesh = mesh_it->second;
        else
            std::cerr << "ERROR: Mesh with identifier " << e.meshIdentifer << " could not be found\n";

        g_enemyPrototypes.push_back(std::make_shared<EnemyObject>(new_enemy));
    }

    // Create some sprites
    g_sprites.resize(1024);
    for (auto& sprite : g_sprites)
    {
        sprite.position = { 
            static_cast<float>(rand() % 200 - 100), 
            static_cast<float>(rand() % 200), 
            static_cast<float>(rand() % 200 - 100)};
        sprite.radius = 1.0f;
        sprite.textureIndex = rand() % 2;
    }

    // Create the sprite renderer object
    std::vector<std::wstring> sprite_file_names = {L"resources/parTrailGatlingDiffuse.dds", L"resources/parTrailPlasmaDiffuse.dds" };
    g_spriteRenderer = std::make_unique<SpriteRenderer>(sprite_file_names);
}

//--------------------------------------------------------------------------------------
// Deinitialize the app 
//--------------------------------------------------------------------------------------
void DeinitApp()
{
    g_gameObjects.clear();
    g_enemyObjects.clear();
    g_enemyPrototypes.clear();
    g_meshes.clear();
    g_spriteRenderer = nullptr;
    g_cameraObject = nullptr;
    g_terrainObject = nullptr;
}

//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for 
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_txtHelper->Begin();
    g_txtHelper->SetInsertionPos( 5, 5 );
    g_txtHelper->SetForegroundColor(XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f));
    g_txtHelper->DrawTextLine( DXUTGetFrameStats(true)); //DXUTIsVsyncEnabled() ) );
    g_txtHelper->DrawTextLine( DXUTGetDeviceStats() );
    g_txtHelper->End();
}

//--------------------------------------------------------------------------------------
// Reject any D3D11 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D11DeviceAcceptable( const CD3D11EnumAdapterInfo *, UINT, const CD3D11EnumDeviceInfo *,
        DXGI_FORMAT, bool, void* )
{
    return true;
}

//--------------------------------------------------------------------------------------
// Specify the initial device settings
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	UNREFERENCED_PARAMETER(pDeviceSettings);
	UNREFERENCED_PARAMETER(pUserContext);

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if (pDeviceSettings->d3d11.DriverType == D3D_DRIVER_TYPE_REFERENCE)
        {
            DXUTDisplaySwitchingToREFWarning();
        }
    }
    //// Enable anti aliasing
    pDeviceSettings->d3d11.sd.SampleDesc.Count = 4;
    pDeviceSettings->d3d11.sd.SampleDesc.Quality = 1;

    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11CreateDevice( ID3D11Device* pd3dDevice,
        const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	UNREFERENCED_PARAMETER(pBackBufferSurfaceDesc);
	UNREFERENCED_PARAMETER(pUserContext);

    HRESULT hr;

    ID3D11DeviceContext* pd3dImmediateContext = DXUTGetD3D11DeviceContext(); // http://msdn.microsoft.com/en-us/library/ff476891%28v=vs.85%29
    V_RETURN( g_dialogResourceManager.OnD3D11CreateDevice( pd3dDevice, pd3dImmediateContext ) );
    V_RETURN( g_settingsDlg.OnD3D11CreateDevice( pd3dDevice ) );
    g_txtHelper = new CDXUTTextHelper( pd3dDevice, pd3dImmediateContext, &g_dialogResourceManager, 15 );

    V_RETURN( ReloadShader(pd3dDevice) );
    
	// Create the terrain
	V_RETURN(g_terrain.create(pd3dDevice));
    
    // Update height values
    for (auto& g : g_gameObjects)
        if (g.parent == g_terrainObject)
            g.position.y += g_terrain.get_height_at(g.position.x, g.position.z);
    
    // Create all meshes
    V_RETURN(Mesh::createInputLayout(pd3dDevice, g_gameEffect.meshPass1));
    for (auto& m : g_meshes)
        V_RETURN(m.second->create(pd3dDevice));

    // Create the sprite renderer
    V_RETURN(g_spriteRenderer->create(pd3dDevice));
    
    // Initialize the camera
    float center_height = g_terrain.get_height_at(0.0f, 0.0f) + 20.0f;
	XMVECTOR vEye = XMVectorSet(0.0f, center_height, 0.0f, 0.0f);   // Camera eye is here
    XMVECTOR vAt = XMVectorSet(1.0f, center_height, 0.0f, 1.0f);               // ... facing at this position
    g_camera.SetViewParams(vEye, vAt); // http://msdn.microsoft.com/en-us/library/windows/desktop/bb206342%28v=vs.85%29.aspx
	g_camera.SetScalers(g_cameraRotateScaler, g_cameraMoveScaler);

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice( void* pUserContext )
{
	UNREFERENCED_PARAMETER(pUserContext);

    g_dialogResourceManager.OnD3D11DestroyDevice();
    g_settingsDlg.OnD3D11DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    
	// Destroy the terrain
	g_terrain.destroy();
    
    // Destroy meshes
    Mesh::destroyInputLayout();
    for (auto& m : g_meshes)
        m.second->destroy();

    // Destroy the sprite renderer
    g_spriteRenderer->destroy();

    SAFE_DELETE( g_txtHelper );
    ReleaseShader();
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D11ResizedSwapChain( ID3D11Device* pd3dDevice, IDXGISwapChain* pSwapChain,
        const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	UNREFERENCED_PARAMETER(pSwapChain);
	UNREFERENCED_PARAMETER(pUserContext);

    HRESULT hr;
    
    // Intialize the user interface

    V_RETURN( g_dialogResourceManager.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_settingsDlg.OnD3D11ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    g_hud.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_hud.SetSize( 170, 170 );
    g_sampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 300 );
    g_sampleUI.SetSize( 170, 300 );

    // Initialize the camera

    g_cameraParams.aspect = pBackBufferSurfaceDesc->Width / ( FLOAT )pBackBufferSurfaceDesc->Height;
    g_cameraParams.fovy = 0.785398f;
    g_cameraParams.nearPlane = 1.f;
    g_cameraParams.farPlane = 5000.f;

    g_camera.SetProjParams(g_cameraParams.fovy, g_cameraParams.aspect, g_cameraParams.nearPlane, g_cameraParams.farPlane);
	g_camera.SetEnablePositionMovement(g_cameraMovement);
	g_camera.SetRotateButtons(true, false, false);
	g_camera.SetScalers( g_cameraRotateScaler, g_cameraMoveScaler );
	g_camera.SetDrag( true );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11ReleasingSwapChain( void* pUserContext )
{
	UNREFERENCED_PARAMETER(pUserContext);
    g_dialogResourceManager.OnD3D11ReleasingSwapChain();
}

//--------------------------------------------------------------------------------------
// Loads the effect from file
// and retrieves all dependent variables
//--------------------------------------------------------------------------------------
HRESULT ReloadShader(ID3D11Device* pd3dDevice)
{
    assert(pd3dDevice != NULL);

    HRESULT hr;

    ReleaseShader();
	V_RETURN(g_gameEffect.create(pd3dDevice));

    g_spriteRenderer->reloadShader(pd3dDevice);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Release resources created in ReloadShader
//--------------------------------------------------------------------------------------
void ReleaseShader()
{
    g_spriteRenderer->releaseShader();
	g_gameEffect.destroy();
}

//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
	UNREFERENCED_PARAMETER(pUserContext);

    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_dialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_settingsDlg.IsActive() )
    {
        g_settingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_hud.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_sampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
        
    // Use the mouse weel to control the movement speed
    if(uMsg == WM_MOUSEWHEEL) {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_cameraMoveScaler *= (1 + zDelta / 500.0f);
        if (g_cameraMoveScaler < 0.1f)
          g_cameraMoveScaler = 0.1f;
        g_camera.SetScalers(g_cameraRotateScaler, g_cameraMoveScaler);
    }

    // Pass all remaining windows messages to camera so it can respond to user input
    g_camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}

//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	UNREFERENCED_PARAMETER(nChar);
	UNREFERENCED_PARAMETER(bKeyDown);
	UNREFERENCED_PARAMETER(bAltDown);
	UNREFERENCED_PARAMETER(pUserContext);
	
    if (nChar == 'M' && bKeyDown)
    {
        g_cameraMovement = !g_cameraMovement;
        g_camera.SetEnablePositionMovement(g_cameraMovement);
    }
	
	if(nChar == 'A' && g_PlasmaGunIsReady)
	{
		std::cout<<"Plasma Gun shooting." << endl;
		g_PlasmaGunIsReady = false;
		g_PlasmaGunTimer = 0.0f;
		SpriteVertex new_Projectiles;
        new_Projectiles.textureIndex =g_ConfigParser.get_Guns().at(1).spriteTexIndex;
        new_Projectiles.radius = g_ConfigParser.get_Guns().at(1).spriteRadius;
		XMVECTOR posTemp = XMLoadFloat3(&g_ConfigParser.get_Guns().at(1).spawnPosView);
		posTemp = XMVector3Transform(posTemp, g_camera.GetWorldMatrix());
		new_Projectiles.position.x = XMVectorGetX(posTemp);
		new_Projectiles.position.y = XMVectorGetY(posTemp);
		new_Projectiles.position.z = XMVectorGetZ(posTemp);
		new_Projectiles.velocity = g_camera.GetWorldAhead() * g_ConfigParser.get_Guns().at(1).speed;
        g_projectiles.push_back(new_Projectiles);
	}

	if(nChar == 'D' && g_GatlingGunIsReady)
	{
		std::cout<<"Gatling Gun shooting."<<endl;
		g_GatlingGunIsReady = false;
		g_GatlingGunTimer = 0.0f;
		SpriteVertex new_Projectiles;
		new_Projectiles.textureIndex = g_ConfigParser.get_Guns().at(0).spriteTexIndex;
        new_Projectiles.radius = g_ConfigParser.get_Guns().at(0).spriteRadius;
        XMVECTOR posTemp = XMLoadFloat3(&g_ConfigParser.get_Guns().at(0).spawnPosView);
		posTemp = XMVector3Transform(posTemp, g_camera.GetWorldMatrix());
		new_Projectiles.position.x = XMVectorGetX(posTemp);
		new_Projectiles.position.y = XMVectorGetY(posTemp);
		new_Projectiles.position.z = XMVectorGetZ(posTemp);
		new_Projectiles.velocity = g_camera.GetWorldAhead() * g_ConfigParser.get_Guns().at(0).speed;
        g_projectiles.push_back(new_Projectiles);
	}
	
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	UNREFERENCED_PARAMETER(nEvent);
	UNREFERENCED_PARAMETER(pControl);
	UNREFERENCED_PARAMETER(pUserContext);

    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_settingsDlg.SetActive( !g_settingsDlg.IsActive() ); break;
        /*case IDC_TOGGLESPIN:
            g_terrainSpinning = g_sampleUI.GetCheckBox( IDC_TOGGLESPIN )->GetChecked();
            break;*/
		case IDC_RELOAD_SHADERS:
			ReloadShader(DXUTGetD3D11Device ());
			break;
    }
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	UNREFERENCED_PARAMETER(pUserContext);
    // Update the camera's position based on user input 
    g_camera.FrameMove( fElapsedTime );
    
    // Initialize the terrain world matrix
    // http://msdn.microsoft.com/en-us/library/windows/desktop/bb206365%28v=vs.85%29.aspx
    g_terrainWorld = XMMatrixScaling(
        g_ConfigParser.get_TerrainWidth(), 
        g_ConfigParser.get_TerrainHeight(), 
        g_ConfigParser.get_TerrainDepth());

    if( g_terrainSpinning ) 
    {
		// If spinning enabled, rotate the world matrix around the y-axis
        g_terrainObject->worldMatrix = XMMatrixRotationY(30.0f * DEG2RAD((float)fTime)); // Rotate around world-space "up" axis
        g_terrainWorld *= g_terrainObject->getWorldMatrix();
    }

	// Set the light vector
    g_lightDir = XMVectorSet(1, 1, 1, 0); // Direction to the directional light in world space    
    g_lightDir = XMVector3Normalize(g_lightDir);

    // Delete enemies
    g_enemyObjects.remove_if([](EnemyObject& e) {
        return e.position.x * e.position.x + e.position.z * e.position.z 
            > g_ConfigParser.get_SpawnBehaviour().spawn_radius * g_ConfigParser.get_SpawnBehaviour().spawn_radius + 1;
        });
    // Update enemies
    for (auto& e : g_enemyObjects)
    {
        e.position.x += e.velocity.x * fElapsedTime;
        e.position.y += e.velocity.y * fElapsedTime;
        e.position.z += e.velocity.z * fElapsedTime;
    }
    // Spawn enemies
    g_timeSinceLastEnemy += fElapsedTime;
    if (g_timeSinceLastEnemy > g_ConfigParser.get_SpawnBehaviour().interval)
    {
        // Uses the copy constructor to copy the prototype in its current state
        int rand_num = rand() % g_enemyPrototypes.size();
        EnemyObject enemy = EnemyObject(*g_enemyPrototypes[rand_num]);

        enemy.type = g_enemyPrototypes[rand_num];

        float spawn_circle = XM_2PI * static_cast<float>(rand()) / RAND_MAX;
        float spawn_height = (static_cast<float>(rand()) / RAND_MAX)
            * (g_ConfigParser.get_SpawnBehaviour().max_height - g_ConfigParser.get_SpawnBehaviour().min_height)
            + g_ConfigParser.get_SpawnBehaviour().min_height;
        enemy.position.x = g_ConfigParser.get_SpawnBehaviour().spawn_radius * sin(spawn_circle);
        enemy.position.y = spawn_height * g_ConfigParser.get_TerrainHeight();
        enemy.position.z = g_ConfigParser.get_SpawnBehaviour().spawn_radius * cos(spawn_circle);


        enemy.scale = { 1.0f, 1.0f, 1.0f };

        float target_cicle = XM_2PI * static_cast<float>(rand()) / RAND_MAX;
        XMFLOAT3 target_pos;
        target_pos.x = g_ConfigParser.get_SpawnBehaviour().target_radius * sin(target_cicle);
        target_pos.y = enemy.position.y;
        target_pos.z = g_ConfigParser.get_SpawnBehaviour().target_radius * cos(target_cicle);

        XMFLOAT3 vel;
        vel.x = target_pos.x - enemy.position.x;
        vel.y = 0.0f;
        vel.z = target_pos.z - enemy.position.z;
        float length = sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
        enemy.velocity.x *= vel.x / length;
        enemy.velocity.y *= vel.y / length;
        enemy.velocity.z *= vel.z / length;

        enemy.rotation = { 0.0f, atan2(vel.x, vel.z), 0.0f };

        g_enemyObjects.push_back(enemy);
        g_timeSinceLastEnemy -= g_ConfigParser.get_SpawnBehaviour().interval;
    }
	g_PlasmaGunTimer += fElapsedTime;
	g_GatlingGunTimer += fElapsedTime;
	if(g_PlasmaGunTimer > g_ConfigParser.get_Guns().at(1).cooldown)
        g_PlasmaGunIsReady = true;
	if(g_GatlingGunTimer > g_ConfigParser.get_Guns().at(0).cooldown)
         g_GatlingGunIsReady = true;

	//update the position of the projectiles
	
	for (auto& p : g_projectiles) 
    {
		//x direction
		p.position.x += XMVectorGetX(p.velocity);
		p.position.z += XMVectorGetZ(p.velocity);
		//y direction
		XMVectorSetY(p.velocity, XMVectorGetY(p.velocity) - g_ConfigParser.get_Guns().at(p.textureIndex).gravity);
		p.position.y += XMVectorGetY(p.velocity);

        for (auto& e : g_enemyObjects) {
            
            XMVECTOR vector1 = XMLoadFloat3(&e.position);
            XMVECTOR vector2 = XMLoadFloat3(&p.position);
            XMVECTOR vectorSub = XMVectorSubtract(vector1, vector2);
            XMVECTOR length = XMVector3Length(vectorSub);
            
            /*
            float distance = sqrt((e.position.x - p.position.x) * (e.position.x - p.position.x) + 
                                  (e.position.y - p.position.y) * (e.position.y - p.position.y) + 
                                  (e.position.z - p.position.z) * (e.position.z - p.position.z));
            */
            float distance = 0.0f;
            XMStoreFloat(&distance, length);

            float min = sqrt((e.size + p.radius) * (e.size + p.radius));
            if (distance <= min) {
                std::cout << "Hit enemy!" << endl;

                
                //g_projectiles.erase(g_projectiles.begin() + i);
                //g_projectiles.erase(find(g_projectiles.begin(), g_projectiles.end(), p));

                if (p.textureIndex == (int)g_ConfigParser.get_Guns()[0].spriteTexIndex) {
                    e.health -= g_ConfigParser.get_Guns()[0].damage;
                }
                else
                {
                    e.health -= g_ConfigParser.get_Guns()[1].damage;
                }
                /*
                if (e.health <= 0) {
                    g_enemyObjects.remove(e);
                }
                */
                
                break;
            }
        }
	}

    g_enemyObjects.remove_if([](EnemyObject& e)
		{
			return e.health <= 0;
		});


	std::remove_if(g_projectiles.begin(), g_projectiles.end(), [](SpriteVertex& p)
	{
		return p.position.x > g_ConfigParser.get_TerrainWidth() || p.position.z > g_ConfigParser.get_TerrainWidth();
	});

    std::remove_if(g_enemyObjects.begin(), g_enemyObjects.end(), [](EnemyObject& e)
        {
            return e.health <=0;
        });
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D11 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, double fTime,
        float fElapsedTime, void* pUserContext )
{
	UNREFERENCED_PARAMETER(pd3dDevice);
	UNREFERENCED_PARAMETER(fTime);
	UNREFERENCED_PARAMETER(pUserContext);

    HRESULT hr;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_settingsDlg.IsActive() )
    {
        g_settingsDlg.OnRender( fElapsedTime );
        return;
    }     

    ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	float clearColor[4] = { 0.025525f * 10.0f, 0.045511f * 10.0f, 0.088005f * 10.0f, 1.0f};
    pd3dImmediateContext->ClearRenderTargetView( pRTV, clearColor );
        
	if(g_gameEffect.effect == NULL) {
        g_txtHelper->Begin();
        g_txtHelper->SetInsertionPos( 5, 5 );
        g_txtHelper->SetForegroundColor( XMVectorSet( 1.0f, 1.0f, 0.0f, 1.0f ) );
        g_txtHelper->DrawTextLine( L"SHADER ERROR" );
        g_txtHelper->End();
        return;
    }

    // Clear the depth stencil
    ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
    pd3dImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );
    
    // Update variables that change once per frame
    XMMATRIX const view = g_camera.GetViewMatrix(); // http://msdn.microsoft.com/en-us/library/windows/desktop/bb206342%28v=vs.85%29.aspx
    XMMATRIX const proj = g_camera.GetProjMatrix(); // http://msdn.microsoft.com/en-us/library/windows/desktop/bb147302%28v=vs.85%29.aspx
    g_cameraObject->worldMatrix = g_camera.GetWorldMatrix();
	V(g_gameEffect.lightDirEV->SetFloatVector( ( float* )&g_lightDir ));
    V(g_gameEffect.cameraPosWorldEV->SetFloatVector((float*)&g_camera.GetEyePt()));
    
    // Render objects
    for (const auto& o : g_gameObjects)
        o.render(pd3dImmediateContext, view * proj);
    for (const auto& e : g_enemyObjects)
        e.render(pd3dImmediateContext, view * proj);
    
    // Render terrain
    XMMATRIX worldViewProj = g_terrainWorld * view * proj;
	V(g_gameEffect.worldEV->SetMatrix( ( float* )&g_terrainWorld ));
	V(g_gameEffect.worldViewProjectionEV->SetMatrix( ( float* )&worldViewProj ));
    V(g_gameEffect.worldNormalsEV->SetMatrix( ( float* )&XMMatrixTranspose(XMMatrixInverse(nullptr, g_terrainWorld))));
	g_terrain.render(pd3dImmediateContext, g_gameEffect.pass0);

    // Render Sprites
	//todo: change g_sprites to g_projectiles and make some adjustment should work
    //std::sort(g_sprites.begin(), g_sprites.end(),);
    struct SVSort
    {
        bool operator()(SpriteVertex& a, SpriteVertex& b) 
        {
            float first = XMVector3Dot(XMLoadFloat3(&a.position),g_camera.GetWorldAhead()).m128_f32[0];
            float second = XMVector3Dot(XMLoadFloat3(&b.position), g_camera.GetWorldAhead()).m128_f32[0];
            return first < second;
        }
    };

    std::sort(g_projectiles.begin(), g_projectiles.end(), SVSort());

    g_spriteRenderer->renderSprites(pd3dImmediateContext, g_projectiles, g_camera);

    // Render HUD
    DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
    V(g_hud.OnRender( fElapsedTime ));
    V(g_sampleUI.OnRender( fElapsedTime ));
    RenderText();
    DXUT_EndPerfEvent();

    static DWORD dwTimefirst = GetTickCount();
    if ( GetTickCount() - dwTimefirst > 5000 )
    {    
        OutputDebugString( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
        OutputDebugString( L"\n" );
        dwTimefirst = GetTickCount();
    }
}
