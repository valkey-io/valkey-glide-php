/*
  +----------------------------------------------------------------------+
  +----------------------------------------------------------------------+
  | Copyright (c) 2023-2025 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_response.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"

/* Helper functions for batch state management */
static void clear_batch_state(valkey_glide_object* valkey_glide);
static int  buffer_command_for_batch(valkey_glide_object* valkey_glide,
                                     enum RequestType     cmd_type,
                                     uint8_t**            args,
                                     uintptr_t*           arg_lengths,
                                     uintptr_t            arg_count,
                                     const char*          key,
                                     size_t               key_len);
static void expand_command_buffer(valkey_glide_object* valkey_glide);
int         buffer_current_command_generic(valkey_glide_object* valkey_glide,
                                           enum RequestType     request_type,
                                           int                  argc,
                                           zval*                this_ptr);

/* Helper function to process array arguments for FCALL commands */
static void process_array_to_args(zval*          array,
                                  uintptr_t*     cmd_args,
                                  unsigned long* args_len,
                                  unsigned long* arg_index);

/* Execute a WAIT command using the Valkey Glide client - MIGRATED TO CORE FRAMEWORK */
int execute_wait_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 numreplicas, timeout;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Oll", &object, ce, &numreplicas, &timeout) ==
        FAILURE) {
        return 0;
    }
    if (numreplicas < 0) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Wait;

        /* WAIT is a server-level command (not key-based) with 2 arguments: numreplicas, timeout */
        args.args[0].type                = CORE_ARG_TYPE_LONG;
        args.args[0].data.long_arg.value = numreplicas;

        args.args[1].type                = CORE_ARG_TYPE_LONG;
        args.args[1].data.long_arg.value = timeout;
        args.arg_count                   = 2;

        long result_value;
        if (execute_core_command(&args, &result_value, process_core_int_result)) {
            /* Return the number of replicas that acknowledged the write */
            ZVAL_LONG(return_value, result_value);
            return 1;
        }
    }

    return 0;
}

/* Helper function implementations */

/* Clear batch state and free buffered commands */
static void clear_batch_state(valkey_glide_object* valkey_glide) {
    if (!valkey_glide) {
        return;
    }

    valkey_glide->is_in_batch_mode = false;
    valkey_glide->batch_type       = MULTI;
    valkey_glide->command_count    = 0;

    if (valkey_glide->buffered_commands) {
        /* Free each buffered command */
        size_t i, j;
        for (i = 0; i < valkey_glide->command_count; i++) {
            struct batch_command* cmd = &valkey_glide->buffered_commands[i];

            /* Free argument arrays */
            if (cmd->args) {
                for (j = 0; j < cmd->arg_count; j++) {
                    if (cmd->args[j]) {
                        efree(cmd->args[j]);
                    }
                }
                efree(cmd->args);
            }

            if (cmd->arg_lengths) {
                efree(cmd->arg_lengths);
            }

            if (cmd->key) {
                efree(cmd->key);
            }

            if (cmd->route_info) {
                efree(cmd->route_info);
            }
        }

        efree(valkey_glide->buffered_commands);
        valkey_glide->buffered_commands = NULL;
        valkey_glide->command_capacity  = 0;
    }
}

/* Expand command buffer capacity */
static void expand_command_buffer(valkey_glide_object* valkey_glide) {
    if (!valkey_glide) {
        return;
    }

    size_t new_capacity = valkey_glide->command_capacity * 2;
    if (new_capacity == 0) {
        new_capacity = 16;
    }

    struct batch_command* new_buffer = (struct batch_command*) erealloc(
        valkey_glide->buffered_commands, new_capacity * sizeof(struct batch_command));

    if (new_buffer) {
        valkey_glide->buffered_commands = new_buffer;
        valkey_glide->command_capacity  = new_capacity;

        /* Initialize new slots */
        size_t i;
        for (i = valkey_glide->command_count; i < new_capacity; i++) {
            memset(&valkey_glide->buffered_commands[i], 0, sizeof(struct batch_command));
        }
    }
}

/* Buffer a command for batch execution */
static int buffer_command_for_batch(valkey_glide_object* valkey_glide,
                                    enum RequestType     cmd_type,
                                    uint8_t**            args,
                                    uintptr_t*           arg_lengths,
                                    uintptr_t            arg_count,
                                    const char*          key,
                                    size_t               key_len) {
    if (!valkey_glide || !valkey_glide->is_in_batch_mode) {
        return 0;
    }

    /* Expand buffer if needed */
    if (valkey_glide->command_count >= valkey_glide->command_capacity) {
        expand_command_buffer(valkey_glide);
        if (valkey_glide->command_count >= valkey_glide->command_capacity) {
            return 0; /* Failed to expand */
        }
    }

    struct batch_command* cmd = &valkey_glide->buffered_commands[valkey_glide->command_count];

    /* Store command details */
    cmd->request_type = cmd_type;
    cmd->arg_count    = arg_count;

    /* Copy arguments */
    if (arg_count > 0 && args && arg_lengths) {
        cmd->args        = (uint8_t**) emalloc(arg_count * sizeof(uint8_t*));
        cmd->arg_lengths = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));

        if (!cmd->args || !cmd->arg_lengths) {
            if (cmd->args)
                efree(cmd->args);
            if (cmd->arg_lengths)
                efree(cmd->arg_lengths);
            return 0;
        }

        uintptr_t i;
        for (i = 0; i < arg_count; i++) {
            if (args[i] && arg_lengths[i] > 0) {
                cmd->args[i] = (uint8_t*) emalloc(arg_lengths[i] + 1);
                if (cmd->args[i]) {
                    memcpy(cmd->args[i], args[i], arg_lengths[i]);
                    cmd->args[i][arg_lengths[i]] = '\0';
                    cmd->arg_lengths[i]          = arg_lengths[i];
                } else {
                    cmd->arg_lengths[i] = 0;
                }
            } else {
                cmd->args[i]        = NULL;
                cmd->arg_lengths[i] = 0;
            }
        }
    } else {
        cmd->args        = NULL;
        cmd->arg_lengths = NULL;
    }

    /* Copy key if provided */
    if (key && key_len > 0) {
        cmd->key = (char*) emalloc(key_len + 1);
        if (cmd->key) {
            memcpy(cmd->key, key, key_len);
            cmd->key[key_len] = '\0';
            cmd->key_len      = key_len;
        } else {
            cmd->key_len = 0;
        }
    } else {
        cmd->key     = NULL;
        cmd->key_len = 0;
    }

    cmd->route_info = NULL; /* TODO: Handle routing info if needed */

    valkey_glide->command_count++;
    return 1;
}

