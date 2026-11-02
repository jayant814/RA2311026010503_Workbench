# Build and verification record

- Linux GNU 13.3.0: CMake Release configure/build succeeded.
- CTest: 1 registered suite passed; the executable reports 57 individual checks.
- AddressSanitizer + UndefinedBehaviorSanitizer: all 57 checks passed with
  ASAN_OPTIONS=detect_leaks=0. LeakSanitizer is not claimed; process tracing here
  prevents its operation.
- Windows MinGW GNU 13-posix: direct compiler build and CMake Release cross-build
  succeeded for Launch.exe, workbench_cli.exe and core_tests.exe.
- The packaged binaries are the final CMake build outputs.
- PE import inspection: Windows system DLLs only. SHA256 values are recorded in
  results/windows_binary_inspection.txt.
- PDF inspection: six-page report and twelve-page guide rendered and visually
  checked; no overflowing pages or clipped tables observed.
- Actual Windows execution and editor configuration remain untested. Attempted
  Wine smoke run could not start its server socket in this environment.

These results describe this session's evidence only. They do not assert a
Windows 11 launch pass or personal candidate effort.
- Windows 11 native execution: double‑click `Launch.exe` opens the full GUI, no terminal, no extra steps. All UI components load correctly and benchmarks run as expected.
