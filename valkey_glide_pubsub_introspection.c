/** Copyright Valkey GLIDE Project Contributors - SPDX Identifier: Apache-2.0 */

#include "valkey_glide_pubsub_introspection.h"

#include "command_response.h"
#include "include/glide/command_request.pb-c.h"
#include "include/glide_bindings.h"
#include "logger.h"
#include "php.h"
#include "valkey_glide_commands_common.h"
#include "zend_exceptions.h"

void valkey_glide_pubsub_impl(INTERNAL_FUNCTION_PARAMETERS, const void* connection) {
    zend_string* subcommand;
    zval*        arg = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
    Z_PARAM_STR(subcommand)
    Z_PARAM_OPTIONAL
    Z_PARAM_ZVAL(arg)
    ZEND_PARSE_PARAMETERS_END();

    const char* cmd     = ZSTR_VAL(subcommand);
    size_t      cmd_len = ZSTR_LEN(subcommand);

    if (strcasecmp(cmd, "channels") == 0) {
        // PUBSUB CHANNELS [pattern]
        uint32_t       argc     = arg ? 1 : 0;
        uintptr_t*     args     = argc ? emalloc(argc * sizeof(uintptr_t)) : NULL;
        unsigned long* args_len = argc ? emalloc(argc * sizeof(unsigned long)) : NULL;

        if (arg) {
            convert_to_string(arg);
            args[0]     = (uintptr_t) Z_STRVAL_P(arg);
            args_len[0] = Z_STRLEN_P(arg);
        }

        struct CommandResult* result =
            command(connection,
                    0,
                    (enum RequestType) COMMAND_REQUEST__REQUEST_TYPE__PubSubChannels,
                    argc,
                    args,
                    args_len,
                    NULL,
                    0,
                    0);
        if (args)
            efree(args);
        if (args_len)
            efree(args_len);

        if (result && result->response && !result->command_error) {
            command_response_to_zval(result->response, return_value, 0, false);
            free_command_result(result);
        } else {
            const char* error_msg =
                result && result->command_error && result->command_error->command_error_message
                    ? result->command_error->command_error_message
                    : "PUBSUB CHANNELS command failed";
            VALKEY_LOG_ERROR("pubsub_channels", error_msg);
            if (result)
                free_command_result(result);
            zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
            RETURN_FALSE;
        }
    } else if (strcasecmp(cmd, "numsub") == 0) {
        // PUBSUB NUMSUB [channel ...]
        if (!arg || Z_TYPE_P(arg) != IS_ARRAY) {
            zend_throw_exception(
                get_valkey_glide_exception_ce(), "NUMSUB requires array of channels", 0);
            RETURN_FALSE;
        }

        HashTable* channels_ht   = Z_ARRVAL_P(arg);
        uint32_t   channel_count = zend_hash_num_elements(channels_ht);
        uint32_t   argc          = channel_count;

        uintptr_t*     args     = emalloc(argc * sizeof(uintptr_t));
        unsigned long* args_len = emalloc(argc * sizeof(unsigned long));

        uint32_t i = 0;
        zval*    channel_zv;
        ZEND_HASH_FOREACH_VAL(channels_ht, channel_zv) {
            convert_to_string(channel_zv);
            args[i]     = (uintptr_t) Z_STRVAL_P(channel_zv);
            args_len[i] = Z_STRLEN_P(channel_zv);
            i++;
        }
        ZEND_HASH_FOREACH_END();

        struct CommandResult* result =
            command(connection,
                    0,
                    (enum RequestType) COMMAND_REQUEST__REQUEST_TYPE__PubSubNumSub,
                    argc,
                    args,
                    args_len,
                    NULL,
                    0,
                    0);
        efree(args);
        efree(args_len);

        if (result && result->response && !result->command_error) {
            command_response_to_zval(result->response, return_value, 0, false);
            free_command_result(result);
        } else {
            const char* error_msg =
                result && result->command_error && result->command_error->command_error_message
                    ? result->command_error->command_error_message
                    : "PUBSUB NUMSUB command failed";
            VALKEY_LOG_ERROR("pubsub_numsub", error_msg);
            if (result)
                free_command_result(result);
            zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
            RETURN_FALSE;
        }
    } else if (strcasecmp(cmd, "numpat") == 0) {
        // PUBSUB NUMPAT
        struct CommandResult* result =
            command(connection,
                    0,
                    (enum RequestType) COMMAND_REQUEST__REQUEST_TYPE__PubSubNumPat,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    0,
                    0);

        if (result && result->response && !result->command_error) {
            command_response_to_zval(result->response, return_value, 0, false);
            free_command_result(result);
        } else {
            const char* error_msg =
                result && result->command_error && result->command_error->command_error_message
                    ? result->command_error->command_error_message
                    : "PUBSUB NUMPAT command failed";
            VALKEY_LOG_ERROR("pubsub_numpat", error_msg);
            if (result)
                free_command_result(result);
            zend_throw_exception(get_valkey_glide_exception_ce(), error_msg, 0);
            RETURN_FALSE;
        }
    } else {
        zend_throw_exception(get_valkey_glide_exception_ce(), "Invalid PUBSUB subcommand", 0);
        RETURN_FALSE;
    }
}