/* Generic command buffering function for batch execution */
int buffer_current_command_generic(valkey_glide_object* valkey_glide,
                                   enum RequestType     request_type,
                                   int                  argc,
                                   zval*                this_ptr) {
    (void) this_ptr; /* Unused parameter */

    if (!valkey_glide || !valkey_glide->is_in_batch_mode) {
        return 0;
    }

    /* Use modern parameter parsing API */
    zval* args      = NULL;
    int   arg_count = 0;

    /* Parse all arguments using variadic syntax */
    if (zend_parse_parameters(argc, "*", &args, &arg_count) == FAILURE) {
        return 0;
    }

    /* Convert PHP zvals to FFI-compatible format */
    uint8_t**  cmd_args    = NULL;
    uintptr_t* arg_lengths = NULL;

    if (arg_count > 0 && args) {
        cmd_args    = (uint8_t**) emalloc(arg_count * sizeof(uint8_t*));
        arg_lengths = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));

        if (!cmd_args || !arg_lengths) {
            if (cmd_args)
                efree(cmd_args);
            if (arg_lengths)
                efree(arg_lengths);
            return 0;
        }

        int i;
        for (i = 0; i < arg_count; i++) {
            /* Convert each argument to string */
            zval temp_zval;
            ZVAL_COPY(&temp_zval, &args[i]);
            convert_to_string(&temp_zval);

            /* Allocate and copy the string data immediately */
            size_t str_len = Z_STRLEN(temp_zval);
            cmd_args[i]    = (uint8_t*) emalloc(str_len + 1);
            if (cmd_args[i]) {
                memcpy(cmd_args[i], Z_STRVAL(temp_zval), str_len);
                cmd_args[i][str_len] = '\0';
                arg_lengths[i]       = str_len;
            } else {
                arg_lengths[i] = 0;
            }

            zval_dtor(&temp_zval);
        }
    }

    /* Buffer the command */
    int result = buffer_command_for_batch(
        valkey_glide, request_type, cmd_args, arg_lengths, arg_count, NULL, 0);

    /* Cleanup */
    if (cmd_args)
        efree(cmd_args);
    if (arg_lengths)
        efree(arg_lengths);

    return result;
}

/* Helper function to process array arguments for FCALL commands */
static void process_array_to_args(zval*          array,
                                  uintptr_t*     cmd_args,
                                  unsigned long* args_len,
                                  unsigned long* arg_index) {
    if (array && Z_TYPE_P(array) == IS_ARRAY) {
        zval* val;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), val) {
            if (Z_TYPE_P(val) != IS_STRING) {
                zval temp;
                ZVAL_COPY(&temp, val);
                convert_to_string(&temp);
                cmd_args[*arg_index] = (uintptr_t) Z_STRVAL(temp);
                args_len[*arg_index] = Z_STRLEN(temp);
                zval_dtor(&temp);
            } else {
                cmd_args[*arg_index] = (uintptr_t) Z_STRVAL_P(val);
                args_len[*arg_index] = Z_STRLEN_P(val);
            }
            (*arg_index)++;
        }
        ZEND_HASH_FOREACH_END();
    }
}

/* Execute a FUNCTION command using the Valkey Glide client */
int execute_function_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args;
    int                  args_count;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &z_args, &args_count) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Check if args are valid */
        if (!z_args || args_count <= 0) {
            return 0;
        }

        /* Prepare command arguments */
        unsigned long  arg_count = args_count;
        uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
        unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

        if (!cmd_args || !args_len) {
            if (cmd_args)
                efree(cmd_args);
            if (args_len)
                efree(args_len);
            return 0;
        }

        /* Convert arguments to strings if needed */
        int i;
        for (i = 0; i < args_count; i++) {
            zval* arg = &z_args[i];

            /* If not string, convert to one */
            if (Z_TYPE_P(arg) != IS_STRING) {
                zval temp;
                ZVAL_COPY(&temp, arg);
                convert_to_string(&temp);
                cmd_args[i] = (uintptr_t) Z_STRVAL(temp);
                args_len[i] = Z_STRLEN(temp);
                zval_dtor(&temp);
            } else {
                cmd_args[i] = (uintptr_t) Z_STRVAL_P(arg);
                args_len[i] = Z_STRLEN_P(arg);
            }
        }

        /* Set the first argument as "FUNCTION" */
        const char*    function_cmd = "FUNCTION";
        uintptr_t*     final_args   = (uintptr_t*) emalloc((arg_count + 1) * sizeof(uintptr_t));
        unsigned long* final_args_len =
            (unsigned long*) emalloc((arg_count + 1) * sizeof(unsigned long));

        if (!final_args || !final_args_len) {
            if (cmd_args)
                efree(cmd_args);
            if (args_len)
                efree(args_len);
            if (final_args)
                efree(final_args);
            if (final_args_len)
                efree(final_args_len);
            return 0;
        }

        final_args[0]     = (uintptr_t) function_cmd;
        final_args_len[0] = strlen(function_cmd);

        /* Copy the rest of the arguments */
        for (i = 0; i < arg_count; i++) {
            final_args[i + 1]     = cmd_args[i];
            final_args_len[i + 1] = args_len[i];
        }

        /* Execute the command */
        CommandResult* result =
            execute_command(valkey_glide->glide_client,
                            CustomCommand, /* FUNCTION commands use custom command type */
                            arg_count + 1, /* FUNCTION command + args */
                            final_args,    /* arguments */
                            final_args_len /* argument lengths */
            );

        /* Free the argument arrays */
        efree(cmd_args);
        efree(args_len);
        efree(final_args);
        efree(final_args_len);

        /* Handle the result directly */
        int status = 0;
        if (result) {
            if (result->command_error) {
                /* Command failed */
                free_command_result(result);
                return 0;
            }

            if (result->response) {
                /* FUNCTION can return various types based on subcommand */
                status = command_response_to_zval(result->response,
                                                  return_value,
                                                  COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP_FUNCTION,
                                                  true);
                free_command_result(result);
                return status;
            }
            free_command_result(result);
        }
    }

    return 0;
}

