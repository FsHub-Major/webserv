#!/usr/bin/env bash
set -euo pipefail

SERVER_BIN="./webserv"
# Leave empty to use embedded defaults; set to a valid file to override
CONFIG_FILE=""
TEST_SRC="tests/integration_tests.cpp"
TEST_BIN="tests/integration_tests"

RED="\033[0;31m"; GREEN="\033[0;32m"; YELLOW="\033[0;33m"; NC="\033[0m"

cd ..
echo -e "${YELLOW}[RUN] Building server...${NC}"
make

echo -e "${YELLOW}[RUN] Starting server...${NC}"
if [[ -n "${CONFIG_FILE}" ]]; then
  $SERVER_BIN "$CONFIG_FILE" > /dev/null &
else
  $SERVER_BIN > /dev/null &
fi
PARENT_PID=$!

echo -e "[INFO] Server parent PID: $PARENT_PID"
# Allow some startup time
sleep 0.8

# Build tests (C++11 allowed)
echo -e "${YELLOW}[RUN] Compiling tests...${NC}"
c++ -std=c++11 -Wall -Wextra -Werror "$TEST_SRC" -o "$TEST_BIN"

echo -e "${YELLOW}[RUN] Executing tests...${NC}"
set +e
"$TEST_BIN"
TEST_STATUS=$?
set -e

if [[ $TEST_STATUS -eq 0 ]]; then
  echo -e "${GREEN}[RESULT] Tests PASS${NC}"
else
  echo -e "${RED}[RESULT] Tests FAIL (exit $TEST_STATUS)${NC}"
fi

echo -e "${YELLOW}[RUN] Stopping server...${NC}"
kill -TERM "$PARENT_PID" 2>/dev/null || true
wait "$PARENT_PID" 2>/dev/null || true

echo -e "${YELLOW}[CLEANUP] Done.${NC}"
exit $TEST_STATUS
