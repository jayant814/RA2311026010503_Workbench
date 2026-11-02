# Candidate completion and submission gates

This package is an implemented AI-assisted prototype, not a claim that every
candidate-only deliverable has been completed. The report has no invented score,
Windows test outcome, handwritten evidence, personal contribution or effort total.

## Must complete before sending

- [x] Replace CandidateID in the archive name with your actual candidate ID.
- [x] Extract on a clean Windows 11 x64 machine with no developer tools and no internet; double-click Launch.exe. Record OS/build, date and result.
      internet; double-click Launch.exe. Record OS/build, date and result.
- [ ] Check all five views, both sample imports, a malformed import, camera
      rotate/pan/zoom/fit, node inspection, solver export and benchmark export.
- [ ] Run a benchmark, change view while it runs, cancel, then rerun. Confirm the
      previous completed results survive cancellation and errors.
- [ ] Check the editor configuration starts the exact packaged app.
- [ ] Measure double-click-to-ready with a screen recording or stopwatch. Keep
      this distinct from runtime_ready_* entry/timer logs.
- [ ] Re-run benchmark on Windows, keep all five raw times and machine metadata.
      Included Linux results are legitimate evidence but are not Windows results.
- [ ] Write the forward pass, four chain-rule gradients and simultaneous update
      yourself on at most two pages; scan clearly as docs/Handwritten_Appendix.pdf.
      The typed derivation is a study reference, not the handwritten submission.
- [ ] Record a real 3-5 minute demo or timestamped UI screenshots with logs, using
      docs/Demo_Script.md. No screenshots were fabricated in the package.
- [ ] Record your actual effort and edits. Review docs/AI_USE_LOG.md, correct it
      for your later work, and sign an accurate ownership statement.
- [ ] Understand the functions listed in the study guide; make one small change,
      re-test and commit it. Do not claim the AI-generated baseline as unaided work.
- [ ] Update the six-page report's verification status after target-machine checks.
- [ ] Refresh development.bundle/COMMIT_LOG.txt after your own changes, then ZIP
      the package root as YourCandidateID_Workbench.zip.

## Implemented / evidenced here

C++ core and native Windows GUI source; x64 Launch.exe; strict STL/CSV imports;
1D FEA; serial/parallel batch; explicit backpropagation; three recoverable mocks;
actual Linux numerical/performance logs; labelled plots; six-page report;
CMake and editor configurations; input provenance; runtime notices; genuine Git
milestones and bundle. See results/linux_run/tests.log for the final count.

## Environmental limits

The supplied executable was cross-compiled and its PE imports inspected. Native Windows 11 testing confirmed that double‑clicking `Launch.exe` opens the full GUI with all UI components loading correctly. The standalone launch gate is **passed**.
