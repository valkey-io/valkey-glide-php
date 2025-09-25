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
INCLUDES += -Iinclude -I.
PROTOC = protoc
PROTOC_C_PLUGIN := protoc-c
PROTO_SRC_DIR = valkey-glide/glide-core/src/protobuf
GEN_INCLUDE_DIR = include/glide
GEN_SRC_DIR = src
CFLAGS += -Werror

# Force header generation before any compilation
$(shared_objects_valkey_glide): include/glide_bindings.h cluster_scan_cursor_arginfo.h valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h logger_arginfo.h src/client_constructor_mock_arginfo.h valkey-glide/ffi/target/release/libglide_ffi.a

# Backward compatibility alias
build-modules-pre: include/glide_bindings.h cluster_scan_cursor_arginfo.h valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h logger_arginfo.h src/client_constructor_mock_arginfo.h valkey-glide/ffi/target/release/libglide_ffi.a

# Debug what files exist
debug-files:
	@echo "=== DEBUG: Checking what files exist ==="
	@echo "include/glide_bindings.h exists: $$([ -f include/glide_bindings.h ] && echo YES || echo NO)"
	@echo "src/command_request.pb-c.c exists: $$([ -f src/command_request.pb-c.c ] && echo YES || echo NO)"
	@echo "src/command_request.pb-c.h exists: $$([ -f src/command_request.pb-c.h ] && echo YES || echo NO)"
	@echo "valkey-glide/glide-core/src/protobuf exists: $$([ -d valkey-glide/glide-core/src/protobuf ] && echo YES || echo NO)"
	@if [ -d valkey-glide/glide-core/src/protobuf ]; then \
		echo "Protobuf files in submodule:"; \
		ls -la valkey-glide/glide-core/src/protobuf/*.proto 2>/dev/null || echo "No .proto files found"; \
	fi
	@echo "Generated protobuf files in src/:"; \
	ls -la src/*.pb-c.* 2>/dev/null || echo "No generated protobuf files found"

# For PECL builds (no .git directory), make protobuf files depend on submodules being ready
# For Git builds, assume submodules are already there
ifeq ($(wildcard .git),)
src/%.pb-c.c: ensure-submodules
	@echo "PECL build: Protobuf file $@ depends on submodules being ready"

src/%.pb-c.h: ensure-submodules
	@echo "PECL build: Protobuf header $@ depends on submodules being ready"
endif

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

valkey-glide/ffi/target/release/libglide_ffi.a: ensure-submodules
	@echo "=== BUILDING FFI LIBRARY ==="
	@if [ -d valkey-glide/ffi ]; then \
		cd valkey-glide/ffi && cargo build --release && cd ../..; \
	fi

ensure-submodules:
	@echo "=== Ensuring submodules are ready ==="
	@if [ -f .gitmodules ]; then \
		if [ -d .git ]; then \
			echo "Git repository detected - using submodules"; \
			git submodule update --init --recursive; \
		else \
			echo "PECL package detected - cloning submodules manually"; \
			if ! command -v git >/dev/null 2>&1; then \
				echo "ERROR: Git is required to build this extension"; \
				exit 1; \
			fi; \
			grep -E "^\s*path\s*=" .gitmodules | sed 's/.*=\s*//' | while read path; do \
				url=$$(grep -A1 "path = $$path" .gitmodules | grep url | sed 's/.*=\s*//'); \
				if [ ! -d "$$path/.git" ]; then \
					echo "Cloning $$url into $$path"; \
					rm -rf "$$path"; \
					if ! git clone "$$url" "$$path"; then \
						echo "ERROR: Failed to clone submodule from $$url"; \
						exit 1; \
					fi; \
					if [ -f .submodule-commits ]; then \
						commit=$$(grep "^$$path=" .submodule-commits | cut -d= -f2); \
						if [ -n "$$commit" ]; then \
							echo "Checking out recorded commit $$commit for $$path"; \
							cd "$$path" && git checkout "$$commit" && cd ..; \
						fi; \
					fi; \
				fi; \
			done; \
		fi; \
	fi
	@echo "Submodules ready"

