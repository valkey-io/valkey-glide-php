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
#include "ext/standard/php_var.h"
#include "include/glide_bindings.h"
#include "valkey_glide_commands_common.h"
#include "valkey_glide_core_common.h"
#include "valkey_glide_hash_common.h"
#include "valkey_glide_z_common.h"

/* Helper functions for batch state management */
static void clear_batch_state(valkey_glide_object* valkey_glide);

static void expand_command_buffer(valkey_glide_object* valkey_glide);

/* Helper function to process array arguments for FCALL commands */
static void process_array_to_args(zval*          array,
                                  uintptr_t*     cmd_args,
                                  unsigned long* args_len,
                                  unsigned long* arg_index);

/* Helper functions to reduce code duplication */
static int convert_zval_args_to_strings(zval*           args,
                                        int             args_count,
                                        uintptr_t**     cmd_args,
                                        unsigned long** args_len,
                                        char***         allocated_strings,
                                        int*            allocated_count);

static enum RequestType determine_client_command_type(zval* args, int args_count);

static void cleanup_allocated_strings(char** allocated_strings, int allocated_count);

static int command_response_to_zval_wrapper(CommandResponse* response,
                                            void*            output,
                                            zval*            return_value);

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


        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_int_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }

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
int buffer_command_for_batch(valkey_glide_object* valkey_glide,
                             enum RequestType     cmd_type,
                             const uintptr_t*     args,
                             const unsigned long* arg_lengths,
                             uintptr_t            arg_count,
                             void*                result_ptr,
                             z_result_processor_t process_result) {
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
    cmd->request_type   = cmd_type;
    cmd->arg_count      = arg_count;
    cmd->result_ptr     = result_ptr;
    cmd->process_result = process_result;


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
                    memcpy(cmd->args[i], ((uint8_t**) args)[i], arg_lengths[i]);
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

    valkey_glide->command_count++;
    return 1;
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

/* Helper function to convert zval arguments to strings - reduces duplication */
static int convert_zval_args_to_strings(zval*           args,
                                        int             args_count,
                                        uintptr_t**     cmd_args,
                                        unsigned long** args_len,
                                        char***         allocated_strings,
                                        int*            allocated_count) {
    if (!args || args_count <= 0) {
        return 0;
    }
    *cmd_args          = (uintptr_t*) emalloc(args_count * sizeof(uintptr_t));
    *args_len          = (unsigned long*) emalloc(args_count * sizeof(unsigned long));
    *allocated_strings = (char**) emalloc(args_count * sizeof(char*));
    *allocated_count   = 0;

    if (!*cmd_args || !*args_len || !*allocated_strings) {
        if (*cmd_args)
            efree(*cmd_args);
        if (*args_len)
            efree(*args_len);
        if (*allocated_strings)
            efree(*allocated_strings);
        return 0;
    }
    for (int i = 0; i < args_count; i++) {
        zval* arg = &args[i];

        if (Z_TYPE_P(arg) == IS_STRING) {
            /* Already a string, use directly */
            (*cmd_args)[i] = (uintptr_t) Z_STRVAL_P(arg);
            (*args_len)[i] = Z_STRLEN_P(arg);
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

            (*cmd_args)[i] = (uintptr_t) str;
            (*args_len)[i] = str_len;

            /* Track allocated string for cleanup */
            (*allocated_strings)[(*allocated_count)++] = str;

            zval_dtor(&copy);
        }
    }

    return 1;
}

/* Helper function to determine CLIENT command type - reduces duplication */
static enum RequestType determine_client_command_type(zval* args, int args_count) {
    if (args_count > 0 && Z_TYPE(args[0]) == IS_STRING) {
        const char* subcmd = Z_STRVAL(args[0]);
        if (strcasecmp(subcmd, "KILL") == 0) {
            return args_count > 1 ? ClientKill : ClientKillSimple;
        } else if (strcasecmp(subcmd, "LIST") == 0) {
            return ClientList;
        } else if (strcasecmp(subcmd, "GETNAME") == 0) {
            return ClientGetName;
        } else if (strcasecmp(subcmd, "ID") == 0) {
            return ClientId;
            //  } else if (strcasecmp(subcmd, "SETNAME") == 0) {
            //    return ClientSetName;
        } else if (strcasecmp(subcmd, "PAUSE") == 0) {
            return ClientPause;
        } else if (strcasecmp(subcmd, "UNPAUSE") == 0) {
            return ClientUnpause;
        } else if (strcasecmp(subcmd, "REPLY") == 0) {
            return ClientReply;
        } else if (strcasecmp(subcmd, "INFO") == 0) {
            return ClientInfo;
        } else if (strcasecmp(subcmd, "NO-EVICT") == 0) {
            return ClientNoEvict;
        }
    }
    return InvalidRequest; /* Default */
}

/* Helper function to determine FUNCTION command type - reduces duplication */
static enum RequestType determine_function_command_type(zval* args, int args_count) {
    if (args_count > 0 && Z_TYPE(args[0]) == IS_STRING) {
        const char* subcmd = Z_STRVAL(args[0]);
        if (strcasecmp(subcmd, "DELETE") == 0) {
            return FunctionDelete;
        } else if (strcasecmp(subcmd, "DUMP") == 0) {
            return FunctionDump;
        } else if (strcasecmp(subcmd, "FLUSH") == 0) {
            return FunctionFlush;
        } else if (strcasecmp(subcmd, "KILL") == 0) {
            return FunctionKill;
        } else if (strcasecmp(subcmd, "LIST") == 0) {
            return FunctionList;
        } else if (strcasecmp(subcmd, "LOAD") == 0) {
            return FunctionLoad;
        } else if (strcasecmp(subcmd, "RESTORE") == 0) {
            return FunctionRestore;
        } else if (strcasecmp(subcmd, "STATS") == 0) {
            return FunctionStats;
        }
    }
    return InvalidRequest; /* Invalid function subcommand */
}

/* Helper function to cleanup allocated strings - reduces duplication */
static void cleanup_allocated_strings(char** allocated_strings, int allocated_count) {
    if (allocated_strings) {
        for (int i = 0; i < allocated_count; i++) {
            if (allocated_strings[i]) {
                efree(allocated_strings[i]);
            }
        }
        efree(allocated_strings);
    }
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

/* Wrapper function to match z_result_processor_t signature */
static int command_response_to_zval_wrapper(CommandResponse* response,
                                            void*            output,
                                            zval*            return_value) {
    enum RequestType command_type = *((enum RequestType*) output);
    efree(output);
    if (command_type == ClientList && response->response_type == String) {
        return parse_client_list_response(
            response->string_value, response->string_value_len, return_value);
    } else {
        return command_response_to_zval(
            response, return_value, COMMAND_RESPONSE_NOT_ASSOSIATIVE, false);
    }
}

static int process_function_command_reposonse(CommandResponse* response,
                                              void*            output,
                                              zval*            return_value) {
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP_FUNCTION, true);
}

static int process_fcall_command_reposonse(CommandResponse* response,
                                           void*            output,
                                           zval*            return_value) {
    return command_response_to_zval(
        response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
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

        /* Determine the specific function command type */
        enum RequestType function_command_type =
            determine_function_command_type(z_args, args_count);
        if (function_command_type == InvalidRequest) {
            /* Unknown function subcommand */
            return 0;
        }

        /* Use helper function to convert remaining arguments to strings (skip the subcommand) */
        uintptr_t*     cmd_args          = NULL;
        unsigned long* args_len          = NULL;
        char**         allocated_strings = NULL;
        int            allocated_count   = 0;
        unsigned long  final_arg_count   = 0;

        /* If there are arguments after the subcommand, convert them */
        if (args_count > 1) {
            if (!convert_zval_args_to_strings(&z_args[1],
                                              args_count - 1,
                                              &cmd_args,
                                              &args_len,
                                              &allocated_strings,
                                              &allocated_count)) {
                return 0;
            }
            final_arg_count = args_count - 1;
        }

        /* Check if we're in batch mode */
        if (valkey_glide->is_in_batch_mode) {
            /* Convert arguments to uint8_t** format for batch processing */
            uintptr_t*     batch_args  = NULL;
            unsigned long* arg_lengths = NULL;

            if (final_arg_count > 0) {
                batch_args  = (uintptr_t*) emalloc(final_arg_count * sizeof(uintptr_t*));
                arg_lengths = (uintptr_t*) emalloc(final_arg_count * sizeof(unsigned long));

                /* Copy arguments to batch format */
                for (unsigned long i = 0; i < final_arg_count; i++) {
                    batch_args[i]  = cmd_args[i];
                    arg_lengths[i] = args_len[i];
                }
            }

            enum RequestType* output = emalloc(sizeof(enum RequestType));
            *output                  = function_command_type;

            /* Buffer the command for batch execution */
            int buffer_result = buffer_command_for_batch(valkey_glide,
                                                         function_command_type,
                                                         batch_args,
                                                         arg_lengths,
                                                         final_arg_count,
                                                         output,
                                                         process_function_command_reposonse);

            /* Free the argument arrays */
            cleanup_allocated_strings(allocated_strings, allocated_count);
            if (cmd_args)
                efree(cmd_args);
            if (args_len)
                efree(args_len);
            if (batch_args)
                efree(batch_args);
            if (arg_lengths)
                efree(arg_lengths);

            if (buffer_result) {
                /* In batch mode, return $this for method chaining */
                ZVAL_COPY(return_value, object);
                return 1;
            }
            return 0;
        } else {
            /* Execute the command directly */
            CommandResult* result = execute_command(valkey_glide->glide_client,
                                                    function_command_type,
                                                    final_arg_count,
                                                    cmd_args,
                                                    args_len);

            /* Free the argument arrays using helper function */
            cleanup_allocated_strings(allocated_strings, allocated_count);
            if (cmd_args)
                efree(cmd_args);
            if (args_len)
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
                    /* FUNCTION can return various types based on subcommand */
                    status =
                        process_function_command_reposonse(result->response, NULL, return_value);
                    free_command_result(result);
                    return status;
                }
                free_command_result(result);
            }
        }
    }

    return 0;
}

