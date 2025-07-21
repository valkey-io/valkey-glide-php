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

# Cargo and tool detection
CARGO_HOME ?= $(HOME)/.cargo
CBINDGEN := $(shell which cbindgen 2>/dev/null || echo $(CARGO_HOME)/bin/cbindgen)

PROTO_FILES = connection_request.proto command_request.proto response.proto

define proto_rule
$(GEN_INCLUDE_DIR)/$(basename $(1)).pb-c.h $(GEN_INCLUDE_DIR)/$(basename $(1)).pb-c.c: $(PROTO_SRC_DIR)/$(1)
	@mkdir -p $(GEN_INCLUDE_DIR)
	$(PROTOC) --c_out=$(GEN_INCLUDE_DIR) -I $(PROTO_SRC_DIR) $(PROTO_SRC_DIR)/$(1)

$(GEN_SRC_DIR)/$(basename $(1)).pb-c.c: $(GEN_INCLUDE_DIR)/$(basename $(1)).pb-c.c
	@mkdir -p $(GEN_SRC_DIR)
	mv $(GEN_INCLUDE_DIR)/$(basename $(1)).pb-c.c $(GEN_SRC_DIR)/$(basename $(1)).pb-c.c
	sed -i.bak 's|"$(basename $(1)).pb-c.h"|<glide/$(basename $(1)).pb-c.h>|' $(GEN_SRC_DIR)/$(basename $(1)).pb-c.c
	rm -f $(GEN_SRC_DIR)/$(basename $(1)).pb-c.c.bak
endef

$(foreach proto,$(PROTO_FILES),$(eval $(call proto_rule,$(proto))))

PROTO_HEADERS := $(foreach proto,$(PROTO_FILES),$(GEN_INCLUDE_DIR)/$(basename $(proto)).pb-c.h)
PROTO_SOURCES := $(foreach proto,$(PROTO_FILES),$(GEN_SRC_DIR)/$(basename $(proto)).pb-c.c)

generate-proto: $(PROTO_HEADERS) $(PROTO_SOURCES)
	@echo "Generated C protobuf bindings."

clean-proto:
	rm -f $(PROTO_HEADERS)
	rm -f $(PROTO_SOURCES)

generate-bindings:
	@echo "Generating C bindings from Rust code..."
	@mkdir -p $(top_srcdir)/include
	@cp $(top_srcdir)/valkey-glide/ffi/src/lib.rs $(top_srcdir)/include/lib.rs
	@cd $(top_srcdir)/valkey-glide/ffi && $(CBINDGEN) --config cbindgen.toml --crate glide-ffi --output $(top_srcdir)/include/glide_bindings.h
	@printf '#pragma once\n' > $(top_srcdir)/include/glide_bindings.h.tmp
	@cat $(top_srcdir)/include/glide_bindings.h >> $(top_srcdir)/include/glide_bindings.h.tmp
	@mv $(top_srcdir)/include/glide_bindings.h.tmp $(top_srcdir)/include/glide_bindings.h
	
valkey_glide_arginfo.h: valkey_glide.stub.php
	@echo "Generating arginfo from valkey_glide.stub.php"
	$(PHP_EXECUTABLE) build/gen_stub.php --no-legacy-arginfo valkey_glide.stub.php

valkey_glide_cluster_arginfo.h: valkey_glide_cluster.stub.php
	@echo "Generating arginfo from valkey_glide_cluster.stub.php"
	$(PHP_EXECUTABLE) build/gen_stub.php --no-legacy-arginfo valkey_glide_cluster.stub.php

cluster_scan_cursor_arginfo.h: cluster_scan_cursor.stub.php
	@echo "Generating arginfo from cluster_scan_cursor.stub.php"
	$(PHP_EXECUTABLE) build/gen_stub.php --no-legacy-arginfo cluster_scan_cursor.stub.php

ARGINFO_HEADERS = valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h cluster_scan_cursor_arginfo.h

all: $(ARGINFO_HEADERS)

.PHONY: build-modules-pre

build-modules-pre: valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h cluster_scan_cursor_arginfo.h
	@$(MAKE) generate-proto
	@$(MAKE) generate-bindings

# Wrap the original build-modules
build-modules: $(PHP_MODULES) $(PHP_ZEND_EX)

