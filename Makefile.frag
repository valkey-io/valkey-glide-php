# Platform-specific configuration
ifeq ($(shell uname),Darwin)
    INCLUDES += -I/opt/homebrew/include
    VALKEY_GLIDE_SHARED_LIBADD = valkey-glide/ffi/target/release/libglide_ffi.a -lresolv -lSystem -Wl,-rpath,/opt/homebrew/lib -L/opt/homebrew/lib
else
    # Linux - check for target-specific build first, fallback to release
    ifneq ($(wildcard valkey-glide/ffi/target/x86_64-unknown-linux-gnu/release/libglide_ffi.a),)
        VALKEY_GLIDE_SHARED_LIBADD = valkey-glide/ffi/target/x86_64-unknown-linux-gnu/release/libglide_ffi.a -lresolv -lprotobuf-c
    else ifneq ($(wildcard valkey-glide/ffi/target/aarch64-unknown-linux-gnu/release/libglide_ffi.a),)
        VALKEY_GLIDE_SHARED_LIBADD = valkey-glide/ffi/target/aarch64-unknown-linux-gnu/release/libglide_ffi.a -lresolv -lprotobuf-c
    else
        VALKEY_GLIDE_SHARED_LIBADD = valkey-glide/ffi/target/release/libglide_ffi.a -lresolv -lprotobuf-c
    endif
endif
INCLUDES += -Iinclude
PROTOC = protoc
PROTOC_C_PLUGIN := protoc-c
PROTO_SRC_DIR = valkey-glide/glide-core/src/protobuf
GEN_INCLUDE_DIR = include/glide
GEN_SRC_DIR = src

ASAN_ENABLE ?= 0
ifeq ($(ASAN_ENABLE),1)
  ASAN_CFLAGS  := -O1 -g -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope
  ASAN_LDFLAGS := -fsanitize=address

  CFLAGS       += $(ASAN_CFLAGS)
  CFLAGS_CLEAN += $(ASAN_CFLAGS)
  LDFLAGS      += $(ASAN_LDFLAGS)
  EXTRA_LDFLAGS+= $(ASAN_LDFLAGS)
endif

