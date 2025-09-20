/*=================================================================================
 *
 * swl - Simple Window Library is a header only library designed to make creating
 *       and handling events on Windows easy using the Win32 API.
 *
 * LICENSE : MIT License
 *
 * Copyright (c) 2025 aelf0x800
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *===============================================================================*/

#pragma once

#include <exception>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>

namespace SWL
{
    /*=========================================================================
     * ApplicationException definition
     *=========================================================================*/
    class ApplicationException : std::exception
    {
    private:
        std::wstring m_info{};

    public:
        ApplicationException(LPCWSTR lpInfo);

        void ShowMessageBox();
        void ShowDebugOutput();
    };

    /*=========================================================================
     * Application definition
     *=========================================================================*/
    template<class DerivedType>
    class Application
    {
    protected:
        HINSTANCE m_hInstance;
        HWND m_hWnd;

    public:
        Application(PCWSTR lpWindowName,
            int nWidth = CW_USEDEFAULT,
            int nHeight = CW_USEDEFAULT,
            int x = CW_USEDEFAULT,
            int y = CW_USEDEFAULT,
            DWORD dwStyle = WS_OVERLAPPEDWINDOW,
            DWORD dwExStyle = WS_EX_COMPOSITED);

        static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // Message polling/waiting functions
        void WaitMessage();
        void PollMessage();

    protected:
        // Message handling functions to be overrided
        virtual void OnPaint(HDC hDC, PAINTSTRUCT ps) {}
        virtual void OnKeyDown(ULONGLONG ulKey) {}
        virtual void OnKeyUp(ULONGLONG ulKey) {}
        virtual void OnMouseButtonDown(UINT uButton) {}
        virtual void OnMouseButtonUp(UINT uButton) {}
        virtual void OnMouseMove(int x, int y) {}
        virtual void OnClose() {}
        virtual BOOL HandleOtherMessages(UINT uMsg) { return FALSE; }

    };
}

#ifdef SWL_IMPLEMENTATION
namespace SWL
{
    /*=========================================================================
     * ApplicationException implementation
     *=========================================================================*/
    ApplicationException::ApplicationException(LPCWSTR lpInfo)
    {
        m_info = {};
        m_info += lpInfo;
        m_info += L" | Error code : ";
        m_info += std::to_wstring(GetLastError());
    }

    void ApplicationException::ShowMessageBox() { MessageBoxW(NULL, m_info.c_str(), NULL, MB_ICONERROR); }

    void ApplicationException::ShowDebugOutput() { OutputDebugStringW(m_info.c_str()); }

    /*=========================================================================
     * Application implementation
     *=========================================================================*/
    template<class DerivedType>
    Application<DerivedType>::Application(PCWSTR lpWindowName, int nWidth, int nHeight, int x, int y,
        DWORD dwStyle, DWORD dwExStyle)
    {
        m_hInstance = GetModuleHandleW(NULL);

        WNDCLASS wndClass = {};
        wndClass.lpfnWndProc = DerivedType::WndProc;
        wndClass.hInstance = m_hInstance;
        wndClass.lpszClassName = lpWindowName;
        if (!RegisterClassW(&wndClass))
            throw ApplicationException(L"Failed to register the window class (RegisterClassW)");


        m_hWnd = CreateWindowExW(dwExStyle, lpWindowName, lpWindowName, dwStyle, x, y,
            nWidth, nHeight, NULL, NULL, m_hInstance, this);
        if (m_hWnd == nullptr)
            throw ApplicationException(L"Failed to create a window (CreateWindowEx)");

        ShowWindow(m_hWnd, SW_SHOW);
    }

    template<class DerivedType>
    LRESULT CALLBACK Application<DerivedType>::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        DerivedType* pDerivedType = NULL;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pDerivedType = (DerivedType*)pCreate->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pDerivedType);

            pDerivedType->m_hWnd = hWnd;
        }
        else
        {
            pDerivedType = (DerivedType*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        }

        if (pDerivedType)
        {
            switch (uMsg)
            {
            // Painting handling
            case WM_PAINT:
            {
                PAINTSTRUCT ps = {};
                HDC hDC = BeginPaint(hWnd, &ps);
                pDerivedType->OnPaint(hDC, ps);
                EndPaint(hWnd, &ps);
            }
            return TRUE;

            // Keyboard handling
            case WM_KEYDOWN: pDerivedType->OnKeyDown(wParam); return TRUE;
            case WM_KEYUP: pDerivedType->OnKeyUp(wParam); return TRUE;

            // Mouse handling
            case WM_LBUTTONDOWN: pDerivedType->OnMouseButtonDown(VK_LBUTTON); return TRUE;
            case WM_MBUTTONDOWN: pDerivedType->OnMouseButtonDown(VK_MBUTTON); return TRUE;
            case WM_RBUTTONDOWN: pDerivedType->OnMouseButtonDown(VK_RBUTTON); return TRUE;
            case WM_LBUTTONUP: pDerivedType->OnMouseButtonUp(VK_LBUTTON); return TRUE;
            case WM_MBUTTONUP: pDerivedType->OnMouseButtonUp(VK_MBUTTON); return TRUE;
            case WM_RBUTTONUP: pDerivedType->OnMouseButtonUp(VK_RBUTTON); return TRUE;
            case WM_MOUSEMOVE:
            {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                pDerivedType->OnMouseMove(x, y);
            }
            return TRUE;

            // Close handling
            case WM_CLOSE:
            {
                pDerivedType->OnClose();
                PostQuitMessage(0);
            }
            return TRUE;

            // Handle other messages that are not handled by SWL
            default:
                if (pDerivedType->HandleOtherMessages(uMsg))
                    return TRUE;
            }
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    template<class DerivedType>
    void Application<DerivedType>::WaitMessage()
    {
        MSG msg = {};
        if (GetMessageW(&msg, NULL, 0, 0) == -1)
            throw ApplicationException(L"Failed to get a message (GetMessageW)");
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    template<class DerivedType>
    void Application<DerivedType>::PollMessage()
    {
        MSG msg = {};
        PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE);
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
#endif