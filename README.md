# ThermoBridge: Unified Engineering Workbench

An offline C++17 assessment prototype for PASS Science Works. The best-fit idea is a
small, verifiable engineering workbench, not a new research project. Geometry,
thermal FEA, multicore experiments, embedded mocks and handwritten learning all
belong to one workflow. No commercial solver, physical hardware or online service
is used by the application.

## Start here

1. Extract the complete ZIP to a writable folder on **Windows 11 x64**.
2. Double-click **Launch.exe**. No first-run build, installation, terminal,
   administrator permission or internet connection is required by the app.
3. Geometry > STL sample / CSV sample. Drag to rotate, right-drag to pan,
   scroll to zoom, Fit to view to reset. Click near a node to inspect it.
4. Thermal Analysis > Solve. Defaults match the assignment. Export CSV includes
   both nodes and interpolated element midpoints. Convergence uses the four
   prescribed meshes and offers CSV export.
5. Performance > Run benchmark. Cases and mesh size are configurable. Switch
   views during a run; Cancel retains the previous complete result.
6. Hardware > choose each mock > Connect > Read telemetry. Inject timeout,
   Read telemetry (failure), Read telemetry again (recovery), Disconnect.
7. Backpropagation > Gradient check; inspect the loss curve and export training.

**Submission status:** implemented prototype with genuine automated test and
benchmark evidence. The candidate must complete the handwritten appendix, real
Windows 11 clean-machine verification, personal effort/ownership confirmation,
and candidate ID. Do not describe Linux or Wine checks as Windows 11 validation.
See docs/Submission_Checklist.md and the six-page report for remaining gates.

## Prerequisites and packaging

Running: Windows 11 x64, writable extracted folder, standard desktop display.
Recommended display: at least 1180 x 850 application pixels. High-DPI/small-display
accessibility is a known limitation. The precompiled x64 executable statically
links MinGW C++ and thread runtimes; its dynamic imports are Windows system DLLs.
The package includes their applicable runtime notices in licenses/. No Qt,
WebView, browser, Python, Node, GPU driver add-on or editor is needed to run it.
The unsigned executable may be treated differently by local security policies;
no code signing or security-policy bypass is provided.

In VS Code on Windows, open this folder, install Microsoft C/C++ extension, select
**ThermoBridge packaged app (Windows)** in Run and Debug, and press the Run button
(or Ctrl+F5). This launches the same Launch.exe without rebuilding. A built-in
Tasks alternative is **Run packaged ThermoBridge**; it needs no extension.
The standalone app has no editor dependency. This configuration still needs
candidate testing in the actual Windows editor.

## Architecture and source map

- src/core/thermal.cpp: parameter validation, element assembly, boundary
  elimination, tridiagonal solution, analytical and midpoint verification.
- src/core/import.cpp: bounded file IO, ASCII STL and strict nodal CSV parsing.
- src/core/experiments.cpp: seeded batch generation, independent bounded worker
  execution, timing, explicit backpropagation, evidence export, machine metadata.
- src/core/hardware.cpp: discovery and three simulated device state machines.
- src/ui/winmain.cpp: native Win32 controls, file dialogs, GDI presentation,
  camera projection, background-job lifecycle; no numerical solver in the view.
- src/cli.cpp: reproducible C++ evidence command; not required for normal use.
- tests/core_tests.cpp: positive, malformed-input, numerical and concurrency tests.
- data/: self-created geometry, synthetic temperature nodes and negative fixture.
- results/linux_run/: actual C++ computation outputs and host information.
- results/plots/: labelled plots made from those outputs for the report.
- docs/: report, detailed guide, derivation reference, design and submission checks.

UI and core share an in-process C++ API; there is no HTTP transport. Rendering is
wireframe orthographic 3D with camera rotation. STL and nodal data are independent;
this is not a general CAD modeller or a 3D thermal solid solver.

## Source builds (separate from packaged execution)

Install CMake >= 3.16 and a C++17 compiler only if rebuilding. Windows options:
MinGW-w64 POSIX threading compiler, or Visual Studio 2022 C++ Build Tools.

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For a single-configuration MinGW build, copy build/Launch.exe to the package root.
For a Visual Studio build, copy build/Release/Launch.exe and satisfy the MSVC
runtime requirements of that new build. That differs from the supplied static
MinGW binary. CMake does not itself install or package MSVC redistributables.

Linux core tests use the same commands; the GUI target is Windows-only. Linux
cross-build with MinGW POSIX installed:

```text
cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-win --parallel 4
```

Reproduce evidence in a **new** directory (the safe writer will not overwrite
existing files on Windows):

