// HostBackdrop.cpp : 定义应用程序的入口点。
//
#include "pch.h"
#include <dwmapi.h>
#include <SDKDDKVer.h>
#pragma comment(lib, "dwmapi")
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace ABI::Windows::UI;
using namespace ABI::Windows::UI::Composition;
using namespace ABI::Windows::UI::Composition::Desktop;

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System;

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

auto CreateDispatcherQueueController()
{
	DispatcherQueueOptions options
	{
		sizeof(DispatcherQueueOptions),
		DQTYPE_THREAD_CURRENT,
		DQTAT_COM_STA
	};
	ComPtr<IDispatcherQueueController> pQueueController{nullptr};
	HR(CreateDispatcherQueueController(options, &pQueueController));
	return pQueueController;
}

auto CreateDesktopWindowTarget(HWND hwnd, ICompositor* pCompositor)
{
	IDesktopWindowTarget* pDesktopTarget{nullptr};

	HR(RoCast<ICompositorDesktopInterop>(pCompositor)->CreateDesktopWindowTarget(hwnd, false, &pDesktopTarget));
	return pDesktopTarget;
}

auto SetBackdropVisual(IDesktopWindowTarget* pDesktopTarget, ICompositor* pCompositor)
{
	ComPtr<ISpriteVisual> pVisual{nullptr};
	ComPtr<ICompositionBackdropBrush> pBackdropBrush{nullptr};

	HR(RoCast<ICompositor3>(pCompositor)->CreateHostBackdropBrush(&pBackdropBrush));
	HR(pCompositor->CreateSpriteVisual(&pVisual));
	HR(RoCast<IVisual2>(pVisual)->put_RelativeSizeAdjustment({1.f, 1.f}));
	HR(pVisual->put_Brush(RoCast<ICompositionBrush>(pBackdropBrush).Get()));
	HR(RoCast<ICompositionTarget>(pDesktopTarget)->put_Root(RoCast<IVisual>(pVisual).Get()));
}

// 此函数不能在WM_NCCREATE期间调用
auto EnableBackdropSupport(HWND hwnd, bool bEnable)
{
	struct ACCENT_POLICY
	{
		DWORD AccentState;
		DWORD AccentFlags;
		DWORD GradientColor;
		DWORD AnimationId;
	};
	struct WINDOWCOMPOSITIONATTRIBDATA
	{
		DWORD Attrib;
		PVOID pvData;
		SIZE_T cbData;
	};

	ACCENT_POLICY policy = {static_cast<DWORD>(bEnable ? 5 : 0), 0, 0, 0};
	WINDOWCOMPOSITIONATTRIBDATA data = {static_cast<DWORD>(19), &policy, sizeof(ACCENT_POLICY)};
	static const auto& SetWindowCompositionAttribute = (BOOL(WINAPI*)(HWND, PVOID))GetProcAddress(GetModuleHandle(L"User32"), "SetWindowCompositionAttribute");

	return SetWindowCompositionAttribute(hwnd, &data);
}

auto AllocateWindow(HINSTANCE hInstance)
{
	WNDCLASS wc{};
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = hInstance;
	wc.lpszClassName = L"Sample";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	RegisterClass(&wc);
	HR(HRESULT_FROM_WIN32(GetLastError()));
	CreateWindowEx(
	    WS_EX_NOREDIRECTIONBITMAP,
	    wc.lpszClassName,
	    L"Sample",
	    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	    nullptr, nullptr, wc.hInstance, nullptr
	);
	HR(HRESULT_FROM_WIN32(GetLastError()));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_CREATE:
		{
			ComPtr<ICompositor> pCompositor{nullptr};
			ComPtr<IDesktopWindowTarget> pDesktopTarget{nullptr};

			EnableBackdropSupport(hwnd, true);
			ActivateInstanceWithFactory(HStringReference(RuntimeClass_Windows_UI_Composition_Compositor).Get(), &pCompositor);
			pDesktopTarget = CreateDesktopWindowTarget(hwnd, pCompositor.Get());
			SetBackdropVisual(pDesktopTarget.Get(), pCompositor.Get());
			//ComPtr<IAcrylicBrush> brush;
			//ComPtr<IInspectable> baseInterface;
			//ComPtr<IInspectable> innerInterface;
			//ComPtr<IAcrylicBrushFactory> factory;
			//factory = GetActivationFactory<IAcrylicBrushFactory>(HStringReference(RuntimeClass_Windows_UI_Xaml_Media_AcrylicBrush).Get());
			//HR(factory->CreateInstance(baseInterface.Get(), &innerInterface, &brush));
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

int APIENTRY wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HR(RoInitialize(RO_INIT_SINGLETHREADED));
	ComPtr<IDispatcherQueueController> pQueueController{CreateDispatcherQueueController()};

	MSG msg = {};
	AllocateWindow(hInstance);

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ComPtr<IAsyncAction> pAsyncAction;
	pQueueController->ShutdownQueueAsync(&pAsyncAction);
	RoUninitialize();
	return (int) msg.wParam;
}