# Linting targets for PHP and C code
lint: lint-c lint-php

lint-c:
	@echo "Checking C code formatting..."
	@if command -v clang-format >/dev/null 2>&1; then \
		if find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | grep -q .; then \
			find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | xargs clang-format --dry-run --Werror; \
			echo "✓ C code formatting check passed"; \
		else \
			echo "No C files found to check"; \
		fi; \
	else \
		echo "Warning: clang-format not found, skipping C code formatting check"; \
	fi
	@echo "Running C static analysis..."
	

lint-php:
	@echo "Running PHP linting..."
	@if [ -f "composer.json" ] && [ ! -d "vendor" ]; then \
		echo "Installing composer dependencies..."; \
		if command -v composer >/dev/null 2>&1; then \
			composer install --dev --no-progress --quiet; \
		else \
			echo "Warning: composer not found, some PHP linting tools may not be available"; \
		fi; \
	fi
	@if command -v phpcs >/dev/null 2>&1 || [ -f "vendor/bin/phpcs" ]; then \
		echo "Running PHP CodeSniffer..."; \
		if [ -f "vendor/bin/phpcs" ]; then \
			./vendor/bin/phpcs --standard=phpcs.xml; \
		else \
			phpcs --standard=phpcs.xml; \
		fi; \
		echo "✓ PHP CodeSniffer passed"; \
	else \
		echo "Warning: phpcs not found, skipping PHP coding standards check"; \
	fi
	@if command -v phpstan >/dev/null 2>&1 || [ -f "vendor/bin/phpstan" ]; then \
		echo "Running PHPStan static analysis..."; \
		if [ -f "vendor/bin/phpstan" ]; then \
			./vendor/bin/phpstan analyze --no-progress; \
		else \
			phpstan analyze --no-progress; \
		fi; \
		echo "✓ PHPStan analysis passed"; \
	else \
		echo "Warning: phpstan not found, skipping PHP static analysis"; \
	fi

lint-fix:
	@echo "Fixing C code formatting..."
	@if command -v clang-format >/dev/null 2>&1; then \
		if find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | grep -q .; then \
			find . -name "*.c" -o -name "*.h" | grep -v "\.pb-c\." | xargs clang-format -i; \
			echo "✓ C code formatting fixed"; \
		else \
			echo "No C files found to format"; \
		fi; \
	else \
		echo "Warning: clang-format not found, cannot fix C code formatting"; \
	fi
	@echo "Fixing PHP code formatting..."
	@if command -v phpcbf >/dev/null 2>&1 || [ -f "vendor/bin/phpcbf" ]; then \
		if [ -f "vendor/bin/phpcbf" ]; then \
			./vendor/bin/phpcbf --standard=phpcs.xml || true; \
		else \
			phpcbf --standard=phpcs.xml || true; \
		fi; \
		echo "✓ PHP code formatting fixed"; \
	else \
		echo "Warning: phpcbf not found, cannot fix PHP code formatting"; \
	fi

install-build-tools:
	@echo "Installing build tools..."
	@if command -v cargo >/dev/null 2>&1; then \
		cargo install cbindgen; \
		echo "✓ cbindgen installed via Cargo"; \
	else \
		echo "Warning: cargo not found, please install Rust first"; \
	fi

install-lint-tools:
	@echo "Installing linting tools..."
	@if command -v composer >/dev/null 2>&1; then \
		composer install --dev --no-progress; \
		echo "✓ PHP linting tools installed via Composer"; \
	else \
		echo "Warning: composer not found, please install composer first"; \
	fi
	@echo "Please ensure clang-format are installed for C code linting"
	@echo "Ubuntu/Debian: sudo apt-get install clang-format"
	@echo "macOS: brew install clang-format"

install-tools: install-build-tools install-lint-tools

# ASAN configuration for macOS only
ASAN_OPTIONS_ENV = halt_on_error=0:abort_on_error=0:symbolize=1:print_stacktrace=1:detect_stack_use_after_return=1:log_path=./asan_logs:fast_unwind_on_malloc=0:print_module_map=1

# ASAN (AddressSanitizer) targets
build-asan:
	@echo "Building with AddressSanitizer..."
	@$(MAKE) build-modules-pre $(ARGINFO_HEADERS)
	@$(MAKE)
	@echo "✓ ASAN build completed"

