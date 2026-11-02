# Four-minute candidate demonstration script

This is a recording plan, not evidence that a recording already occurred.
Use the actual packaged app on Windows 11, capture the clock/date and OS details,
and save the recording or screenshots alongside the generated logs.

| Time | Action and spoken explanation | Evidence to capture |
|---|---|---|
| 00:00-00:25 | Show extracted ZIP folder. Double-click Launch.exe. State that the standalone app needs no build or internet. | Launch duration, window visible, Windows version |
| 00:25-01:05 | Import STL sample, rotate/pan/zoom, Fit. Open synthetic CSV, inspect a coloured node and legend. Try invalid.csv. | Geometry, CSV legend, node details, actionable error; previous data retained |
| 01:05-01:50 | Solve default rod. Explain element assembly and endpoint elimination. Show midpoint errors at 10/20/40/80, ratio 4. Export CSV. | Curve, convergence CSV and one midpoint row |
| 01:50-02:35 | Run benchmark, switch views while running, inspect median/speedup/efficiency and zero agreement difference. | Five raw times, machine info; repeat with longer workload if too fast to observe |
| 02:35-03:10 | Connect each mock. Read telemetry, inject timeout, read (failure), retry (success), disconnect. | All three types and recoverable failure, SIMULATED label |
| 03:10-03:45 | Show handwritten derivation, gradient-check dialog and 101-update loss curve. Explain simultaneous updates. | Two handwritten pages, error below 1e-5, loss CSV |
| 03:45-04:00 | Show tests/history/AI disclosure, identify actual remaining limitations. | Test log, commit log, honest contributions |

For screenshot-equivalent evidence, name images 01_launch.png etc.; add wall-clock
timestamps and action/result captions in Walkthrough.md, plus matching exported
CSVs/logs. Never present a planned timestamp as the time of an actual action.
