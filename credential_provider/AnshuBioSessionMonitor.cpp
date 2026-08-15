/**
 * AnshuBio Unlock - Native Win32 Session & Power Monitor Helper
 * Runs lightweight Win32 message loop with WTSRegisterSessionNotification
 * and WM_POWERBROADCAST for ultra-low latency OS session state tracking.
 * Publisher: AnshuCore
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <signal.h>

#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "user32.lib")

static HWND g_hWnd = NULL;
static volatile BOOL g_bRunning = TRUE;

static void SignalHandler(int sig)
{
    g_bRunning = FALSE;
    if (g_hWnd)
    {
        PostMessage(g_hWnd, WM_CLOSE, 0, 0);
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        if (WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION))
        {
            printf("WTS_READY\n");
            fflush(stdout);
        }
        else
        {
            fprintf(stderr, "WTSRegisterSessionNotification failed: %lu\n", GetLastError());
            fflush(stderr);
        }
        return 0;

    case WM_WTSSESSION_CHANGE:
        switch (wParam)
        {
        case WTS_SESSION_LOCK:
            printf("EVENT:WTS_SESSION_LOCK\n");
            fflush(stdout);
            break;
        case WTS_SESSION_UNLOCK:
            printf("EVENT:WTS_SESSION_UNLOCK\n");
            fflush(stdout);
            break;
        case WTS_SESSION_LOGON:
            printf("EVENT:WTS_SESSION_LOGON\n");
            fflush(stdout);
            break;
        case WTS_SESSION_LOGOFF:
            printf("EVENT:WTS_SESSION_LOGOFF\n");
            fflush(stdout);
            break;
        case WTS_REMOTE_CONNECT:
        case WTS_REMOTE_DISCONNECT:
            printf("EVENT:WTS_SESSION_CHANGE\n");
            fflush(stdout);
            break;
        }
        return 0;

    case WM_POWERBROADCAST:
        if (wParam == PBT_APMSUSPEND)
        {
            printf("EVENT:PBT_APMSUSPEND\n");
            fflush(stdout);
        }
        else if (wParam == PBT_APMRESUMESUSPEND || wParam == PBT_APMRESUMEAUTOMATIC)
        {
            printf("EVENT:PBT_APMRESUMESUSPEND\n");
            fflush(stdout);
        }
        return 0;

    case WM_DESTROY:
        WTSUnRegisterSessionNotification(hWnd);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int main()
{
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"AnshuBioSessionMonitorClass";

    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"AnshuBioSessionMonitorWindow",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE, // Message-only window
        NULL,
        wc.hInstance,
        NULL
    );

    if (!g_hWnd)
    {
        fprintf(stderr, "CreateWindowEx failed: %lu\n", GetLastError());
        fflush(stderr);
        return 1;
    }

    MSG msg;
    while (g_bRunning && GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
