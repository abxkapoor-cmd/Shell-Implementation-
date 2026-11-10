#!/usr/bin/env bash
set -eu
cd "$(dirname "$0")/.."
S3="${S3:-./build/s3}" python3 tests/test_shell.py
