PHP_ARG_ENABLE(valkey_glide, whether to enable Valkey Glide support,
[  --enable-valkey-glide   Enable Valkey Glide support])

PHP_ARG_ENABLE(valkey_glide_asan, whether to enable AddressSanitizer for Valkey Glide,
[  --enable-valkey-glide-asan   Enable AddressSanitizer for debugging (requires clang/gcc with ASAN support)], no, no)

if test "$PHP_VALKEY_GLIDE" != "no"; then

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

  EXTRA_DIST="$EXTRA_DIST valkey_glide.stub.php valkey_glide_cluster.stub.php logger.stub.php"
  AC_SUBST(EXTRA_DIST)
fi

PHP_SUBST(PROTOC)
PHP_SUBST(PROTO_SRC_DIR)
PHP_SUBST(GEN_INCLUDE_DIR)
PHP_SUBST(GEN_SRC_DIR)

PHP_ADD_MAKEFILE_FRAGMENT(Makefile.frag)
