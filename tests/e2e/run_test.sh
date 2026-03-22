#!/bin/bash
# End-to-end тест: сравнивает вывод OBNC (эталон) и o7rc+RARS.
#
# Использование:
#   run_test.sh <o7rc> <java> <rars.jar> <obnc> <source.obr>
#
# Коды возврата:
#   0 — тест пройден
#   1 — ошибка компиляции o7rc
#   2 — ошибка запуска RARS
#   3 — вывод не совпадает с эталоном OBNC
#   4 — ошибка компиляции/запуска OBNC

set -euo pipefail

O7RC="$1"
JAVA="$2"
RARS_JAR="$3"
OBNC="$4"
SOURCE="$5"

TEST_NAME=$(basename "${SOURCE}" .obr)
TMPDIR=$(mktemp -d)
ASM="${TMPDIR}/${TEST_NAME}.asm"

cleanup() { rm -rf "${TMPDIR}"; }
trap cleanup EXIT

MODULE_NAME=$(sed -n 's/^MODULE \([A-Za-z_][A-Za-z0-9_]*\).*/\1/p' "${SOURCE}" | head -1)
if [ -z "${MODULE_NAME}" ]; then
    echo "FAIL [${TEST_NAME}]: cannot extract MODULE name"
    exit 4
fi

OBNC_DIR="${TMPDIR}/obnc_build"
mkdir -p "${OBNC_DIR}"
cp "${SOURCE}" "${OBNC_DIR}/${MODULE_NAME}.obn"

OBNC_BIN_DIR=$(dirname "${OBNC}")
OBNC_PREFIX=$(cd "${OBNC_BIN_DIR}/.." && pwd)

export C_INCLUDE_PATH="${OBNC_PREFIX}/include:${C_INCLUDE_PATH:-}"
export LIBRARY_PATH="${OBNC_PREFIX}/lib/obnc:${LIBRARY_PATH:-}"

if ! (cd "${OBNC_DIR}" && "${OBNC}" "${MODULE_NAME}.obn" 2>"${TMPDIR}/obnc.err"); then
    echo "FAIL [${TEST_NAME}]: OBNC compilation error"
    cat "${TMPDIR}/obnc.err"
    exit 4
fi

EXPECTED=$("${OBNC_DIR}/${MODULE_NAME}" 2>/dev/null) || {
    echo "FAIL [${TEST_NAME}]: OBNC binary crashed"
    exit 4
}

if ! "${O7RC}" "${SOURCE}" -o "${ASM}" 2>"${TMPDIR}/compile.err"; then
    echo "FAIL [${TEST_NAME}]: o7rc compilation error"
    cat "${TMPDIR}/compile.err"
    exit 1
fi

ACTUAL=$("${JAVA}" -jar "${RARS_JAR}" nc sm "${ASM}" 2>"${TMPDIR}/rars.err") || {
    RC=$?
    echo "FAIL [${TEST_NAME}]: RARS exited with code ${RC}"
    cat "${TMPDIR}/rars.err"
    echo "--- asm ---"
    cat "${ASM}"
    exit 2
}

if [ "${ACTUAL}" = "${EXPECTED}" ]; then
    echo "PASS [${TEST_NAME}]"
    exit 0
else
    echo "FAIL [${TEST_NAME}]: output mismatch"
    echo "--- expected (OBNC) ---"
    echo "${EXPECTED}"
    echo "--- actual (o7rc+RARS) ---"
    echo "${ACTUAL}"
    echo "--- asm ---"
    cat "${ASM}"
    exit 3
fi
