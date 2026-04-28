#include <Windows.h>

#include "VulkanUtils.h"

LRESULT CALLBACK VulkanWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_SIZE:
		RECT rect;
		GetClientRect(hWnd, &rect);
		break;

	case WM_CLOSE:
		PostQuitMessage(0); // 发送WM_QUIT消息，通知消息循环退出
		break;
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_  HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	WNDCLASSEX wndClass;
	memset(&wndClass, 0, sizeof(WNDCLASSEX));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = NULL;
	wndClass.hIconSm = NULL;
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = VulkanWndProc;
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = L"VulkanWindow";
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	ATOM atom = RegisterClassEx(&wndClass);
	RECT rect = { 0,0,1280,720 }; // 客户区大小
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	HWND hwnd = CreateWindowEx(
		NULL, L"VulkanWindow", L"Vulkan Render Window", WS_OVERLAPPEDWINDOW,
		200, 200, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL
	);
	InitVulkan(hwnd, 1280, 720);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	MSG msg;
	while (true) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return 0;
}