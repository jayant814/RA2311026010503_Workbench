# Versions, dependencies and licences

No downloaded frontend framework, CAD library or solver code is needed. The app
uses C++17 standard library, Win32/GDI/common dialogs and the Windows system CRT.

| Item | Version in this build | Purpose | Licence / distribution |
|---|---|---|---|
| GNU C++ (Linux) | 13.3.0 | Core tests and reference benchmarks | Build tool; not shipped |
| MinGW GCC POSIX x64 | 13.2.0 Ubuntu package, reports 13-posix | Windows cross-build; static C++/thread runtime | GCC GPL with Runtime Library Exception; MinGW runtime notices in licenses/ |
| MinGW-w64 headers/runtime | 11.0.1 | Windows API bindings/runtime | Multiple permissive/public-domain terms; complete package copyright preserved |
| CMake | 3.28.3 used; minimum 3.16 | Source build | BSD-3-Clause; not shipped |
| clang-format | 18.1.3 | Source formatting | Build/editor tool; not shipped |
| Windows SDK/system APIs | OS-provided | Native GUI and machine metadata | Not redistributed |
| Python + ReportLab + Matplotlib | Document-generation environment only | Prepare reports/plots from C++ CSV outputs | No Python runtime or application backend feature is shipped |

Windows binary command: g++ -std=c++17 -O3 -DNDEBUG -municode -mwindows -static
-static-libgcc -static-libstdc++ -DUNICODE -D_UNICODE -DNOMINMAX, then all core
sources plus winmain.cpp, and comdlg32/gdi32/user32/shell32/advapi32. This matches
the supplied CMake Release target's relevant settings. Source paths are relative.
The app makes no outbound connections, handles no credentials, and uses no keys.

Runtime imports are recorded in results/windows_binary_inspection.txt. A static
runtime link does not eliminate system DLLs, and an unsigned executable does not
prove clean-machine acceptance. Verify on the target system before submission.