/* Execute a MULTI command using the Valkey Glide client - UPDATED FOR BUFFERING */
int execute_multi_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 batch_type = MULTI; /* Default to MULTI */

    /* Parse optional batch type parameter */
    if (zend_parse_method_parameters(argc, object, "O|l", &object, ce, &batch_type) == FAILURE) {
        return 0;
    }

    /* Validate batch type using existing constants */
    if (batch_type != MULTI && batch_type != PIPELINE && batch_type != ATOMIC) {
        php_error_docref(NULL, E_WARNING, "Invalid batch type. Use MULTI, PIPELINE, or ATOMIC");
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Initialize batch mode */
    valkey_glide->is_in_batch_mode = true;
    valkey_glide->batch_type       = (int) batch_type;
    valkey_glide->command_count    = 0;

    /* Initialize buffer if needed */
    if (!valkey_glide->buffered_commands) {
        valkey_glide->command_capacity  = 16; /* Initial capacity */
        valkey_glide->buffered_commands = (struct batch_command*) ecalloc(
            valkey_glide->command_capacity, sizeof(struct batch_command));
    }

    /* Return $this for method chaining */
    ZVAL_COPY(return_value, object);
    return 1;
}

/* Execute a DISCARD command using the Valkey Glide client - UPDATED FOR BUFFERING */
int execute_discard_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Clear batch state if we're in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        clear_batch_state(valkey_glide);
        ZVAL_TRUE(return_value);
        return 1;
    }

    /* If not in batch mode, execute normal DISCARD command */
    core_command_args_t args = {0};
    args.glide_client        = valkey_glide->glide_client;
    args.cmd_type            = Discard;

    if (execute_core_command(&args, NULL, process_core_bool_result)) {
        ZVAL_TRUE(return_value);
        return 1;
    }

    return 0;
}

/* Execute an EXEC command using the Valkey Glide client - UPDATED FOR BUFFERING */
int execute_exec_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Check if we're in batch mode and have buffered commands */
    if (!valkey_glide->is_in_batch_mode || valkey_glide->command_count == 0) {
        ZVAL_FALSE(return_value);
        return 0;
    }

    /* Convert buffered commands to FFI BatchInfo structure */
    struct CmdInfo** cmd_infos =
        (struct CmdInfo**) emalloc(valkey_glide->command_count * sizeof(struct CmdInfo*));
    if (!cmd_infos) {
        clear_batch_state(valkey_glide);
        ZVAL_FALSE(return_value);
        return 0;
    }

    /* Create CmdInfo structures for each buffered command */
    size_t i;
    for (i = 0; i < valkey_glide->command_count; i++) {
        struct batch_command* buffered = &valkey_glide->buffered_commands[i];
        struct CmdInfo*       cmd_info = (struct CmdInfo*) emalloc(sizeof(struct CmdInfo));

        if (!cmd_info) {
            /* Cleanup on error */
            for (size_t j = 0; j < i; j++) {
                efree(cmd_infos[j]);
            }
            efree(cmd_infos);
            clear_batch_state(valkey_glide);
            ZVAL_FALSE(return_value);
            return 0;
        }

        cmd_info->request_type = buffered->request_type;
        cmd_info->args         = (const uint8_t* const*) buffered->args;
        cmd_info->arg_count    = buffered->arg_count;
        cmd_info->args_len     = (const uintptr_t*) buffered->arg_lengths;

        cmd_infos[i] = cmd_info;
    }

    /* Create BatchInfo structure */
    struct BatchInfo batch_info = {
        .cmd_count = valkey_glide->command_count,
        .cmds      = (const struct CmdInfo* const*) cmd_infos,
        .is_atomic = (valkey_glide->batch_type == MULTI || valkey_glide->batch_type == ATOMIC)};

    /* Execute via FFI batch() function */
    struct CommandResult* result = batch(valkey_glide->glide_client,
                                         0, /* callback_index (not used for sync) */
                                         &batch_info,
                                         false, /* raise_on_error */
                                         NULL,  /* options */
                                         0      /* span_ptr */
    );

    /* Free CmdInfo structures */
    for (i = 0; i < valkey_glide->command_count; i++) {
        efree(cmd_infos[i]);
    }
    efree(cmd_infos);

    /* Process results and clear batch state */
    int status = 0;
    if (result) {
        if (result->command_error) {
            /* Command failed */
            free_command_result(result);
            clear_batch_state(valkey_glide);
            ZVAL_FALSE(return_value);
            return 0;
        }

        if (result->response) {
            status = command_response_to_zval(
                result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
            free_command_result(result);
            clear_batch_state(valkey_glide);
            return status ? 1 : 0;
        }
        free_command_result(result);
    }

    clear_batch_state(valkey_glide);
    ZVAL_FALSE(return_value);
    return 0;
}

