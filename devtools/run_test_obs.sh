#!/bin/bash

TYPE="${1}"
UNFI="${2}"

if [[ "xd" != "x${TYPE}" && "xr" != "x${TYPE}" && "xa" != "x${TYPE}" &&
      "xl" != "x${TYPE}" && "xt" != "x${TYPE}" && "xu" != "x${TYPE}" ]]; then
	echo "$0 <d|r|a|l|t|u> [unfiltered]"
	echo
	echo "debug, release, asan, lsan, tsan, usan"
	echo
	echo "[unfiltered] adds --unfiltered_log which will flood the log with everything obs debug mode talks about."
	exit 0;
fi

if [[ "xunfiltered" = "x${UNFI}" ]]; then
	UNFI="--unfiltered_log"
else
	UNFI=""
fi

xHERE="$(readlink -f "$(dirname $0)/../")"
xINT="${xHERE}/install"

if [[ "xr" != "x${TYPE}" ]]; then
	xINT="${xINT}_${TYPE}"
fi

xoldPATH="$PATH"
export PATH="${xINT}/bin:${xoldPATH}"

xoldLDP="$LD_LIBRARY_PATH"
export LD_LIBRARY_PATH="${xINT}/lib64:${xoldLDP}"

COMMON_OPTIONS="allow_addr2line=true strip_path_prefix=${xHERE}/ verbosity=1 detect_leaks=true leak_check_at_exit=true"


ASAN_OPTIONS="${COMMON_OPTIONS} quarantine_size_mb=512 check_initialization_order=true                        \
    detect_stack_use_after_return=true print_stats=true suppressions=${xHERE}/devtools/asan_suppressions.lst" \
LSAN_OPTIONS="${COMMON_OPTIONS} report_objects=1 suppressions=${xHERE}/devtools/lsan_suppressions.lst"        \
TSAN_OPTIONS="${COMMON_OPTIONS} history_size=7 force_seq_cst_atomics=1 second_deadlock_stack=1                \
    suppressions=${xHERE}/devtools/tsan_suppressions.lst"                                                     \
	${xINT}/bin/obs --verbose ${UNFI}


export LD_LIBRARY_PATH="${xoldLDP}"
export PATH="${xoldPATH}"