```text
workbench_cli.exe --evidence results/my_windows_run 1000 4000
```

On Linux use build/workbench_cli. Benchmark timing excludes input generation,
verification, disk IO, rendering and compilation; includes allocations, thread
creation and joining. It uses one untimed warm-up per worker count and five
measured repetitions. Comparison with the serial reference is outside timing.
Both paths use identical assembly and solution operations, and preserve input
order. Cases: mt19937 seed 2027, k in [5,25], q in [1000,20000], both positive.

## Supported input formats and tested limits

ASCII STL only: `solid`, complete facet-normal/outer-loop/three-vertex/endloop/
endfacet blocks and `endsolid`. Binary STL, zero-area triangles, trailing data,
missing tokens and non-finite coordinates are rejected. Normals are accepted
but not used by the wireframe renderer. Solid watertightness is not checked.
STL is unitless: this prototype explicitly assumes coordinates in meters.

CSV: UTF-8 without BOM, exact header
`node_id,x_m,y_m,z_m,temperature_C`; exactly five unquoted comma-separated fields.
Decimal point is '.', node IDs are unique nonnegative integers, coordinates are
meters and temperatures Celsius. Blank rows, duplicate IDs, extra fields,
non-numeric tokens, NaN/Inf and magnitudes over 1e12 are rejected. Quoted fields,
locale commas and native ANSYS binary/project files are unsupported. Real ANSYS
exports must be normalized to this schema; no claim of a live ANSYS connection.

Limits: 8 MiB/file, 20,000 STL triangles, 50,000 CSV nodes. See test log for the
exact boundary fixtures exercised. These are prototype safety caps, not a claim
of smooth interaction at every cap on every GPU/CPU. Single rod: 2..200,000
uniform elements. Batch: 1..20,000 cases, 2..20,000 elements/case, at most 20 million
nodal values. Concurrent reference/results and assembly require extra memory;
allow roughly 0.5 GB for the maximum batch. See validation code for physical
parameter bounds. UI operations on imported data may slow down near limits.

## Numerical contract

-k*A*T''=q*A. Linear element K=(k*A/h)[[1,-1],[-1,1]], f=(q*A*h/2)[1,1].
Prescribed columns are moved to the RHS before endpoint rows are replaced.
The tridiagonal Thomas algorithm costs O(N) memory and time. Endpoint values are
exactly prescribed. Verification uses FE midpoint interpolation, not a rerun of
the analytical formula. Default midpoint errors: 1.25, 0.3125, 0.078125,
0.01953125 C for 10,20,40,80 elements; ratio 4, observed order 2.

Backpropagation uses explicit chain-rule gradients with central differences
(epsilon 1e-6). CSV step 0 is initial parameters, step 1 is the required manual
update, steps 2..101 are the 100 subsequent updates. Loss is for one sample only;
convergence is not evidence of machine-learning generalization.

## Reliability and known limitations

The worker owns temporary outputs and publishes only a completed result. Cancel
is checked between batch cases; a single solve/import finishes safely and its
result is discarded if cancellation was requested. Closing joins the worker.
Only one background job runs at once. A failed import leaves previous data intact.
Export uses a sibling temporary file then rename. Choose a new export name if
an existing destination is rejected. This is not a transaction-safe database.

No real hardware driver, 3D FEA, contact analysis, GPU solver, native CAD solid
import or ANSYS result database import. No unverified speedup promises. Host
physical CPU topology inside a VM is not the underlying host's true topology.
The ready-time log measures entry-to-first-ready-timer, including a 200 ms delay;
it excludes OS loader time, so a real double-click-to-ready measurement remains
necessary on Windows. Display and editor behavior require target-machine checks.

## Honesty, licences and ownership

AI-assisted implementation prepared in this session. Development commits are
real sequential milestones from this session, not a fabricated 15-day history.
See docs/AI_USE_LOG.md. The student must review and understand the code, make and
record their own changes, and sign only an accurate ownership statement.
Original project source is supplied under MIT (LICENSE). Synthetic examples are
CC0; provenance is in data/PROVENANCE.md. No employer code or personal data is
bundled. GCC/MinGW runtime terms are preserved in licenses/.

## Version-control evidence

`development.bundle` contains the genuine development commits from this session.
To inspect independently: `git clone development.bundle review-history`.
`COMMIT_LOG.txt` is a readable log. The bundle excludes packaged executables and
build folders intentionally; source, tests, reports and evidence are tracked.
After your own edits, commit them and recreate the bundle using
`git bundle create development.bundle --all` from your working repository.
