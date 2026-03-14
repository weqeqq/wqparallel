#!/bin/sh

set -eu

output_dir=$1
shift

mkdir -p "$output_dir"
exec "$@"
