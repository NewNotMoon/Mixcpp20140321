/*!
 * @brief	MixC++勉強会@Tokyo 新々月｢Direct2DとDirect3D11の共有(DXGI!.2)｣のサンプルコード
 * 
 * Copyright (c) 2014 新々月. All rights reserved.
 */
#define _WIN32_WINNT 0x0601

#include <Windows.h>
#include <tchar.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include "Result.hpp"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d2d1.lib" )
#pragma comment( lib, "dwrite.lib" )

LRESULT CALLBACK procedure( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

HWND setupWindow( int width, int height )
{
	WNDCLASSEX wcex;
	wcex.cbSize         = sizeof( WNDCLASSEX );
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = procedure;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = (HMODULE)GetModuleHandle( 0 );
	wcex.hIcon          = nullptr;
	wcex.hCursor        = LoadCursor( nullptr, IDC_ARROW );
	wcex.hbrBackground  = (HBRUSH)( COLOR_WINDOW + 1 );
	wcex.lpszMenuName   = nullptr;
	wcex.lpszClassName  = _T( "うんこ" );
	wcex.hIconSm        = nullptr;
	if( !RegisterClassEx( &wcex ) )
	{
		throw std::runtime_error{ "うぃんどうとうろくできませんでした" };
	}

	RECT rect ={ 0, 0, width, height };
	AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );
	const int windowWidth  = ( rect.right - rect.left );
	const int windowHeight = ( rect.bottom - rect.top );

	HWND window = CreateWindow( _T( "うんこ" ), _T( "MixC++勉強会 新々月 20140321" ),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, windowWidth, windowHeight,
		nullptr, nullptr, nullptr, nullptr );
	if( !window )
	{
		throw std::runtime_error{ "うぃんどうつくれませんでした" };
	}

	return window;
}

class Sample
{
private:
	HWND window;
	struct
	{
		ID2D1Device*			device;
		ID2D1DeviceContext*		deviceContext;
		ID2D1SolidColorBrush*	brush;
		ID2D1Bitmap1*			target;
	} direct2D;
	struct
	{
		D3D_FEATURE_LEVEL		featureLevel;
		ID3D11Device*			device;
		ID3D11DeviceContext*	deviceContext;
		ID3D11Texture2D*		buffer;
		ID3D11RenderTargetView*	viewRenderTarget;
	} direct3D;
	struct
	{
		IDXGISwapChain1*		swapchain;
		IDXGIDevice1*			device;
	} DXGI;
	IDWriteTextLayout* layout;

public:
	Sample()
	{
		this->initialize();
	}
	~Sample()
	{
		this->release();
	}
	void release()
	{
#define RELEASE( i ) if( (i) ) { (i)->Release(); (i) = nullptr; }
		RELEASE( this->direct2D.device );
		RELEASE( this->direct2D.deviceContext );
		RELEASE( this->direct2D.target );
		RELEASE( this->direct3D.device );
		RELEASE( this->direct3D.deviceContext );
		RELEASE( this->direct3D.buffer );
		RELEASE( this->direct3D.viewRenderTarget );
		RELEASE( this->DXGI.device );
		RELEASE( this->DXGI.swapchain );
#undef RELEASE
	}

