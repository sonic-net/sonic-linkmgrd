#!/usr/bin/env bash
set -euo pipefail
mode="${1:?expected prepare or trace}"
base="${LINKMGRD_CODEQL_OUTPUT_BASE:?missing dedicated output base}"
flags=(--jobs=2 --compilation_mode=opt --spawn_strategy=local
       --action_env=PATH=/usr/bin:/bin
       --noremote_accept_cached --noremote_upload_local_results
       --disk_cache= --remote_cache= --remote_executor=)

case "$mode" in
  prepare)
    "$BAZEL_BINARY" --batch --output_base="$base" aquery 'deps(//:linkmgrd)' \
      "${flags[@]}" --output=jsonproto > bazel-evidence/codeql-actions.json
    python3 tools/bazel/audit_actions.py bazel-evidence/codeql-actions.json \
      > bazel-evidence/codeql-action-audit.json
    # These public owner targets prepare the compiler/runtime tools and source
    # dependencies. None compiles this repository's C++ translation units.
    "$BAZEL_BINARY" --batch --output_base="$base" build "${flags[@]}" \
      @sonic_swss_common//:libswsscommon_consolidated.so \
      @libnl3//:libnl_3_shared @libnl3//:libnl_route_3_shared \
      @trixie//libboost-date-time1.83-dev:libboost_date_time_shared \
      @trixie//libboost-thread1.83-dev:libboost_thread_shared \
      @trixie//libboost-log1.83-dev:libboost_log_shared \
      @trixie//libboost-log1.83-dev:libboost_log_setup_shared \
      @trixie//libboost-program-options1.83-dev:libboost_program_options_shared \
      @trixie//libboost-filesystem1.83-dev:libboost_filesystem_shared
    ;;
  trace)
    # This owner's compile actions have never run in the dedicated base. Keep
    # prepared tool actions cached so the tracer cannot enter their private ABI.
    "$BAZEL_BINARY" --batch --output_base="$base" build "${flags[@]}" //:linkmgrd
    ;;
  *) printf 'Unknown stage: %s\n' "$mode" >&2; exit 2 ;;
esac
