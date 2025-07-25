#!/usr/bin/env sh

git submodule update --init --recursive

python3 utils/remove_optional_from_proto.py
cd valkey-glide/ffi
cargo build --release
cd ../../

phpize
./configure --enable-valkey-glide
make -j4 build-modules-pre
make install