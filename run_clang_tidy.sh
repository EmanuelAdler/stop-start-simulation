#!/usr/bin/env bash
clang-tidy "$@" -- -std=c99 -D_POSIX_C_SOURCE=199309L
