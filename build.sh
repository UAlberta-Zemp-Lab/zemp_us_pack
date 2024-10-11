#!/bin/sh

cc=${CC:-cc}

${cc} -march=native -O3 -std=c11 -c zemp_us_pack.h -o zus -lzstd
