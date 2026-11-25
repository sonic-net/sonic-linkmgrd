# Native AMD64 Linkmgrd

The Bazel build compiles this repository's 39 production C++ translation units
and 14 original test translation units with the declared GCC 14.2/Trixie
toolchain. Source globs are checked against the existing Make manifests.
Production keeps C++17, `-O3`, LTO, pthreads and the existing Boost logging
defines; tests use `-O0`. Per-file `__FILENAME__` values retain the Make layout.

SWSS Common and the patched libnl libraries come from their source owners.
Boost 1.83 comes from the shared snapshot-pinned APT set through its native
shared-library interfaces. The source correction in `SwssLogBackend.cpp` passes
the already formatted message through a literal `%s`, as required by the
compiler's format-security check.

| Public target | Contract |
| --- | --- |
| `//:linkmgrd_core` | Native C++ library and owned headers |
| `//:linkmgrd` | Unstripped build executable |
| `//:sonic_linkmgrd_runtime` | `/usr/sbin/linkmgrd` and the compressed Debian changelog in a deterministic TAR |
| `//:sonic_linkmgrd_debug` | Matching Build-ID debug files, separate from runtime |
| `//:runtime_packages.json` | Package identity and actual runtime dependencies derived from the existing recipe and source outputs |

No target generates a Debian package. Existing Debian/Make packaging remains
available separately. The root `.bazelrc` uses the maintained registry and BCR.
`MODULE.bazel.lock` is generated and retained as CI evidence, not committed.

Native CI and local builds use the maintained registry. Run the following
on a native Trixie checkout with Bazelisk/Bazel 8.5.1 and the execution
tools listed in CI.

Install the `python3-defusedxml` package to read the Bazel JUnit reports.

```sh
python3 tools/bazel/validate.py audit
python3 tools/bazel/validate.py build
python3 tools/bazel/validate.py incremental
python3 tools/bazel/validate.py collect
```

The validation commands require a clean committed source checkout. They retain
the repository and action caches, and do not run `bazel clean`. Incremental
validation changes the CLI help text, verifies that the runtime/debug outputs
change, restores the source, and requires the original output hashes to return.
Its timing describes a retained-cache build, not a cache-free benchmark.

The original mux/link-state-machine tests keep their existing fake database and
packet fixtures. They execute with a declared matching Trixie loader/library
closure. The packet checksum test seeds its random packet ID explicitly; the
socket-error test temporarily exhausts the process file-descriptor limit and
restores it on exit. These fixtures preserve the original assertions across
test orderings and containers with different raw-socket capabilities.
Installed-package tests run the actual CLI, including its existing
exit-code-1 help behavior, and check file layout, ownership, modes, changelog
content, stripped ELF, debug Build-ID/CRC and GDB source/type lookup. GDB's
negative control disables symbols, automatic scripts and debuginfod.

CI collects tests from the current Bazel build-event report and binds them to
the source revision, generated module lock and exact runtime/debug hashes.
Collection requires successful original and restored outputs. The separate
informational CodeQL job prepares dependency tools before tracing, compiles all
owned C++ sources under tracing, and checks the extracted definitions and source
bytes. It does not generate DEBs or turn a failed scan into success.

This producer is scoped to native AMD64/Trixie. Docker-mux composition and
process acceptance belong to buildimage issue #123; physical dual-ToR/cable
acceptance remains outside VS testing. The legacy Makefile filters most
`DEB_CXXFLAGS_MAINT_APPEND` flags; this migration retains the effective component
flags and uses the shared native toolchain's hardening defaults, rather than
claiming that every declared Debian hardening option was previously effective.