	void initialize()
	{
		const int width		= 640;
		const int height	= 480;
		this->window = setupWindow( width, height );
		ShowWindow( this->window, SW_SHOW );
		UpdateWindow( this->window );
		
		D3D_FEATURE_LEVEL features[] =
		{
			D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
		};

		RESULT
		(
			"Direct3Dデバイスの作成",
			D3D11CreateDevice
			(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				0,
				D3D11_CREATE_DEVICE_BGRA_SUPPORT,
				features,
				ARRAYSIZE( features ),
				D3D11_SDK_VERSION,
				&this->direct3D.device,
				&this->direct3D.featureLevel,
				&this->direct3D.deviceContext
			)
		);

		RESULT
		(
			"DXGIデバイスの作成",
			this->direct3D.device->QueryInterface<IDXGIDevice1>( &this->DXGI.device )
		);
		this->DXGI.device->SetMaximumFrameLatency( 1 );

		ID2D1Factory1 *d2dFactory;
		{
			D2D1_FACTORY_OPTIONS option;
			ZeroMemory( &option, sizeof option );
			RESULT
			(
				"Direct2Dファクトリの作成",
				D2D1CreateFactory
				(
					D2D1_FACTORY_TYPE_SINGLE_THREADED,
					__uuidof( ID2D1Factory1 ),
					&option,
					reinterpret_cast<void**>( &d2dFactory )
				)
			);
		}
		float dpiX;
		float dpiY;
		d2dFactory->GetDesktopDpi( &dpiX, &dpiY );
		RESULT
		(
			"Direct2Dデバイスの作成",
			d2dFactory->CreateDevice(
				this->DXGI.device,
				&this->direct2D.device
			)
		);
		d2dFactory->Release();

		RESULT
		(
			"Direct2Dデバイスコンテキストの作成",
			this->direct2D.device->CreateDeviceContext(
				D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
				&this->direct2D.deviceContext
			)
		);
		RESULT
		(
			"Direct2Dブラシの作成",
			this->direct2D.deviceContext->CreateSolidColorBrush( D2D1::ColorF( 0, 0, 0 ), &this->direct2D.brush )
		);

		IDXGIAdapter *adapter;
		RESULT
		(
			"DXGIアダプタの取得",
			this->DXGI.device->GetAdapter( &adapter )
		);

		IDXGIFactory2 *factory;
		RESULT
		(
			"DXGIファクトリの作成",
			adapter->GetParent( IID_PPV_ARGS( &factory ) )
		);
		adapter->Release();

		DXGI_SWAP_CHAIN_DESC1 swapchainDescribe;
		swapchainDescribe.Width					= width;
		swapchainDescribe.Height				= height;
		swapchainDescribe.Format				= DXGI_FORMAT_B8G8R8A8_UNORM;
		swapchainDescribe.Scaling				= DXGI_SCALING_STRETCH;
		swapchainDescribe.Stereo				= 0;
		swapchainDescribe.AlphaMode				= DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchainDescribe.BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDescribe.BufferCount			= 2;
		swapchainDescribe.SwapEffect			= DXGI_SWAP_EFFECT_SEQUENTIAL;
		swapchainDescribe.SampleDesc.Count		= 1;
		swapchainDescribe.SampleDesc.Quality	= 0;
		swapchainDescribe.Flags					= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		RESULT
		(
			"スワップチェインを作成(for HWND)",
			factory->CreateSwapChainForHwnd
			(
				this->direct3D.device,
				this->window,
				&swapchainDescribe,
				nullptr,
				nullptr,
				&this->DXGI.swapchain
			)
		);
		factory->Release();

		RESULT
		(
			"レンダーターゲットの取得",
			this->DXGI.swapchain->GetBuffer
			(
				0,
				IID_PPV_ARGS( &this->direct3D.buffer )
			)
		);

		RESULT
		(
			"レンダーターゲットビューの作成",
			this->direct3D.device->CreateRenderTargetView
			(
				this->direct3D.buffer,
				nullptr,
				&this->direct3D.viewRenderTarget
			)
		);


		IDXGISurface *surface;
		RESULT
		(
			"DXGIサーフェイスの取得",
			this->DXGI.swapchain->GetBuffer
			(
				0,
				IID_PPV_ARGS( &surface )
			)
		);

		D2D1_BITMAP_PROPERTIES1 properties =
			D2D1::BitmapProperties1
			(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED ),
				dpiX,
				dpiY
			);

		RESULT
		(
			"Direct2Dのレンダーターゲットを作成、設定",
			this->direct2D.deviceContext->CreateBitmapFromDxgiSurface
			(
				surface,
				&properties,
				&this->direct2D.target
			)
		);
		surface->Release();
		this->direct2D.deviceContext->SetTarget( this->direct2D.target );

		IDWriteFactory* dwrite;
		IDWriteTextFormat* format;
		RESULT
		(
			"DirectWriteファクトリ作成",
			DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_SHARED,
				__uuidof( IDWriteFactory ),
				reinterpret_cast<IUnknown**>( &dwrite )
			)
		);
		dwrite->CreateTextFormat
		(
			L"メイリオ",
			NULL,
			DWRITE_FONT_WEIGHT_REGULAR,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			30,
			L"",
			&format
        );
		dwrite->CreateTextLayout
		(
			L"MixC++勉強会@Tokyo by 新々月\nDirect2DとDirect3Dの共有(DXGI1.2版)",
			sizeof( L"MixC++勉強会@Tokyo by 新々月\nDirect2DとDirect3Dの共有(DXGI1.2版)" ) / sizeof L"",
			format,
			600,
			100,
			&layout
		);
	}

	void draw()
	{
		float clearColor[] = { 1, 1, 1, 1 };
		this->direct3D.deviceContext->ClearRenderTargetView( this->direct3D.viewRenderTarget, clearColor );
		
		this->direct2D.deviceContext->BeginDraw();
		this->direct2D.deviceContext->DrawTextLayout
		(
			D2D1::Point2F( 0,0 ),
			this->layout,
			this->direct2D.brush
		);
		this->direct2D.deviceContext->EndDraw();

		this->DXGI.swapchain->Present( 1, 0 );
	}
};

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
#ifdef _DEBUG
	::_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	MSG msg;
	try
	{
		Sample sample;
		do
		{
			BOOL r = PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE );
			if( r == 0 )
			{
				sample.draw();
			}
			else
			{
				DispatchMessage( &msg );
			}
			Sleep( 1 );
		}
		while( msg.message != WM_QUIT );
	}
	catch( const char* e )
	{
		OutputDebugStringA( e );
	}
	return static_cast<int>( msg.wParam );
}