/* Internal function to execute FCALL/FCALL_RO commands using the Valkey Glide client */
static int execute_fcall_command_internal(const void*      glide_client,
                                          char*            name,
                                          size_t           name_len,
                                          zval*            keys_array,
                                          zval*            args_array,
                                          enum RequestType command_type,
                                          zval*            return_value) {
    /* Check if name is valid */
    if (!name || name_len <= 0) {
        return 0;
    }

    /* Calculate numkeys from keys array */
    long numkeys = 0;
    if (keys_array && Z_TYPE_P(keys_array) == IS_ARRAY) {
        numkeys = zend_hash_num_elements(Z_ARRVAL_P(keys_array));
    }

    /* Calculate args count from args array */
    long args_count = 0;
    if (args_array && Z_TYPE_P(args_array) == IS_ARRAY) {
        args_count = zend_hash_num_elements(Z_ARRVAL_P(args_array));
    }

    /* Prepare numkeys as string */
    char numkeys_str[32];
    snprintf(numkeys_str, sizeof(numkeys_str), "%ld", numkeys);

    /* Calculate total arguments: function_name + numkeys + keys + args */
    unsigned long  arg_count = 2 + numkeys + args_count;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!cmd_args || !args_len) {
        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);
        return 0;
    }

    /* Set function name and numkeys */
    cmd_args[0] = (uintptr_t) name;
    args_len[0] = name_len;
    cmd_args[1] = (uintptr_t) numkeys_str;
    args_len[1] = strlen(numkeys_str);

    unsigned long arg_index = 2;

    /* Process keys array */
    process_array_to_args(keys_array, cmd_args, args_len, &arg_index);

    /* Process args array */
    process_array_to_args(args_array, cmd_args, args_len, &arg_index);

    /* Execute the command */
    CommandResult* result = execute_command(glide_client,
                                            command_type, /* FCall or FCallReadOnly */
                                            arg_count,    /* number of arguments */
                                            cmd_args,     /* arguments */
                                            args_len      /* argument lengths */
    );

    /* Free the argument arrays */
    efree(cmd_args);
    efree(args_len);

    /* Handle the result directly */
    int status = 0;
    if (result) {
        if (result->command_error) {
            /* Command failed */
            free_command_result(result);
            return 0;
        }

        if (result->response) {
            /* FCALL can return various types */
            status = command_response_to_zval(
                result->response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
            free_command_result(result);
            return status;
        }
        free_command_result(result);
    }

    return 0;
}

/* Execute an FCALL command using the Valkey Glide client */
int execute_fcall_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                name = NULL;
    size_t               name_len;
    zval*                keys_array = NULL;
    zval*                args_array = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osa|a", &object, ce, &name, &name_len, &keys_array, &args_array) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        return execute_fcall_command_internal(valkey_glide->glide_client,
                                              name,
                                              name_len,
                                              keys_array,
                                              args_array,
                                              FCall,
                                              return_value);
    }

    return 0;
}

/* Execute an FCALL_RO command using the Valkey Glide client */
int execute_fcall_ro_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                name = NULL;
    size_t               name_len;
    zval*                keys_array = NULL;
    zval*                args_array = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Osa|a", &object, ce, &name, &name_len, &keys_array, &args_array) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        return execute_fcall_command_internal(valkey_glide->glide_client,
                                              name,
                                              name_len,
                                              keys_array,
                                              args_array,
                                              FCallReadOnly,
                                              return_value);
    }

    return 0;
}

/* Execute a DUMP command using the Valkey Glide client */
int execute_dump_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                key = NULL;
    size_t               key_len;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc, object, "Os", &object, ce, &key, &key_len) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        core_command_args_t args = {0};
        args.glide_client        = valkey_glide->glide_client;
        args.cmd_type            = Dump;
        args.key                 = key;
        args.key_len             = key_len;

        char*  output     = NULL;
        size_t output_len = 0;
        struct {
            char**  result;
            size_t* result_len;
        } out = {&output, &output_len};

        if (execute_core_command(&args, &out, process_core_string_result)) {
            if (output) {
                /* Return serialized value */
                ZVAL_STRINGL(return_value, output, output_len);
                efree(output);
                return 1;
            } else {
                /* Key doesn't exist */
                ZVAL_FALSE(return_value);
                return 1;
            }
        }
    }

    return 0;
}

