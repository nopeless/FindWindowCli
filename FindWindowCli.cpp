#include <Windows.h>
#include <iostream>
#include <string>

struct EnumWindowsData {
    const char* classNameToFind;
    int skip = 0;
    // This one is kinda not used but we are passing by reference anyway
    // so its fine
    HWND parentHWnd = NULL;
    /*
    Result
    */
    HWND hWnd = NULL;
};

BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);

    // Recursive call
    EnumChildWindows(hwnd, EnumChildWindowsProc, lParam);

    if (data->hWnd) {
        return FALSE;
    }

    char className[256];
    if (!GetClassNameA(hwnd, className, sizeof(className))) return TRUE;

    std::cout << "Child window handle: " << hwnd << " " << className << std::endl;
    
    if (strcmp(className, data->classNameToFind)) return TRUE;

    if (data->skip > 0) {
        data->skip--;
        return TRUE;
    }

    data->hWnd = hwnd;

    return FALSE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);

    // only find windows that are children of parent (if set)
    if (data->parentHWnd && !IsChild(data->parentHWnd, hwnd)) {
        return TRUE;
    }

    // Check if child windows
    EnumChildWindows(hwnd, EnumChildWindowsProc, lParam);

    if (data->hWnd) {
        return FALSE;
    }

    char className[256];
    if (!GetClassNameA(hwnd, className, sizeof(className))) return TRUE;

    std::cout << "class name: " << hwnd << " " << className << std::endl;

    if (strcmp(className, data->classNameToFind)) return TRUE;

    if (data->skip > 0) {
        data->skip--;
        return TRUE;
    }

    data->hWnd = hwnd;

    return FALSE; // Stop enumeration
}

int main(int argc, char* argv[]) {
    const char* windowClass = nullptr;
    
    const char* className = nullptr;
    int cIdx = -1;
    int classNameIdx = 0;

    bool verbose = false;
    bool repeatMode = false;

    // most genius argument parsing I've done in my entire programming career
    int i = 1;
    for (; i < argc; i++) {
        //std::cout << "argv[" << i << "]:" << argv[i] << std::endl;

        if (argv[i][0] != '-') {
            cIdx = i;
            continue;
        }

        char* t = argv[i];

        for (int j = 1; t[j] != '\0'; j++) {
            //std::cout << "argv[" << i << "][" << j << "]:" << argv[i][j] << std::endl;
            switch (argv[i][j]) {
            case 'h':
                std::cout << "Usage: " << argv[0] << " " << "[-hvni] [<parent class>]"  << std::endl;
                std::cout << "  -h: Show this help message." << std::endl;
                std::cout << "  -v: Verbose mode." << std::endl;
                std::cout << "  -n <class name>: Find window with class name." << std::endl;
                std::cout << "  -i <index>: Find window with class name at index. (0 indexed)" << std::endl;
                std::cout << "  <parent class>: Window class name. Restricts the scope of class name" << std::endl;
                return 0;
            case 'v':
                verbose = true;
                continue;
            case 'n':
                if (i++ < argc) {
                    className = argv[i];
                    break;
                }
                else {
                    std::cout << "Missing class name after -n flag." << std::endl;
                    return 1;
                }
            case 'i':
                if (i++ < argc) {
                    classNameIdx = std::stoi(argv[i]);
                    break;
                }
                else {
                    std::cout << "Missing index after -i flag." << std::endl;
                    return 1;
                }
            case 'r':
                repeatMode = true;
                break;
            default:
                std::cout << "Unknown flag: " << argv[i][j] << std::endl;
                return 1;
            }
        }
    }

    if (cIdx < 0) {
        if (verbose) {
            std::cout << "No <class name> provided" << std::endl;
        }
    }
    else {
        windowClass = argv[cIdx];
    }

    if (!windowClass && !className) {
        std::cerr << "Must either provide window class or class name" << std::endl;
        return 1;
    }

    do {
        if (verbose)
        {
            if (windowClass) {
                std::cout << "finding window: " << windowClass << std::endl;
            }
            if (className) {
                std::cout << "finding class name: " << className << std::endl;
            }
        }

        HWND hWnd = nullptr;

        if (windowClass) {
            int cnl = MultiByteToWideChar(CP_UTF8, 0, windowClass, -1, NULL, 0);

            wchar_t* wccn = new wchar_t[cnl];

            if (!MultiByteToWideChar(CP_UTF8, 0, windowClass, -1, wccn, cnl)) {
                std::cerr << "MultiByteToWideChar failed." << std::endl;
                delete[] wccn;
                return 1;
            }
    
            hWnd = FindWindow(wccn, NULL);
        }

        if (className) {
            EnumWindowsData data{};
            if (windowClass) {
                data.parentHWnd = hWnd;
            }

            data.classNameToFind = className;
            data.skip = classNameIdx;

            if (verbose) {
                std::cout << "Parent window: " << hWnd << std::endl;
            }

            EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));

            hWnd = data.hWnd;
        }

        if (!hWnd) {
            std::cout << "handle:0" << std::endl;
            return 0;
        }

        std::cout << "handle:" << hWnd << "\n";

        RECT windowRect;
        if (!GetWindowRect(hWnd, &windowRect)) {
            std::cerr << "GetWindowRect failed." << std::endl;
            return 1;
        }

        int l = windowRect.left;
        int t = windowRect.top;
        int r = windowRect.right;
        int b = windowRect.bottom;

        int w = r - l;
        int h = b - t;

        const char* props[] = {
            "left", "top", "right", "bottom", "width", "height"
        };

        const int vals[] = {
            l, t, r, b, w, h
        };

        for (int i = 0; i < 6; i++) {
            std::cout << props[i] << ":" << vals[i] << "\n";
        }

        std::cout << std::flush;
    } while (repeatMode && std::cin.get() != 'q');

    return 0;
}