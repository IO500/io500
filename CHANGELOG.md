# Changelog

## SC-26

- Update `ior-rnd4K-easy-read` to use IOR's LFSR random offset method (`-z -z -z`, `-s=-1`) and rename its source file to `phase_ior_rnd4k_easy_read.c`.
- Remove deprecated experimental random IOR phases (`ior-rnd4K` and `ior-rnd1MB` read/write) and consolidate phase configuration.
- Add `--list-mandatory` (`-lm`) CLI option to output only mandatory INI configuration sections, and mark parent wrapper phases as optional.
- Refactor the CLI help menu, add `--verbose=<level>` alongside repeatable `-v`, and overhaul `README.md` build, configuration, and verification instructions.
- Allow customizing `IO500_EXTRA_CFLAGS`, `IO500_EXTRA_LDFLAGS`, and `IOR_EXTRA_CONFIGURE_FLAGS` via `prepare.sh` variables or environment variables.
- Fix verifier (`--verify` and `io500-verify`) score hash mismatches by excluding optional and unscored phases from the score hash calculation.
- Restrict `Makefile` `git describe` version detection to `io500-*_vN` tags so binaries always report the exact versioned release tag.
- Add GitHub Actions CI workflow to build, execute, and verify `io500`.
- Update `IOR` and `pfind` versions, fixing `ior-rnd4K-easy-read` stonewall bandwidth calculation, coalesced MPI-IO stonewall truncation, read-only file opens during `mdtest` read phases, `autoconf` 2.69 builds on RHEL/Rocky/Alma 9, and `pfind` `printf` issues.

## ISC-26

- Update `ior-rnd4K-easy-read` to support shared `ior-easy` files, shift reads one additional node away to avoid client caching, and refine random read task assignment flags (removing `-Q`).
- Print the actual configured `stonewall-time` value in validation messages when it differs from 300 seconds.
- Remove deprecated `system-information.txt` generation instructions and warnings from `io500.sh`.
- Fix missing `mdtest_easy_write_get_result()` function prototype and compiler warnings in `mdworkbench`.
- Update `IOR` and `pfind` versions in `prepare.sh`.

## SC-25

- Add `config-sample-all.ini` full sample configuration file and clarify comments in `config-scc.ini`.
- Update `IOR` version in `prepare.sh`.

## ISC-25

- Add unscored `ior-rnd4K-easy-read` pilot phase performing 4 KiB random reads against data written during `ior-easy-write`, including segment count and stonewall wear-out fixes.
- Downgrade unrecognized backend API option prefix checks from a fatal error to a warning while forwarding the option.
- Improve `prepare.sh` to skip re-downloading `ior`, `pfind`, and `cdcl-schema-tools` during offline rebuilds, fix `build/` directory paths, and handle systems without `nproc` (`NPROC` override).
- Update `IOR` (including 64-bit random fix) and `pfind` versions, and update `.github/CODEOWNERS` to `@io500/board`.

## SC-24

- Update `IOR` version in `prepare.sh` to incorporate upstream fixes and improvements.

## ISC-24

- Adapt storage backend cleanup to upstream AIORI interface rename (`aiori->delete` to `aiori->remove`).
- Update `IOR` version in `prepare.sh`, including `strict-prototypes` compilation fixes.

## SC-23

- No benchmark code changes since `ISC-23` (`io500-sc23` points to the same release commit as `io500-isc23`).

## ISC-23

- Set the global MPI rank variable before initializing AIORI storage backend drivers in `main`.
- Update `pfind` version in `prepare.sh` with upstream bug fixes.

## SC-22

- Add `pause-dir` debugging option to pause execution before individual phases (e.g., for testing tiered storage), and mark runs with `pause-dir` enabled as invalid for official submission.
- Add S3 benchmarking guide, sample configuration, and `prepare-s3.sh` script under `contrib/s3/`.
- Replace hardcoded path buffer sizes with `PATH_MAX`.
- Update `IOR` to latest upstream `main`, including IBM Spectrum Scale performance optimizations and minor fixes.

## ISC-22

- Add `config-scc.ini` configuration template for the Student Cluster Competition (first tagged in `io500-sc21-scc`).
- Include the number of MPI processes (`procs`) in benchmark run output for easier debugging.
- Update `IOR` version in `prepare.sh`.

## SC-21

- Split optional random I/O phases into separate 4 KiB (`ior-rnd4K`) and 1 MiB (`ior-rnd1MB`) read/write workloads.
- Add `dataPacketType` option across `IOR`, `mdtest`, and `mdworkbench` phases, including experimental random data packet support.
- Add `run` toggle parameter to all individual `IOR` sub-phases and compute scores even when skipped phases report `0`.
- Support `mdtest-hard` `files-per-dir` (`-I`) for debugging or committee-approved runs, and mark runs invalid by default when `files-per-dir` is set or `stonewall-time` is below the required minimum.
- Expand INI option descriptions for `IOR` and `mdtest`, and fix `--mode=extended --list` when an INI file is provided.
- Update `IOR` version in `prepare.sh`.

## ISC-21

- Add `--mode=standard|extended` execution modes, marking extended phases (`ior-rnd`, `mdworkbench`, `find-easy`, `find-hard`) as optional and validating/scoring extended runs separately.
- Integrate optional random I/O (`ior-rnd`), `mdworkbench` metadata, and `find-easy`/`find-hard` workloads, ordering random I/O after `ior-easy` to reduce write-caching effects.
- Save per-rank performance CSV files for `IOR`, `mdtest`, and `pfind`, and format `result.txt` as a valid INI file.
- Seed `IOR` data patterns using run timestamp/phase and support random buffer offsets in `mdtest` to prevent deduplication artifacts; add GPU buffer allocation support.
- Add `ior-hard` `collective` option and `mdtest` `files-per-dir` (`-I`) option; remove legacy `posix.odirect` and MPIIO `hintsFileName` options in favor of native `IOR` API flags.
- Rename phase `noRun` option to `run`, add `config-debug-run.ini`, move build artifacts into `./build`, and integrate `cdcl-schema-tools` into `prepare.sh`.
- Update score hash generation, `io500-verify`, cleanup phase handling, stonewall throughput calculation, and Lustre configuration logging.

## SC-20

- Remove legacy bash benchmark script (`io-500-dev`); submissions use the C application exclusively.
- Move non-POSIX control files (`<phase>.stonewall` and timestamp files) to `resultdir` and support global AIORI backend API parsing.
- Add Student Cluster Competition (`scc`) option and `find` phase disable option (`noRun`).
- Fix bandwidth score unit (`GiB/s` instead of `GB/s`), score cross-check validation, and empty INI option parsing; remove obsolete sample `.ini` files in favor of `--list`.
- Update `IOR` to the final `SC-20` version.

## ISC-20

- Run combined bash script and C application benchmarks via `io500.sh` to validate continuity of results.
- Support hashing of configuration options and obtained results, standalone verification (`--verify` and `io500-verify`), `--cleanup`, and `--dry-run` modes.
- Support generic AIORI storage API selection with backend options in INI configuration, and add `uniqueDir` option to `ior-easy`.
- Provide `pfind` options for parallelizing large directories and outputting load-imbalance diagnostics.
