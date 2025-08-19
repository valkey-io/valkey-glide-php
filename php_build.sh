#!/usr/bin/env sh

git submodule update --init --recursive

python3 utils/remove_optional_from_proto.py
cd valkey-glide/ffi
cargo build --release
cd ../../

protoc --proto_path=./valkey-glide/glide-core/src/protobuf --php_out=./tests/generated ./valkey-glide/glide-core/src/protobuf/connection_request.proto

phpize
./configure --enable-valkey-glide
make -j4 build-modules-pre
make install