# Prefer Homebrew LLVM ASan runtime; fallback to clang resource dir
ASAN_DYLIB := $(shell ls /opt/homebrew/opt/llvm/lib/clang/*/lib/darwin/libclang_rt.asan_osx_dynamic.dylib 2>/dev/null | tail -n1)
ifeq ($(ASAN_DYLIB),)
  ASAN_DYLIB := $(shell $(CC) -print-resource-dir 2>/dev/null)/lib/darwin/libclang_rt.asan_osx_dynamic.dylib
endif

# Symbolizer path (pretty stacks)
ASAN_SYMBOLIZER_PATH := $(or $(shell command -v llvm-symbolizer 2>/dev/null),/opt/homebrew/opt/llvm/bin/llvm-symbolizer)
export ASAN_SYMBOLIZER_PATH

# Sensible defaults (override by exporting ASAN_OPTIONS if you want)
ASAN_OPTIONS ?= detect_stack_use_after_return=1,strict_string_checks=1,allocator_may_return_null=1,halt_on_error=1
export ASAN_OPTIONS


# Force header generation before any compilation
$(shared_objects_valkey_glide): include/glide_bindings.h cluster_scan_cursor_arginfo.h valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h logger_arginfo.h src/client_constructor_mock_arginfo.h valkey-glide/ffi/target/release/libglide_ffi.a

# Backward compatibility alias
build-modules-pre: include/glide_bindings.h cluster_scan_cursor_arginfo.h valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h logger_arginfo.h src/client_constructor_mock_arginfo.h valkey-glide/ffi/target/release/libglide_ffi.a

# Arginfo header dependencies
cluster_scan_cursor_arginfo.h: cluster_scan_cursor.stub.php
	@php -f $(top_srcdir)/build/gen_stub.php cluster_scan_cursor.stub.php || echo "cluster_scan_cursor arginfo generation failed"

valkey_glide_arginfo.h: valkey_glide.stub.php
	@php -f $(top_srcdir)/build/gen_stub.php valkey_glide.stub.php || echo "valkey_glide arginfo generation failed"

valkey_glide_cluster_arginfo.h: valkey_glide_cluster.stub.php
	@php -f $(top_srcdir)/build/gen_stub.php valkey_glide_cluster.stub.php || echo "valkey_glide_cluster arginfo generation failed"

logger_arginfo.h: logger.stub.php
	@php -f $(top_srcdir)/build/gen_stub.php logger.stub.php || echo "logger arginfo generation failed"

src/client_constructor_mock_arginfo.h: src/client_constructor_mock.stub.php
	@php -f $(top_srcdir)/build/gen_stub.php src/client_constructor_mock.stub.php || echo "client_constructor_mock arginfo generation failed"

valkey-glide/ffi/target/release/libglide_ffi.a:
	@echo "=== BUILDING FFI LIBRARY ==="
	@if [ -f .gitmodules ] && [ -d .git ]; then \
		git submodule update --init --recursive; \
	fi
	@if [ -d valkey-glide/ffi ]; then \
		cd valkey-glide/ffi && CFLAGS= CXXFLAGS= LDFLAGS= RUSTFLAGS= cargo build --release && cd ../..; \
	fi

include/glide_bindings.h:
	@echo "=== GENERATING HEADER FILE ==="
	@if [ -f .gitmodules ] && [ -d .git ]; then \
		git submodule update --init --recursive; \
	fi
	@python3 utils/remove_optional_from_proto.py || true
	@if [ -d valkey-glide/ffi ]; then \
		cd valkey-glide/ffi && CFLAGS= CXXFLAGS= LDFLAGS= RUSTFLAGS= cargo build --release && cd ../..; \
	fi
	@mkdir -p include
	@if [ -d valkey-glide/ffi ] && command -v cbindgen >/dev/null 2>&1; then \
		cd valkey-glide/ffi && cbindgen --output ../../include/glide_bindings.h && cd ../..; \
		echo '#ifndef GLIDE_BINDINGS_H' > include/glide_bindings_tmp.h; \
		echo '#define GLIDE_BINDINGS_H' >> include/glide_bindings_tmp.h; \
		cat include/glide_bindings.h >> include/glide_bindings_tmp.h; \
		echo '#endif /* GLIDE_BINDINGS_H */' >> include/glide_bindings_tmp.h; \
		mv include/glide_bindings_tmp.h include/glide_bindings.h; \
	fi
	@echo "=== GENERATING PROTOBUF HEADERS ==="
	@mkdir -p $(GEN_INCLUDE_DIR) $(GEN_SRC_DIR)
	@if command -v protoc-c >/dev/null 2>&1; then \
		for proto in $(PROTO_SRC_DIR)/*.proto; do \
			if [ -f "$$proto" ]; then \
				protoc-c --c_out=$(GEN_SRC_DIR) --proto_path=$(PROTO_SRC_DIR) "$$proto" || echo "Failed to generate $$proto"; \
			fi; \
		done; \
		cp $(GEN_SRC_DIR)/*.h $(GEN_INCLUDE_DIR)/ 2>/dev/null || true; \
	else \
		echo "protoc-c not found"; \
	fi
	@echo "=== HEADER GENERATION COMPLETE ==="

test:
	@echo "Running ValkeyGlide tests..."
	@if [ ! -f "$(CURDIR)/modules/valkey_glide.so" ]; then \
		echo "❌ ERROR: Extension not found at $(CURDIR)/modules/valkey_glide.so"; \
		echo "Please build the extension first with: make"; \
		exit 1; \
	fi
	@if [ ! -f "tests/TestValkeyGlide.php" ]; then \
		echo "❌ ERROR: Test file not found at tests/TestValkeyGlide.php"; \
		exit 1; \
	fi
	@if [ ! -f "tests/start_valkey_with_replicas.sh" ]; then \
		echo "❌ ERROR: Setup script not found at tests/start_valkey_with_replicas.sh"; \
		exit 1; \
	fi
	@if [ ! -f "tests/create-valkey-cluster.sh" ]; then \
		echo "❌ ERROR: Setup script not found at tests/create-valkey-cluster.sh"; \
		exit 1; \
	fi
	@echo "Setting up Valkey infrastructure..."
	@echo "Starting Valkey with replicas..."
	@cd tests && ./start_valkey_with_replicas.sh
	@echo "Creating Valkey cluster..."
	@cd tests && ./create-valkey-cluster.sh
	@echo "Running PHP tests..."
	php -n -d extension=./modules/valkey_glide.so tests/TestValkeyGlide.php
	@echo "✓ Tests completed"

.PHONY: asan-test
asan-test: all
	@if [ -z "$(ASAN_DYLIB)" ] || [ ! -f "$(ASAN_DYLIB)" ]; then \
		echo "❌ ASan runtime not found. Install Homebrew llvm: brew install llvm"; exit 1; \
	fi
	@echo "Using ASAN_DYLIB=$(ASAN_DYLIB)"
	USE_ZEND_ALLOC=0 MallocNanoZone=0 \
	DYLD_INSERT_LIBRARIES="$(ASAN_DYLIB)" \
	ASAN_SYMBOLIZER_PATH="$(ASAN_SYMBOLIZER_PATH)" \
	ASAN_OPTIONS="$(ASAN_OPTIONS)" \
	php -n -d extension=./modules/valkey_glide.so tests/TestValkeyGlide.php