test-asan: build-asan
	@echo "Running tests with AddressSanitizer (macOS only)..."
	@if [ "$$(uname -s)" != "Darwin" ]; then \
		echo "❌ ERROR: ASAN tests are only supported on macOS"; \
		echo "Current platform: $$(uname -s)"; \
		exit 1; \
	fi
	@mkdir -p asan_logs
	@echo "=== ASAN Test Configuration ==="
	@echo "Extension: $(CURDIR)/modules/valkey_glide.so"
	@echo "Test file: tests/TestValkeyGlide.php"
	@echo "ASAN options: $(ASAN_OPTIONS_ENV)"
	@echo ""
	@echo "=== Checking Prerequisites ==="
	@if [ ! -f "$(CURDIR)/modules/valkey_glide.so" ]; then \
		echo "❌ ERROR: Extension not found at $(CURDIR)/modules/valkey_glide.so"; \
		exit 1; \
	fi
	@if [ ! -f "tests/TestValkeyGlide.php" ]; then \
		echo "❌ ERROR: Test file not found at tests/TestValkeyGlide.php"; \
		exit 1; \
	fi
	@echo "✓ Extension file exists: $(CURDIR)/modules/valkey_glide.so"
	@echo "✓ Test file exists: tests/TestValkeyGlide.php"
	@echo ""
	@echo "=== Analyzing Extension Dependencies ==="
	@echo "macOS: Analyzing extension dependencies with otool..."
	@otool -L $(CURDIR)/modules/valkey_glide.so 2>/dev/null || echo "otool failed"
	@echo ""
	@echo "=== Finding and Testing ASAN Libraries ==="
	@ASAN_LIB=""; \
	EXTENSION_ASAN_LIB=$$(otool -L $(CURDIR)/modules/valkey_glide.so 2>/dev/null | grep asan | awk '{print $$1}' | head -1); \
	echo "Extension expects ASAN library: $$EXTENSION_ASAN_LIB"; \
	echo "Searching for macOS ASAN libraries..."; \
	for lib_path in \
		"$$EXTENSION_ASAN_LIB" \
		"$$(clang -print-file-name=libclang_rt.asan_osx_dynamic.dylib 2>/dev/null || echo '')" \
		"/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/*/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" \
		"/opt/homebrew/lib/clang/*/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" \
		"/usr/local/lib/clang/*/lib/darwin/libclang_rt.asan_osx_dynamic.dylib" \
		"/Library/Developer/CommandLineTools/usr/lib/clang/*/lib/darwin/libclang_rt.asan_osx_dynamic.dylib"; do \
		if [ -n "$$lib_path" ] && [ -f "$$lib_path" ]; then \
			echo "Testing ASAN library: $$lib_path"; \
			if env DYLD_INSERT_LIBRARIES="$$lib_path" ASAN_OPTIONS="$(ASAN_OPTIONS_ENV)" php -n -d extension=$(CURDIR)/modules/valkey_glide.so -r "echo 'ASAN test OK';" >/dev/null 2>&1; then \
				echo "✅ SUCCESS: Extension loads with $$lib_path"; \
				ASAN_LIB="$$lib_path"; \
				break; \
			else \
				echo "❌ FAILED: Extension cannot load with $$lib_path"; \
			fi; \
		fi; \
	done; \
	if [ -z "$$ASAN_LIB" ]; then \
		echo ""; \
		echo "❌ CRITICAL ERROR: No working ASAN library found"; \
		echo "Available ASAN libraries on system:"; \
		find /Applications/Xcode.app /opt/homebrew /usr/local /Library/Developer -name "*asan*" -type f 2>/dev/null | head -10 || echo "  None found"; \
		echo ""; \
		echo "This usually means:"; \
		echo "1. Extension was compiled with ASAN but ASAN runtime is not properly installed"; \
		echo "2. Version mismatch between compile-time and runtime ASAN libraries"; \
		echo "3. Extension dependencies require a specific ASAN library version"; \
		echo ""; \
		echo "macOS: Install Xcode Command Line Tools: xcode-select --install"; \
		echo ""; \
		echo "Trying to run test without ASAN (will likely show ASAN runtime error):"; \
		env ASAN_OPTIONS="$(ASAN_OPTIONS_ENV)" php -n -d extension=$(CURDIR)/modules/valkey_glide.so -r "echo 'Direct test';" 2>&1 || true; \
		exit 1; \
	fi; \
	echo ""; \
	echo "=== Running ASAN Tests ==="; \
	echo "Using ASAN library: $$ASAN_LIB"; \
	echo "Command: env DYLD_INSERT_LIBRARIES=\"$$ASAN_LIB\" ASAN_OPTIONS=\"$(ASAN_OPTIONS_ENV)\" php -n -d extension=$(CURDIR)/modules/valkey_glide.so tests/TestValkeyGlide.php"; \
	echo ""; \
	env DYLD_INSERT_LIBRARIES="$$ASAN_LIB" ASAN_OPTIONS="$(ASAN_OPTIONS_ENV)" php -n -d extension=$(CURDIR)/modules/valkey_glide.so tests/TestValkeyGlide.php; \
	TEST_RESULT=$$?; \
	echo ""; \
	echo "=== Test Results ==="; \
	if [ $$TEST_RESULT -eq 0 ]; then \
		echo "✅ ASAN tests completed successfully (exit code: $$TEST_RESULT)"; \
	else \
		echo "❌ ASAN tests failed (exit code: $$TEST_RESULT)"; \
		echo ""; \
		echo "=== Error Analysis ==="; \
		if [ -d "./asan_logs" ] && [ "$$(ls -A ./asan_logs 2>/dev/null)" ]; then \
			echo "📄 ASAN reports found:"; \
			for log_file in ./asan_logs/*; do \
				if [ -f "$$log_file" ]; then \
					echo "--- $$log_file ---"; \
					cat "$$log_file"; \
					echo ""; \
				fi; \
			done; \
		else \
			echo "📄 No ASAN log files generated - this may indicate the test failed before ASAN could detect issues"; \
		fi; \
		echo "=== System State ==="; \
		echo "Working directory: $$(pwd)"; \
		echo "Extension exists: $$(test -f '$(CURDIR)/modules/valkey_glide.so' && echo 'YES' || echo 'NO')"; \
		echo "Test file exists: $$(test -f 'tests/TestValkeyGlide.php' && echo 'YES' || echo 'NO')"; \
		echo "ASAN library used: $$ASAN_LIB"; \
		echo ""; \
		exit $$TEST_RESULT; \
	fi
	@if [ -d "./asan_logs" ] && [ "$$(ls -A ./asan_logs 2>/dev/null)" ]; then \
		echo "=== ASAN Reports Found ==="; \
		for log_file in ./asan_logs/*; do \
			if [ -f "$$log_file" ]; then \
				echo "=== Contents of $$log_file ==="; \
				cat "$$log_file"; \
				echo "=== End of $$log_file ==="; \
			fi; \
		done; \
	else \
		echo "✓ No ASAN issues detected"; \
	fi

clean-asan:
	@echo "Cleaning ASAN artifacts..."
	@rm -rf asan_logs
	@$(MAKE) clean

help-asan:
	@echo "ASAN (AddressSanitizer) targets (macOS only):"
	@echo "  build-asan    - Build extension with AddressSanitizer enabled"
	@echo "  test-asan     - Build and run tests with AddressSanitizer"
	@echo "  clean-asan    - Clean ASAN artifacts and build files"
	@echo "  help-asan     - Show this help message"
	@echo ""
	@echo "Manual ASAN build steps (macOS only):"
	@echo "  1. make clean && phpize --clean && phpize"
	@echo "  2. ./configure --enable-valkey-glide --enable-valkey-glide-asan"
	@echo "  3. make"
	@echo "  4. ASAN_OPTIONS='halt_on_error=0:abort_on_error=0:symbolize=1:print_stacktrace=1' \\"
	@echo "     DYLD_INSERT_LIBRARIES=\$$(clang -print-file-name=libclang_rt.asan_osx_dynamic.dylib) \\"
	@echo "     php -n -d extension=\$$(pwd)/modules/valkey_glide.so tests/TestValkeyGlide.php"
	@echo ""
	@echo "Note: ASAN tests are only supported on macOS. Linux support has been removed."

.PHONY: lint lint-c lint-php lint-fix install-build-tools install-lint-tools install-tools build-asan test-asan clean-asan help-asan