/* Execute a RESTORE command using the Valkey Glide client */
int execute_restore_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char *               key = NULL, *serialized = NULL;
    size_t               key_len, serialized_len;
    long                 ttl;
    zval*                options = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(argc,
                                     object,
                                     "Osls|a",
                                     &object,
                                     ce,
                                     &key,
                                     &key_len,
                                     &ttl,
                                     &serialized,
                                     &serialized_len,
                                     &options) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Check if key and serialized value are valid */
        if (!key || key_len <= 0 || !serialized || serialized_len <= 0) {
            return 0;
        }

        /* Convert TTL to string */
        char ttl_str[32];
        snprintf(ttl_str, sizeof(ttl_str), "%ld", ttl);

        /* Start with basic arguments: key + ttl + serialized */
        unsigned long  base_arg_count = 3;
        unsigned long  max_args       = 10; /* Maximum possible arguments */
        uintptr_t*     args           = (uintptr_t*) emalloc(max_args * sizeof(uintptr_t));
        unsigned long* args_len       = (unsigned long*) emalloc(max_args * sizeof(unsigned long));

        if (!args || !args_len) {
            if (args)
                efree(args);
            if (args_len)
                efree(args_len);
            return 0;
        }

        /* Set up base arguments */
        args[0]     = (uintptr_t) key;
        args_len[0] = key_len;
        args[1]     = (uintptr_t) ttl_str;
        args_len[1] = strlen(ttl_str);
        args[2]     = (uintptr_t) serialized;
        args_len[2] = serialized_len;

        unsigned long arg_count = base_arg_count;

        /* Process options if provided */
        if (options && Z_TYPE_P(options) == IS_ARRAY) {
            HashTable*   ht = Z_ARRVAL_P(options);
            zval*        val;
            zend_string* key_str;
            zend_ulong   num_key;

            /* Variables for option values */
            zend_bool has_replace = 0;
            zend_bool has_absttl  = 0;
            long      idletime    = -1;
            long      freq        = -1;

            /* Parse the array */
            ZEND_HASH_FOREACH_KEY_VAL(ht, num_key, key_str, val) {
                /* Handle indexed array elements (like ['REPLACE', 'ABSTTL']) */
                if (!key_str && Z_TYPE_P(val) == IS_STRING) {
                    const char* flag = Z_STRVAL_P(val);
                    if (strcmp(flag, "REPLACE") == 0) {
                        has_replace = 1;
                    } else if (strcmp(flag, "ABSTTL") == 0) {
                        has_absttl = 1;
                    }
                }
                /* Handle associative array elements (like ['IDLETIME' => 200]) */
                else if (key_str) {
                    const char* opt_name = ZSTR_VAL(key_str);
                    if (strcmp(opt_name, "REPLACE") == 0) {
                        has_replace = 1;
                    } else if (strcmp(opt_name, "ABSTTL") == 0) {
                        has_absttl = 1;
                    } else if (strcmp(opt_name, "IDLETIME") == 0 && Z_TYPE_P(val) == IS_LONG) {
                        idletime = Z_LVAL_P(val);
                    } else if (strcmp(opt_name, "FREQ") == 0 && Z_TYPE_P(val) == IS_LONG) {
                        freq = Z_LVAL_P(val);
                    }
                }
            }
            ZEND_HASH_FOREACH_END();

            /* Add REPLACE if needed */
            if (has_replace && arg_count < max_args) {
                const char* replace_str = "REPLACE";
                args[arg_count]         = (uintptr_t) replace_str;
                args_len[arg_count]     = strlen(replace_str);
                arg_count++;
            }

            /* Add ABSTTL if needed */
            if (has_absttl && arg_count < max_args) {
                const char* absttl_str = "ABSTTL";
                args[arg_count]        = (uintptr_t) absttl_str;
                args_len[arg_count]    = strlen(absttl_str);
                arg_count++;
            }

            /* Add IDLETIME if provided */
            if (idletime >= 0 && arg_count + 1 < max_args) {
                const char* idletime_str = "IDLETIME";
                args[arg_count]          = (uintptr_t) idletime_str;
                args_len[arg_count]      = strlen(idletime_str);
                arg_count++;

                /* Convert idletime to string */
                char* idletime_val = (char*) emalloc(32);
                snprintf(idletime_val, 32, "%ld", idletime);
                args[arg_count]     = (uintptr_t) idletime_val;
                args_len[arg_count] = strlen(idletime_val);
                arg_count++;
            }

            /* Add FREQ if provided */
            if (freq >= 0 && arg_count + 1 < max_args) {
                const char* freq_str = "FREQ";
                args[arg_count]      = (uintptr_t) freq_str;
                args_len[arg_count]  = strlen(freq_str);
                arg_count++;

                /* Convert freq to string */
                char* freq_val = (char*) emalloc(32);
                snprintf(freq_val, 32, "%ld", freq);
                args[arg_count]     = (uintptr_t) freq_val;
                args_len[arg_count] = strlen(freq_val);
                arg_count++;
            }
        }

        /* Execute the command */
        CommandResult* result = execute_command(valkey_glide->glide_client,
                                                Restore,   /* command type */
                                                arg_count, /* number of arguments */
                                                args,      /* arguments */
                                                args_len   /* argument lengths */
        );

        /* Free any dynamically allocated option values */
        int i;
        for (i = base_arg_count; i < arg_count; i++) {
            /* Check if this is a dynamically allocated string (IDLETIME/FREQ values) */
            char* str = (char*) args[i];
            if (str && str[0] >= '0' && str[0] <= '9') {
                efree(str);
            }
        }

        /* Free the argument arrays */
        efree(args);
        efree(args_len);

        /* Process the result */
        int status = 0;
        if (result) {
            if (result->command_error) {
                /* Command failed */
                free_command_result(result);
                return 0;
            }

            if (result->response) {
                if (result->response->response_type == Ok) {
                    ZVAL_TRUE(return_value);
                    status = 1;
                } else {
                    ZVAL_FALSE(return_value);
                    status = 0;
                }
            }
            free_command_result(result);
        }

        return status;
    }

    return 0;
}

