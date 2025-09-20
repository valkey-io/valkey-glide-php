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
  PHP_NEW_EXTENSION(valkey_glide,
    valkey_glide.c valkey_glide_cluster.c cluster_scan_cursor.c command_response.c logger.c valkey_glide_commands.c valkey_glide_commands_2.c valkey_glide_commands_3.c valkey_glide_core_commands.c valkey_glide_core_common.c valkey_glide_expire_commands.c valkey_glide_geo_commands.c valkey_glide_geo_common.c valkey_glide_hash_common.c valkey_glide_list_common.c valkey_glide_s_common.c valkey_glide_str_commands.c valkey_glide_x_commands.c valkey_glide_x_common.c valkey_glide_z.c valkey_glide_z_common.c valkey_z_php_methods.c src/command_request.pb-c.c src/connection_request.pb-c.c src/response.pb-c.c src/client_constructor_mock.c,
    $ext_shared)

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

  dnl Detect PECL vs PIE builds:
  dnl - PECL: Has .submodule-commits file (created specifically for PECL packages)
  dnl - PIE: Has .gitmodules file (full git repository)
  if test -f "$PECL_SOURCE_DIR/.submodule-commits"; then
    AC_MSG_CHECKING([for header generation (PECL build detected - has .submodule-commits)])
    
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
    
    dnl Add common cargo paths to PATH
    export PATH="$HOME/.cargo/bin:/usr/local/cargo/bin:/opt/cargo/bin:$PATH"
    AC_MSG_RESULT([Debug: PATH after cargo=$PATH])
    AC_MSG_RESULT([Debug: cargo after PATH=$(which cargo || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: cbindgen after PATH=$(which cbindgen || echo "NOT FOUND")])
    AC_MSG_RESULT([Debug: protoc-c after PATH=$(which protoc-c || echo "NOT FOUND")])
    
    dnl Final tool check
    if test "$(which cargo)" = "NOT FOUND"; then
      AC_MSG_ERROR([cargo not found - please install Rust])
    fi
    if test "$(which cbindgen)" = "NOT FOUND"; then
      AC_MSG_ERROR([cbindgen not found - please install with: cargo install cbindgen])
    fi
    if test "$(which protoc-c)" = "NOT FOUND"; then
      AC_MSG_ERROR([protoc-c not found - please install protobuf-c-compiler])
    fi
    if test "$(which python3)" = "NOT FOUND"; then
      AC_MSG_ERROR([python3 not found - please install Python 3])
    fi
    
    dnl Work in source directory for PECL builds
    cd "$PECL_SOURCE_DIR"
    AC_MSG_RESULT([Debug: changed to source directory $(pwd)])
    
    dnl For PECL builds, clone submodules using .submodule-commits file
    if test -f ".submodule-commits" && test ! -d "valkey-glide/.git"; then
      AC_MSG_RESULT([cloning submodules from .submodule-commits])
      
      dnl Read the commit hash from .submodule-commits
      SUBMODULE_COMMIT=$(cat .submodule-commits | head -1)
      AC_MSG_RESULT([Debug: submodule commit=$SUBMODULE_COMMIT])
      
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
      cd valkey-glide/ffi && cargo build --release && cbindgen --output ../../include/glide_bindings.h && cd ../.. || AC_MSG_ERROR([Rust build or header generation failed])
    else
      AC_MSG_ERROR([ffi directory not found])
    fi
    
    AC_MSG_RESULT([header generation complete])
    AC_MSG_RESULT([Debug: generated files=$(ls -la include/ src/ 2>/dev/null || echo "none")])
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