/* Common function to initialize batch mode */
static int initialize_batch_mode(valkey_glide_object* valkey_glide,
                                 int                  batch_type,
                                 zval*                object,
                                 zval*                return_value) {
    if (!valkey_glide || !valkey_glide->glide_client) {
        return 0;
    }

    /* Initialize batch mode */
    valkey_glide->is_in_batch_mode = true;
    valkey_glide->batch_type       = batch_type;
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

/* Execute a MULTI command using the Valkey Glide client - UPDATED FOR BUFFERING */
int execute_multi_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    long                 batch_type = MULTI; /* Default to MULTI */

    /* Parse optional batch type parameter */
    if (zend_parse_method_parameters(argc, object, "O|l", &object, ce, &batch_type) == FAILURE) {
        return 0;
    }

    /* Validate batch type using existing constants */
    if (batch_type != MULTI && batch_type != PIPELINE) {
        php_error_docref(NULL, E_WARNING, "Invalid batch type. Use MULTI or PIPELINE");
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    return initialize_batch_mode(valkey_glide, (int) batch_type, object, return_value);
}

/* Execute a PIPELINE command using the Valkey Glide client - wrapper using common function */
int execute_pipeline_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;

    /* Parse parameters - pipeline takes no additional parameters */
    if (zend_parse_method_parameters(argc, object, "O", &object, ce) == FAILURE) {
        return 0;
    }

    /* Get ValkeyGlide object */
    valkey_glide = VALKEY_GLIDE_PHP_ZVAL_GET_OBJECT(valkey_glide_object, object);

    return initialize_batch_mode(valkey_glide, PIPELINE, object, return_value);
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
    } else {
        ZVAL_FALSE(return_value);
        return 0;
    }
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
    struct BatchInfo batch_info = {.cmd_count = valkey_glide->command_count,
                                   .cmds      = (const struct CmdInfo* const*) cmd_infos,
                                   .is_atomic = (valkey_glide->batch_type == MULTI)};

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
        status = 1; /* Assume success unless we find issues */
        if (result->response) {
            if (result->response->response_type != Array ||
                result->response->array_value_len != valkey_glide->command_count) {
                ZVAL_FALSE(return_value);
                status = 0;
                free_command_result(result);
                clear_batch_state(valkey_glide);
                return status;
            }
            array_init(return_value);
            for (int64_t idx = 0; idx < result->response->array_value_len; idx++) {
                zval value;
                int  process_status = valkey_glide->buffered_commands[idx].process_result(
                    &result->response->array_value[idx],
                    valkey_glide->buffered_commands[idx].result_ptr,
                    &value);

                if (process_status) {
                    /* Add the processed result to return array */
                    add_next_index_zval(return_value, &value);

                } else {
                    /* Process_result failed, use raw response */

                    ZVAL_FALSE(&value);
                    add_next_index_zval(return_value, &value);
                }
            }
        } else {
            /* Failed to get responses array, return false */
            ZVAL_FALSE(return_value);
            status = 0;
        }
    }

    free_command_result(result);
    clear_batch_state(valkey_glide);
    return status;
}