/* Execute a CONFIG command using the Valkey Glide client */
int execute_config_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                operation = NULL;
    size_t               operation_len;
    zval *               key = NULL, *value = NULL;

    /* Parse parameters */
    if (zend_parse_method_parameters(
            argc, object, "Os|z!z!", &object, ce, &operation, &operation_len, &key, &value) ==
        FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    /* If we have a Glide client, use it */
    if (valkey_glide->glide_client) {
        /* Check if operation is valid */
        if (!operation) {
            return 0;
        }

        /* Determine the CONFIG operation type */
        enum RequestType command_type;

        if (strncasecmp(operation, "GET", operation_len) == 0) {
            command_type = ConfigGet;
        } else if (strncasecmp(operation, "SET", operation_len) == 0) {
            command_type = ConfigSet;
        } else if (strncasecmp(operation, "RESETSTAT", operation_len) == 0) {
            command_type = ConfigResetStat;
        } else if (strncasecmp(operation, "REWRITE", operation_len) == 0) {
            command_type = ConfigRewrite;
        } else {
            php_error_docref(NULL, E_WARNING, "Unknown CONFIG operation '%s'", operation);
            return 0;
        }

        /* Prepare command arguments */
        unsigned long  arg_count         = 0;
        uintptr_t*     args              = NULL;
        unsigned long* args_len          = NULL;
        char**         temp_strings      = NULL;
        int            temp_string_count = 0;

        /* For CONFIG GET */
        if (command_type == ConfigGet) {
            if (!key) {
                php_error_docref(NULL, E_WARNING, "CONFIG GET requires a parameter");
                return 0;
            }

            /* Handle string or array parameter */
            if (Z_TYPE_P(key) == IS_STRING) {
                arg_count = 1;
                args      = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
                args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

                args[0]     = (uintptr_t) Z_STRVAL_P(key);
                args_len[0] = Z_STRLEN_P(key);
            } else if (Z_TYPE_P(key) == IS_ARRAY) {
                HashTable* ht = Z_ARRVAL_P(key);
                arg_count     = zend_hash_num_elements(ht);

                if (arg_count == 0) {
                    php_error_docref(NULL, E_WARNING, "CONFIG GET array cannot be empty");
                    return 0;
                }

                args         = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
                args_len     = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
                temp_strings = (char**) ecalloc(arg_count, sizeof(char*));

                zval* z_param;
                int   i = 0;
                ZEND_HASH_FOREACH_VAL(ht, z_param) {
                    zval temp;
                    ZVAL_COPY(&temp, z_param);
                    convert_to_string(&temp);

                    temp_strings[temp_string_count] = estrdup(Z_STRVAL(temp));
                    args[i]                         = (uintptr_t) temp_strings[temp_string_count];
                    args_len[i]                     = Z_STRLEN(temp);
                    temp_string_count++;
                    i++;

                    zval_dtor(&temp);
                }
                ZEND_HASH_FOREACH_END();
            } else {
                php_error_docref(NULL, E_WARNING, "CONFIG GET parameter must be a string or array");
                return 0;
            }
        }
        /* For CONFIG SET */
        else if (command_type == ConfigSet) {
            if (!key || (Z_TYPE_P(key) != IS_ARRAY && !value)) {
                php_error_docref(NULL, E_WARNING, "CONFIG SET requires key and value parameters");
                return 0;
            }

            /* Handle two strings or an array */
            if (Z_TYPE_P(key) == IS_STRING && Z_TYPE_P(value) != IS_NULL) {
                /* CONFIG SET key value */
                arg_count    = 2;
                args         = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
                args_len     = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
                temp_strings = (char**) ecalloc(2, sizeof(char*));

                /* Key */
                args[0]     = (uintptr_t) Z_STRVAL_P(key);
                args_len[0] = Z_STRLEN_P(key);

                /* Value */
                zval temp;
                ZVAL_COPY(&temp, value);
                convert_to_string(&temp);

                temp_strings[0]   = estrdup(Z_STRVAL(temp));
                args[1]           = (uintptr_t) temp_strings[0];
                args_len[1]       = Z_STRLEN(temp);
                temp_string_count = 1;

                zval_dtor(&temp);
            } else if (Z_TYPE_P(key) == IS_ARRAY && value == NULL) {
                /* CONFIG SET from array */
                HashTable* ht = Z_ARRVAL_P(key);
                arg_count     = zend_hash_num_elements(ht) * 2;

                if (arg_count == 0) {
                    php_error_docref(NULL, E_WARNING, "CONFIG SET array cannot be empty");
                    return 0;
                }

                args         = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
                args_len     = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));
                temp_strings = (char**) ecalloc(zend_hash_num_elements(ht), sizeof(char*));

                zend_string* zkey;
                zval*        zvalue;
                int          i = 0;
                ZEND_HASH_FOREACH_STR_KEY_VAL(ht, zkey, zvalue) {
                    if (!zkey) {
                        php_error_docref(NULL, E_WARNING, "CONFIG SET array must be associative");
                        goto cleanup;
                    }

                    /* Add key */
                    args[i]     = (uintptr_t) ZSTR_VAL(zkey);
                    args_len[i] = ZSTR_LEN(zkey);
                    i++;

                    /* Add value */
                    zval temp;
                    ZVAL_COPY(&temp, zvalue);
                    convert_to_string(&temp);

                    temp_strings[temp_string_count] = estrdup(Z_STRVAL(temp));
                    args[i]                         = (uintptr_t) temp_strings[temp_string_count];
                    args_len[i]                     = Z_STRLEN(temp);
                    temp_string_count++;
                    i++;

                    zval_dtor(&temp);
                }
                ZEND_HASH_FOREACH_END();
            } else {
                php_error_docref(NULL, E_WARNING, "CONFIG SET requires two strings or an array");
                return 0;
            }
        }
        /* CONFIG RESETSTAT and CONFIG REWRITE have no additional arguments */
        else if (command_type == ConfigResetStat || command_type == ConfigRewrite) {
            arg_count = 0; /* No arguments needed */
            args      = NULL;
            args_len  = NULL;
        } else {
            php_error_docref(NULL, E_WARNING, "Unknown CONFIG operation '%s'", operation);
            return 0;
        }

        /* Execute the command */
        CommandResult* result =
            execute_command(valkey_glide->glide_client, command_type, arg_count, args, args_len);

        /* Free temporary strings */
        if (temp_strings) {
            for (int i = 0; i < temp_string_count; i++) {
                if (temp_strings[i])
                    efree(temp_strings[i]);
            }
            efree(temp_strings);
        }

        /* Free the argument arrays */
        if (args)
            efree(args);
        if (args_len)
            efree(args_len);

        /* Handle the result */
        int status = 0;
        if (result) {
            if (result->command_error) {
                /* Command failed */
                free_command_result(result);
                return 0;
            }

            if (result->response) {
                if (command_type == ConfigGet) {
                    /* CONFIG GET returns a Map - convert to associative array */
                    status = command_response_to_zval(result->response,
                                                      return_value,
                                                      COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP,
                                                      false);
                } else {
                    /* CONFIG SET/RESETSTAT/REWRITE return OK */
                    if (result->response->response_type == Ok) {
                        ZVAL_TRUE(return_value);
                        status = 1;
                    } else {
                        ZVAL_FALSE(return_value);
                        status = 0;
                    }
                }
            }
            free_command_result(result);
        }

        return status;

    cleanup:
        /* Cleanup on error */
        if (temp_strings) {
            for (int i = 0; i < temp_string_count; i++) {
                if (temp_strings[i])
                    efree(temp_strings[i]);
            }
            efree(temp_strings);
        }
        if (args)
            efree(args);
        if (args_len)
            efree(args_len);
    }

    return 0;
}


/* Helper function to parse CLIENT LIST response into array of associative arrays */
static int parse_client_list_response(const char* response_str,
                                      size_t      response_len,
                                      zval*       return_value) {
    if (!response_str || response_len == 0) {
        array_init(return_value);
        return 1;
    }

    array_init(return_value);

    /* Split response by newlines to get individual client entries */
    const char* line_start   = response_str;
    const char* line_end     = response_str;
    const char* response_end = response_str + response_len;

    while (line_start < response_end) {
        /* Find end of current line */
        while (line_end < response_end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }

        /* Skip empty lines */
        if (line_end > line_start) {
            size_t line_length = line_end - line_start;

            /* Create associative array for this client */
            zval client_array;
            array_init(&client_array);

            /* Parse key=value pairs in this line */
            const char* token_start = line_start;
            const char* token_end   = line_start;

            while (token_start < line_start + line_length) {
                /* Find end of current token (space-separated) */
                while (token_end < line_start + line_length && *token_end != ' ') {
                    token_end++;
                }

                if (token_end > token_start) {
                    /* Look for '=' in token to split key and value */
                    const char* equals_pos = token_start;
                    while (equals_pos < token_end && *equals_pos != '=') {
                        equals_pos++;
                    }

                    if (equals_pos < token_end && *equals_pos == '=') {
                        /* Extract key and value */
                        size_t key_len   = equals_pos - token_start;
                        size_t value_len = token_end - equals_pos - 1;

                        if (key_len > 0) {
                            /* Create null-terminated key string */
                            char* key = emalloc(key_len + 1);
                            memcpy(key, token_start, key_len);
                            key[key_len] = '\0';

                            /* Create null-terminated value string */
                            char* value = emalloc(value_len + 1);
                            if (value_len > 0) {
                                memcpy(value, equals_pos + 1, value_len);
                            }
                            value[value_len] = '\0';

                            /* Add to client array */
                            add_assoc_string(&client_array, key, value);

                            /* Free temporary strings */
                            efree(key);
                            efree(value);
                        }
                    }
                }

                /* Move to next token */
                token_start = token_end;
                /* Skip spaces */
                while (token_start < line_start + line_length && *token_start == ' ') {
                    token_start++;
                }
                token_end = token_start;
            }

            /* Add client array to main result array */
            add_next_index_zval(return_value, &client_array);
        }

        /* Move to next line */
        while (line_end < response_end && (*line_end == '\n' || *line_end == '\r')) {
            line_end++;
        }
        line_start = line_end;
    }

    return 1;
}

