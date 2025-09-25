PHP_ARG_ENABLE(valkey_glide, whether to enable Valkey Glide support,
[  --enable-valkey-glide   Enable Valkey Glide support])

PHP_ARG_ENABLE(valkey_glide_asan, whether to enable AddressSanitizer for Valkey Glide,
[  --enable-valkey-glide-asan   Enable AddressSanitizer for debugging (requires clang/gcc with ASAN support)], no, no)

PHP_ARG_ENABLE(valkey_glide_debug, whether to enable debug mode,
[  --enable-valkey-glide-debug   Enable debug mode], no, no)

PHP_ARG_ENABLE(debug, whether to enable debug mode (alias for valkey-glide-debug),
[  --enable-debug   Enable debug mode (alias for valkey-glide-debug)], no, no)

if test "$PHP_VALKEY_GLIDE" != "no"; then

  AC_MSG_RESULT([=== VALKEY GLIDE CONFIG START ===])

  dnl If --enable-debug is used, set valkey-glide-debug to yes
  if test "$PHP_DEBUG" = "yes"; then
    PHP_VALKEY_GLIDE_DEBUG="yes"
  fi

  dnl Check if ASAN is enabled
  if test "$PHP_VALKEY_GLIDE_ASAN" = "yes"; then
    AC_MSG_CHECKING([for AddressSanitizer support])
    
    dnl Detect platform to skip -fsanitize=address on macOS
    UNAME_S=`uname -s`
    if test "$UNAME_S" = "Darwin"; then
      AC_MSG_RESULT([detected macOS, skipping -fsanitize=address])
      ASAN_CFLAGS="-fno-omit-frame-pointer -g -O1"
      ASAN_LDFLAGS=""
    else
      ASAN_CFLAGS="-fsanitize=address -fno-omit-frame-pointer -g -O1"
      ASAN_LDFLAGS="-fsanitize=address"
    fi

    dnl Test if compiler supports the flags
    old_CFLAGS="$CFLAGS"
    old_LDFLAGS="$LDFLAGS"
    CFLAGS="$CFLAGS $ASAN_CFLAGS"
    LDFLAGS="$LDFLAGS $ASAN_LDFLAGS"
    
    AC_TRY_COMPILE([], [return 0;], [
      AC_MSG_RESULT([yes])
      PHP_VALKEY_GLIDE_CFLAGS="$ASAN_CFLAGS"
      PHP_VALKEY_GLIDE_LDFLAGS="$ASAN_LDFLAGS"
      AC_DEFINE([VALKEY_GLIDE_ASAN_ENABLED], [1], [Define if AddressSanitizer is enabled])
    ], [
      AC_MSG_RESULT([no])
      AC_MSG_ERROR([AddressSanitizer requested but compiler does not support it])
    ])
    
    CFLAGS="$old_CFLAGS"
    LDFLAGS="$old_LDFLAGS"
  else
    PHP_VALKEY_GLIDE_CFLAGS=""
    PHP_VALKEY_GLIDE_LDFLAGS=""
  fi

  dnl Apply the flags to the extension
  if test -n "$PHP_VALKEY_GLIDE_CFLAGS"; then
    CFLAGS="$CFLAGS $PHP_VALKEY_GLIDE_CFLAGS"
  fi
  if test -n "$PHP_VALKEY_GLIDE_LDFLAGS"; then
    LDFLAGS="$LDFLAGS $PHP_VALKEY_GLIDE_LDFLAGS"
  fi
  
  dnl Add protobuf-c library linking
  AC_MSG_CHECKING([for protobuf-c library])
  AC_CHECK_LIB([protobuf-c], [protobuf_c_empty_string], [
    AC_MSG_RESULT([found])
    PHP_ADD_LIBRARY(protobuf-c, 1, VALKEY_GLIDE_SHARED_LIBADD)
  ], [
    AC_MSG_ERROR([protobuf-c library not found. Please install libprotobuf-c-dev])
  ])
  
  dnl Add Rust FFI library linking (file will be built during make)
  AC_MSG_RESULT([Adding Rust FFI library to linking flags])
  VALKEY_GLIDE_SHARED_LIBADD="$VALKEY_GLIDE_SHARED_LIBADD \$(top_builddir)/valkey-glide/ffi/target/release/libglide_ffi.a -lresolv"
  
  PHP_SUBST(VALKEY_GLIDE_SHARED_LIBADD)
  
  PHP_NEW_EXTENSION(valkey_glide,
    valkey_glide.c valkey_glide_cluster.c cluster_scan_cursor.c command_response.c logger.c valkey_glide_commands.c valkey_glide_commands_2.c valkey_glide_commands_3.c valkey_glide_core_commands.c valkey_glide_core_common.c valkey_glide_expire_commands.c valkey_glide_geo_commands.c valkey_glide_geo_common.c valkey_glide_hash_common.c valkey_glide_list_common.c valkey_glide_s_common.c valkey_glide_str_commands.c valkey_glide_x_commands.c valkey_glide_x_common.c valkey_glide_z.c valkey_glide_z_common.c valkey_z_php_methods.c src/command_request.pb-c.c src/connection_request.pb-c.c src/response.pb-c.c src/client_constructor_mock.c,
    $ext_shared,, $VALKEY_GLIDE_SHARED_LIBADD)

  dnl Set protobuf-related variables
  PROTOC="protoc"
  PROTO_SRC_DIR="valkey-glide/glide-core/src/protobuf"
  GEN_INCLUDE_DIR="include/glide"
  GEN_SRC_DIR="src"

  dnl Debug build environment
  AC_MSG_RESULT([Debug: PEAR_INSTALLDIR=$PEAR_INSTALLDIR])
  AC_MSG_RESULT([Debug: PEAR_TEMP_DIR=$PEAR_TEMP_DIR])
  AC_MSG_RESULT([Debug: configure script=$0])
  AC_MSG_RESULT([Debug: current directory=$(pwd)])
  AC_MSG_RESULT([Debug: files in current dir=$(ls -la . | head -10)])
  
  dnl Extract source directory from configure script path
  PECL_SOURCE_DIR=$(dirname "$0")
  AC_MSG_RESULT([Debug: PECL source dir=$PECL_SOURCE_DIR])
  AC_MSG_RESULT([Debug: .submodule-commits in source=$(test -f "$PECL_SOURCE_DIR/.submodule-commits" && echo "yes" || echo "no")])
  AC_MSG_RESULT([Debug: .gitmodules in source=$(test -f "$PECL_SOURCE_DIR/.gitmodules" && echo "yes" || echo "no")])
  AC_MSG_RESULT([Debug: valkey-glide in source=$(test -d "$PECL_SOURCE_DIR/valkey-glide" && echo "yes" || echo "no")])
  AC_MSG_RESULT([Debug: files in source dir=$(ls -la "$PECL_SOURCE_DIR" | head -10)])

  dnl Detect PECL vs PIE builds:
  dnl - PECL: Has .submodule-commits file (created specifically for PECL packages)
  dnl - PIE: Has .gitmodules file (full git repository)
  if test -f "$PECL_SOURCE_DIR/.submodule-commits"; then
    AC_MSG_CHECKING([for header generation (PECL build detected - has .submodule-commits)])
    
    dnl Save current build directory before changing to source
    BUILD_DIR=$(pwd)
    AC_MSG_RESULT([Debug: starting in build directory: $BUILD_DIR])
    
    dnl Search for arginfo.h files in starting directory and subdirectories
    AC_MSG_RESULT([Debug: searching for arginfo.h files in build area])
    ARGINFO_FILES=$(find "$BUILD_DIR" -name "*arginfo.h" 2>/dev/null || echo "none")
    AC_MSG_RESULT([Debug: arginfo files found: $ARGINFO_FILES])
    
    dnl Also search in parent directories in case they're elsewhere
    PARENT_DIR=$(dirname "$BUILD_DIR")
    AC_MSG_RESULT([Debug: searching parent directory: $PARENT_DIR])
    PARENT_ARGINFO=$(find "$PARENT_DIR" -name "*arginfo.h" 2>/dev/null | head -5 || echo "none")
    AC_MSG_RESULT([Debug: arginfo files in parent: $PARENT_ARGINFO])
    
    dnl Debug tool availability
    AC_MSG_RESULT([Debug: git=$(which git || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: git path=$(command -v git || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: git version=$(git --version 2>/dev/null || echo "FAILED")])
    AC_MSG_RESULT([Debug: cargo=$(which cargo || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: cbindgen=$(which cbindgen || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: protoc-c=$(which protoc-c || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: python3=$(which python3 || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: HOME=$HOME])
    AC_MSG_RESULT([Debug: USER=$USER])
    
    dnl Try to find and source cargo environment
    if test -f "$HOME/.cargo/env"; then
      AC_MSG_RESULT([Debug: sourcing $HOME/.cargo/env])
      . "$HOME/.cargo/env"
    fi
    
    dnl Find cargo, cbindgen, and rustc, create symlinks if needed
    CARGO_PATH=$(which cargo 2>/dev/null || echo "")
    CBINDGEN_PATH=$(which cbindgen 2>/dev/null || echo "")
    RUSTC_PATH=$(which rustc 2>/dev/null || echo "")
    
    dnl Search for cargo in common locations if not in PATH
    if test -z "$CARGO_PATH"; then
      AC_MSG_RESULT([Debug: searching for cargo with find])
      CARGO_PATH=$(find /usr/bin /usr/local/bin ~/.cargo/bin /home -name "cargo" -type f -executable 2>/dev/null | head -1)
      if test -n "$CARGO_PATH"; then
        AC_MSG_RESULT([Debug: found cargo at $CARGO_PATH])
      fi
    fi
    
    dnl Search for cbindgen in common locations if not in PATH
    if test -z "$CBINDGEN_PATH"; then
      AC_MSG_RESULT([Debug: searching for cbindgen with find])
      CBINDGEN_PATH=$(find /usr/bin /usr/local/bin ~/.cargo/bin /home -name "cbindgen" -type f -executable 2>/dev/null | head -1)
      if test -n "$CBINDGEN_PATH"; then
        AC_MSG_RESULT([Debug: found cbindgen at $CBINDGEN_PATH])
      fi
    fi
    
    dnl Search for rustc in common locations if not in PATH
    if test -z "$RUSTC_PATH"; then
      AC_MSG_RESULT([Debug: searching for rustc with find])
      RUSTC_PATH=$(find /usr/bin /usr/local/bin ~/.cargo/bin /home -name "rustc" -type f -executable 2>/dev/null | head -1)
      if test -n "$RUSTC_PATH"; then
        AC_MSG_RESULT([Debug: found rustc at $RUSTC_PATH])
      fi
    fi
    
    dnl Work in source directory for PECL builds
    cd "$PECL_SOURCE_DIR"
    AC_MSG_RESULT([Debug: changed to source directory $(pwd)])
    
    dnl Generate arginfo.h files from .stub.php files during configure
    AC_MSG_RESULT([Debug: generating arginfo.h files from .stub.php files])
    
    dnl Use php-config to find the correct paths
    AC_MSG_RESULT([Debug: using php-config to find gen_stub.php])
    if command -v php-config >/dev/null 2>&1; then
      PHP_PREFIX=$(php-config --prefix)
      PHP_INCLUDE_DIR=$(php-config --include-dir)
      AC_MSG_RESULT([Debug: PHP prefix: $PHP_PREFIX])
      AC_MSG_RESULT([Debug: PHP include dir: $PHP_INCLUDE_DIR])
      
      dnl Try locations based on php-config, focusing on build directories
      for location in "$PHP_PREFIX/lib/php/build/gen_stub.php" "$PHP_PREFIX/share/php/build/gen_stub.php" "$PHP_PREFIX/build/gen_stub.php" "$PHP_INCLUDE_DIR/scripts/gen_stub.php" "$PHP_INCLUDE_DIR/../build/gen_stub.php" "$PHP_INCLUDE_DIR/build/gen_stub.php"; do
        AC_MSG_RESULT([Debug: checking $location])
        if test -f "$location"; then
          GEN_STUB_PHP="$location"
          AC_MSG_RESULT([Debug: found gen_stub.php at $location])
          break
        fi
      done
    else
      AC_MSG_RESULT([Debug: php-config not found])
    fi
    
    dnl Fallback to common build directory locations
    if test -z "$GEN_STUB_PHP"; then
      AC_MSG_RESULT([Debug: trying fallback build directory locations])
      for location in "build/gen_stub.php" "./build/gen_stub.php" "/usr/share/php/build/gen_stub.php" "/usr/lib/php/build/gen_stub.php" "/usr/local/lib/php/build/gen_stub.php" "/usr/build/gen_stub.php" "/usr/local/build/gen_stub.php" "/root/build/gen_stub.php" "/build/gen_stub.php" "$HOME/build/gen_stub.php"; do
        AC_MSG_RESULT([Debug: checking fallback $location])
        if test -f "$location"; then
          GEN_STUB_PHP="$location"
          AC_MSG_RESULT([Debug: found gen_stub.php at fallback $location])
          break
        fi
      done
    fi
    
    if test -n "$GEN_STUB_PHP"; then
      AC_MSG_RESULT([Debug: using gen_stub.php at $GEN_STUB_PHP])
      for stub_file in *.stub.php src/*.stub.php; do
        if test -f "$stub_file"; then
          arginfo_file="${stub_file%.stub.php}_arginfo.h"
          AC_MSG_RESULT([Debug: generating $arginfo_file from $stub_file])
          php -f "$GEN_STUB_PHP" "$stub_file" 2>&1 || AC_MSG_RESULT([Debug: failed to generate $arginfo_file from $stub_file])
          if test -f "$arginfo_file"; then
            AC_MSG_RESULT([Debug: successfully generated $arginfo_file])
          else
            AC_MSG_RESULT([Debug: $arginfo_file was not created])
          fi
        fi
      done
    else
      AC_MSG_RESULT([Debug: gen_stub.php not found anywhere, arginfo.h files will be generated during make])
    fi
    
    dnl Check for .stub.php files to confirm they exist
    AC_MSG_RESULT([Debug: stub files in source=$(ls -la *.stub.php 2>/dev/null || echo "none")])
    
    dnl Debug where glide_bindings.h exists - this is the real issue
    AC_MSG_RESULT([Debug: searching for all glide_bindings.h files in build area])
    find /tmp/pear -name "glide_bindings.h" 2>/dev/null | while read file; do
      AC_MSG_RESULT([Debug: found glide_bindings.h at: $file])
      AC_MSG_RESULT([Debug: copying $file to /tmp/debug_glide_bindings_configure.h])
      cp "$file" /tmp/debug_glide_bindings_configure.h 2>/dev/null || true
    done
    
    AC_MSG_RESULT([Debug: checking local glide_bindings.h locations])
    test -f "include/glide_bindings.h" && AC_MSG_RESULT([Debug: include/glide_bindings.h EXISTS]) || AC_MSG_RESULT([Debug: include/glide_bindings.h does not exist])
    test -f "glide_bindings.h" && AC_MSG_RESULT([Debug: ./glide_bindings.h EXISTS]) || AC_MSG_RESULT([Debug: ./glide_bindings.h does not exist])
    
    if test -f "include/glide_bindings.h"; then
      AC_MSG_RESULT([Debug: copying include/glide_bindings.h to /tmp for inspection])
      cp include/glide_bindings.h /tmp/debug_glide_bindings_configure.h 2>/dev/null || true
    fi
    
    dnl Search more broadly for arginfo.h files that might already exist
    AC_MSG_RESULT([Debug: searching entire temp area for arginfo files])
    TEMP_ARGINFO=$(find /tmp/pear/temp -name "*arginfo.h" 2>/dev/null | head -10 || echo "none")
    AC_MSG_RESULT([Debug: arginfo files in temp area: $TEMP_ARGINFO])
    
    dnl Create symlinks in source directory where they'll be used
    if test -n "$CARGO_PATH" && test ! -x "./cargo"; then
      AC_MSG_RESULT([Debug: creating cargo symlink from $CARGO_PATH])
      ln -sf "$CARGO_PATH" ./cargo
    fi
    
    if test -n "$CBINDGEN_PATH" && test ! -x "./cbindgen"; then
      AC_MSG_RESULT([Debug: creating cbindgen symlink from $CBINDGEN_PATH])
      ln -sf "$CBINDGEN_PATH" ./cbindgen
    fi
    
    if test -n "$RUSTC_PATH" && test ! -x "./rustc"; then
      AC_MSG_RESULT([Debug: creating rustc symlink from $RUSTC_PATH])
      ln -sf "$RUSTC_PATH" ./rustc
    fi
    
    AC_MSG_RESULT([Debug: cargo available=$(test -x "./cargo" && echo "yes" || echo "no")])
    AC_MSG_RESULT([Debug: cbindgen available=$(test -x "./cbindgen" && echo "yes" || echo "no")])
    AC_MSG_RESULT([Debug: rustc available=$(test -x "./rustc" && echo "yes" || echo "no")])
    AC_MSG_RESULT([Debug: protoc-c available=$(which protoc-c || echo "NOT FOUND")])
    
    AC_MSG_RESULT([Debug: cargo available=$(test -x "./cargo" && echo "yes" || echo "no")])
    AC_MSG_RESULT([Debug: cbindgen available=$(test -x "./cbindgen" && echo "yes" || echo "no")])
    AC_MSG_RESULT([Debug: protoc-c available=$(which protoc-c || echo "NOT FOUND")])
    
    dnl Final tool check using symlinks in source directory
    if test "$(which git)" = "NOT FOUND"; then
      AC_MSG_ERROR([git not found - please install git])
    fi
    if test ! -x "./cargo"; then
      AC_MSG_ERROR([cargo not found - please install Rust and ensure cargo is accessible])
    fi
    if test ! -x "./cbindgen"; then
      AC_MSG_ERROR([cbindgen not found - please install with: cargo install cbindgen])
    fi
    if test "$(which protoc-c)" = "NOT FOUND"; then
      AC_MSG_ERROR([protoc-c not found - please install protobuf-c-compiler])
    fi
    if test "$(which python3)" = "NOT FOUND"; then
      AC_MSG_ERROR([python3 not found - please install Python 3])
    fi
    
    dnl For PECL builds, handle submodules using .submodule-commits file
    if test -f ".submodule-commits" && test ! -d "valkey-glide/.git"; then
      AC_MSG_RESULT([cloning submodules from .submodule-commits])
      
      dnl Read and parse the commit hash from .submodule-commits (format: valkey-glide=HASH)
      SUBMODULE_LINE=$(cat .submodule-commits | head -1)
      SUBMODULE_COMMIT=$(echo "$SUBMODULE_LINE" | cut -d'=' -f2)
      AC_MSG_RESULT([Debug: submodule line=$SUBMODULE_LINE])
      AC_MSG_RESULT([Debug: parsed commit=$SUBMODULE_COMMIT])
      
      dnl Remove existing directory if it exists but isn't a git repo
      if test -d "valkey-glide"; then
        AC_MSG_RESULT([removing existing valkey-glide directory])
        rm -rf valkey-glide
      fi
      
      dnl Clone the submodule at the specific commit
      git clone --depth 1 https://github.com/valkey-io/valkey-glide.git valkey-glide || AC_MSG_ERROR([Failed to clone valkey-glide])
      cd valkey-glide
      git fetch --depth 1 origin "$SUBMODULE_COMMIT" || AC_MSG_ERROR([Failed to fetch commit $SUBMODULE_COMMIT])
      git checkout "$SUBMODULE_COMMIT" || AC_MSG_ERROR([Failed to checkout commit $SUBMODULE_COMMIT])
      cd ..
    else
      AC_MSG_RESULT([submodules already exist or no .submodule-commits])
    fi
    
    dnl Generate protobuf files
    mkdir -p include/glide src
    
    dnl Run Python script to modify proto files (like in Makefile.frag)
    if test -f "utils/remove_optional_from_proto.py"; then
      AC_MSG_RESULT([running proto modification script])
      python3 utils/remove_optional_from_proto.py || AC_MSG_RESULT([proto script failed, continuing])
    fi
    
    if test -d "valkey-glide/glide-core/src/protobuf"; then
      AC_MSG_RESULT([generating protobuf files])
      for proto in valkey-glide/glide-core/src/protobuf/*.proto; do
        if test -f "$proto"; then
          AC_MSG_RESULT([processing $proto])
          protoc-c --c_out=src --proto_path=valkey-glide/glide-core/src/protobuf "$proto" || AC_MSG_ERROR([Failed to generate protobuf])
        fi
      done
      cp src/*.pb-c.h include/glide/ 2>/dev/null || true
    else
      AC_MSG_ERROR([protobuf directory not found])
    fi
    
    dnl Generate main header
    if test -d "valkey-glide/ffi"; then
      AC_MSG_RESULT([building rust and generating header])
      AC_MSG_RESULT([Debug: checking for cargo symlink])
      if test -x "./cargo"; then
        AC_MSG_RESULT([Debug: cargo symlink exists])
      else
        AC_MSG_RESULT([Debug: cargo symlink missing])
      fi
      if test -x "./cbindgen"; then
        AC_MSG_RESULT([Debug: cbindgen symlink exists])
      else
        AC_MSG_RESULT([Debug: cbindgen symlink missing])
      fi
      
      dnl Set up Rust environment variables for cargo
      CARGO_DIR=$(dirname "$CARGO_PATH")
      RUST_TOOLCHAIN_DIR=$(dirname "$CARGO_DIR")
      AC_MSG_RESULT([Debug: CARGO_HOME=$RUST_TOOLCHAIN_DIR])
      AC_MSG_RESULT([Debug: CARGO_DIR=$CARGO_DIR])
      
      cd valkey-glide/ffi && CARGO_HOME="$RUST_TOOLCHAIN_DIR" PATH="$CARGO_DIR:$PATH" ../../cargo build --release && CARGO_HOME="$RUST_TOOLCHAIN_DIR" PATH="$CARGO_DIR:$PATH" ../../cbindgen --output ../../include/glide_bindings.h && cd ../.. || AC_MSG_ERROR([Rust build or header generation failed])
      
      dnl Add include guards to the generated header immediately
      if test -f "include/glide_bindings.h"; then
        AC_MSG_RESULT([Debug: adding include guards to glide_bindings.h in configure])
        echo '#ifndef GLIDE_BINDINGS_H' > include/glide_bindings_tmp.h
        echo '#define GLIDE_BINDINGS_H' >> include/glide_bindings_tmp.h
        cat include/glide_bindings.h >> include/glide_bindings_tmp.h
        echo '#endif /* GLIDE_BINDINGS_H */' >> include/glide_bindings_tmp.h
        rm include/glide_bindings.h
        mv include/glide_bindings_tmp.h include/glide_bindings.h
        AC_MSG_RESULT([Debug: include guards added to glide_bindings.h])
      fi
    else
      AC_MSG_ERROR([ffi directory not found])
    fi
    
    AC_MSG_RESULT([header generation complete])
    AC_MSG_RESULT([Debug: generated files=$(ls -la include/ src/ 2>/dev/null || echo "none")])
    AC_MSG_RESULT([Debug: current directory after generation: $(pwd)])
    
    dnl Copy generated files back to build directory for PECL make
    if test "$BUILD_DIR" != "$PECL_SOURCE_DIR"; then
      AC_MSG_RESULT([Debug: copying generated files from source to build directory])
      AC_MSG_RESULT([Debug: source dir=$PECL_SOURCE_DIR])
      AC_MSG_RESULT([Debug: build dir=$BUILD_DIR])
      
      dnl Check what arginfo.h files exist in source
      AC_MSG_RESULT([Debug: arginfo files in source=$(ls -la "$PECL_SOURCE_DIR"/*arginfo.h 2>/dev/null || echo "none")])
      
      cp -r "$PECL_SOURCE_DIR/include" "$BUILD_DIR/" 2>/dev/null || true
      cp -r "$PECL_SOURCE_DIR/src" "$BUILD_DIR/" 2>/dev/null || true
      cp -r "$PECL_SOURCE_DIR/valkey-glide" "$BUILD_DIR/" 2>/dev/null || true
      
      dnl Copy arginfo.h files explicitly
      for arginfo_file in cluster_scan_cursor_arginfo.h valkey_glide_arginfo.h valkey_glide_cluster_arginfo.h logger_arginfo.h; do
        if test -f "$PECL_SOURCE_DIR/$arginfo_file"; then
          AC_MSG_RESULT([Debug: copying $arginfo_file])
          cp "$PECL_SOURCE_DIR/$arginfo_file" "$BUILD_DIR/"
        else
          AC_MSG_RESULT([Debug: $arginfo_file not found in source])
        fi
      done
      
      dnl Check what was copied to build directory
      AC_MSG_RESULT([Debug: arginfo files in build=$(ls -la "$BUILD_DIR"/*arginfo.h 2>/dev/null || echo "none")])
      
      AC_MSG_RESULT([Debug: files copied to build dir])
    fi
    
    dnl Return to build directory where PECL expects to run make
    cd "$BUILD_DIR"
    AC_MSG_RESULT([Debug: returned to build directory: $(pwd)])
    AC_MSG_RESULT([Debug: files in build dir=$(ls -la . | head -10)])
  else
    AC_MSG_RESULT([PIE build detected - headers will be generated via Makefile (no .submodule-commits)])
  fi

  EXTRA_DIST="$EXTRA_DIST valkey_glide.stub.php valkey_glide_cluster.stub.php logger.stub.php"
  AC_SUBST(EXTRA_DIST)
fi

PHP_SUBST(PROTOC)
PHP_SUBST(PROTO_SRC_DIR)
PHP_SUBST(GEN_INCLUDE_DIR)
PHP_SUBST(GEN_SRC_DIR)

PHP_ADD_MAKEFILE_FRAGMENT(Makefile.frag)
