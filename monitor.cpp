#include <windows.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <sstream>
#include <ctime>
#include <locale>
#include <codecvt>
#include <thread>

class FileSystemMonitor {
private:
    std::wstring rootPath;
    std::ofstream logFile;
    HWND hwnd;
    HANDLE directoryHandle;
    bool running;
    static FileSystemMonitor* instance;
    std::thread watcherThread;

    std::string wstring_to_string(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::string timestamp = std::ctime(&time);
        timestamp.pop_back();
        return timestamp;
    }

    void logMessage(const std::string& message) {
        if (logFile.is_open()) {
            logFile << "[" << getCurrentTimestamp() << "] " << message << std::endl;
        }
    }

    void watchDirectory() {
        char buffer[4096];
        DWORD bytesReturned;
        FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;
        
        while (running) {
            if (ReadDirectoryChangesW(
                directoryHandle,
                buffer,
                sizeof(buffer),
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME |
                FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_ATTRIBUTES |
                FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE |
                FILE_NOTIFY_CHANGE_SECURITY,
                &bytesReturned,
                nullptr,
                nullptr)) {
                
                do {
                    std::wstring filename(event->FileName, event->FileNameLength / sizeof(WCHAR));
                    std::string action;
                    
                    switch (event->Action) {
                        case FILE_ACTION_ADDED:
                            action = "File Created";
                            break;
                        case FILE_ACTION_REMOVED:
                            action = "File Deleted";
                            break;
                        case FILE_ACTION_MODIFIED:
                            action = "File Modified";
                            break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                            action = "File Renamed (Old Name)";
                            break;
                        case FILE_ACTION_RENAMED_NEW_NAME:
                            action = "File Renamed (New Name)";
                            break;
                    }
                    
                    logMessage(action + ": " + wstring_to_string(filename));
                    
                    if (event->NextEntryOffset == 0) break;
                    event = (FILE_NOTIFY_INFORMATION*)((BYTE*)event + event->NextEntryOffset);
                } while (true);
            }
        }
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (uMsg == WM_CLIPBOARDUPDATE && instance) {
            instance->handleClipboardUpdate();
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    void handleClipboardUpdate() {
        if (!IsClipboardFormatAvailable(CF_UNICODETEXT) && 
            !IsClipboardFormatAvailable(CF_TEXT) &&
            !IsClipboardFormatAvailable(CF_HDROP)) {
            return;
        }

        if (!OpenClipboard(nullptr)) {
            logMessage("Failed to open clipboard");
            return;
        }

        if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData != nullptr) {
                wchar_t* text = static_cast<wchar_t*>(GlobalLock(hData));
                if (text != nullptr) {
                    logMessage("Clipboard Text: " + wstring_to_string(std::wstring(text)));
                    GlobalUnlock(hData);
                }
            }
        }

        if (IsClipboardFormatAvailable(CF_HDROP)) {
            HANDLE hDrop = GetClipboardData(CF_HDROP);
            if (hDrop != nullptr) {
                HDROP hdrop = static_cast<HDROP>(GlobalLock(hDrop));
                if (hdrop != nullptr) {
                    UINT fileCount = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
                    for (UINT i = 0; i < fileCount; i++) {
                        wchar_t filePath[MAX_PATH];
                        DragQueryFileW(hdrop, i, filePath, MAX_PATH);  
                        std::wstring path(filePath);
                        
                        if (path.find(rootPath) == 0) {
                            logMessage("Attempted file operation from protected path: " + 
                                     wstring_to_string(path));
                            EmptyClipboard();
                            break;
                        }
                    }
                    GlobalUnlock(hDrop);
                }
            }
        }
        CloseClipboard();
    }

public:
    FileSystemMonitor(const std::wstring& root) : rootPath(root), running(true) {
        instance = this;
        logFile.open("file_monitor.log", std::ios::app);

        directoryHandle = CreateFileW(
            rootPath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );

        if (directoryHandle != INVALID_HANDLE_VALUE) {
            watcherThread = std::thread(&FileSystemMonitor::watchDirectory, this);
        }
        
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"FileMonitorWindow";
        
        RegisterClassExW(&wc);
        
        hwnd = CreateWindowExW(
            0,
            L"FileMonitorWindow",
            L"FileMonitor",
            0,
            0, 0, 0, 0,
            HWND_MESSAGE,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );
        if (hwnd) {
            AddClipboardFormatListener(hwnd);
        }
        
        logMessage("File System Monitor initialized for path: " + wstring_to_string(rootPath));
    }

    ~FileSystemMonitor() {
        running = false;
        if (watcherThread.joinable()) {
            watcherThread.join();
        }
        if (directoryHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(directoryHandle);
        }
        if (hwnd) {
            RemoveClipboardFormatListener(hwnd);
            DestroyWindow(hwnd);
        }
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void start() {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

FileSystemMonitor* FileSystemMonitor::instance = nullptr;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::wstring rootPath = L"C:\\Path\\To\\Your\\Protected\\Folder";
    FileSystemMonitor monitor(rootPath);
    monitor.start();
    return 0;
}