/* Execute a CLIENT command using the Valkey Glide client */
int execute_client_command_internal(
    const void* glide_client, zval* args, int args_count, zval* return_value, zval* route) {
    /* Check if client and args are valid */
    if (!glide_client || !args || args_count <= 0 || !return_value) {
        return 0;
    }

    /* Create argument arrays */
    unsigned long  arg_count = args_count;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!cmd_args || !args_len) {
        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);
        return 0;
    }

    /* Keep track of allocated strings for cleanup */
    char** allocated     = (char**) emalloc(args_count * sizeof(char*));
    int    allocated_idx = 0;

    /* Convert arguments to strings if needed */
    int i;

    for (i = 1; i < args_count; i++) {
        zval* arg = &args[i];

        /* If string, use directly */
        if (Z_TYPE_P(arg) == IS_STRING) {
            cmd_args[i - 1] = (uintptr_t) Z_STRVAL_P(arg);
            args_len[i - 1] = Z_STRLEN_P(arg);
        } else {
            /* Convert non-string types to string */
            zval   copy;
            size_t str_len;
            char*  str;

            ZVAL_DUP(&copy, arg);
            convert_to_string(&copy);

            str_len = Z_STRLEN(copy);
            str     = emalloc(str_len + 1);
            memcpy(str, Z_STRVAL(copy), str_len);
            str[str_len] = '\0';

            cmd_args[i - 1] = (uintptr_t) str;
            args_len[i - 1] = str_len;

            /* Track allocated string for cleanup */
            allocated[allocated_idx++] = str;

            zval_dtor(&copy);
        }
    }

    /* Determine the appropriate client command type based on the first argument */
    enum RequestType command_type = ClientInfo; /* Default to ClientInfo */

    if (args_count > 0 && Z_TYPE(args[0]) == IS_STRING) {
        const char* subcmd = Z_STRVAL(args[0]);
        if (strcasecmp(subcmd, "KILL") == 0) {
            if (args_count > 1)
                command_type = ClientKill;
            else
                command_type = ClientKillSimple;
        } else if (strcasecmp(subcmd, "LIST") == 0)
            command_type = ClientList;
        else if (strcasecmp(subcmd, "GETNAME") == 0)
            command_type = ClientGetName;
        else if (strcasecmp(subcmd, "ID") == 0)
            command_type = ClientId;
        //        else if (strcasecmp(subcmd, "SETNAME") == 0)
        //            command_type = ClientSetName;
        else if (strcasecmp(subcmd, "PAUSE") == 0)
            command_type = ClientPause;
        else if (strcasecmp(subcmd, "UNPAUSE") == 0)
            command_type = ClientUnpause;
        else if (strcasecmp(subcmd, "REPLY") == 0)
            command_type = ClientReply;
        else if (strcasecmp(subcmd, "INFO") == 0)
            command_type = ClientInfo;
        else if (strcasecmp(subcmd, "NO-EVICT") == 0)
            command_type = ClientNoEvict;
        else
            return 0; /* Unknown command */
    }

    /* Set additional client prefix for custom commands */
    uintptr_t*     final_args      = cmd_args;
    unsigned long* final_args_len  = args_len;
    unsigned long  final_arg_count = arg_count - 1;


    /* Execute the command with or without routing */
    CommandResult* result;
    if (route) {
        /* Use cluster routing */
        result = execute_command_with_route(glide_client,
                                            command_type,    /* command type */
                                            final_arg_count, /* number of arguments */
                                            final_args,      /* arguments */
                                            final_args_len,  /* argument lengths */
                                            route            /* route parameter */
        );
    } else {
        /* No routing (standalone mode) */
        result = execute_command(glide_client,
                                 command_type,    /* command type */
                                 final_arg_count, /* number of arguments */
                                 final_args,      /* arguments */
                                 final_args_len   /* argument lengths */
        );
    }

    /* Free allocated memory */
    for (i = 0; i < allocated_idx; i++)
        efree(allocated[i]);
    efree(allocated);

    /* If we created a new args array for CustomCommand, free it */
    if (command_type == CustomCommand) {
        efree(final_args);
        efree(final_args_len);
    }

    efree(cmd_args);
    efree(args_len);

    /* Process the result */
    int status = 0;


    if (result) {
        if (result->command_error) {
            /* Command failed */
            free_command_result(result);
            return 0;
        }

        if (result->response) {
            /* Special handling for CLIENT LIST - convert string to array of associative arrays */
            if (command_type == ClientList && result->response->response_type == String) {
                status = parse_client_list_response(result->response->string_value,
                                                    result->response->string_value_len,
                                                    return_value);
            } else {
                /* Convert the response to PHP value using normal processing */
                status = command_response_to_zval(
                    result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
            }
        }
        free_command_result(result);
    }

    return status;
}

