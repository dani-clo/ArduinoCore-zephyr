#!/bin/sh

REPO_URL="https://github.com/bcmi-labs/orchestrator.git"
BRANCH_NAME="main"

set -xe

TMP_DIR=$(mktemp -d)

# pull the repo
git clone "$REPO_URL" "$TMP_DIR"
cd "$TMP_DIR"
git checkout "$BRANCH_NAME" 

# compile remoteocd
go build -o build/remoteocd ./cmd/remoteocd

# move into arduino tools
TARGET_PATH=$(arduino-cli config get directories.data)/packages/arduino/tools/remoteocd/0.0.1
mkdir -p "$TARGET_PATH"
cp build/remoteocd "$TARGET_PATH"

rm -rf "$TMP_DIR"

echo "remoteocd is in $TARGET_PATH/remoteocd"
