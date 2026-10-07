#!/bin/sh
# Agustin Blaumann (ab3211) -- CS 214 Project I
set -eu
testdir=$(mktemp -d)
trap 'rm -rf "$testdir"' EXIT HUP INT TERM

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

./correctness >"$testdir/out" 2>"$testdir/err" || fail "valid allocations"
test ! -s "$testdir/err" || fail "unexpected correctness diagnostic"
grep -qx 'Correctness tests passed' "$testdir/out" || fail "correctness result"
./memtest >"$testdir/out" 2>"$testdir/err" || fail "provided memtest"
test ! -s "$testdir/err" || fail "unexpected memtest diagnostic"
grep -qx '0 incorrect bytes' "$testdir/out" || fail "memtest data corruption"

for mode in bounds exhaustion fragmentation leak padded-leak stack interior double coalesced-double header null; do
    expected_status=0
    case "$mode" in
        stack|interior|double|coalesced-double|header|null) expected_status=2 ;;
    esac
    status=0
    ./correctness "$mode" >"$testdir/out" 2>"$testdir/err" || status=$?
    test "$status" -eq "$expected_status" || fail "$mode: exit $status, expected $expected_status"
    case "$mode" in
        interior|header) echo 'mymalloc: 16 bytes leaked in 1 objects.' >>"$testdir/out" ;;
    esac
    cmp -s "$testdir/out" "$testdir/err" || {
        cat "$testdir/err" >&2
        fail "$mode: diagnostic, byte count, or source location mismatch"
    }
done

./memtest-leak >"$testdir/out" 2>"$testdir/err" || fail "provided leak test"
grep -qx '0 incorrect bytes' "$testdir/out" || fail "leak test data corruption"
echo 'mymalloc: 3584 bytes leaked in 64 objects.' >"$testdir/expected"
cmp -s "$testdir/expected" "$testdir/err" || fail "provided leak count"
./memgrind >"$testdir/out" 2>"$testdir/err" || fail "stress workload"
test ! -s "$testdir/err" || fail "stress workload errors or leaks"
grep -q '^Average workload time over 50 runs: ' "$testdir/out" || fail "timing output"
cat "$testdir/out"
echo 'All tests passed'