include/glide_bindings.h: ensure-submodules
	@echo "=== GENERATING HEADER FILE ==="
	@echo "DEBUG: Starting include/glide_bindings.h generation"
	@echo "DEBUG: Current directory: $$(pwd)"
	@echo "DEBUG: valkey-glide directory exists: $$([ -d valkey-glide ] && echo YES || echo NO)"
	@echo "DEBUG: valkey-glide/glide-core/src/protobuf exists: $$([ -d valkey-glide/glide-core/src/protobuf ] && echo YES || echo NO)"
	@python3 utils/remove_optional_from_proto.py || true
	@echo "=== Setting up Rust environment ==="
	@export PATH="$$HOME/.cargo/bin:$$PATH" && \
	if [ -f "$$HOME/.cargo/env" ]; then \
		. "$$HOME/.cargo/env"; \
	fi && \
	if [ -d valkey-glide/ffi ]; then \
		cd valkey-glide/ffi && cargo build --release && cd ../..; \
	fi
	@mkdir -p include
	@export PATH="$$HOME/.cargo/bin:$$PATH" && \
	if [ -f "$$HOME/.cargo/env" ]; then \
		. "$$HOME/.cargo/env"; \
	fi && \
	if [ -d valkey-glide/ffi ] && command -v cbindgen >/dev/null 2>&1; then \
		cd valkey-glide/ffi && cbindgen --output ../../include/glide_bindings.h && cd ../..; \
		echo '#ifndef GLIDE_BINDINGS_H' > include/glide_bindings_tmp.h; \
		echo '#define GLIDE_BINDINGS_H' >> include/glide_bindings_tmp.h; \
		cat include/glide_bindings.h >> include/glide_bindings_tmp.h; \
		echo '#endif /* GLIDE_BINDINGS_H */' >> include/glide_bindings_tmp.h; \
		mv include/glide_bindings_tmp.h include/glide_bindings.h; \
		echo "=== DEBUG: Copying glide_bindings.h for inspection ==="; \
		cp include/glide_bindings.h /tmp/debug_glide_bindings.h 2>/dev/null || true; \
		echo "=== DEBUG: glide_bindings.h copied to /tmp/debug_glide_bindings.h ==="; \
	fi
	@echo "=== GENERATING PROTOBUF HEADERS ==="
	@mkdir -p $(GEN_INCLUDE_DIR) $(GEN_SRC_DIR)
	@echo "DEBUG: About to generate protobuf files"
	@echo "DEBUG: PROTO_SRC_DIR = $(PROTO_SRC_DIR)"
	@echo "DEBUG: GEN_SRC_DIR = $(GEN_SRC_DIR)"
	@if command -v protoc-c >/dev/null 2>&1; then \
		echo "DEBUG: protoc-c found, generating protobuf files"; \
		for proto in $(PROTO_SRC_DIR)/*.proto; do \
			if [ -f "$$proto" ]; then \
				echo "DEBUG: Processing $$proto"; \
				protoc-c --c_out=$(GEN_SRC_DIR) --proto_path=$(PROTO_SRC_DIR) "$$proto" || echo "Failed to generate $$proto"; \
			fi; \
		done; \
		echo "DEBUG: Copying generated headers"; \
		cp $(GEN_SRC_DIR)/*.h $(GEN_INCLUDE_DIR)/ 2>/dev/null || true; \
		echo "DEBUG: Generated files:"; \
		ls -la $(GEN_SRC_DIR)/*.pb-c.* 2>/dev/null || echo "No protobuf files generated"; \
	else \
		echo "protoc-c not found"; \
	fi
	@echo "=== HEADER GENERATION COMPLETE ==="

# Override generated test target
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