/* Internal function to execute FCALL/FCALL_RO commands using the Valkey Glide client */
static int execute_fcall_command_internal(zval*                object,
                                          valkey_glide_object* valkey_glide,
                                          char*                name,
                                          size_t               name_len,
                                          zval*                keys_array,
                                          zval*                args_array,
                                          enum RequestType     command_type,
                                          zval*                return_value) {
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


    CommandResult* result = NULL;
    /* Check for batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* Create batch-compatible processor wrapper */
        int res = buffer_command_for_batch(valkey_glide,
                                           command_type,
                                           cmd_args,
                                           args_len,
                                           arg_count,
                                           NULL,
                                           process_fcall_command_reposonse);
    } else {
        result = execute_command(valkey_glide->glide_client,
                                 command_type, /* command type */
                                 arg_count,    /* number of arguments */
                                 cmd_args,     /* arguments */
                                 args_len      /* argument lengths */
        );
    }


    /* Free the argument arrays */
    efree(cmd_args);
    efree(args_len);

    /* Handle the result directly */
    int status = 0;
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, return $this for method chaining */
        ZVAL_COPY(return_value, object);
        return 1;
    }
    if (result) {
        if (result->command_error) {
            /* Command failed */
            free_command_result(result);
            return 0;
        }

        else if (result->response) {
            /* FCALL can return various types */
            status = process_fcall_command_reposonse(result->response, NULL, return_value);
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
        return execute_fcall_command_internal(
            object, valkey_glide, name, name_len, keys_array, args_array, FCall, return_value);
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
        return execute_fcall_command_internal(object,
                                              valkey_glide,
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


        if (execute_core_command(
                valkey_glide, &args, NULL, process_core_string_result, return_value)) {
            if (valkey_glide->is_in_batch_mode) {
                /* In batch mode, return $this for method chaining */
                /* Note: out will be freed later in process_core_string_result */
                ZVAL_COPY(return_value, object);
                return 1;
            }


            return 1; /* Success */
        } else {
            /* Key doesn't exist */
            return 1;
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
        CommandResult* result = NULL;
        if (valkey_glide->is_in_batch_mode) {
            /* Create batch-compatible processor wrapper */
            int res = buffer_command_for_batch(
                valkey_glide, Restore, args, args_len, arg_count, NULL, process_h_ok_result_async);
        } else {
            /* Execute the command */
            result = execute_command(valkey_glide->glide_client,
                                     Restore,   /* command type */
                                     arg_count, /* number of arguments */
                                     args,      /* arguments */
                                     args_len   /* argument lengths */
            );
        }
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
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            status = 1;
        } else {
            if (result) {
                if (result->command_error) {
                    /* Command failed */
                    free_command_result(result);
                    return 0;
                }
                status = process_h_ok_result_async(result->response, NULL, return_value);
                free_command_result(result);
            }
        }

        return status;
    }

    return 0;
}

static int process_config_command_respose(CommandResponse* response,
                                          void*            output,
                                          zval*            return_value) {
    int              status       = 0;
    enum RequestType command_type = *(enum RequestType*) output;
    if (command_type == ConfigGet) {
        /* CONFIG GET returns a Map - convert to associative array */
        status = command_response_to_zval(
            response, return_value, COMMAND_RESPONSE_ASSOSIATIVE_ARRAY_MAP, false);
    } else {
        /* CONFIG SET/RESETSTAT/REWRITE return OK */
        if (response->response_type == Ok) {
            ZVAL_TRUE(return_value);
            status = 1;
        } else {
            ZVAL_FALSE(return_value);
            status = 0;
        }
    }
    return status;
}


/* Execute a CONFIG command using the Valkey Glide client */
int execute_config_command(zval* object, int argc, zval* return_value, zend_class_entry* ce) {
    valkey_glide_object* valkey_glide;
    char*                operation = NULL;
    size_t               operation_len;
    zval *               key = NULL, *value = NULL;
    /* Handle the result */
    int status = 0;
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

        if (valkey_glide->is_in_batch_mode) {
            enum RequestType* output = emalloc(sizeof(enum RequestType));
            *output                  = command_type;
            /* In batch mode, buffer the command and return $this for method chaining */
            if (buffer_command_for_batch(valkey_glide,
                                         command_type,
                                         args,
                                         args_len,
                                         arg_count,

                                         output,
                                         process_config_command_respose)) {
                /* Return $this */
                ZVAL_COPY(return_value, object);
                status = 1;
                goto cleanup;
            } else {
                php_error_docref(
                    NULL, E_WARNING, "Command buffer full, cannot buffer more commands");
                status = 0;
                goto cleanup;
            }
        } else {
            /* Execute the command */
            CommandResult* result = execute_command(
                valkey_glide->glide_client, command_type, arg_count, args, args_len);

            if (result) {
                if (result->command_error) {
                    /* Command failed */
                    free_command_result(result);
                    return 0;
                }

                if (result->response) {
                    status = process_config_command_respose(
                        result->response, &command_type, return_value);
                }
                free_command_result(result);
            }
        }


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

    return status;
}


/* Execute a CLIENT command using the Valkey Glide client */
int execute_client_command_internal(
    const void* glide_client, zval* args, int args_count, zval* return_value, zval* route) {
    /* Check if client and args are valid */
    if (!glide_client || !args || args_count <= 0 || !return_value) {
        return 0;
    }

    /* Use helper function to convert arguments (skip first argument which is the command) */
    uintptr_t*     cmd_args          = NULL;
    unsigned long* args_len          = NULL;
    char**         allocated_strings = NULL;
    int            allocated_count   = 0;

    if (args_count > 1) {
        if (!convert_zval_args_to_strings(&args[1],
                                          args_count - 1,
                                          &cmd_args,
                                          &args_len,
                                          &allocated_strings,
                                          &allocated_count)) {
            return 0;
        }
    }


    /* Use helper function to determine command type */
    enum RequestType command_type = determine_client_command_type(args, args_count);
    if (command_type == InvalidRequest) {
        cleanup_allocated_strings(allocated_strings, allocated_count);
        efree(cmd_args);
        efree(args_len);
        return 0; /* Unknown command */
    }

    /* Execute the command with or without routing */
    CommandResult* result;
    unsigned long  final_arg_count = args_count - 1;

    if (route) {
        /* Use cluster routing */
        result = execute_command_with_route(glide_client,
                                            command_type,    /* command type */
                                            final_arg_count, /* number of arguments */
                                            cmd_args,        /* arguments */
                                            args_len,        /* argument lengths */
                                            route            /* route parameter */
        );
    } else {
        /* No routing (standalone mode) */
        result = execute_command(glide_client,
                                 command_type,    /* command type */
                                 final_arg_count, /* number of arguments */
                                 cmd_args,        /* arguments */
                                 args_len         /* argument lengths */
        );
    }
    /* Free allocated memory using helper function */
    cleanup_allocated_strings(allocated_strings, allocated_count);
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
            /* Special handling for CLIENT LIST - convert string to array of associative arrays
             */
            enum RequestType* ctx = emalloc(sizeof(enum RequestType));
            *ctx                  = command_type;
            status = command_response_to_zval_wrapper(result->response, ctx, return_value);
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

    /* Parse parameters first to get all arguments */
    if (zend_parse_method_parameters(argc, object, "O*", &object, ce, &z_args, &arg_count) ==
        FAILURE) {
        return 0;
    }

    /* Check if we're in batch mode */
    if (valkey_glide->is_in_batch_mode) {
        /* In batch mode, ignore routing and treat all arguments as command arguments */
        if (arg_count == 0) {
            /* Need at least one command argument */
            return 0;
        }

        /* Use helper function to determine command type */
        enum RequestType command_type = determine_client_command_type(z_args, arg_count);
        if (command_type == InvalidRequest) {
            return 0; /* Unknown command */
        }

        /* Convert arguments to uint8_t** format for batch processing */
        uintptr_t*     batch_args  = NULL;
        unsigned long* arg_lengths = NULL;

        if (arg_count > 1) {
            batch_args  = (uintptr_t*) emalloc((arg_count - 1) * sizeof(uintptr_t));
            arg_lengths = emalloc((arg_count - 1) * sizeof(unsigned long));

            if (!batch_args || !arg_lengths) {
                if (batch_args)
                    efree(batch_args);
                if (arg_lengths)
                    efree(arg_lengths);
                return 0;
            }

            /* Convert each argument to string and store */
            for (int i = 1; i < arg_count; i++) {
                zval* arg = &z_args[i];

                if (Z_TYPE_P(arg) == IS_STRING) {
                    /* Already a string, use directly */
                    batch_args[i - 1] = (uintptr_t) Z_STRVAL_P(arg);

                    arg_lengths[i - 1] = Z_STRLEN_P(arg);
                } else {
                    /* Convert to string */
                    zval temp;
                    ZVAL_COPY(&temp, arg);
                    convert_to_string(&temp);

                    batch_args[i - 1]  = (uintptr_t) Z_STRVAL(temp);
                    arg_lengths[i - 1] = Z_STRLEN(temp);

                    /* Note: We're not freeing temp here as batch_args points to its string data
                     */
                }
            }
        }
        enum RequestType* output = emalloc(sizeof(enum RequestType));
        *output                  = command_type;
        /* Buffer the command for batch execution */
        int buffer_result = buffer_command_for_batch(valkey_glide,
                                                     command_type,
                                                     batch_args,
                                                     arg_lengths,
                                                     arg_count - 1, /* number of args */
                                                     output,        /* result_ptr */
                                                     command_response_to_zval_wrapper);

        /* Free the argument arrays */
        if (batch_args)
            efree(batch_args);
        if (arg_lengths)
            efree(arg_lengths);

        if (buffer_result) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 0;
    }

    /* Non-batch mode execution - preserve existing logic */
    if (is_cluster) {
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
        /* For non-cluster, we already have all arguments parsed */
        if (arg_count == 0) {
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
    if (is_cluster && valkey_glide->is_in_batch_mode) {
        /* DBSIZE cannot be used in batch mode */
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
    if (execute_core_command(
            valkey_glide, &core_args, &dbsize, process_core_int_result, return_value)) {
        if (valkey_glide->is_in_batch_mode) {
            /* In batch mode, return $this for method chaining */
            ZVAL_COPY(return_value, object);
            return 1;
        }
        return 1;
    }

    return 0;
}