/* Execute a RAWCOMMAND command using the Valkey Glide client */
int execute_rawcommand_command_internal(
    const void* glide_client, zval* args, int args_count, zval* return_value, zval* route) {
    /* Check if client and args are valid */
    if (!glide_client || !args || args_count <= 0 || !return_value) {
        return 0;
    }

    /* Create argument arrays */
    unsigned long  arg_count = args_count;
    uintptr_t*     cmd_args  = (uintptr_t*) emalloc(arg_count * sizeof(uintptr_t));
    unsigned long* args_len  = (unsigned long*) emalloc(arg_count * sizeof(unsigned long));

    if (!cmd_args || !args_len) {
        if (cmd_args)
            efree(cmd_args);
        if (args_len)
            efree(args_len);
        return 0;
    }

    /* Keep track of allocated strings for cleanup */
    char** allocated     = (char**) emalloc(args_count * sizeof(char*));
    int    allocated_idx = 0;

    /* Convert arguments to strings if needed */
    int i;
    for (i = 0; i < args_count; i++) {
        zval* arg = &args[i];

        /* If string, use directly */
        if (Z_TYPE_P(arg) == IS_STRING) {
            cmd_args[i] = (uintptr_t) Z_STRVAL_P(arg);
            args_len[i] = Z_STRLEN_P(arg);
        } else {
            /* Convert non-string types to string */
            zval   copy;
            size_t str_len;
            char*  str;

            ZVAL_DUP(&copy, arg);
            convert_to_string(&copy);

            str_len = Z_STRLEN(copy);
            str     = emalloc(str_len + 1);
            memcpy(str, Z_STRVAL(copy), str_len);
            str[str_len] = '\0';

            cmd_args[i] = (uintptr_t) str;
            args_len[i] = str_len;

            /* Track allocated string for cleanup */
            allocated[allocated_idx++] = str;

            zval_dtor(&copy);
        }
    }

    /* Execute the command with or without routing */
    CommandResult* result;
    if (route) {
        /* Use cluster routing */
        result = execute_command_with_route(glide_client,
                                            CustomCommand, /* command type for raw commands */
                                            arg_count,     /* number of arguments */
                                            cmd_args,      /* arguments */
                                            args_len,      /* argument lengths */
                                            route          /* route parameter */
        );
    } else {
        /* No routing (standalone mode) */
        result = execute_command(glide_client,
                                 CustomCommand, /* command type for raw commands */
                                 arg_count,     /* number of arguments */
                                 cmd_args,      /* arguments */
                                 args_len       /* argument lengths */
        );
    }

    /* Free allocated memory */
    for (i = 0; i < allocated_idx; i++)
        efree(allocated[i]);
    efree(allocated);
    efree(cmd_args);
    efree(args_len);

    /* Process the result */
    int status = 0;

    if (result) {
        if (result->command_error) {
            /* Command failed */
            free_command_result(result);
            return 0;
        }

        if (result->response) {
            /* Convert the response to PHP value */
            status = command_response_to_zval(
                result->response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
        }
        free_command_result(result);
    }

    return status;
}

/* Execute client command - UNIFIED IMPLEMENTATION */
int execute_client_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args     = NULL;
    int                  arg_count  = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());
    zval*                route      = NULL;


    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (is_cluster) {
        /* Parse parameters for cluster - route + command arguments */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &z_args, &arg_count) ==
            FAILURE) {
            return 0;
        }

        if (arg_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* First argument is route, rest are command arguments */
        route     = &z_args[0];
        z_args    = &z_args[1];    /* Skip route parameter */
        arg_count = arg_count - 1; /* Reduce count by 1 */

        if (arg_count == 0) {
            /* Need at least one command argument after route */
            return 0;
        }
    } else {
        /* Parse parameters for non-cluster - just command arguments */
        if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &arg_count) ==
            FAILURE) {
            return 0;
        }
    }

    /* Execute the client command using the Glide client */
    if (execute_client_command_internal(
            valkey_glide->glide_client, z_args, arg_count, return_value, route)) {
        /* Return value already set in execute_client_command */
        return 1;
    }

    return 0;
}

/* Execute rawcommand command - UNIFIED IMPLEMENTATION */
int execute_rawcommand_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                z_args     = NULL;
    int                  arg_count  = 0;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());
    zval*                route      = NULL;

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    if (is_cluster) {
        /* Parse parameters for cluster - route + command arguments */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &z_args, &arg_count) ==
            FAILURE) {
            return 0;
        }

        if (arg_count == 0) {
            /* Need at least the route parameter */
            return 0;
        }

        /* First argument is route, rest are command arguments */
        route     = &z_args[0];
        z_args    = &z_args[1];    /* Skip route parameter */
        arg_count = arg_count - 1; /* Reduce count by 1 */

        if (arg_count == 0) {
            /* Need at least one command argument after route */
            return 0;
        }
    } else {
        /* Parse parameters for non-cluster - just command arguments */
        if (zend_parse_method_parameters(argc, object, "O+", &object, ce, &z_args, &arg_count) ==
            FAILURE) {
            return 0;
        }
    }

    /* Execute the raw command using the Glide client */
    if (execute_rawcommand_command_internal(
            valkey_glide->glide_client, z_args, arg_count, return_value, route)) {
        /* Return value already set in execute_rawcommand_command */
        return 1;
    }

    return 0;
}

/* Execute dbSize command - UNIFIED IMPLEMENTATION */
int execute_dbsize_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    zval*                args       = NULL;
    int                  args_count = 0;
    long                 dbsize;
    zend_bool            is_cluster = (ce == get_valkey_glide_cluster_ce());

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Setup core command arguments */
    core_command_args_t core_args = {0};
    core_args.glide_client        = valkey_glide->glide_client;
    core_args.cmd_type            = DBSize;
    core_args.is_cluster          = is_cluster;

    if (is_cluster) {
        /* Parse parameters for cluster - route parameter is required */
        if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &args, &args_count) ==
            FAILURE) {
            return 0;
        }

        if (args_count == 0) {
            /* Need the route parameter */
            return 0;
        }

        /* Set up routing */
        core_args.has_route   = 1;
        core_args.route_param = &args[0];
    } else {
        /* Non-cluster case - parse no parameters */
        if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
            return 0;
        }
    }

    /* Execute using unified core framework */
    if (execute_core_command(&core_args, &dbsize, process_core_int_result)) {
        ZVAL_LONG(return_value, dbsize);
        return 1;
    }

    return 0;
}
