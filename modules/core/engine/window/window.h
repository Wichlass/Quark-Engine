#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#include <functional>
#include <string>

#include "../../../headeronly/globaltypes.h"

class Window
{
public:
	Window();
	~Window();

#ifdef _WIN32
	using fnEventCallback = std::function<void(UINT msg, WPARAM wParam, LPARAM lParam)>;
	using fnCloseEventCallback = std::function<void()>;

	bool windowInit(
		int width,
		int height,
		const std::string& title,
		bool fullscreen = false,
		bool borderless = false
	);

	void windowShutdown();
	void pollMessages();
	qWndh getWindowHandler() const { return static_cast<qWndh>(hwnd); }
	qWndInstance getWindowInstance() const { return static_cast<qWndInstance>(hInstance); }
	void setWindowTitle(const std::string& newTitle);
	void setWindowSize(int width, int height);
	int getWindowWidth() const { return width; }
	int getWindowHeight() const { return height; }
#endif

	void setWindowEventCallback(const fnEventCallback& callback) { eventCallback = callback; }
	void setWindowCloseEventCallback(const fnCloseEventCallback& _closeEventCallback) { closeEventCallback = _closeEventCallback; }

private:
#ifdef _WIN32
	HWND hwnd;
	HINSTANCE hInstance;
	int width, height;
	bool fullscreen;
	bool borderless;
	std::string title;

	static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	std::wstring UTF8ToWide(const std::string& str);
#endif

	fnEventCallback eventCallback;
	fnCloseEventCallback closeEventCallback;
};