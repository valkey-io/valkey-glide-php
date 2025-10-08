<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

/*
* --------------------------------------------------------------------
*                   The PHP License, version 3.01
* Copyright (c) 1999 - 2010 The PHP Group. All rights reserved.
* --------------------------------------------------------------------
*
* Redistribution and use in source and binary forms, with or without
* modification, is permitted provided that the following conditions
* are met:
*
*   1. Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in
*      the documentation and/or other materials provided with the
*      distribution.
*
*   3. The name "PHP" must not be used to endorse or promote products
*      derived from this software without prior written permission. For
*      written permission, please contact group@php.net.
*
*   4. Products derived from this software may not be called "PHP", nor
*      may "PHP" appear in their name, without prior written permission
*      from group@php.net.  You may indicate that your software works in
*      conjunction with PHP by saying "Foo for PHP" instead of calling
*      it "PHP Foo" or "phpfoo"
*
*   5. The PHP Group may publish revised and/or new versions of the
*      license from time to time. Each version will be given a
*      distinguishing version number.
*      Once covered code has been published under a particular version
*      of the license, you may always continue to use it under the terms
*      of that version. You may also choose to use such covered code
*      under the terms of any subsequent version of the license
*      published by the PHP Group. No one other than the PHP Group has
*      the right to modify the terms applicable to covered code created
*      under this License.
*
*   6. Redistributions of any form whatsoever must retain the following
*      acknowledgment:
*      "This product includes PHP software, freely available from
*      <http://www.php.net/software/>".
*
* THIS SOFTWARE IS PROVIDED BY THE PHP DEVELOPMENT TEAM ``AS IS'' AND
* ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
* THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PHP
* DEVELOPMENT TEAM OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
* --------------------------------------------------------------------
*
* This software consists of voluntary contributions made by many
* individuals on behalf of the PHP Group.
*
* The PHP Group can be contacted via Email at group@php.net.
*
* For more information on the PHP Group and the PHP project,
* please see <http://www.php.net>.
*
* PHP includes the Zend Engine, freely available at
* <http://www.zend.com>.
*/



class ValkeyGlide
{
    /**
     * Hash field condition constants
     * @var string
     */
    public const CONDITION_NX = "NX";  // Only if field doesn't exist
    
    /**
     * @var string
     */
    public const CONDITION_XX = "XX";  // Only if field exists

    /**
     * Time unit constants for hash field expiration
     * @var string
     */
    public const TIME_UNIT_SECONDS = "EX";           // Expire in seconds
    
    /**
     * @var string
     */
    public const TIME_UNIT_MILLISECONDS = "PX";      // Expire in milliseconds
    
    /**
     * @var string
     */
    public const TIME_UNIT_TIMESTAMP_SECONDS = "EXAT";   // Expire at timestamp (seconds)
    
    /**
     * @var string
     */
    public const TIME_UNIT_TIMESTAMP_MILLISECONDS = "PXAT"; // Expire at timestamp (milliseconds)

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_NOT_FOUND
     *
     */
    public const VALKEY_GLIDE_NOT_FOUND = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_STRING
     *
     */
    public const VALKEY_GLIDE_STRING = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_SET
     *
     */
    public const VALKEY_GLIDE_SET = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_LIST
     *
     */
    public const VALKEY_GLIDE_LIST = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_ZSET
     *
     */
    public const VALKEY_GLIDE_ZSET = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_HASH
     *
     */
    public const VALKEY_GLIDE_HASH = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue VALKEY_GLIDE_STREAM
     *
     */
    public const VALKEY_GLIDE_STREAM = UNKNOWN;

          /**
           *  @var int
           * Always get from primary, in order to get the freshest data.
           */
    public const  READ_FROM_PRIMARY = 0;

          /**
           *  @var int
           * Spread the requests between all replicas in a round robin manner.
           * If no replica is available, route the requests to the primary.
           */
    public const  READ_FROM_PREFER_REPLICA = 1;

          /**
           *  @var int
           * Spread the read requests between replicas in the same client's AZ (Availability zone)
           * in a round robin manner, falling back to other replicas or the primary if needed.
           */
    public const  READ_FROM_AZ_AFFINITY = 2;

          /**
           *  @var int
           * Spread the read requests among nodes within the client's Availability Zone (AZ)
           * in a round robin manner, prioritizing local replicas, then the local primary,
           * and falling back to any replica or the primary if needed.
           */
    public const  READ_FROM_AZ_AFFINITY_REPLICAS_AND_PRIMARY = 3;


    /**
     *
     * @var string
     *
     */
    public const BEFORE = "before";

    /**
     *
     * @var string
     *
     */
    public const AFTER = "after";

    /**
     *
     * @var string
     *
     */
    public const LEFT = "left";

    /**
     *
     * @var string
     *
     */
    public const RIGHT = "right";

    /**
     *
     * @var int
     * @cvalue MULTI
     *
     */
    public const MULTI = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue PIPELINE
     *
     */
    public const PIPELINE = UNKNOWN;

    /**
     * Create a new ValkeyGlide instance with the provided configuration.
     *
     * @param array $addresses                  Array of server addresses [['host' => 'localhost', 'port' => 6379], ...].
     * @param bool $use_tls                     Whether to use TLS encryption.
     * @param array|null $credentials           Authentication credentials ['password' => 'xxx', 'username' => 'yyy'].
     * @param int $read_from                    Read strategy for the client.
     * @param int|null $request_timeout         Request timeout in milliseconds.
     * @param array|null $reconnect_strategy    Reconnection strategy ['num_of_retries' => 3, 'factor' => 2,
     *                                          'exponent_base' => 10, 'jitter_percent' => 15].
     * @param int|null $database_id             Database ID to select (0-15)
     * @param string|null $client_name          Client name identifier.
     * @param string|null $client_az            Client availability zone.
     * @param array|null $advanced_config       Advanced configuration ['connection_timeout' => 5000,
     *                                          'tls_config' => ['use_insecure_tls' => false]].
     *                                          connection_timeout is in milliseconds.
     * @param bool|null $lazy_connect           Whether to use lazy connection.
     */
    public function __construct(
        array $addresses,
        bool $use_tls = false,
        ?array $credentials = null,
        int $read_from = ValkeyGlide::READ_FROM_PRIMARY,
        ?int $request_timeout = null,
        ?array $reconnect_strategy = null,
        ?int $database_id = null,
        ?string $client_name = null,
        ?string $client_az = null,
        ?array $advanced_config = null,
        ?bool $lazy_connect = null
    );

    public function __destruct();



    /**
     * Append data to a ValkeyGlide STRING key.
     *
     * @param string $key   The key in question
     * @param mixed $value  The data to append to the key.
     *
     * @return ValkeyGlide|int|false The new string length of the key or false on failure.
     *
     * @see https://valkey.io/commands/append
     *
     * @example
     * $valkey_glide->set('foo', 'hello);
     * $valkey_glide->append('foo', 'world');
     */
    public function append(string $key, mixed $value): ValkeyGlide|int|false;

    /**
     * Count the number of set bits in a ValkeyGlide string.
     *
     * @see https://valkey.io/commands/bitcount/
     *
     * @param string $key     The key in question (must be a string key)
     * @param int    $start   The index where ValkeyGlide should start counting.  If omitted it
     *                        defaults to zero, which means the start of the string.
     * @param int    $end     The index where ValkeyGlide should stop counting.  If omitted it
     *                        defaults to -1, meaning the very end of the string.
     *
     * @param bool   $bybit   Whether or not ValkeyGlide should treat $start and $end as bit
     *                        positions, rather than bytes.
     *
     * @return ValkeyGlide|int|false The number of bits set in the requested range.
     *
     */
    public function bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false): ValkeyGlide|int|false;

    public function bitop(string $operation, string $deskey, string $srckey, string ...$other_keys): ValkeyGlide|int|false;

    /**
     * Return the position of the first bit set to 0 or 1 in a string.
     *
     * @see https://valkey.io/commands/bitpos/
     *
     * @param string $key   The key to check (must be a string)
     * @param bool   $bit   Whether to look for an unset (0) or set (1) bit.
     * @param int    $start Where in the string to start looking.
     * @param int    $end   Where in the string to stop looking.
     * @param bool   $bybit If true, ValkeyGlide will treat $start and $end as BIT values and not bytes, so if start
     *                      was 0 and end was 2, ValkeyGlide would only search the first two bits.
     *
     * @return ValkeyGlide|int|false The position of the first set or unset bit.
     **/
    public function bitpos(string $key, bool $bit, int $start = 0, int $end = -1, bool $bybit = false): ValkeyGlide|int|false;

    /**
     * Pop an element off the beginning of a ValkeyGlide list or lists, potentially blocking up to a specified
     * timeout.  This method may be called in two distinct ways, of which examples are provided below.
     *
     * @see https://valkey.io/commands/blpop/
     *
     * @param string|array     $key_or_keys    This can either be a string key or an array of one or more
     *                                         keys.
     * @param string|float|int $timeout_or_key If the previous argument was a string key, this can either
     *                                         be an additional key, or the timeout you wish to send to
     *                                         the command.
     *
     * @return ValkeyGlide|array|null|false Can return various things depending on command and data in ValkeyGlide.
     *
     * @example
     * $valkey_glide->blPop('list1', 'list2', 'list3', 1.5);
     * $relay->blPop(['list1', 'list2', 'list3'], 1.5);
     */
    public function blPop(string|array $key_or_keys, string|float|int $timeout_or_key, mixed ...$extra_args): ValkeyGlide|array|null|false;

    /**
     * Pop an element off of the end of a ValkeyGlide list or lists, potentially blocking up to a specified timeout.
     * The calling convention is identical to ValkeyGlide::blPop() so see that documentation for more details.
     *
     * @see https://valkey.io/commands/brpop/
     * @see ValkeyGlide::blPop()
     *
     */
    public function brPop(string|array $key_or_keys, string|float|int $timeout_or_key, mixed ...$extra_args): ValkeyGlide|array|null|false;

    /**
     * POP the maximum scoring element off of one or more sorted sets, blocking up to a specified
     * timeout if no elements are available.
     *
     * Following are examples of the two main ways to call this method.
     *
     * **NOTE**:  We recommend calling this function with an array and a timeout as the other strategy
     *            may be deprecated in future versions of PhpValkeyGlide
     *
     * @see https://valkey.io/commands/bzpopmax
     *
     * @param string|array $key_or_keys    Either a string key or an array of one or more keys.
     * @param string|int  $timeout_or_key  If the previous argument was an array, this argument
     *                                     must be a timeout value.  Otherwise it could also be
     *                                     another key.
     * @param mixed       $extra_args      Can consist of additional keys, until the last argument
     *                                     which needs to be a timeout.
     *
     * @return ValkeyGlide|array|false The popped elements.
     *
     * @example
     * $valkey_glide->bzPopMax('key1', 'key2', 'key3', 1.5);
     * $valkey_glide->bzPopMax(['key1', 'key2', 'key3'], 1.5);
     */
    public function bzPopMax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): ValkeyGlide|array|false;

    /**
     * POP the minimum scoring element off of one or more sorted sets, blocking up to a specified timeout
     * if no elements are available
     *
     * This command is identical in semantics to bzPopMax so please see that method for more information.
     *
     * @see https://valkey.io/commands/bzpopmin
     * @see ValkeyGlide::bzPopMax()
     *
     */
    public function bzPopMin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): ValkeyGlide|array|false;

    /**
     * POP one or more elements from one or more sorted sets, blocking up to a specified amount of time
     * when no elements are available.
     *
     * @param float  $timeout How long to block if there are no element available
     * @param array  $keys    The sorted sets to pop from
     * @param string $from    The string 'MIN' or 'MAX' (case insensitive) telling ValkeyGlide whether you wish to
     *                        pop the lowest or highest scoring members from the set(s).
     * @param int    $count   Pop up to how many elements.
     *
     * @return ValkeyGlide|array|null|false This function will return an array of popped elements, or false
     *                                depending on whether any elements could be popped within the
     *                                specified timeout.
     *
     */
    public function bzmpop(float $timeout, array $keys, string $from, int $count = 1): ValkeyGlide|array|null|false;

    /**
     * POP one or more of the highest or lowest scoring elements from one or more sorted sets.
     *
     * @see https://valkey.io/commands/zmpop
     *
     * @param array  $keys  One or more sorted sets
     * @param string $from  The string 'MIN' or 'MAX' (case insensitive) telling ValkeyGlide whether you want to
     *                      pop the lowest or highest scoring elements.
     * @param int    $count Pop up to how many elements at once.
     *
     * @return ValkeyGlide|array|null|false An array of popped elements or false if none could be popped.
     */
    public function zmpop(array $keys, string $from, int $count = 1): ValkeyGlide|array|null|false;

    /**
     * Pop one or more elements from one or more ValkeyGlide LISTs, blocking up to a specified timeout when
     * no elements are available.
     *
     * @see https://valkey.io/commands/blmpop
     *
     * @param float  $timeout The number of seconds ValkeyGlide will block when no elements are available.
     * @param array  $keys    One or more ValkeyGlide LISTs to pop from.
     * @param string $from    The string 'LEFT' or 'RIGHT' (case insensitive), telling ValkeyGlide whether
     *                        to pop elements from the beginning or end of the LISTs.
     * @param int    $count   Pop up to how many elements at once.
     *
     * @return ValkeyGlide|array|null|false One or more elements popped from the list(s) or false if all LISTs
     *                                were empty.
     */
    public function blmpop(float $timeout, array $keys, string $from, int $count = 1): ValkeyGlide|array|null|false;

    /**
     * Pop one or more elements off of one or more ValkeyGlide LISTs.
     *
     * @see https://valkey.io/commands/lmpop
     *
     * @param array  $keys  An array with one or more ValkeyGlide LIST key names.
     * @param string $from  The string 'LEFT' or 'RIGHT' (case insensitive), telling ValkeyGlide whether to pop\
     *                      elements from the beginning or end of the LISTs.
     * @param int    $count The maximum number of elements to pop at once.
     *
     * @return ValkeyGlide|array|null|false One or more elements popped from the LIST(s) or false if all the LISTs
     *                                were empty.
     *
     */
    public function lmpop(array $keys, string $from, int $count = 1): ValkeyGlide|array|null|false;


    public function client(string $opt, mixed ...$args): mixed;

    public function close(): bool;


    /**
     *  Execute the ValkeyGlide CONFIG command in a variety of ways.
     *
     *  What the command does in particular depends on the `$operation` qualifier.
     *  Operations that PhpValkeyGlide supports are: RESETSTAT, REWRITE, GET, and SET.
     *
     * @param string $operation The CONFIG operation to execute (e.g. GET, SET, REWRITE).
     * @param array|string|null $key_or_settings One or more keys or values.
     * @param string $value The value if this is a `CONFIG SET` operation.
     * @see https://valkey.io/commands/config
     *
     * @example
     * $valkey_glide->config('GET', 'timeout');
     * $valkey_glide->config('GET', ['timeout', 'databases']);
     * $valkey_glide->config('SET', 'timeout', 30);
     * $valkey_glide->config('SET', ['timeout' => 30, 'loglevel' => 'warning']);
     */
    public function config(string $operation, array|string|null $key_or_settings = null, ?string $value = null): mixed;


    /**
     * Make a copy of a key.
     *
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     *
     * @param string $src     The key to copy
     * @param string $dst     The name of the new key created from the source key.
     * @param array  $options An array with modifiers on how COPY should operate.
     *                        <code>
     *                        $options = [
     *                            'REPLACE' => true|false # Whether to replace an existing key.
     *                            'DB' => int             # Copy key to specific db.
     *                        ];
     *                        </code>
     *
     * @return ValkeyGlide|bool True if the copy was completed and false if not.
     *
     * @see https://valkey.io/commands/copy
     *
     * @example
     * $valkey_glide->pipeline()
     *       ->select(1)
     *       ->del('newkey')
     *       ->select(0)
     *       ->del('newkey')
     *       ->mset(['source1' => 'value1', 'exists' => 'old_value'])
     *       ->exec();
     *
     * var_dump($valkey_glide->copy('source1', 'newkey'));
     * var_dump($valkey_glide->copy('source1', 'newkey', ['db' => 1]));
     * var_dump($valkey_glide->copy('source1', 'exists'));
     * var_dump($valkey_glide->copy('source1', 'exists', ['REPLACE' => true]));
     */
    public function copy(string $src, string $dst, ?array $options = null): ValkeyGlide|bool;

    /**
     * Return the number of keys in the currently selected ValkeyGlide database.
     *
     * @see https://valkey.io/commands/dbsize
     *
     * @return ValkeyGlide|int The number of keys or false on failure.
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     * $valkey_glide->flushdb();
     * $valkey_glide->set('foo', 'bar');
     * var_dump($valkey_glide->dbsize());
     * $valkey_glide->mset(['a' => 'a', 'b' => 'b', 'c' => 'c', 'd' => 'd']);
     * var_dump($valkey_glide->dbsize());
     */
    public function dbSize(): ValkeyGlide|int|false;

    /**
     * Decrement a ValkeyGlide integer by 1 or a provided value.
     *
     * @param string $key The key to decrement
     * @param int    $by  How much to decrement the key.  Note that if this value is
     *                    not sent or is set to `1`, PhpValkeyGlide will actually invoke
     *                    the 'DECR' command.  If it is any value other than `1`
     *                    PhpValkeyGlide will actually send the `DECRBY` command.
     *
     * @return ValkeyGlide|int|false The new value of the key or false on failure.
     *
     * @see https://valkey.io/commands/decr
     * @see https://valkey.io/commands/decrby
     *
     * @example $valkey_glide->decr('counter');
     * @example $valkey_glide->decr('counter', 2);
     */
    public function decr(string $key, int $by = 1): ValkeyGlide|int|false;

    /**
     * Decrement a valkey integer by a value
     *
     * @param string $key   The integer key to decrement.
     * @param int    $value How much to decrement the key.
     *
     * @return ValkeyGlide|int|false The new value of the key or false on failure.
     *
     * @see https://valkey.io/commands/decrby
     *
     * @example $valkey_glide->decrby('counter', 1);
     * @example $valkey_glide->decrby('counter', 2);
     */
    public function decrBy(string $key, int $value): ValkeyGlide|int|false;

    /**
     * Delete one or more keys from ValkeyGlide.
     *
     * This method can be called in two distinct ways.  The first is to pass a single array
     * of keys to delete, and the second is to pass N arguments, all names of keys.  See
     * below for an example of both strategies.
     *
     * @param array|string $key_or_keys Either an array with one or more key names or a string with
     *                                  the name of a key.
     * @param string       $other_keys  One or more additional keys passed in a variadic fashion.
     *
     * @return ValkeyGlide|int|false The number of keys that were deleted
     *
     * @see https://valkey.io/commands/del
     *
     * @example $valkey_glide->del('key:0', 'key:1');
     * @example $valkey_glide->del(['key:2', 'key:3', 'key:4']);
     */
    public function del(array|string $key, string ...$other_keys): ValkeyGlide|int|false;

    /**
     * Discard a transaction currently in progress.
     *
     * @return ValkeyGlide|bool  True if we could discard the transaction.
     *
     * @example
     * $valkey_glide->set('foo', 'bar');
     * $valkey_glide->discard();
     */
    public function discard(): ValkeyGlide|bool;

    /**
     * Dump ValkeyGlide' internal binary representation of a key.
     *
     * <code>
     * $valkey_glide->zRange('new-zset', 0, -1, true);
     * </code>
     *
     * @param string $key The key to dump.
     *
     * @return ValkeyGlide|string A binary string representing the key's value.
     *
     * @see https://valkey.io/commands/dump
     *
     * @example
     * $valkey_glide->zadd('zset', 0, 'zero', 1, 'one', 2, 'two');
     * $binary = $valkey_glide->dump('zset');
     * $valkey_glide->restore('new-zset', 0, $binary);
     */
    public function dump(string $key): ValkeyGlide|string|false;

    /**
     * Have ValkeyGlide repeat back an arbitrary string to the client.
     *
     * @param string $str The string to echo
     *
     * @return ValkeyGlide|string|false The string sent to ValkeyGlide or false on failure.
     *
     * @see https://valkey.io/commands/echo
     *
     * @example $valkey_glide->echo('Hello, World');
     */
    public function echo(string $str): ValkeyGlide|string|false;

    /**
     * Execute a LUA script on the valkey server.
     *
     * @see https://valkey.io/commands/eval/
     *
     * @param string $script   A string containing the LUA script
     * @param array  $args     An array of arguments to pass to this script
     * @param int    $num_keys How many of the arguments are keys.  This is needed
     *                         as valkey distinguishes between key name arguments
     *                         and other data.
     *
     * @return mixed LUA scripts may return arbitrary data so this method can return
     *               strings, arrays, nested arrays, etc.
     */
   /*TODO  public function eval(string $script, array $args = [], int $num_keys = 0): mixed;*/

    /**
     * This is simply the read-only variant of eval, meaning the underlying script
     * may not modify data in valkey.
     *
     * @see ValkeyGlide::eval_ro()
     */
    /* TODO public function eval_ro(string $script_sha, array $args = [], int $num_keys = 0): mixed; */

    /**
     * Execute a LUA script on the server but instead of sending the script, send
     * the SHA1 hash of the script.
     *
     * @param string $script_sha The SHA1 hash of the lua code.  Note that the script
     *                           must already exist on the server, either having been
     *                           loaded with `SCRIPT LOAD` or having been executed directly
     *                           with `EVAL` first.
     * @param array  $args       Arguments to send to the script.
     * @param int    $num_keys   The number of arguments that are keys
     *
     * @return mixed Returns whatever the specific script does.
     *
     * @see https://valkey.io/commands/evalsha/
     * @see ValkeyGlide::eval();
     *
     */
   /* TODO public function evalsha(string $sha1, array $args = [], int $num_keys = 0): mixed; */

    /**
     * This is simply the read-only variant of evalsha, meaning the underlying script
     * may not modify data in valkey.
     *
     * @see ValkeyGlide::evalsha()
     */
    /* TODO public function evalsha_ro(string $sha1, array $args = [], int $num_keys = 0): mixed; */

    /**
     * Execute either a MULTI or PIPELINE block and return the array of replies.
     *
     * @return ValkeyGlide|array|false The array of pipeline'd or multi replies or false on failure.
     *
     * @see https://valkey.io/commands/exec
     * @see https://valkey.io/commands/multi
     * @see ValkeyGlide::pipeline()
     * @see ValkeyGlide::multi()
     *
     * @example
     * $res = $valkey_glide->multi()
     *              ->set('foo', 'bar')
     *              ->get('foo')
     *              ->del('list')
     *              ->rpush('list', 'one', 'two', 'three')
     *              ->exec();
     */
    public function exec(): ValkeyGlide|array|false;

    /**
     * Test if one or more keys exist.
     *
     * @param mixed $key         Either an array of keys or a string key
     * @param mixed $other_keys  If the previous argument was a string, you may send any number of
     *                           additional keys to test.
     *
     * @return ValkeyGlide|int|bool    The number of keys that do exist and false on failure
     *
     * @see https://valkey.io/commands/exists
     *
     * @example $valkey_glide->exists(['k1', 'k2', 'k3']);
     * @example $valkey_glide->exists('k4', 'k5', 'notakey');
     */
    public function exists(mixed $key, mixed ...$other_keys): ValkeyGlide|int|bool;

    /**
     * Sets an expiration in seconds on the key in question.  If connected to
     * valkey-server >= 7.0.0 you may send an additional "mode" argument which
     * modifies how the command will execute.
     *
     * @param string      $key  The key to set an expiration on.
     * @param int         $timeout  The number of seconds after which key will be automatically deleted.
     * @param string|null $mode  A two character modifier that changes how the
     *                      command works.
     *                      <code>
     *                      NX - Set expiry only if key has no expiry
     *                      XX - Set expiry only if key has an expiry
     *                      LT - Set expiry only when new expiry is < current expiry
     *                      GT - Set expiry only when new expiry is > current expiry
     *                      </code>
     *
     * @return ValkeyGlide|bool True if an expiration was set and false otherwise.
     * @see https://valkey.io/commands/expire
     *
     */
    public function expire(string $key, int $timeout, ?string $mode = null): ValkeyGlide|bool;

    /*
     * Set a key's expiration to a specific Unix timestamp in seconds.
     *
     * If connected to ValkeyGlide >= 7.0.0 you can pass an optional 'mode' argument.
     * @see ValkeyGlide::expire() For a description of the mode argument.
     *
     * @param string $key The key to set an expiration on.
     *
     * @return ValkeyGlide|bool True if an expiration was set, false if not.
     *
     */

    /**
     * Set a key to expire at an exact unix timestamp.
     *
     * @param string      $key The key to set an expiration on.
     * @param int         $timestamp The unix timestamp to expire at.
     * @param string|null $mode An option 'mode' that modifies how the command acts (see {@link ValkeyGlide::expire}).
     * @return ValkeyGlide|bool True if an expiration was set, false if not.
     *
     * @see https://valkey.io/commands/expireat
     * @see https://valkey.io/commands/expire
     * @see ValkeyGlide::expire()
     */
    public function expireAt(string $key, int $timestamp, ?string $mode = null): ValkeyGlide|bool;


    /**
     * Get the expiration of a given key as a unix timestamp
     *
     * @param string $key      The key to check.
     *
     * @return ValkeyGlide|int|false The timestamp when the key expires, or -1 if the key has no expiry
     *                         and -2 if the key doesn't exist.
     *
     * @see https://valkey.io/commands/expiretime
     *
     * @example
     * $valkey_glide->setEx('mykey', 60, 'myval');
     * $valkey_glide->expiretime('mykey');
     */
    public function expiretime(string $key): ValkeyGlide|int|false;

    /**
     * Get the expiration timestamp of a given ValkeyGlide key but in milliseconds.
     *
     * @see https://valkey.io/commands/pexpiretime
     * @see ValkeyGlide::expiretime()
     *
     * @param string $key      The key to check
     *
     * @return ValkeyGlide|int|false The expiration timestamp of this key (in milliseconds) or -1 if the
     *                         key has no expiration, and -2 if it does not exist.
     */
    public function pexpiretime(string $key): ValkeyGlide|int|false;

    /**
     * Invoke a function.
     *
     * @param string $fn    The name of the function
     * @param array  $keys  Optional list of keys
     * @param array  $args  Optional list of args
     *
     * @return mixed        Function may return arbitrary data so this method can return
     *                      strings, arrays, nested arrays, etc.
     *
     * @see https://valkey.io/commands/fcall
     */
    public function fcall(string $fn, array $keys = [], array $args = []): mixed; 

    /**
     * This is a read-only variant of the FCALL command that cannot execute commands that modify data.
     *
     * @param string $fn    The name of the function
     * @param array  $keys  Optional list of keys
     * @param array  $args  Optional list of args
     *
     * @return mixed        Function may return arbitrary data so this method can return
     *                      strings, arrays, nested arrays, etc.
     *
     * @see https://valkey.io/commands/fcall_ro
     */
    public function fcall_ro(string $fn, array $keys = [], array $args = []): mixed; 

    /**
     * Deletes every key in all ValkeyGlide databases
     *
     * @param  bool  $sync Whether to perform the task in a blocking or non-blocking way.
     * @return bool
     *
     * @see https://valkey.io/commands/flushall
     */
    public function flushAll(?bool $sync = null): ValkeyGlide|bool;

    /**
     * Deletes all the keys of the currently selected database.
     *
     * @param  bool  $sync Whether to perform the task in a blocking or non-blocking way.
     * @return bool
     *
     * @see https://valkey.io/commands/flushdb
     */
    public function flushDB(?bool $sync = null): ValkeyGlide|bool;

    /**
     * Functions is an API for managing code to be executed on the server.
     *
     * @param string $operation         The subcommand you intend to execute.  Valid options are as follows
     *                                  'LOAD'      - Create a new library with the given library name and code.
     *                                  'DELETE'    - Delete the given library.
     *                                  'LIST'      - Return general information on all the libraries
     *                                  'STATS'     - Return information about the current function running
     *                                  'KILL'      - Kill the current running function
     *                                  'FLUSH'     - Delete all the libraries
     *                                  'DUMP'      - Return a serialized payload representing the current libraries
     *                                  'RESTORE'   - Restore the libraries represented by the given payload
     * @param member $args              Additional arguments
     *
     * @return ValkeyGlide|bool|string|array  Depends on subcommand.
     *
     * @see https://valkey.io/commands/function
     */
    public function function(string $operation, mixed ...$args): ValkeyGlide|bool|string|array;

    /**
     * Add one or more members to a geospacial sorted set
     *
     * @param string $key The sorted set to add data to.
     * @param float  $lng The longitude of the first member
     * @param float  $lat The latitude of the first member.
     * @param member $other_triples_and_options You can continue to pass longitude, latitude, and member
     *               arguments to add as many members as you wish.  Optionally, the final argument may be
     *               a string with options for the command @see ValkeyGlide documentation for the options.
     *
     * @return ValkeyGlide|int|false The number of added elements is returned.  If the 'CH' option is specified,
     *                         the return value is the number of members *changed*.
     *
     * @example $valkey_glide->geoAdd('cities', -121.8374, 39.7284, 'Chico', -122.03218, 37.322, 'Cupertino');
     * @example $valkey_glide->geoadd('cities', -121.837478, 39.728494, 'Chico', ['XX', 'CH']);
     *
     * @see https://valkey.io/commands/geoadd
     */

    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples_and_options): ValkeyGlide|int|false;

    /**
     * Get the distance between two members of a geospacially encoded sorted set.
     *
     * @param string $key  The Sorted set to query.
     * @param string $src  The first member.
     * @param string $dst  The second member.
     * @param string $unit Which unit to use when computing distance, defaulting to meters.
     *                     <code>
     *                     M  - meters
     *                     KM - kilometers
     *                     FT - feet
     *                     MI - miles
     *                     </code>
     *
     * @return ValkeyGlide|float|false The calculated distance in whichever units were specified or false
     *                           if one or both members did not exist.
     *
     * @example $valkey_glide->geodist('cities', 'Chico', 'Cupertino', 'mi');
     *
     * @see https://valkey.io/commands/geodist
     */
    public function geodist(string $key, string $src, string $dst, ?string $unit = null): ValkeyGlide|float|false;

    /**
     * Retrieve one or more GeoHash encoded strings for members of the set.
     *
     * @param string $key           The key to query
     * @param string $member        The first member to request
     * @param string $other_members One or more additional members to request.
     *
     * @return ValkeyGlide|array|false    An array of GeoHash encoded values.
     *
     * @see https://valkey.io/commands/geohash
     * @see https://en.wikipedia.org/wiki/Geohash
     *
     * @example $valkey_glide->geohash('cities', 'Chico', 'Cupertino');
     */
    public function geohash(string $key, string $member, string ...$other_members): ValkeyGlide|array|false;

    /**
     * Return the longitude and latitude for one or more members of a geospacially encoded sorted set.
     *
     * @param string $key           The set to query.
     * @param string $member        The first member to query.
     * @param string $other_members One or more members to query.
     *
     * @return An array of longitude and latitude pairs.
     *
     * @see https://valkey.io/commands/geopos
     *
     * @example $valkey_glide->geopos('cities', 'Seattle', 'New York');
     */
    public function geopos(string $key, string $member, string ...$other_members): ValkeyGlide|array|false;


    /**
     * Search a geospacial sorted set for members in various ways.
     *
     * @param string          $key      The set to query.
     * @param array|string    $position Either a two element array with longitude and latitude, or
     *                                  a string representing a member of the set.
     * @param array|int|float $shape    Either a number representine the radius of a circle to search, or
     *                                  a two element array representing the width and height of a box
     *                                  to search.
     * @param string          $unit     The unit of our shape.  See {@link ValkeyGlide::geodist} for possible units.
     * @param array           $options  @see {@link ValkeyGlide::georadius} for options.  Note that the `STORE`
     *                                  options are not allowed for this command.
     */
    public function geosearch(string $key, array|string $position, array|int|float $shape, string $unit, array $options = []): array;

    /**
     * Search a geospacial sorted set for members within a given area or range, storing the results into
     * a new set.
     *
     * @param string $dst The destination where results will be stored.
     * @param string $src The key to query.
     * @param array|string    $position Either a two element array with longitude and latitude, or
     *                                  a string representing a member of the set.
     * @param array|int|float $shape    Either a number representine the radius of a circle to search, or
     *                                  a two element array representing the width and height of a box
     *                                  to search.
     * @param string          $unit     The unit of our shape.  See {@link ValkeyGlide::geodist} for possible units.
     * @param array           $options
     *                        <code>
     *                        $options = [
     *                            'ASC' | 'DESC',  # The sort order of returned members
     *                            'WITHDIST'       # Also store distances.
     *
     *                            # Limit to N returned members.  Optionally a two element array may be
     *                            # passed as the `LIMIT` argument, and the `ANY` argument.
     *                            'COUNT' => [<int>], or [<int>, <bool>]
     *                        ];
     *                        </code>
     */
    public function geosearchstore(string $dst, string $src, array|string $position, array|int|float $shape, string $unit, array $options = []): ValkeyGlide|array|int|false;

    /**
     * Retrieve a string keys value.
     *
     * @param  string  $key The key to query
     * @return mixed   The keys value or false if it did not exist.
     *
     * @see https://valkey.io/commands/get
     *
     * @example $valkey_glide->get('foo');
     */
    public function get(string $key): mixed;


    /**
     * Get the bit at a given index in a string key.
     *
     * @param string $key The key to query.
     * @param int    $idx The Nth bit that we want to query.
     *
     * @example $valkey_glide->getbit('bitmap', 1337);
     *
     * @see https://valkey.io/commands/getbit
     */
    public function getBit(string $key, int $idx): ValkeyGlide|int|false;

    /**
     * Get the value of a key and optionally set it's expiration.
     *
     * @param string $key    The key to query
     * @param array $options Options to modify how the command works.
     *                       <code>
     *                       $options = [
     *                           'EX'     => <seconds>      # Expire in N seconds
     *                           'PX'     => <milliseconds> # Expire in N milliseconds
     *                           'EXAT'   => <timestamp>    # Expire at a unix timestamp (in seconds)
     *                           'PXAT'   => <mstimestamp>  # Expire at a unix timestamp (in milliseconds);
     *                           'PERSIST'                  # Remove any configured expiration on the key.
     *                       ];
     *                       </code>
     *
     * @return ValkeyGlide|string|bool The key's value or false if it didn't exist.
     *
     * @see https://valkey.io/commands/getex
     *
     * @example $valkey_glide->getEx('mykey', ['EX' => 60]);
     */
    public function getEx(string $key, array $options = []): ValkeyGlide|string|bool;


    /**
     * Get a key from ValkeyGlide and delete it in an atomic operation.
     *
     * @param string $key The key to get/delete.
     * @return ValkeyGlide|string|bool The value of the key or false if it didn't exist.
     *
     * @see https://valkey.io/commands/getdel
     *
     * @example $valkey_glide->getdel('token:123');
     */
    public function getDel(string $key): ValkeyGlide|string|bool;

    /**
     * Retrieve a substring of a string by index.
     *
     * @param string $key   The string to query.
     * @param int    $start The zero-based starting index.
     * @param int    $end   The zero-based ending index.
     *
     * @return ValkeyGlide|string|false The substring or false on failure.
     *
     * @see https://valkey.io/commands/getrange
     *
     * @example
     * $valkey_glide->set('silly-word', 'Supercalifragilisticexpialidocious');
     * echo $valkey_glide->getRange('silly-word', 0, 4) . "\n";
     */
    public function getRange(string $key, int $start, int $end): ValkeyGlide|string|false;

    /**
     * Get the longest common subsequence between two string keys.
     *
     * @param string $key1    The first key to check
     * @param string $key2    The second key to check
     * @param array  $options An optional array of modifiers for the command.
     *
     *                        <code>
     *                        $options = [
     *                            'MINMATCHLEN'  => int  # Exclude matching substrings that are less than this value
     *
     *                            'WITHMATCHLEN' => bool # Whether each match should also include its length.
     *
     *                            'LEN'                  # Return the length of the longest subsequence
     *
     *                            'IDX'                  # Each returned match will include the indexes where the
     *                                                   # match occurs in each string.
     *                        ];
     *                        </code>
     *
     *                        NOTE:  'LEN' cannot be used with 'IDX'.
     *
     * @return ValkeyGlide|string|array|int|false Various reply types depending on options.
     *
     * @see https://valkey.io/commands/lcs
     *
     * @example
     * $valkey_glide->set('seq1', 'gtaggcccgcacggtctttaatgtatccctgtttaccatgccatacctgagcgcatacgc');
     * $valkey_glide->set('seq2', 'aactcggcgcgagtaccaggccaaggtcgttccagagcaaagactcgtgccccgctgagc');
     * echo $valkey_glide->lcs('seq1', 'seq2') . "\n";
     */
    public function lcs(string $key1, string $key2, ?array $options = null): ValkeyGlide|string|array|int|false;


    /**
     * Sets a key and returns any previously set value, if the key already existed.
     *
     * @param string $key The key to set.
     * @param mixed $value The value to set the key to.
     *
     * @return ValkeyGlide|string|false The old value of the key or false if it didn't exist.
     *
     * @see https://valkey.io/commands/getset
     *
     * @example
     * $valkey_glide->getset('captain', 'Pike');
     * $valkey_glide->getset('captain', 'Kirk');
     */
    public function getset(string $key, mixed $value): ValkeyGlide|string|false;


    /**
     * Remove one or more fields from a hash.
     *
     * @param string $key          The hash key in question.
     * @param string $field        The first field to remove
     * @param string $other_fields One or more additional fields to remove.
     *
     * @return ValkeyGlide|int|false     The number of fields actually removed.
     *
     * @see https://valkey.io/commands/hdel
     *
     * @example $valkey_glide->hDel('communication', 'Alice', 'Bob');
     */
    public function hDel(string $key, string $field, string ...$other_fields): ValkeyGlide|int|false;

    /**
     * Checks whether a field exists in a hash.
     *
     * @param string $key   The hash to query.
     * @param string $field The field to check
     *
     * @return ValkeyGlide|bool   True if it exists, false if not.
     *
     * @see https://valkey.io/commands/hexists
     *
     * @example $valkey_glide->hExists('communication', 'Alice');
     */
    public function hExists(string $key, string $field): ValkeyGlide|bool;

    public function hGet(string $key, string $member): mixed;

    /**
     * Read every field and value from a hash.
     *
     * @param string $key The hash to query.
     * @return ValkeyGlide|array<string|int, mixed>|false All fields and values or false if the key didn't exist.
     *
     * @see https://valkey.io/commands/hgetall
     *
     * @example $valkey_glide->hgetall('myhash');
     */
    public function hGetAll(string $key): ValkeyGlide|array|false;

    /**
     * Increment a hash field's value by an integer
     *
     * @param string $key   The hash to modify
     * @param string $field The field to increment
     * @param int    $value How much to increment the value.
     *
     * @return ValkeyGlide|int|false The new value of the field.
     *
     * @see https://valkey.io/commands/hincrby
     *
     * @example
     * $valkey_glide->hMSet('player:1', ['name' => 'Alice', 'score' => 0]);
     * $valkey_glide->hincrby('player:1', 'score', 10);
     *
     */
    public function hIncrBy(string $key, string $field, int $value): ValkeyGlide|int|false;

    /**
     * Increment a hash field by a floating point value
     *
     * @param string $key The hash with the field to increment.
     * @param string $field The field to increment.
     *
     * @return ValkeyGlide|float|false The field value after incremented.
     *
     * @see https://valkey.io/commands/hincrbyfloat
     *
     * @example
     * $valkey_glide->hincrbyfloat('numbers', 'tau', 2 * 3.1415926);
     */
    public function hIncrByFloat(string $key, string $field, float $value): ValkeyGlide|float|false;

    /**
     * Retrieve all of the fields of a hash.
     *
     * @param string $key The hash to query.
     *
     * @return ValkeyGlide|list<string>|false The fields in the hash or false if the hash doesn't exist.
     *
     * @see https://valkey.io/commands/hkeys
     *
     * @example $valkey_glide->hkeys('myhash');
     */
    public function hKeys(string $key): ValkeyGlide|array|false;

    /**
     * Get the number of fields in a hash.
     *
     * @see https://valkey.io/commands/hlen
     *
     * @param string $key The hash to check.
     *
     * @return ValkeyGlide|int|false The number of fields or false if the key didn't exist.
     *
     * @example $valkey_glide->hlen('myhash');
     */
    public function hLen(string $key): ValkeyGlide|int|false;

    /**
     * Get one or more fields from a hash.
     *
     * @param string $key    The hash to query.
     * @param array  $fields One or more fields to query in the hash.
     *
     * @return ValkeyGlide|array|false The fields and values or false if the key didn't exist.
     *
     * @see https://valkey.io/commands/hmget
     *
     * @example $valkey_glide->hMGet('player:1', ['name', 'score']);
     */
    public function hMget(string $key, array $fields): ValkeyGlide|array|false;

    /**
     * Add or update one or more hash fields and values
     *
     * @param string $key        The hash to create/update
     * @param array  $fieldvals  An associative array with fields and their values.
     *
     * @return ValkeyGlide|bool True if the operation was successful
     *
     * @see https://valkey.io/commands/hmset
     *
     * @example $valkey_glide->hmset('updates', ['status' => 'starting', 'elapsed' => 0]);
     */
    public function hMset(string $key, array $fieldvals): ValkeyGlide|bool;

    /**
     * Get one or more random field from a hash.
     *
     * @param string $key     The hash to query.
     * @param array  $options An array of options to modify how the command behaves.
     *
     *                        <code>
     *                        $options = [
     *                            'COUNT'      => int  # An optional number of fields to return.
     *                            'WITHVALUES' => bool # Also return the field values.
     *                        ];
     *                        </code>
     *
     * @return ValkeyGlide|array|string One or more random fields (and possibly values).
     *
     * @see https://valkey.io/commands/hrandfield
     *
     * @example $valkey_glide->hrandfield('settings');
     * @example $valkey_glide->hrandfield('settings', ['count' => 2, 'withvalues' => true]);
     */
    public function hRandField(string $key, ?array $options = null): ValkeyGlide|string|array|false;

    /**
     * Add or update one or more hash fields and values.
     *
     * @param string $key             The hash to create/update.
     * @param mixed $fields_and_vals Field-value pairs or associative array
     *
     * @return ValkeyGlide|int|false The number of fields that were added, or false on failure.
     *
     * @see https://valkey.io/commands/hset/
     *
     * @example $valkey_glide->hSet('player:1', 'name', 'Kim', 'score', 78);
     * @example $valkey_glide->hSet('player:1', ['name' => 'Kim', 'score' => 78]);
     */
    public function hSet(string $key, mixed ...$fields_and_vals): ValkeyGlide|int|false;

    /**
     * Set a hash field and value, but only if that field does not exist
     *
     * @param string $key   The hash to update.
     * @param string $field The value to set.
     *
     * @return ValkeyGlide|bool True if the field was set and false if not.
     *
     * @see https://valkey.io/commands/hsetnx
     *
     * @example
     * $valkey_glide->hsetnx('player:1', 'lock', 'enabled');
     * $valkey_glide->hsetnx('player:1', 'lock', 'enabled');
     */
    public function hSetNx(string $key, string $field, mixed $value): ValkeyGlide|bool;

    /**
     * Get the string length of a hash field
     *
     * @param string $key   The hash to query.
     * @param string $field The field to query.
     *
     * @return ValkeyGlide|int|false The string length of the field or false.
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     * $valkey_glide->del('hash');
     * $valkey_glide->hmset('hash', ['50bytes' => str_repeat('a', 50)]);
     * $valkey_glide->hstrlen('hash', '50bytes');
     *
     * @see https://valkey.io/commands/hstrlen
     */
    public function hStrLen(string $key, string $field): ValkeyGlide|int|false;

    /**
     * Get all of the values from a hash.
     *
     * @param string $key The hash to query.
     *
     * @return ValkeyGlide|list<mixed>|false The values from the hash.
     *
     * @see https://valkey.io/commands/hvals
     *
     * @example $valkey_glide->hvals('player:1');
     */
    public function hVals(string $key): ValkeyGlide|array|false;

    /**
     * Set hash fields with expiration time in seconds.
     *
     * @see https://valkey.io/commands/hsetex/
     *
     * @param string $key The key of the hash.
     * @param int $seconds The expiration time in seconds.
     * @param string|null $mode Optional condition (NX, XX).
     * @param string $field The field name.
     * @param mixed $value The field value.
     * @param mixed $fields_and_vals Additional field-value pairs.
     *
     * @return ValkeyGlide|int|false Returns the number of fields that were added or updated, or false on failure.
     */
    public function hSetEx(string $key, int $seconds, ?string $mode, string $field, mixed $value, mixed ...$fields_and_vals): ValkeyGlide|int|false;

    /**
     * Set hash fields with expiration time in milliseconds.
     *
     * @see https://valkey.io/commands/hsetex/
     *
     * @param string $key The key of the hash.
     * @param int $milliseconds The expiration time in milliseconds.
     * @param string|null $mode Optional condition (NX, XX).
     * @param string $field The field name.
     * @param mixed $value The field value.
     * @param mixed $fields_and_vals Additional field-value pairs.
     *
     * @return ValkeyGlide|int|false Returns the number of fields that were added or updated, or false on failure.
     */
    public function hPSetEx(string $key, int $milliseconds, ?string $mode, string $field, mixed $value, mixed ...$fields_and_vals): ValkeyGlide|int|false;

    /**
     * Get hash field values with optional expiration settings.
     *
     * @see https://valkey.io/commands/hgetex/
     *
     * @param string $key The key of the hash.
     * @param array $fields Array of field names to retrieve.
     * @param array|null $options Optional expiration settings.
     *
     * @return ValkeyGlide|array|false Returns an associative array of field-value pairs, or false on failure.
     */
    public function hGetEx(string $key, array $fields, ?array $options = null): ValkeyGlide|array|false;

    /**
     * Set an expiration time (in seconds) on one or more fields of a given hash key.
     * Similar to expire but for hash fields.
     *
     * @see https://valkey.io/commands/hexpire/
     *
     * @param string $key The key of the hash.
     * @param int $seconds The expiration time in seconds.
     * @param string|null $mode Optional condition (NX, XX, GT, LT).
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array indicating the result for each field, or false on failure.
     *
     * @example $valkey_glide->hExpire('player:1', 3600, 'score', 'level');
     */
    public function hExpire(string $key, int $seconds, ?string $mode, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Set an expiration time (in milliseconds) on one or more fields of a given hash key.
     * Similar to pexpire but for hash fields.
     *
     * @see https://valkey.io/commands/hexpire/
     *
     * @param string $key The key of the hash.
     * @param int $milliseconds The expiration time in milliseconds.
     * @param string|null $mode Optional condition (NX, XX, GT, LT).
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array indicating the result for each field, or false on failure.
     *
     * @example $valkey_glide->hPExpire('player:1', 3600000, 'score', 'level');
     */
    public function hPExpire(string $key, int $milliseconds, ?string $mode, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Sets the expiration time for hash fields using Unix timestamp in seconds.
     *
     * @see https://valkey.io/commands/hexpireat/
     *
     * @param string $key The key of the hash.
     * @param int $unix_timestamp The expiration time as Unix timestamp in seconds.
     * @param string|null $mode Optional condition (NX, XX, GT, LT).
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     * @return ValkeyGlide|array|false Returns an array of expiration results, or false on failure.
     */
    public function hExpireAt(string $key, int $unix_timestamp, ?string $mode, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Set an expiration time (unix timestamp in milliseconds) on one or more fields of a given hash key.
     * Similar to pexpireAt but for hash fields.
     *
     * @see https://valkey.io/commands/hexpireat/
     *
     * @param string $key The key of the hash.
     * @param int $unix_timestamp_ms The expiration time as unix timestamp in milliseconds.
     * @param string|null $mode Optional condition (NX, XX, GT, LT).
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array indicating the result for each field, or false on failure.
     *
     * @example $valkey_glide->hPExpireAt('player:1', 1640995200000, 'score', 'level');
     */
    public function hPExpireAt(string $key, int $unix_timestamp_ms, ?string $mode, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Remove the expiration time from one or more fields of a given hash key.
     * Similar to persist but for hash fields.
     *
     * @param string $key The key of the hash.
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array indicating the result for each field, or false on failure.
     *
     * @example $valkey_glide->hPersist('player:1', 'score', 'level');
     */

    /**
     * Get the remaining time to live (in seconds) of one or more fields of a given hash key.
     * Similar to ttl but for hash fields.
     *
     * @see https://valkey.io/commands/httl/
     *
     * @param string $key The key of the hash.
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array with TTL values for each field, or false on failure.
     *
     * @example $valkey_glide->hTtl('player:1', 'score', 'level');
     */
    public function hTtl(string $key, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Get the remaining time to live (in milliseconds) of one or more fields of a given hash key.
     * Similar to pttl but for hash fields.
     *
     * @see https://valkey.io/commands/httl/
     *
     * @param string $key The key of the hash.
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array with TTL values in milliseconds for each field, or false on failure.
     *
     * @example $valkey_glide->hPTtl('player:1', 'score', 'level');
     */
    public function hPTtl(string $key, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Get the expiration time (unix timestamp in seconds) of one or more fields of a given hash key.
     * Similar to expireTime but for hash fields.
     *
     * @see https://valkey.io/commands/hexpiretime/
     *
     * @param string $key The key of the hash.
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array with expiration timestamps for each field, or false on failure.
     *
     * @example $valkey_glide->hExpireTime('player:1', 'score', 'level');
     */
    public function hExpireTime(string $key, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Get the expiration time (unix timestamp in milliseconds) of one or more fields of a given hash key.
     * Similar to pexpireTime but for hash fields.
     *
     * @see https://valkey.io/commands/hexpiretime/
     *
     * @param string $key The key of the hash.
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array with expiration timestamps in milliseconds for each field, or false on failure.
     *
     * @example $valkey_glide->hPExpireTime('player:1', 'score', 'level');
     */
    public function hPExpireTime(string $key, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Remove the expiration time from one or more fields of a given hash key.
     * Similar to persist but for hash fields.
     *
     * @see https://valkey.io/commands/hpersist/
     *
     * @param string $key The key of the hash.
     * @param string $field The field name.
     * @param string $other_fields Additional field names.
     *
     * @return ValkeyGlide|array|false Returns an array indicating the result for each field, or false on failure.
     *
     * @example $valkey_glide->hPersist('player:1', 'score', 'level');
     */
    public function hPersist(string $key, string $field, string ...$other_fields): ValkeyGlide|array|false;

    /**
     * Iterate over the fields and values of a hash in an incremental fashion.
     *
     * @see https://valkey.io/commands/hscan
     * @see https://valkey.io/commands/scan
     *
     * @param string $key       The hash to query.
     * @param int    $iterator  The scan iterator, which should be initialized to NULL before the first call.
     *                          This value will be updated after every call to hscan, until it reaches zero
     *                          meaning the scan is complete.
     * @param string|null $pattern An optional glob-style pattern to filter fields with.
     * @param int    $count     An optional hint to ValkeyGlide about how many fields and values to return per HSCAN.
     *
     * @return ValkeyGlide|array|bool An array with a subset of fields and values.
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     *
     * $valkey_glide->del('big-hash');
     *
     * for ($i = 0; $i < 1000; $i++) {
     *     $fields["field:$i"] = "value:$i";
     * }
     *
     * $valkey_glide->hmset('big-hash', $fields);
     *
     * $it = null;
     *
     * do {
     *     // Scan the hash but limit it to fields that match '*:1?3'
     *     $fields = $valkey_glide->hscan('big-hash', $it, '*:1?3');
     *
     *     foreach ($fields as $field => $value) {
     *         echo "[$field] => $value\n";
     *     }
     * } while ($it != "0");
     */
    public function hscan(string $key, null|string &$iterator, ?string $pattern = null, int $count = 0): ValkeyGlide|array|bool;


    /**
     * Increment a key's value, optionally by a specific amount.
     *
     * @see https://valkey.io/commands/incr
     * @see https://valkey.io/commands/incrby
     *
     * @param string $key The key to increment
     * @param int    $by  An optional amount to increment by.
     *
     * @return ValkeyGlide|int|false  The new value of the key after incremented.
     *
     * @example $valkey_glide->incr('mycounter');
     * @example $valkey_glide->incr('mycounter', 10);
     */
    public function incr(string $key, int $by = 1): ValkeyGlide|int|false;

    /**
     * Increment a key by a specific integer value
     *
     * @see https://valkey.io/commands/incrby
     *
     * @param string $key   The key to increment.
     * @param int    $value The amount to increment.
     *
     * @example
     * $valkey_glide->set('primes', 2);
     * $valkey_glide->incrby('primes', 1);
     * $valkey_glide->incrby('primes', 2);
     * $valkey_glide->incrby('primes', 2);
     * $valkey_glide->incrby('primes', 4);
     */
    public function incrBy(string $key, int $value): ValkeyGlide|int|false;

    /**
     * Increment a numeric key by a floating point value.
     *
     * @param string $key The key to increment
     * @param floag $value How much to increment (or decrement) the value.
     *
     * @return ValkeyGlide|float|false The new value of the key or false if the key didn't contain a string.
     *
     * @example
     * $valkey_glide->incrbyfloat('tau', 3.1415926);
     * $valkey_glide->incrbyfloat('tau', 3.1415926);
     */
    public function incrByFloat(string $key, float $value): ValkeyGlide|float|false;

    /**
     * Retrieve information about the connected valkey-server.  If no arguments are passed to
     * this function, valkey will return every info field.  Alternatively you may pass a specific
     * section you want returned (e.g. 'server', or 'memory') to receive only information pertaining
     * to that section.
     *
     * If connected to ValkeyGlide server >= 7.0.0 you may pass multiple optional sections.
     *
     * @see https://valkey.io/commands/info/
     *
     * @param string $sections Optional section(s) you wish ValkeyGlide server to return.
     *
     * @return ValkeyGlide|array|false
     */
    public function info(string ...$sections): ValkeyGlide|array|false;


    /**
     * @param mixed $elements
     * @return ValkeyGlide|int|false
     */
    public function lInsert(string $key, string $pos, mixed $pivot, mixed $value);

    /**
     * Retrieve the length of a list.
     *
     * @param string $key The list
     *
     * @return ValkeyGlide|int|false The number of elements in the list or false on failure.
     */
    public function lLen(string $key): ValkeyGlide|int|false;

    /**
     * Move an element from one list into another.
     *
     * @param string $src       The source list.
     * @param string $dst       The destination list
     * @param string $wherefrom Where in the source list to retrieve the element.  This can be either
     *                          - `ValkeyGlide::LEFT`, or `ValkeyGlide::RIGHT`.
     * @param string $whereto   Where in the destination list to put the element.  This can be either
     *                          - `ValkeyGlide::LEFT`, or `ValkeyGlide::RIGHT`.
     * @return ValkeyGlide|string|false The element removed from the source list.
     *
     * @example
     * $valkey_glide->rPush('numbers', 'one', 'two', 'three');
     * $valkey_glide->lMove('numbers', 'odds', ValkeyGlide::LEFT, ValkeyGlide::LEFT);
     */
    public function lMove(string $src, string $dst, string $wherefrom, string $whereto): ValkeyGlide|string|false;

    /**
     * Move an element from one list to another, blocking up to a timeout until an element is available.
     *
     * @param string $src       The source list
     * @param string $dst       The destination list
     * @param string $wherefrom Where in the source list to extract the element.
     *                          - `ValkeyGlide::LEFT`, or `ValkeyGlide::RIGHT`.
     * @param string $whereto   Where in the destination list to put the element.
     *                          - `ValkeyGlide::LEFT`, or `ValkeyGlide::RIGHT`.
     * @param float $timeout    How long to block for an element.
     *
     * @return ValkeyGlide|string|false;
     *
     * @example
     * @valkey->lPush('numbers', 'one');
     * @valkey->blmove('numbers', 'odds', ValkeyGlide::LEFT, ValkeyGlide::LEFT 1.0);
     * // This call will block, if no additional elements are in 'numbers'
     * @valkey->blmove('numbers', 'odds', ValkeyGlide::LEFT, ValkeyGlide::LEFT, 1.0);
     */
    public function blmove(string $src, string $dst, string $wherefrom, string $whereto, float $timeout): ValkeyGlide|string|false;

    /**
     * Pop one or more elements off a list.
     *
     * @param string $key   The list to pop from.
     * @param int    $count Optional number of elements to remove.  By default one element is popped.
     * @return ValkeyGlide|null|bool|int|array Will return the element(s) popped from the list or false/NULL
     *                                   if none was removed.
     *
     * @see https://valkey.io/commands/lpop
     *
     * @example $valkey_glide->lpop('mylist');
     * @example $valkey_glide->lpop('mylist', 4);
     */
    public function lPop(string $key, int $count = 0): ValkeyGlide|bool|string|array;

    /**
     * Retrieve the index of an element in a list.
     *
     * @param string $key     The list to query.
     * @param mixed  $value   The value to search for.
     * @param array  $options Options to configure how the command operates
     *                        <code>
     *                        $options = [
     *                            # How many matches to return.  By default a single match is returned.
     *                            # If count is set to zero, it means unlimited.
     *                            'COUNT' => <num-matches>
     *
     *                            # Specify which match you want returned.  `RANK` 1 means "the first match"
     *                            # 2 means the second, and so on.  If passed as a negative number the
     *                            # RANK is computed right to left, so a `RANK` of -1 means "the last match".
     *                            'RANK'  => <rank>
     *
     *                            # This argument allows you to limit how many elements ValkeyGlide will search before
     *                            # returning.  This is useful to prevent ValkeyGlide searching very long lists while
     *                            # blocking the client.
     *                            'MAXLEN => <max-len>
     *                        ];
     *                        </code>
     *
     * @return ValkeyGlide|null|bool|int|array Returns one or more of the matching indexes, or null/false if none were found.
     */
    public function lPos(string $key, mixed $value, ?array $options = null): ValkeyGlide|null|bool|int|array;

    /**
     * Prepend one or more elements to a list.
     *
     * @param string      $key       The list to prepend.
     * @param mixed       $elements  One or more elements to prepend.
     *
     * @return ValkeyGlide|int The new length of the list after prepending.
     *
     * @see https://valkey.io/commands/lpush
     *
     * @example $valkey_glide->lPush('mylist', 'cat', 'bear', 'aligator');
     */
    public function lPush(string $key, mixed ...$elements): ValkeyGlide|int|false;

    /**
     * Append one or more elements to a list.
     *
     * @param string $key      The list to append to.
     * @param mixed  $elements one or more elements to append.
     *
     * @return ValkeyGlide|int|false The new length of the list
     *
     * @see https://valkey.io/commands/rpush
     *
     * @example $valkey_glide->rPush('mylist', 'xray', 'yankee', 'zebra');
     */
    public function rPush(string $key, mixed ...$elements): ValkeyGlide|int|false;

    /**
     * Prepend an element to a list but only if the list exists
     *
     * @param string $key   The key to prepend to.
     * @param mixed  $value The value to prepend.
     *
     * @return ValkeyGlide|int|false The new length of the list.
     *
     */
    public function lPushx(string $key, mixed $value): ValkeyGlide|int|false;

    /**
     * Append an element to a list but only if the list exists
     *
     * @param string $key   The key to prepend to.
     * @param mixed  $value The value to prepend.
     *
     * @return ValkeyGlide|int|false The new length of the list.
     *
     */
    public function rPushx(string $key, mixed $value): ValkeyGlide|int|false;

    /**
     * Set a list element at an index to a specific value.
     *
     * @param string $key   The list to modify.
     * @param int    $index The position of the element to change.
     * @param mixed  $value The new value.
     *
     * @return ValkeyGlide|bool True if the list was modified.
     *
     * @see https://valkey.io/commands/lset
     */
    public function lSet(string $key, int $index, mixed $value): ValkeyGlide|bool;

        /**
     * Get the element of a list by its index.
     *
     * @param string $key   The key to query
     * @param int    $index The index to check.
     * @return mixed The index or NULL/false if the element was not found.
     */
    public function lindex(string $key, int $index): mixed;

    /**
     * Retrieve elements from a list.
     *
     * @param string $key   The list to query.
     * @param int    $start The beginning index to retrieve.  This number can be negative
     *                      meaning start from the end of the list.
     * @param int    $end   The end index to retrieve.  This can also be negative to start
     *                      from the end of the list.
     *
     * @return ValkeyGlide|array|false The range of elements between the indexes.
     *
     * @example $valkey_glide->lrange('mylist', 0, -1);  // the whole list
     * @example $valkey_glide->lrange('mylist', -2, -1); // the last two elements in the list.
     */
    public function lrange(string $key, int $start, int $end): ValkeyGlide|array|false;

    /**
     * Remove one or more matching elements from a list.
     *
     * @param string $key   The list to truncate.
     * @param mixed  $value The value to remove.
     * @param int    $count How many elements matching the value to remove.
     *
     * @return ValkeyGlide|int|false The number of elements removed.
     *
     * @see https://valkey.io/commands/lrem
     */
    public function lrem(string $key, mixed $value, int $count = 0): ValkeyGlide|int|false;

    /**
     * Trim a list to a subrange of elements.
     *
     * @param string $key   The list to trim
     * @param int    $start The starting index to keep
     * @param int    $end   The ending index to keep.
     *
     * @return ValkeyGlide|bool true if the list was trimmed.
     *
     * @example $valkey_glide->ltrim('mylist', 0, 3);  // Keep the first four elements
     */
    public function ltrim(string $key, int $start, int $end): ValkeyGlide|bool;

    /**
     * Get one or more string keys.
     *
     * @param array $keys The keys to retrieve
     * @return ValkeyGlide|array|false an array of keys with their values.
     *
     * @example $valkey_glide->mget(['key1', 'key2']);
     */
    public function mget(array $keys): ValkeyGlide|array|false;



    /**
     * Move a key to a different database on the same valkey instance.
     *
     * @param string $key The key to move
     * @return ValkeyGlide|bool True if the key was moved
     */
    public function move(string $key, int $index): ValkeyGlide|bool;

    /**
     * Set one or more string keys.
     *
     * @param array $key_values An array with keys and their values.
     * @return ValkeyGlide|bool True if the keys could be set.
     *
     * @see https://valkey.io/commands/mset
     *
     * @example $valkey_glide->mSet(['foo' => 'bar', 'baz' => 'bop']);
     */
    public function mset(array $key_values): ValkeyGlide|bool;

    /**
     * Set one or more string keys but only if none of the key exist.
     *
     * @param array $key_values An array of keys with their values.
     *
     * @return ValkeyGlide|bool True if the keys were set and false if not.
     *
     * @see https://valkey.io/commands/msetnx
     *
     * @example $valkey_glide->msetnx(['foo' => 'bar', 'baz' => 'bop']);
     */
    public function msetnx(array $key_values): ValkeyGlide|bool;

    /**
     * Begin a transaction.
     *
     * @param int $value  The type of transaction to start.  This can either be `ValkeyGlide::MULTI` or
     *                    `ValkeyGlide::PIPELINE'.
     *
     * @return ValkeyGlide|bool True if the transaction could be started.
     *
     * @see https://valkey.io/commands/multi
     *
     * @example
     * $valkey_glide->multi();
     * $valkey_glide->set('foo', 'bar');
     * $valkey_glide->get('foo');
     * $valkey_glide->exec();
     */
    public function multi(int $value = ValkeyGlide::MULTI): bool|ValkeyGlide;

    public function object(string $subcommand, string $key): ValkeyGlide|int|string|false;

      /**
     * Remove the expiration from a key.
     *
     * @param string $key The key to operate against.
     *
     * @return ValkeyGlide|bool True if a timeout was removed and false if it was not or the key didn't exist.
     */
    public function persist(string $key): ValkeyGlide|bool;

    /**
     *  Sets an expiration in milliseconds on a given key.  If connected to ValkeyGlide >= 7.0.0
     *  you can pass an optional mode argument that modifies how the command will execute.
     *
     *  @see ValkeyGlide::expire() for a description of the mode argument.
     *
     *  @param string      $key  The key to set an expiration on.
     *  @param int         $timeout  The number of milliseconds after which key will be automatically deleted.
     *  @param string|null $mode  A two character modifier that changes how the
     *                       command works.
     *
     *  @return ValkeyGlide|bool   True if an expiry was set on the key, and false otherwise.
     */
    public function pexpire(string $key, int $timeout, ?string $mode = null): bool;

    /**
     * Set a key's expiration to a specific Unix Timestamp in milliseconds.  If connected to
     * ValkeyGlide >= 7.0.0 you can pass an optional 'mode' argument.
     *
     * @see ValkeyGlide::expire() For a description of the mode argument.
     *
     *  @param string      $key  The key to set an expiration on.
     *  @param int         $timestamp The unix timestamp to expire at.
     *  @param string|null $mode A two character modifier that changes how the
     *                       command works.
     *
     *  @return ValkeyGlide|bool   True if an expiration was set on the key, false otherwise.
     */
    public function pexpireAt(string $key, int $timestamp, ?string $mode = null): ValkeyGlide|bool;

    /**
     * Add one or more elements to a ValkeyGlide HyperLogLog key
     *
     * @see https://valkey.io/commands/pfadd
     *
     * @param string $key      The key in question.
     *
     * @param array  $elements One or more elements to add.
     *
     * @return ValkeyGlide|int Returns 1 if the set was altered, and zero if not.
     */
    public function pfadd(string $key, array $elements): ValkeyGlide|int;

    /**
     * Retrieve the cardinality of a ValkeyGlide HyperLogLog key.
     *
     * @see https://valkey.io/commands/pfcount
     *
     * @param string $key_or_keys Either one key or an array of keys
     *
     * @return ValkeyGlide|int The estimated cardinality of the set.
     */
    public function pfcount(array|string $key_or_keys): ValkeyGlide|int|false;

    /**
     * Merge one or more source HyperLogLog sets into a destination set.
     *
     * @see https://valkey.io/commands/pfmerge
     *
     * @param string $dst     The destination key.
     * @param array  $srckeys One or more source keys.
     *
     * @return ValkeyGlide|bool Always returns true.
     */
    public function pfmerge(string $dst, array $srckeys): ValkeyGlide|bool;

    /**
     * PING the valkey server with an optional string argument.
     *
     * @see https://valkey.io/commands/ping
     *
     * @param string $message An optional string message that ValkeyGlide will reply with, if passed.
     *
     * @return ValkeyGlide|string|false If passed no message, this command will simply return `true`.
     *                            If a message is passed, it will return the message.
     *
     * @example $valkey_glide->ping();
     * @example $valkey_glide->ping('beep boop');
     */
    public function ping(?string $message = null): ValkeyGlide|string|bool;

    /**
     * Enter into pipeline mode.
     *
     * Pipeline mode is the highest performance way to send many commands to ValkeyGlide
     * as they are aggregated into one stream of commands and then all sent at once
     * when the user calls ValkeyGlide::exec().
     *
     * NOTE:  That this is shorthand for ValkeyGlide::multi(ValkeyGlide::PIPELINE)
     *
     * @return ValkeyGlide The valkey object is returned, to facilitate method chaining.
     *
     * @example
     * $valkey_glide->pipeline()
     *       ->set('foo', 'bar')
     *       ->del('mylist')
     *       ->rpush('mylist', 'a', 'b', 'c')
     *       ->exec();
     */
     public function pipeline(): bool|ValkeyGlide;


    /**
     * Set a key with an expiration time in milliseconds
     *
     * @param string $key    The key to set
     * @param int    $expire The TTL to set, in milliseconds.
     * @param mixed  $value  The value to set the key to.
     *
     * @return ValkeyGlide|bool True if the key could be set.
     *
     * @example $valkey_glide->psetex('mykey', 1000, 'myval');
     */
    public function psetex(string $key, int $expire, mixed $value): ValkeyGlide|bool;

    /**
     * Subscribe to one or more glob-style patterns
     *
     * @param array     $patterns One or more patterns to subscribe to.
     * @param callable  $cb       A callback with the following prototype:
     *
     *                            <code>
     *                            function ($valkey_glide, $channel, $message) { }
     *                            </code>
     *
     * @see https://valkey.io/commands/psubscribe
     *
     * @return bool True if we were subscribed.
     */
    /* TODO public function psubscribe(array $patterns, callable $cb): bool; */

    /**
     * Get a keys time to live in milliseconds.
     *
     * @param string $key The key to check.
     *
     * @return ValkeyGlide|int|false The key's TTL or one of two special values if it has none.
     *                         <code>
     *                         -1 - The key has no TTL.
     *                         -2 - The key did not exist.
     *                         </code>
     *
     * @see https://valkey.io/commands/pttl
     *
     * @example $valkey_glide->pttl('ttl-key');
     */
    public function pttl(string $key): ValkeyGlide|int|false;

    /**
     * Publish a message to a pubsub channel
     *
     * @see https://valkey.io/commands/publish
     *
     * @param string $channel The channel to publish to.
     * @param string $message The message itself.
     *
     * @return ValkeyGlide|int The number of subscribed clients to the given channel.
     */
   /* TODO  public function publish(string $channel, string $message): ValkeyGlide|int|false;*/

    /* TODO public function pubsub(string $command, mixed $arg = null): mixed;*/

    /**
     * Unsubscribe from one or more channels by pattern
     *
     * @see https://valkey.io/commands/punsubscribe
     * @see https://valkey.io/commands/subscribe
     * @see ValkeyGlide::subscribe()
     *
     * @param array $patterns One or more glob-style patterns of channel names.
     *
     * @return ValkeyGlide|array|bool  The array of subscribed patterns or false on failure.
     */
   /* public function punsubscribe(array $patterns): ValkeyGlide|array|bool;*/

    /**
     * Pop one or more elements from the end of a list.
     *
     * @param string $key   A valkey LIST key name.
     * @param int    $count The maximum number of elements to pop at once.
     *                      NOTE:  The `count` argument requires ValkeyGlide >= 6.2.0
     *
     * @return ValkeyGlide|array|string|bool One or more popped elements or false if all were empty.
     *
     * @see https://valkey.io/commands/rpop
     *
     * @example $valkey_glide->rPop('mylist');
     * @example $valkey_glide->rPop('mylist', 4);
     */
    public function rPop(string $key, int $count = 0): ValkeyGlide|array|string|bool;

    /**
     * Return a random key from the current database
     *
     * @see https://valkey.io/commands/randomkey
     *
     * @return ValkeyGlide|string|false A random key name or false if no keys exist
     *
     */
    public function randomKey(): ValkeyGlide|string|false;

    /**
     * Execute any arbitrary ValkeyGlide command by name.
     *
     * @param string $command The command to execute
     * @param mixed  $args    One or more arguments to pass to the command.
     *
     * @return mixed Can return any number of things depending on command executed.
     *
     * @example $valkey_glide->rawCommand('del', 'mystring', 'mylist');
     * @example $valkey_glide->rawCommand('set', 'mystring', 'myvalue');
     * @example $valkey_glide->rawCommand('rpush', 'mylist', 'one', 'two', 'three');
     */
    public function rawcommand(string $command, mixed ...$args): mixed;

    /**
     * Unconditionally rename a key from $old_name to $new_name
     *
     * @see https://valkey.io/commands/rename
     *
     * @param string $old_name The original name of the key
     * @param string $new_name The new name for the key
     *
     * @return ValkeyGlide|bool True if the key was renamed or false if not.
     */
    public function rename(string $old_name, string $new_name): ValkeyGlide|bool;

    /**
     * Renames $key_src to $key_dst but only if newkey does not exist.
     *
     * @see https://valkey.io/commands/renamenx
     *
     * @param string $key_src The source key name
     * @param string $key_dst The destination key name.
     *
     * @return ValkeyGlide|bool True if the key was renamed, false if not.
     *
     * @example
     * $valkey_glide->set('src', 'src_key');
     * $valkey_glide->set('existing-dst', 'i_exist');
     *
     * $valkey_glide->renamenx('src', 'dst');
     * $valkey_glide->renamenx('dst', 'existing-dst');
     */
    public function renameNx(string $key_src, string $key_dst): ValkeyGlide|bool;


    /**
     * Restore a key by the binary payload generated by the DUMP command.
     *
     * @param string $key     The name of the key you wish to create.
     * @param int    $ttl     What ValkeyGlide should set the key's TTL (in milliseconds) to once it is created.
     *                        Zero means no TTL at all.
     * @param string $value   The serialized binary value of the string (generated by DUMP).
     * @param array  $options An array of additional options that modifies how the command operates.
     *
     *                        <code>
     *                        $options = [
     *                            'ABSTTL'          # If this is present, the `$ttl` provided by the user should
     *                                              # be an absolute timestamp, in milliseconds()
     *
     *                            'REPLACE'         # This flag instructs ValkeyGlide to store the key even if a key with
     *                                              # that name already exists.
     *
     *                            'IDLETIME' => int # Tells ValkeyGlide to set the keys internal 'idletime' value to a
     *                                              # specific number (see the ValkeyGlide command OBJECT for more info).
     *                            'FREQ'     => int # Tells ValkeyGlide to set the keys internal 'FREQ' value to a specific
     *                                              # number (this relates to ValkeyGlide' LFU eviction algorithm).
     *                        ];
     *                        </code>
     *
     * @return ValkeyGlide|bool     True if the key was stored, false if not.
     *
     * @see https://valkey.io/commands/restore
     * @see https://valkey.io/commands/dump
     * @see ValkeyGlide::dump()
     *
     * @example
     * $valkey_glide->sAdd('captains', 'Janeway', 'Picard', 'Sisko', 'Kirk', 'Archer');
     * $serialized = $valkey_glide->dump('captains');
     *
     * $valkey_glide->restore('captains-backup', 0, $serialized);
     */
    public function restore(string $key, int $ttl, string $value, ?array $options = null): ValkeyGlide|bool;



    /**
     * Add one or more values to a ValkeyGlide SET key.
     *
     * @param string $key           The key name
     * @param mixed  $member        A value to add to the set.
     * @param mixed  $other_members One or more additional values to add
     *
     * @return ValkeyGlide|int|false The number of values added to the set.
     *
     * @see https://valkey.io/commands/sadd
     *
     * @example
     * $valkey_glide->del('myset');
     *
     * $valkey_glide->sadd('myset', 'foo', 'bar', 'baz');
     * $valkey_glide->sadd('myset', 'foo', 'new');
     */
    public function sAdd(string $key, mixed $value, mixed ...$other_values): ValkeyGlide|int|false;


    /**
     * Given one or more ValkeyGlide SETS, this command returns all of the members from the first
     * set that are not in any subsequent set.
     *
     * @param string $key        The first set
     * @param string $other_keys One or more additional sets
     *
     * @return ValkeyGlide|array|false Returns the elements from keys 2..N that don't exist in the
     *                           first sorted set, or false on failure.
     *
     * @see https://valkey.io/commands/sdiff
     *
     * @example
     * $valkey_glide->pipeline()
     *       ->del('set1', 'set2', 'set3')
     *       ->sadd('set1', 'apple', 'banana', 'carrot', 'date')
     *       ->sadd('set2', 'carrot')
     *       ->sadd('set3', 'apple', 'carrot', 'eggplant')
     *       ->exec();
     *
     * $valkey_glide->sdiff('set1', 'set2', 'set3');
     */
    public function sDiff(string $key, string ...$other_keys): ValkeyGlide|array|false;

    /**
     * This method performs the same operation as SDIFF except it stores the resulting diff
     * values in a specified destination key.
     *
     * @see https://valkey.io/commands/sdiffstore
     * @see ValkeyGlide::sdiff()
     *
     * @param string $dst The key where to store the result
     * @param string $key The first key to perform the DIFF on
     * @param string $other_keys One or more additional keys.
     *
     * @return ValkeyGlide|int|false The number of values stored in the destination set or false on failure.
     */
    public function sDiffStore(string $dst, string $key, string ...$other_keys): ValkeyGlide|int|false;

    /**
     * Given one or more ValkeyGlide SET keys, this command will return all of the elements that are
     * in every one.
     *
     * @see https://valkey.io/commands/sinter
     *
     * @param string $key        The first SET key to intersect.
     * @param string $other_keys One or more ValkeyGlide SET keys.
     *
     * @example
     * <code>
     * $valkey_glide->pipeline()
     *       ->del('alice_likes', 'bob_likes', 'bill_likes')
     *       ->sadd('alice_likes', 'asparagus', 'broccoli', 'carrot', 'potato')
     *       ->sadd('bob_likes', 'asparagus', 'carrot', 'potato')
     *       ->sadd('bill_likes', 'broccoli', 'potato')
     *       ->exec();
     *
     * var_dump($valkey_glide->sinter('alice_likes', 'bob_likes', 'bill_likes'));
     * </code>
     */
    public function sInter(array|string $key, string ...$other_keys): ValkeyGlide|array|false;

    /**
     * Compute the intersection of one or more sets and return the cardinality of the result.
     *
     * @param array $keys  One or more set key names.
     * @param int   $limit A maximum cardinality to return.  This is useful to put an upper bound
     *                     on the amount of work ValkeyGlide will do.
     *
     * @return ValkeyGlide|int|false The
     *
     * @see https://valkey.io/commands/sintercard
     *
     * @example
     * <code>
     * $valkey_glide->sAdd('set1', 'apple', 'pear', 'banana', 'carrot');
     * $valkey_glide->sAdd('set2', 'apple',         'banana');
     * $valkey_glide->sAdd('set3',          'pear', 'banana');
     *
     * $valkey_glide->sInterCard(['set1', 'set2', 'set3']);
     * </code>
     */
    public function sintercard(array $keys, int $limit = -1): ValkeyGlide|int|false;

    /**
     * Perform the intersection of one or more ValkeyGlide SETs, storing the result in a destination
     * key, rather than returning them.
     *
     * @param array|string $key_or_keys Either a string key, or an array of keys (with at least two
     *                                  elements, consisting of the destination key name and one
     *                                  or more source keys names.
     * @param string       $other_keys  If the first argument was a string, subsequent arguments should
     *                                  be source key names.
     *
     * @return ValkeyGlide|int|false          The number of values stored in the destination key or false on failure.
     *
     * @see https://valkey.io/commands/sinterstore
     * @see ValkeyGlide::sinter()
     * <code>
     * @example $valkey_glide->sInterStore(['dst', 'src1', 'src2', 'src3']);
     * @example $valkey_glide->sInterStore('dst', 'src1', 'src'2', 'src3');
     * </code>
     */
    public function sInterStore(array|string $key, string ...$other_keys): ValkeyGlide|int|false;

    /**
     * Retrieve every member from a set key.
     *
     * @param string $key The set name.
     *
     * @return ValkeyGlide|array|false Every element in the set or false on failure.
     *
     * @see https://valkey.io/commands/smembers
     *
     * @example
     * $valkey_glide->sAdd('tng-crew', ...['Picard', 'Riker', 'Data', 'Worf', 'La Forge', 'Troi', 'Crusher', 'Broccoli']);
     * $valkey_glide->sMembers('tng-crew');
     */
    public function sMembers(string $key): ValkeyGlide|array|false;

    /**
     * Check if one or more values are members of a set.
     *
     * @see https://valkey.io/commands/smismember
     * @see https://valkey.io/commands/smember
     * @see ValkeyGlide::smember()
     *
     * @param string $key           The set to query.
     * @param string $member        The first value to test if exists in the set.
     * @param string $other_members Any number of additional values to check.
     *
     * @return ValkeyGlide|array|false An array of integers representing whether each passed value
     *                           was a member of the set.
     *
     * @example
     * $valkey_glide->sAdd('ds9-crew', ...["Sisko", "Kira", "Dax", "Worf", "Bashir", "O'Brien"]);
     * $members = $valkey_glide->sMIsMember('ds9-crew', ...['Sisko', 'Picard', 'Data', 'Worf']);
     */
    public function sMisMember(string $key, string $member, string ...$other_members): ValkeyGlide|array|false;

    /**
     * Pop a member from one set and push it onto another.  This command will create the
     * destination set if it does not currently exist.
     *
     * @see https://valkey.io/commands/smove
     *
     * @param string $src   The source set.
     * @param string $dst   The destination set.
     * @param mixed  $value The member you wish to move.
     *
     * @return ValkeyGlide|bool   True if the member was moved, and false if it wasn't in the set.
     *
     * @example
     * $valkey_glide->sAdd('numbers', 'zero', 'one', 'two', 'three', 'four');
     * $valkey_glide->sMove('numbers', 'evens', 'zero');
     * $valkey_glide->sMove('numbers', 'evens', 'two');
     * $valkey_glide->sMove('numbers', 'evens', 'four');
     */
    public function sMove(string $src, string $dst, mixed $value): ValkeyGlide|bool;

    /**
     * Remove one or more elements from a set.
     *
     * @see https://valkey.io/commands/spop
     *
     * @param string $key    The set in question.
     * @param int    $count  An optional number of members to pop.   This defaults to
     *                       removing one element.
     *
     * @example
     * $valkey_glide->del('numbers', 'evens');
     * $valkey_glide->sAdd('numbers', 'zero', 'one', 'two', 'three', 'four');
     * $valkey_glide->sPop('numbers');
     */
    public function sPop(string $key, int $count = 0): ValkeyGlide|string|array|false;

    /**
     * Retrieve one or more random members of a set.
     *
     * @param string $key   The set to query.
     * @param int    $count An optional count of members to return.
     *
     *                      If this value is positive, ValkeyGlide will return *up to* the requested
     *                      number but with unique elements that will never repeat.  This means
     *                      you may receive fewer then `$count` replies.
     *
     *                      If the number is negative, ValkeyGlide will return the exact number requested
     *                      but the result may contain duplicate elements.
     *
     * @return ValkeyGlide|array|string|false One or more random members or false on failure.
     *
     * @see https://valkey.io/commands/srandmember
     *
     * @example $valkey_glide->sRandMember('myset');
     * @example $valkey_glide->sRandMember('myset', 10);
     * @example $valkey_glide->sRandMember('myset', -10);
     */
    public function sRandMember(string $key, int $count = 0): mixed;

    /**
     * Returns the union of one or more ValkeyGlide SET keys.
     *
     * @see https://valkey.io/commands/sunion
     *
     * @param string $key         The first SET to do a union with
     * @param string $other_keys  One or more subsequent keys
     *
     * @return ValkeyGlide|array|false  The union of the one or more input sets or false on failure.
     *
     * @example $valkey_glide->sunion('set1', 'set2');
     */
    public function sUnion(string $key, string ...$other_keys): ValkeyGlide|array|false;

    /**
     * Perform a union of one or more ValkeyGlide SET keys and store the result in a new set
     *
     * @see https://valkey.io/commands/sunionstore
     * @see ValkeyGlide::sunion()
     *
     * @param string $dst        The destination key
     * @param string $key        The first source key
     * @param string $other_keys One or more additional source keys
     *
     * @return ValkeyGlide|int|false   The number of elements stored in the destination SET or
     *                           false on failure.
     */
    public function sUnionStore(string $dst, string $key, string ...$other_keys): ValkeyGlide|int|false;

       /**
     * Incrementally scan the ValkeyGlide keyspace, with optional pattern and type matching.
     *
     * A note about ValkeyGlide::SCAN_NORETRY and ValkeyGlide::SCAN_RETRY.
     *
     * For convenience, PhpValkeyGlide can retry SCAN commands itself when ValkeyGlide returns an empty array of
     * keys with a nonzero iterator.  This can happen when matching against a pattern that very few
     * keys match inside a key space with a great many keys.  The following example demonstrates how
     * to use ValkeyGlide::scan() with the option disabled and enabled.
     *
     * @param string    $iterator The cursor returned by ValkeyGlide for every subsequent call to SCAN.  On
     *                         the initial invocation of the call, it should be initialized by the
     *                         caller to NULL.  Each time SCAN is invoked, the iterator will be
     *                         updated to a new number, until finally ValkeyGlide will set the value to
     *                         "0", indicating that the scan is complete.
     *
     * @param string|null $pattern An optional glob-style pattern for matching key names.  If passed as
     *                         NULL, it is the equivalent of sending '*' (match every key).
     *
     * @param int    $count    A hint to valkey that tells it how many keys to return in a single
     *                         call to SCAN.  The larger the number, the longer ValkeyGlide may block
     *                         clients while iterating the key space.
     *
     * @param string $type     An optional argument to specify which key types to scan (e.g.
     *                         'STRING', 'LIST', 'SET')
     *
     * @return array|false     An array of keys, or false if no keys were returned for this
     *                         invocation of scan.  Note that it is possible for ValkeyGlide to return
     *                         zero keys before having scanned the entire key space, so the caller
     *                         should instead continue to SCAN until the iterator reference is
     *                         returned to zero.
     *
     * @see https://valkey.io/commands/scan
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     *
     *
     * $it = null;
     *
     * do {
     *     $keys = $valkey_glide->scan($it, '*zorg*');
     *     foreach ($keys as $key) {
     *         echo "KEY: $key\n";
     *     }
     * } while ($it != 0);
     *
     * $it = null;
     *
     * // When ValkeyGlide::SCAN_RETRY is enabled, we can use simpler logic, as we will never receive an
     * // empty array of keys when the iterator is nonzero.
     * while (true) {
     *     $keys = $valkey_glide->scan($it, '*zorg*')
     *     if ($it == "0") break;
     *     foreach ($keys as $key) {
     *         echo "KEY: $key\n";
     *     }
     * }
     */
    public function scan(null|string &$iterator, ?string $pattern = null, int $count = 0, ?string $type = null): array|false;

    /**
     * Retrieve the number of members in a ValkeyGlide set.
     *
     * @param string $key The set to get the cardinality of.
     *
     * @return ValkeyGlide|int|false The cardinality of the set or false on failure.
     *
     * @see https://valkey.io/commands/scard
     *
     * @example $valkey_glide->scard('set');
     */
    public function scard(string $key): ValkeyGlide|int|false;

    /**
     * An administrative command used to interact with LUA scripts stored on the server.
     *
     * @see https://valkey.io/commands/script
     *
     * @param string $command The script suboperation to execute.
     * @param mixed  $args    One or more additional argument
     *
     * @return mixed This command returns various things depending on the specific operation executed.
     *
     * @example $valkey_glide->script('load', 'return 1');
     * @example $valkey_glide->script('exists', sha1('return 1'));
     */
    /* TODO public function script(string $command, mixed ...$args): mixed; */

    /**
     * Select a specific ValkeyGlide database.
     *
     * @param int $db The database to select.  Note that by default ValkeyGlide has 16 databases (0-15).
     *
     * @return ValkeyGlide|bool true on success and false on failure
     *
     * @see https://valkey.io/commands/select
     *
     * @example $valkey_glide->select(1);
     */
// public function select(int $db): ValkeyGlide|bool;

    /**
     * Create or set a ValkeyGlide STRING key to a value.
     *
     * @param string    $key     The key name to set.
     * @param mixed     $value   The value to set the key to.
     * @param array|int $options Either an array with options for how to perform the set or an
     *                           integer with an expiration.  If an expiration is set PhpValkeyGlide
     *                           will actually send the `SETEX` command.
     *
     * OPTION                         DESCRIPTION
     * ------------                   --------------------------------------------------------------
     * ['EX' => 60]                   expire 60 seconds.
     * ['PX' => 6000]                 expire in 6000 milliseconds.
     * ['EXAT' => time() + 10]        expire in 10 seconds.
     * ['PXAT' => time()*1000 + 1000] expire in 1 second.
     * ['KEEPTTL' => true]            ValkeyGlide will not update the key's current TTL.
     * ['XX']                         Only set the key if it already exists.
     * ['NX']                         Only set the key if it doesn't exist.
     * ['GET']                        Instead of returning `+OK` return the previous value of the
     *                                key or NULL if the key didn't exist.
     *
     * @return ValkeyGlide|string|bool True if the key was set or false on failure.
     *
     * @see https://valkey.io/commands/set
     * @see https://valkey.io/commands/setex
     *
     * @example $valkey_glide->set('key', 'value');
     * @example $valkey_glide->set('key', 'expires_in_60_seconds', 60);
     */
    public function set(string $key, mixed $value, mixed $options = null): ValkeyGlide|string|bool;

    /**
     * Set a specific bit in a ValkeyGlide string to zero or one
     *
     * @see https://valkey.io/commands/setbit
     *
     * @param string $key    The ValkeyGlide STRING key to modify
     * @param bool   $value  Whether to set the bit to zero or one.
     *
     * @return ValkeyGlide|int|false The original value of the bit or false on failure.
     *
     * @example
     * $valkey_glide->set('foo', 'bar');
     * $valkey_glide->setbit('foo', 7, 1);
     */
    public function setBit(string $key, int $idx, bool $value): ValkeyGlide|int|false;

    /**
     * Update or append to a ValkeyGlide string at a specific starting index
     *
     * @see https://valkey.io/commands/setrange
     *
     * @param string $key    The key to update
     * @param int    $index  Where to insert the provided value
     * @param string $value  The value to copy into the string.
     *
     * @return ValkeyGlide|int|false The new length of the string or false on failure
     *
     * @example
     * $valkey_glide->set('message', 'Hello World');
     * $valkey_glide->setRange('message', 6, 'ValkeyGlide');
     */
    public function setRange(string $key, int $index, string $value): ValkeyGlide|int|false;



    /**
     * Set a ValkeyGlide STRING key with a specific expiration in seconds.
     *
     * @param string $key     The name of the key to set.
     * @param int    $expire  The key's expiration in seconds.
     * @param mixed  $value   The value to set the key.
     *
     * @return ValkeyGlide|bool True on success or false on failure.
     *
     * @example $valkey_glide->setex('60s-ttl', 60, 'some-value');
     */
    public function setex(string $key, int $expire, mixed $value);

    /**
     * Set a key to a value, but only if that key does not already exist.
     *
     * @see https://valkey.io/commands/setnx
     *
     * @param string $key   The key name to set.
     * @param mixed  $value What to set the key to.
     *
     * @return ValkeyGlide|bool Returns true if the key was set and false otherwise.
     *
     * @example $valkey_glide->setnx('existing-key', 'existing-value');
     * @example $valkey_glide->setnx('new-key', 'new-value');
     */
    public function setnx(string $key, mixed $value): ValkeyGlide|bool;

    /**
     * Check whether a given value is the member of a ValkeyGlide SET.
     *
     * @param string $key   The valkey set to check.
     * @param mixed  $value The value to test.
     *
     * @return ValkeyGlide|bool True if the member exists and false if not.
     *
     * @example $valkey_glide->sismember('myset', 'mem1', 'mem2');
     */
    public function sismember(string $key, mixed $value): ValkeyGlide|bool;

    /**
     * Update one or more keys last modified metadata.
     *
     * @see https://valkey.io/commands/touch/
     *
     * @param array|string $key    Either the first key or if passed as the only argument
     *                             an array of keys.
     * @param string $more_keys    One or more keys to send to the command.
     *
     * @return ValkeyGlide|int|false     This command returns the number of keys that exist and
     *                             had their last modified time reset
     */
    public function touch(array|string $key_or_array, string ...$more_keys): ValkeyGlide|int|false;



    /**
     * Sort the contents of a ValkeyGlide key in various ways.
     *
     * @see https://valkey.io/commands/sort/
     *
     * @param string $key     The key you wish to sort
     * @param array  $options Various options controlling how you would like the
     *                        data sorted.  See blow for a detailed description
     *                        of this options array.
     *
     * @return mixed This command can either return an array with the sorted data
     *               or the number of elements placed in a destination set when
     *               using the STORE option.
     *
     * @example
     * $options = [
     *     'SORT'  => 'ASC'|| 'DESC' // Sort in descending or descending order.
     *     'ALPHA' => true || false  // Whether to sort alphanumerically.
     *     'LIMIT' => [0, 10]        // Return a subset of the data at offset, count
     *     'BY'    => 'weight_*'     // For each element in the key, read data from the
     *                                  external key weight_* and sort based on that value.
     *     'GET'   => 'weight_*'     // For each element in the source key, retrieve the
     *                                  data from key weight_* and return that in the result
     *                                  rather than the source keys' element.  This can
     *                                  be used in combination with 'BY'
     * ];
     */
    public function sort(string $key, ?array $options = null): mixed;

    /**
     * This is simply a read-only variant of the sort command
     *
     * @see ValkeyGlide::sort()
     */
    public function sort_ro(string $key, ?array $options = null): mixed;

    /**
     * Remove one or more values from a ValkeyGlide SET key.
     *
     * @see https://valkey.io/commands/srem
     *
     * @param string $key         The ValkeyGlide SET key in question.
     * @param mixed  $value       The first value to remove.
     * @param mixed  $more_values One or more additional values to remove.
     *
     * @return ValkeyGlide|int|false    The number of values removed from the set or false on failure.
     *
     * @example $valkey_glide->sRem('set1', 'mem1', 'mem2', 'not-in-set');
     */
    public function srem(string $key, mixed $value, mixed ...$other_values): ValkeyGlide|int|false;

    /**
     * Scan the members of a valkey SET key.
     *
     * @see https://valkey.io/commands/sscan
     * @see https://valkey.io/commands/scan
     *
     * @param string $key       The ValkeyGlide SET key in question.
     * @param string $iterator  A reference to an iterator which should be initialized to NULL that
     *                          PhpValkeyGlide will update with the value returned from ValkeyGlide after each
     *                          subsequent call to SSCAN.  Once this cursor is "0" you know all
     *                          members have been traversed.
     * @param string|null $pattern An optional glob style pattern to match against, so ValkeyGlide only
     *                          returns the subset of members matching this pattern.
     * @param int    $count     A hint to ValkeyGlide as to how many members it should scan in one command
     *                          before returning members for that iteration.
     *
     * @example
     * $valkey_glide->del('myset');
     * for ($i = 0; $i < 10000; $i++) {
     *     $valkey_glide->sAdd('myset', "member:$i");
     * }
     * $valkey_glide->sadd('myset', 'foofoo');
     *
     * $scanned = 0;
     * $it = null;
     *
     * // Without ValkeyGlide::SCAN_RETRY we may receive empty results and
     * // a nonzero iterator.
     * do {
     *     // Scan members containing '5'
     *     $members = $valkey_glide->sscan('myset', $it, '*5*');
     *     foreach ($members as $member) {
     *          echo "NORETRY: $member\n";
     *          $scanned++;
     *     }
     * } while ($it != 0);
     * echo "TOTAL: $scanned\n";
     *
     * $scanned = 0;
     * $it = null;
     *
     * // With ValkeyGlide::SCAN_RETRY PhpValkeyGlide will never return an empty array
     * // when the cursor is non-zero
     * while (true) {
     *     $members = $valkey_glide->sscan('myset', $it, '*5*');
     *     if ($it == "0") break;
     *     foreach ($members as $member) {
     *         echo "RETRY: $member\n";
     *         $scanned++;
     *     }
     * }
     */
    public function sscan(string $key, null|string &$iterator, ?string $pattern = null, int $count = 0): array|false;

    /**
     * Subscribes the client to the specified shard channels.
     *
     * @param array    $channels One or more channel names.
     * @param callable $cb       The callback PhpValkeyGlide will invoke when we receive a message
     *                           from one of the subscribed channels.
     *
     * @return bool True on success, false on faiilure.  Note that this command will block the
     *              client in a subscribe loop, waiting for messages to arrive.
     *
     * @see https://valkey.io/commands/ssubscribe
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     *
     * $valkey_glide->ssubscribe(['channel-1', 'channel-2'], function ($valkey_glide, $channel, $message) {
     *     echo "[$channel]: $message\n";
     *
     *     // Unsubscribe from the message channel when we read 'quit'
     *     if ($message == 'quit') {
     *         echo "Unsubscribing from '$channel'\n";
     *         $valkey_glide->sunsubscribe([$channel]);
     *     }
     * });
     *
     * // Once we read 'quit' from both channel-1 and channel-2 the subscribe loop will be
     * // broken and this command will execute.
     * echo "Subscribe loop ended\n";
     */
    /* TODO public function ssubscribe(array $channels, callable $cb): bool; */

    /**
     * Retrieve the length of a ValkeyGlide STRING key.
     *
     * @param string $key The key we want the length of.
     *
     * @return ValkeyGlide|int|false The length of the string key if it exists, zero if it does not, and
     *                         false on failure.
     *
     * @see https://valkey.io/commands/strlen
     *
     * @example $valkey_glide->strlen('mykey');
     */
    public function strlen(string $key): ValkeyGlide|int|false;

    /**
     * Subscribe to one or more ValkeyGlide pubsub channels.
     *
     * @param array    $channels One or more channel names.
     * @param callable $cb       The callback PhpValkeyGlide will invoke when we receive a message
     *                           from one of the subscribed channels.
     *
     * @return bool True on success, false on faiilure.  Note that this command will block the
     *              client in a subscribe loop, waiting for messages to arrive.
     *
     * @see https://valkey.io/commands/subscribe
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     *
     * $valkey_glide->subscribe(['channel-1', 'channel-2'], function ($valkey_glide, $channel, $message) {
     *     echo "[$channel]: $message\n";
     *
     *     // Unsubscribe from the message channel when we read 'quit'
     *     if ($message == 'quit') {
     *         echo "Unsubscribing from '$channel'\n";
     *         $valkey_glide->unsubscribe([$channel]);
     *     }
     * });
     *
     * // Once we read 'quit' from both channel-1 and channel-2 the subscribe loop will be
     * // broken and this command will execute.
     * echo "Subscribe loop ended\n";
     */
    /* TODO public function subscribe(array $channels, callable $cb): bool; */

    /**
     * Unsubscribes the client from the given shard channels,
     * or from all of them if none is given.
     *
     * @param array $channels One or more channels to unsubscribe from.
     * @return ValkeyGlide|array|bool The array of unsubscribed channels.
     *
     * @see https://valkey.io/commands/sunsubscribe
     * @see ValkeyGlide::ssubscribe()
     *
     * @example
     * $valkey_glide->ssubscribe(['channel-1', 'channel-2'], function ($valkey_glide, $channel, $message) {
     *     if ($message == 'quit') {
     *         echo "$channel => 'quit' detected, unsubscribing!\n";
     *         $valkey_glide->sunsubscribe([$channel]);
     *     } else {
     *         echo "$channel => $message\n";
     *     }
     * });
     *
     * echo "We've unsubscribed from both channels, exiting\n";
     */
    /* TODO public function sunsubscribe(array $channels): ValkeyGlide|array|bool; */


    /**
     * Retrieve the server time from the connected ValkeyGlide instance.
     *
     * @see https://valkey.io/commands/time
     *
     * @return A two element array consisting of a Unix Timestamp and the number of microseconds
     *         elapsed since the second.
     *
     * @example $valkey_glide->time();
     */
    public function time(): ValkeyGlide|array;

    /**
     * Get the amount of time a ValkeyGlide key has before it will expire, in seconds.
     *
     * @param string $key      The Key we want the TTL for.
     * @return ValkeyGlide|int|false (a) The number of seconds until the key expires, or -1 if the key has
     *                         no expiration, and -2 if the key does not exist.  In the event of an
     *                         error, this command will return false.
     *
     * @see https://valkey.io/commands/ttl
     *
     * @example $valkey_glide->ttl('mykey');
     */
    public function ttl(string $key): ValkeyGlide|int|false;

    /**
     * Get the type of a given ValkeyGlide key.
     *
     * @see https://valkey.io/commands/type
     *
     * @param  string $key     The key to check
     * @return ValkeyGlide|int|false The ValkeyGlide type constant or false on failure.
     *
     * The ValkeyGlide class defines several type constants that correspond with ValkeyGlide key types.
     *
     *     ValkeyGlide::VALKEY_GLIDE_NOT_FOUND
     *     ValkeyGlide::VALKEY_GLIDE_STRING
     *     ValkeyGlide::VALKEY_GLIDE_SET
     *     ValkeyGlide::VALKEY_GLIDE_LIST
     *     ValkeyGlide::VALKEY_GLIDE_ZSET
     *     ValkeyGlide::VALKEY_GLIDE_HASH
     *     ValkeyGlide::VALKEY_GLIDE_STREAM
     *
     * @example
     * foreach ($valkey_glide->keys('*') as $key) {
     *     echo "$key => " . $valkey_glide->type($key) . "\n";
     * }
     */
    public function type(string $key): ValkeyGlide|int|false;

    /**
     * Delete one or more keys from the ValkeyGlide database.  Unlike this operation, the actual
     * deletion is asynchronous, meaning it is safe to delete large keys without fear of
     * ValkeyGlide blocking for a long period of time.
     *
     * @param array|string $key_or_keys Either an array with one or more keys or a string with
     *                                  the first key to delete.
     * @param string       $other_keys  If the first argument passed to this method was a string
     *                                  you may pass any number of additional key names.
     *
     * @return ValkeyGlide|int|false The number of keys deleted or false on failure.
     *
     * @see https://valkey.io/commands/unlink
     * @see https://valkey.io/commands/del
     * @see ValkeyGlide::del()
     *
     * @example $valkey_glide->unlink('key1', 'key2', 'key3');
     * @example $valkey_glide->unlink(['key1', 'key2', 'key3']);
     */
    public function unlink(array|string $key, string ...$other_keys): ValkeyGlide|int|false;

    /**
     * Unsubscribe from one or more subscribed channels.
     *
     * @param array $channels One or more channels to unsubscribe from.
     * @return ValkeyGlide|array|bool The array of unsubscribed channels.
     *
     * @see https://valkey.io/commands/unsubscribe
     * @see ValkeyGlide::subscribe()
     *
     * @example
     * $valkey_glide->subscribe(['channel-1', 'channel-2'], function ($valkey_glide, $channel, $message) {
     *     if ($message == 'quit') {
     *         echo "$channel => 'quit' detected, unsubscribing!\n";
     *         $valkey_glide->unsubscribe([$channel]);
     *     } else {
     *         echo "$channel => $message\n";
     *     }
     * });
     *
     * echo "We've unsubscribed from both channels, exiting\n";
     */
    /* TODO public function unsubscribe(array $channels): ValkeyGlide|array|bool; */

    /**
     * Remove any previously WATCH'ed keys in a transaction.
     *
     * @see https://valkey.io/commands/unwatch
     * @see https://valkey.io/commands/unwatch
     * @see ValkeyGlide::watch()
     *
     * @return True on success and false on failure.
     */
    public function unwatch(): ValkeyGlide|bool;

    /**
     * Watch one or more keys for conditional execution of a transaction.
     *
     * @param array|string $key_or_keys  Either an array with one or more key names, or a string key name
     * @param string       $other_keys   If the first argument was passed as a string, any number of additional
     *                                   string key names may be passed variadically.
     * @return ValkeyGlide|bool
     *
     *
     * @see https://valkey.io/commands/watch
     * @see https://valkey.io/commands/unwatch
     *
     * @example
     * $valkey_glide1 = new ValkeyGlide(['host' => 'localhost']);
     * $valkey_glide2 = new ValkeyGlide(['host' => 'localhost']);
     *
     * // Start watching 'incr-key'
     * $valkey_glide1->watch('incr-key');
     *
     * // Retrieve its value.
     * $val = $valkey_glide1->get('incr-key');
     *
     * // A second client modifies 'incr-key' after we read it.
     * $valkey_glide2->set('incr-key', 0);
     *
     * // Because another client changed the value of 'incr-key' after we read it, this
     * // is no longer a proper increment operation, but because we are `WATCH`ing the
     * // key, this transaction will fail and we can try again.
     * //
     * // If were to comment out the above `$valkey_glide2->set('incr-key', 0)` line the
     * // transaction would succeed.
     * $valkey_glide1->multi();
     * $valkey_glide1->set('incr-key', $val + 1);
     * $res = $valkey_glide1->exec();
     *
     * // bool(false)
     * var_dump($res);
     */
    public function watch(array|string $key, string ...$other_keys): ValkeyGlide|bool;

    /**
     * Block the client up to the provided timeout until a certain number of replicas have confirmed
     * receiving them.
     *
     * @see https://valkey.io/commands/wait
     *
     * @param int $numreplicas The number of replicas we want to confirm write operations
     * @param int $timeout     How long to wait (zero meaning forever).
     *
     * @return ValkeyGlide|int|false The number of replicas that have confirmed or false on failure.
     *
     */
    public function wait(int $numreplicas, int $timeout): int|false;

    /**
     * Acknowledge one or more messages that are pending (have been consumed using XREADGROUP but
     * not yet acknowledged by XACK.)
     *
     * @param string $key   The stream to query.
     * @param string $group The consumer group to use.
     * @param array  $ids   An array of stream entry IDs.
     *
     * @return int|false The number of acknowledged messages
     *
     * @see https://valkey.io/commands/xack
     * @see https://valkey.io/commands/xreadgroup
     * @see ValkeyGlide::xack()
     *
     * @example
     * $valkey_glide->xAdd('ships', '*', ['name' => 'Enterprise']);
     * $valkey_glide->xAdd('ships', '*', ['name' => 'Defiant']);
     *
     * $valkey_glide->xGroup('CREATE', 'ships', 'Federation', '0-0');
     *
     * // Consume a single message with the consumer group 'Federation'
     * $ship = $valkey_glide->xReadGroup('Federation', 'Picard', ['ships' => '>'], 1);
     *
     * /* Retrieve the ID of the message we read.
     * assert(isset($ship['ships']));
     * $id = key($ship['ships']);
     *
     * // The message we just read is now pending.
     * $res = $valkey_glide->xPending('ships', 'Federation'));
     * var_dump($res);
     *
     * // We can tell ValkeyGlide we were able to process the message by using XACK
     * $res = $valkey_glide->xAck('ships', 'Federation', [$id]);
     * assert($res === 1);
     *
     * // The message should no longer be pending.
     * $res = $valkey_glide->xPending('ships', 'Federation');
     * var_dump($res);
     */
    public function xack(string $key, string $group, array $ids): int|false;

    /**
     * Append a message to a stream.
     *
     * @param string $key        The stream name.
     * @param string $id         The ID for the message we want to add.  This can be the special value '*'
     *                           which means ValkeyGlide will generate the ID that appends the message to the
     *                           end of the stream.  It can also be a value in the form <ms>-* which will
     *                           generate an ID that appends to the end of entries with the same <ms> value
     *                           (if any exist).
     * @param int    $maxlen     If specified ValkeyGlide will append the new message but trim any number of the
     *                           oldest messages in the stream until the length is <= $maxlen.
     * @param bool   $approx     Used in conjunction with `$maxlen`, this flag tells ValkeyGlide to trim the stream
     *                           but in a more efficient way, meaning the trimming may not be exactly to
     *                           `$maxlen` values.
     * @param bool   $nomkstream If passed as `TRUE`, the stream must exist for ValkeyGlide to append the message.
     *
     * @see https://valkey.io/commands/xadd
     *
     * @example $valkey_glide->xAdd('ds9-season-1', '1-1', ['title' => 'Emissary Part 1']);
     * @example $valkey_glide->xAdd('ds9-season-1', '1-2', ['title' => 'A Man Alone']);
     */
    public function xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false, bool $nomkstream = false): ValkeyGlide|string|false;

    /**
     * This command allows a consumer to claim pending messages that have been idle for a specified period of time.
     * Its purpose is to provide a mechanism for picking up messages that may have had a failed consumer.
     *
     * @see https://valkey.io/commands/xautoclaim
     * @see https://valkey.io/commands/xclaim
     * @see https://valkey.io/docs/data-types/streams-tutorial/
     *
     * @param string $key      The stream to check.
     * @param string $group    The consumer group to query.
     * @param string $consumer Which consumer to check.
     * @param int    $min_idle The minimum time in milliseconds for the message to have been pending.
     * @param string $start    The minimum message id to check.
     * @param int    $count    An optional limit on how many messages are returned.
     * @param bool   $justid   If the client only wants message IDs and not all of their data.
     *
     * @return ValkeyGlide|array|bool An array of pending IDs or false if there are none, or on failure.
     *
     * @example
     * $valkey_glide->xGroup('CREATE', 'ships', 'combatants', '0-0', true);
     *
     * $valkey_glide->xAdd('ships', '1424-74205', ['name' => 'Defiant']);
     *
     * // Consume the ['name' => 'Defiant'] message
     * $msgs = $valkey_glide->xReadGroup('combatants', "Jem'Hadar", ['ships' => '>'], 1);
     *
     * // The "Jem'Hadar" consumer has the message presently
     * $pending = $valkey_glide->xPending('ships', 'combatants');
     * var_dump($pending);
     *
     * // Assume control of the pending message with a different consumer.
     * $res = $valkey_glide->xAutoClaim('ships', 'combatants', 'Sisko', 0, '0-0');
     *
     * // Now the 'Sisko' consumer owns the message
     * $pending = $valkey_glide->xPending('ships', 'combatants');
     * var_dump($pending);
     */
    public function xautoclaim(string $key, string $group, string $consumer, int $min_idle, string $start, int $count = -1, bool $justid = false): ValkeyGlide|bool|array;

    /**
     * This method allows a consumer to take ownership of pending stream entries, by ID.  Another
     * command that does much the same thing but does not require passing specific IDs is `ValkeyGlide::xAutoClaim`.
     *
     * @see https://valkey.io/commands/xclaim
     * @see https://valkey.io/commands/xautoclaim.
     *
     * @param string $key            The stream we wish to claim messages for.
     * @param string $group          Our consumer group.
     * @param string $consumer       Our consumer.
     * @param int    $min_idle_time  The minimum idle-time in milliseconds a message must have for ownership to be transferred.
     * @param array  $options        An options array that modifies how the command operates.
     *
     *                               <code>
     *                               # Following is an options array describing every option you can pass.  Note that
     *                               # 'IDLE', and 'TIME' are mutually exclusive.
     *                               $options = [
     *                                   'IDLE'       => 3            # Set the idle time of the message to a 3.  By default
     *                                                                # the idle time is set to zero.
     *                                   'TIME'       => 1000*time()  # Same as IDLE except it takes a unix timestamp in
     *                                                                # milliseconds.
     *                                   'RETRYCOUNT' => 0            # Set the retry counter to zero.  By default XCLAIM
     *                                                                # doesn't modify the counter.
     *                                   'FORCE'                      # Creates the pending message entry even if IDs are
     *                                                                # not already
     *                                                                # in the PEL with another client.
     *                                   'JUSTID'                     # Return only an array of IDs rather than the messages
     *                                                                # themselves.
     *                               ];
     *                               </code>
     *
     * @return ValkeyGlide|array|bool      An array of claimed messages or false on failure.
     *
     * @example
     * $valkey_glide->xGroup('CREATE', 'ships', 'combatants', '0-0', true);
     *
     * $valkey_glide->xAdd('ships', '1424-74205', ['name' => 'Defiant']);
     *
     * // Consume the ['name' => 'Defiant'] message
     * $msgs = $valkey_glide->xReadGroup('combatants', "Jem'Hadar", ['ships' => '>'], 1);
     *
     * // The "Jem'Hadar" consumer has the message presently
     * $pending = $valkey_glide->xPending('ships', 'combatants');
     * var_dump($pending);
     *
     * assert($pending && isset($pending[1]));
     *
     * // Claim the message by ID.
     * $claimed = $valkey_glide->xClaim('ships', 'combatants', 'Sisko', 0, [$pending[1]], ['JUSTID']);
     * var_dump($claimed);
     *
     * // Now the 'Sisko' consumer owns the message
     * $pending = $valkey_glide->xPending('ships', 'combatants');
     * var_dump($pending);
     */
    public function xclaim(string $key, string $group, string $consumer, int $min_idle, array $ids, array $options): ValkeyGlide|array|bool;

    /**
     * Remove one or more specific IDs from a stream.
     *
     * @param string $key The stream to modify.
     * @param array $ids One or more message IDs to remove.
     *
     * @return ValkeyGlide|int|false The number of messages removed or false on failure.
     *
     * @example $valkey_glide->xDel('stream', ['1-1', '2-1', '3-1']);
     */
    public function xdel(string $key, array $ids): ValkeyGlide|int|false;

    /**
     * XGROUP
     *
     * Perform various operation on consumer groups for a particular ValkeyGlide STREAM.  What the command does
     * is primarily based on which operation is passed.
     *
     * @see https://valkey.io/commands/xgroup/
     *
     * @param string $operation      The subcommand you intend to execute.  Valid options are as follows
     *                               'HELP'           - ValkeyGlide will return information about the command
     *                                                  Requires: none
     *                               'CREATE'         - Create a consumer group.
     *                                                  Requires:  Key, group, consumer.
     *                               'SETID'          - Set the ID of an existing consumer group for the stream.
     *                                                  Requires:  Key, group, id.
     *                               'CREATECONSUMER' - Create a new consumer group for the stream.  You must
     *                                                  also pass key, group, and the consumer name you wish to
     *                                                  create.
     *                                                  Requires:  Key, group, consumer.
     *                               'DELCONSUMER'    - Delete a consumer from group attached to the stream.
     *                                                  Requires:  Key, group, consumer.
     *                               'DESTROY'        - Delete a consumer group from a stream.
     *                                                  Requires:  Key, group.
     * @param string $key            The STREAM we're operating on.
     * @param string $group          The consumer group we want to create/modify/delete.
     * @param string $id_or_consumer The STREAM id (e.g. '$') or consumer group.  See the operation section
     *                               for information about which to send.
     * @param bool   $mkstream       This flag may be sent in combination with the 'CREATE' operation, and
     *                               cause ValkeyGlide to also create the STREAM if it doesn't currently exist.
     *
     * @param bool   $entriesread    Allows you to set ValkeyGlide' 'entries-read' STREAM value.  This argument is
     *                               only relevant to the 'CREATE' and 'SETID' operations.
     *                               Note:  Requires ValkeyGlide >= 7.0.0.
     *
     * @return mixed                 This command return various results depending on the operation performed.
     */
    public function xgroup(
        string $operation,
        ?string $key = null,
        ?string $group = null,
        ?string $id_or_consumer = null,
        bool $mkstream = false,
        int $entries_read = -2
    ): mixed;

    /**
     * Retrieve information about a stream key.
     *
     * @param string $operation The specific info operation to perform.
     * @param string $arg1      The first argument (depends on operation)
     * @param string $arg2      The second argument
     * @param int    $count     The COUNT argument to `XINFO STREAM`
     *
     * @return mixed This command can return different things depending on the operation being called.
     *
     * @see https://valkey.io/commands/xinfo
     *
     * @example $valkey_glide->xInfo('CONSUMERS', 'stream');
     * @example $valkey_glide->xInfo('GROUPS', 'stream');
     * @example $valkey_glide->xInfo('STREAM', 'stream');
     */
    public function xinfo(string $operation, ?string $arg1 = null, ?string $arg2 = null, int $count = -1): mixed;


    /**
     * Get the number of messages in a ValkeyGlide STREAM key.
     *
     * @param string $key The Stream to check.
     *
     * @return ValkeyGlide|int|false The number of messages or false on failure.
     *
     * @see https://valkey.io/commands/xlen
     *
     * @example $valkey_glide->xLen('stream');
     */
    public function xlen(string $key): ValkeyGlide|int|false;

    /**
     * Interact with stream messages that have been consumed by a consumer group but not yet
     * acknowledged with XACK.
     *
     * @see https://valkey.io/commands/xpending
     * @see https://valkey.io/commands/xreadgroup
     *
     * @param string $key      The stream to inspect.
     * @param string $group    The user group we want to see pending messages from.
     * @param string $start    The minimum ID to consider.
     * @param string $string   The maximum ID to consider.
     * @param string $count    Optional maximum number of messages to return.
     * @param string $consumer If provided, limit the returned messages to a specific consumer.
     *
     * @return ValkeyGlide|array|false The pending messages belonging to the stream or false on failure.
     *
     */
    public function xpending(string $key, string $group, ?string $start = null, ?string $end = null, int $count = -1, ?string $consumer = null): ValkeyGlide|array|false;

    /**
     * Get a range of entries from a STREAM key.
     *
     * @param string $key   The stream key name to list.
     * @param string $start The minimum ID to return.
     * @param string $end   The maximum ID to return.
     * @param int    $count An optional maximum number of entries to return.
     *
     * @return ValkeyGlide|array|bool The entries in the stream within the requested range or false on failure.
     *
     * @see https://valkey.io/commands/xrange
     *
     * @example $valkey_glide->xRange('stream', '0-1', '0-2');
     * @example $valkey_glide->xRange('stream', '-', '+');
     */
    public function xrange(string $key, string $start, string $end, int $count = -1): ValkeyGlide|array|bool;

    /**
     * Consume one or more unconsumed elements in one or more streams.
     *
     * @param array $streams An associative array with stream name keys and minimum id values.
     * @param int   $count   An optional limit to how many entries are returned *per stream*
     * @param int   $block   An optional maximum number of milliseconds to block the caller if no
     *                       data is available on any of the provided streams.
     *
     * @return ValkeyGlide|array|bool An array of read elements or false if there aren't any.
     *
     * @see https://valkey.io/commands/xread
     *
     * @example
     * $valkey_glide->xAdd('s03', '3-1', ['title' => 'The Search, Part I']);
     * $valkey_glide->xAdd('s03', '3-2', ['title' => 'The Search, Part II']);
     * $valkey_glide->xAdd('s03', '3-3', ['title' => 'The House Of Quark']);
     * $valkey_glide->xAdd('s04', '4-1', ['title' => 'The Way of the Warrior']);
     * $valkey_glide->xAdd('s04', '4-3', ['title' => 'The Visitor']);
     * $valkey_glide->xAdd('s04', '4-4', ['title' => 'Hippocratic Oath']);
     *
     * $valkey_glide->xRead(['s03' => '3-2', 's04' => '4-1']);
     */
    public function xread(array $streams, int $count = -1, int $block = -1): ValkeyGlide|array|bool;

    /**
     * Read one or more messages using a consumer group.
     *
     * @param string $group     The consumer group to use.
     * @param string $consumer  The consumer to use.
     * @param array  $streams   An array of stream names and message IDs
     * @param int    $count     Optional maximum number of messages to return
     * @param int    $block     How long to block if there are no messages available.
     *
     * @return ValkeyGlide|array|bool Zero or more unread messages or false on failure.
     *
     * @see https://valkey.io/commands/xreadgroup
     *
     * @example
     * $valkey_glide->xGroup('CREATE', 'episodes', 'ds9', '0-0', true);
     *
     * $valkey_glide->xAdd('episodes', '1-1', ['title' => 'Emissary: Part 1']);
     * $valkey_glide->xAdd('episodes', '1-2', ['title' => 'A Man Alone']);
     *
     * $messages = $valkey_glide->xReadGroup('ds9', 'sisko', ['episodes' => '>']);
     *
     * // After having read the two messages, add another
     * $valkey_glide->xAdd('episodes', '1-3', ['title' => 'Emissary: Part 2']);
     *
     * // Acknowledge the first two read messages
     * foreach ($messages as $stream => $stream_messages) {
     *     $ids = array_keys($stream_messages);
     *     $valkey_glide->xAck('stream', 'ds9', $ids);
     * }
     *
     * // We can now pick up where we left off, and will only get the final message
     * $msgs = $valkey_glide->xReadGroup('ds9', 'sisko', ['episodes' => '>']);
     */
    public function xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1): ValkeyGlide|array|bool;

    /**
     * Get a range of entries from a STREAM key in reverse chronological order.
     *
     * @param string $key   The stream key to query.
     * @param string $end   The maximum message ID to include.
     * @param string $start The minimum message ID to include.
     * @param int    $count An optional maximum number of messages to include.
     *
     * @return ValkeyGlide|array|bool The entries within the requested range, from newest to oldest.
     *
     * @see https://valkey.io/commands/xrevrange
     * @see https://valkey.io/commands/xrange
     *
     * @example $valkey_glide->xRevRange('stream', '0-2', '0-1');
     * @example $valkey_glide->xRevRange('stream', '+', '-');
     */
    public function xrevrange(string $key, string $end, string $start, int $count = -1): ValkeyGlide|array|bool;

    /**
     * Truncate a STREAM key in various ways.
     *
     * @param string $key       The STREAM key to trim.
     * @param string $threshold This can either be a maximum length, or a minimum id.
     *                          MAXLEN - An integer describing the maximum desired length of the stream after the command.
     *                          MINID  - An ID that will become the new minimum ID in the stream, as ValkeyGlide will trim all
     *                                   messages older than this ID.
     * @param bool   $approx    Whether valkey is allowed to do an approximate trimming of the stream.  This is
     *                          more efficient for ValkeyGlide given how streams are stored internally.
     * @param bool   $minid     When set to `true`, users should pass a minimum ID to the `$threshold` argument.
     * @param int    $limit     An optional upper bound on how many entries to trim during the command.
     *
     * @return ValkeyGlide|int|false  The number of entries deleted from the stream.
     *
     * @see https://valkey.io/commands/xtrim
     *
     * @example $valkey_glide->xTrim('stream', 3);
     * @example $valkey_glide->xTrim('stream', '2-1', false, true);
     */
    public function xtrim(string $key, string $threshold, bool $approx = false, bool $minid = false, int $limit = -1): ValkeyGlide|int|false;

    /**
     * Add one or more elements and scores to a ValkeyGlide sorted set.
     *
     * @param string       $key                  The sorted set in question.
     * @param array|float  $score_or_options     Either the score for the first element, or an array of options.
     *                                           <code>
     *                                            $options = [
     *                                                'NX',       # Only update elements that already exist
     *                                                'NX',       # Only add new elements but don't update existing ones.
     *
     *                                                'LT'        # Only update existing elements if the new score is
     *                                                            # less than the existing one.
     *                                                'GT'        # Only update existing elements if the new score is
     *                                                            # greater than the existing one.
     *
     *                                                'CH'        # Instead of returning the number of elements added,
     *                                                            # ValkeyGlide will return the number Of elements that were
     *                                                            # changed in the operation.
     *
     *                                                'INCR'      # Instead of setting each element to the provide score,
     *                                                            # increment the element by the
     *                                                            # provided score, much like ZINCRBY.  When this option
     *                                                            # is passed, you may only send a single score and member.
     *                                            ];
     *
     *                                            Note:  'GX', 'LT', and 'NX' cannot be passed together, and PhpValkeyGlide
     *                                                   will send whichever one is last in the options array.
     *
     * @param mixed        $more_scores_and_mems A variadic number of additional scores and members.
     *
     * @return ValkeyGlide|int|false The return value varies depending on the options passed.
     *
     * Following is information about the options that may be passed as the second argument:
     *
     * @see https://valkey.io/commands/zadd
     *
     * @example $valkey_glide->zadd('zs', 1, 'first', 2, 'second', 3, 'third');
     * @example $valkey_glide->zAdd('zs', ['XX'], 8, 'second', 99, 'new-element');
     */
    public function zAdd(string $key, array|float $score_or_options, mixed ...$more_scores_and_mems): ValkeyGlide|int|float|false;

    /**
     * Return the number of elements in a sorted set.
     *
     * @param string $key The sorted set to retrieve cardinality from.
     *
     * @return ValkeyGlide|int|false The number of elements in the set or false on failure
     *
     * @see https://valkey.io/commands/zcard
     *
     * @example $valkey_glide->zCard('zs');
     */
    public function zCard(string $key): ValkeyGlide|int|false;

    /**
     * Count the number of members in a sorted set with scores inside a provided range.
     *
     * @param string $key The sorted set to check.
     * @param int|string $min The minimum score to include in the count
     * @param int|string $max The maximum score to include in the count
     *
     * NOTE:  In addition to a floating point score you may pass the special values of '-inf' and
     *        '+inf' meaning negative and positive infinity, respectively.
     *
     * @see https://valkey.io/commands/zcount
     *
     * @example $valkey_glide->zCount('fruit-rankings', '0', '+inf');
     * @example $valkey_glide->zCount('fruit-rankings', 50, 60);
     * @example $valkey_glide->zCount('fruit-rankings', '-inf', 0);
     */
    public function zCount(string $key, int|string $start, int|string $end): ValkeyGlide|int|false;

    /**
     * Create or increment the score of a member in a ValkeyGlide sorted set
     *
     * @param string $key   The sorted set in question.
     * @param float  $value How much to increment the score.
     *
     * @return ValkeyGlide|float|false The new score of the member or false on failure.
     *
     * @see https://valkey.io/commands/zincrby
     *
     * @example $valkey_glide->zIncrBy('zs', 5.0, 'bananas');
     * @example $valkey_glide->zIncrBy('zs', 2.0, 'eggplants');
     */
    public function zIncrBy(string $key, float $value, mixed $member): ValkeyGlide|float|false;

    /**
     * Count the number of elements in a sorted set whose members fall within the provided
     * lexographical range.
     *
     * @param string $key The sorted set to check.
     * @param string $min The minimum matching lexographical string
     * @param string $max The maximum matching lexographical string
     *
     * @return ValkeyGlide|int|false The number of members that fall within the range or false on failure.
     *
     * @see https://valkey.io/commands/zlexcount
     *
     * @example
     * $valkey_glide->zAdd('captains', 0, 'Janeway', 0, 'Kirk', 0, 'Picard', 0, 'Sisko', 0, 'Archer');
     * $valkey_glide->zLexCount('captains', '[A', '[S');
     */
    public function zLexCount(string $key, string $min, string $max): ValkeyGlide|int|false;

    /**
     * Retrieve the score of one or more members in a sorted set.
     *
     * @see https://valkey.io/commands/zmscore
     *
     * @param string $key           The sorted set
     * @param mixed  $member        The first member to return the score from
     * @param mixed  $other_members One or more additional members to return the scores of.
     *
     * @return ValkeyGlide|array|false An array of the scores of the requested elements.
     *
     * @example
     * $valkey_glide->zAdd('zs', 0, 'zero', 1, 'one', 2, 'two', 3, 'three');
     *
     * $valkey_glide->zMScore('zs', 'zero', 'two');
     * $valkey_glide->zMScore('zs', 'one', 'not-a-member');
     */
    public function zMscore(string $key, mixed $member, mixed ...$other_members): ValkeyGlide|array|false;

    /**
     * Pop one or more of the highest scoring elements from a sorted set.
     *
     * @param string $key   The sorted set to pop elements from.
     * @param int    $count An optional count of elements to pop.
     *
     * @return ValkeyGlide|array|false All of the popped elements with scores or false on failure
     *
     * @see https://valkey.io/commands/zpopmax
     *
     * @example
     * $valkey_glide->zAdd('zs', 0, 'zero', 1, 'one', 2, 'two', 3, 'three');
     *
     * $valkey_glide->zPopMax('zs');
     * $valkey_glide->zPopMax('zs', 2);.
     */
    public function zPopMax(string $key, ?int $count = null): ValkeyGlide|array|false;

    /**
     * Pop one or more of the lowest scoring elements from a sorted set.
     *
     * @param string $key   The sorted set to pop elements from.
     * @param int    $count An optional count of elements to pop.
     *
     * @return ValkeyGlide|array|false The popped elements with their scores or false on failure.
     *
     * @see https://valkey.io/commands/zpopmin
     *
     * @example
     * $valkey_glide->zAdd('zs', 0, 'zero', 1, 'one', 2, 'two', 3, 'three');
     *
     * $valkey_glide->zPopMin('zs');
     * $valkey_glide->zPopMin('zs', 2);
     */
    public function zPopMin(string $key, ?int $count = null): ValkeyGlide|array|false;

    /**
     * Retrieve a range of elements of a sorted set between a start and end point.
     * How the command works in particular is greatly affected by the options that
     * are passed in.
     *
     * @param string          $key     The sorted set in question.
     * @param mixed           $start   The starting index we want to return.
     * @param mixed           $end     The final index we want to return.
     *
     * @param array|bool|null $options This value may either be an array of options to pass to
     *                                 the command, or for historical purposes a boolean which
     *                                 controls just the 'WITHSCORES' option.
     *                                 <code>
     *                                 $options = [
     *                                     'WITHSCORES' => true,     # Return both scores and members.
     *                                     'LIMIT'      => [10, 10], # Start at offset 10 and return 10 elements.
     *                                     'REV'                     # Return the elements in reverse order
     *                                     'BYSCORE',                # Treat `start` and `end` as scores instead
     *                                     'BYLEX'                   # Treat `start` and `end` as lexicographical values.
     *                                 ];
     *                                 </code>
     *
     *                                 Note:  'BYLEX' and 'BYSCORE' are mutually exclusive.
     *
     *
     * @return ValkeyGlide|array|false  An array with matching elements or false on failure.
     *
     * @see https://valkey.io/commands/zrange/
     * @category zset
     *
     * @example $valkey_glide->zRange('zset', 0, -1);
     * @example $valkey_glide->zRange('zset', '-inf', 'inf', ['byscore']);
     */
    public function zRange(string $key, string|int $start, string|int $end, array|bool|null $options = null): ValkeyGlide|array|false;

    /**
     * Retrieve a range of elements from a sorted set by legographical range.
     *
     * @param string $key    The sorted set to retrieve elements from
     * @param string $min    The minimum legographical value to return
     * @param string $max    The maximum legographical value to return
     * @param int    $offset An optional offset within the matching values to return
     * @param int    $count  An optional count to limit the replies to (used in conjunction with offset)
     *
     * @return ValkeyGlide|array|false An array of matching elements or false on failure.
     *
     * @see https://valkey.io/commands/zrangebylex
     *
     * @example
     * $valkey_glide = new ValkeyGlide(['host' => 'localhost']);
     * $valkey_glide->zAdd('captains', 0, 'Janeway', 0, 'Kirk', 0, 'Picard', 0, 'Sisko', 0, 'Archer');
     *
     * $valkey_glide->zRangeByLex('captains', '[A', '[S');
     * $valkey_glide->zRangeByLex('captains', '[A', '[S', 2, 2);
     */
    public function zRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1): ValkeyGlide|array|false;

    /**
     * Retrieve a range of members from a sorted set by their score.
     *
     * @param string $key     The sorted set to query.
     * @param string $start   The minimum score of elements that ValkeyGlide should return.
     * @param string $end     The maximum score of elements that ValkeyGlide should return.
     * @param array  $options Options that change how ValkeyGlide will execute the command.
     *
     *                        OPTION       TYPE            MEANING
     *                        'WITHSCORES' bool            Whether to also return scores.
     *                        'LIMIT'      [offset, count] Limit the reply to a subset of elements.
     *
     * @return ValkeyGlide|array|false The number of matching elements or false on failure.
     *
     * @see https://valkey.io/commands/zrangebyscore
     *
     * @example $valkey_glide->zRangeByScore('zs', 20, 30, ['WITHSCORES' => true]);
     * @example $valkey_glide->zRangeByScore('zs', 20, 30, ['WITHSCORES' => true, 'LIMIT' => [5, 5]]);
     */
    public function zRangeByScore(string $key, string $start, string $end, array $options = []): ValkeyGlide|array|false;

    /**
     * This command is similar to ZRANGE except that instead of returning the values directly
     * it will store them in a destination key provided by the user
     *
     * @param string           $dstkey  The key to store the resulting element(s)
     * @param string           $srckey  The source key with element(s) to retrieve
     * @param string           $start   The starting index to store
     * @param string           $end     The ending index to store
     * @param array|bool|null  $options Our options array that controls how the command will function.
     *
     * @return ValkeyGlide|int|false The number of elements stored in $dstkey or false on failure.
     *
     * @see https://valkey.io/commands/zrange/
     * @see ValkeyGlide::zRange
     * @category zset
     *
     * See {@link ValkeyGlide::zRange} for a full description of the possible options.
     */
    public function zrangestore(
        string $dstkey,
        string $srckey,
        string $start,
        string $end,
        array|bool|null $options = null
    ): ValkeyGlide|int|false;

    /**
     * Retrieve one or more random members from a ValkeyGlide sorted set.
     *
     * @param string $key     The sorted set to pull random members from.
     * @param array  $options One or more options that determine exactly how the command operates.
     *
     *                        OPTION       TYPE     MEANING
     *                        'COUNT'      int      The number of random members to return.
     *                        'WITHSCORES' bool     Whether to return scores and members instead of
     *
     * @return ValkeyGlide|string|array One or more random elements.
     *
     * @see https://valkey.io/commands/zrandmember
     *
     * @example $valkey_glide->zRandMember('zs', ['COUNT' => 2, 'WITHSCORES' => true]);
     */
    public function zRandMember(string $key, ?array $options = null): ValkeyGlide|string|array;

    /**
     * Get the rank of a member of a sorted set, by score.
     *
     * @param string $key     The sorted set to check.
     * @param mixed  $member The member to test.
     *
     * @return ValkeyGlide|int|false The rank of the requested member.
     * @see https://valkey.io/commands/zrank
     *
     * @example $valkey_glide->zRank('zs', 'zero');
     * @example $valkey_glide->zRank('zs', 'three');
     */
    public function zRank(string $key, mixed $member): ValkeyGlide|int|false;

    /**
     * Remove one or more members from a ValkeyGlide sorted set.
     *
     * @param mixed $key           The sorted set in question.
     * @param mixed $member        The first member to remove.
     * @param mixed $other_members One or more members to remove passed in a variadic fashion.
     *
     * @return ValkeyGlide|int|false The number of members that were actually removed or false on failure.
     *
     * @see https://valkey.io/commands/zrem
     *
     * @example $valkey_glide->zRem('zs', 'mem:0', 'mem:1', 'mem:2', 'mem:6', 'mem:7', 'mem:8', 'mem:9');
     */
    public function zRem(mixed $key, mixed $member, mixed ...$other_members): ValkeyGlide|int|false;

    /**
     * Remove zero or more elements from a ValkeyGlide sorted set by legographical range.
     *
     * @param string $key The sorted set to remove elements from.
     * @param string $min The start of the lexographical range to remove.
     * @param string $max The end of the lexographical range to remove
     *
     * @return ValkeyGlide|int|false The number of elements removed from the set or false on failure.
     *
     * @see https://valkey.io/commands/zremrangebylex
     * @see ValkeyGlide::zrangebylex()
     *
     * @example $valkey_glide->zRemRangeByLex('zs', '[a', '(b');
     * @example $valkey_glide->zRemRangeByLex('zs', '(banana', '(eggplant');
     */
    public function zRemRangeByLex(string $key, string $min, string $max): ValkeyGlide|int|false;

    /**
     * Remove one or more members of a sorted set by their rank.
     *
     * @param string $key    The sorted set where we want to remove members.
     * @param int    $start  The rank when we want to start removing members
     * @param int    $end    The rank we want to stop removing membersk.
     *
     * @return ValkeyGlide|int|false The number of members removed from the set or false on failure.
     *
     * @see https://valkey.io/commands/zremrangebyrank
     *
     * @example $valkey_glide->zRemRangeByRank('zs', 0, 3);
     */
    public function zRemRangeByRank(string $key, int $start, int $end): ValkeyGlide|int|false;

    /**
     * Remove one or more members of a sorted set by their score.
     *
     * @param string $key    The sorted set where we want to remove members.
     * @param int    $start  The lowest score to remove.
     * @param int    $end    The highest score to remove.
     *
     * @return ValkeyGlide|int|false The number of members removed from the set or false on failure.
     *
     * @see https://valkey.io/commands/zremrangebyrank
     *
     * @example
     * $valkey_glide->zAdd('zs', 2, 'two', 4, 'four', 6, 'six');
     * $valkey_glide->zRemRangeByScore('zs', 2, 4);
     */
    public function zRemRangeByScore(string $key, string $start, string $end): ValkeyGlide|int|false;

     /**
     * List elements from a ValkeyGlide sorted set by score, highest to lowest
     *
     * @param string $key     The sorted set to query.
     * @param string $max     The highest score to include in the results.
     * @param string $min     The lowest score to include in the results.
     * @param array  $options An options array that modifies how the command executes.
     *
     *                        <code>
     *                        $options = [
     *                            'WITHSCORES' => true|false # Whether or not to return scores
     *                            'LIMIT' => [offset, count] # Return a subset of the matching members
     *                        ];
     *                        </code>
     *
     *                        NOTE:  For legacy reason, you may also simply pass `true` for the
     *                               options argument, to mean `WITHSCORES`.
     *
     * @return ValkeyGlide|array|false The matching members in reverse order of score or false on failure.
     *
     * @see https://valkey.io/commands/zrevrangebyscore
     *
     * @example
     * $valkey_glide->zadd('oldest-people', 122.4493, 'Jeanne Calment', 119.2932, 'Kane Tanaka',
     *                               119.2658, 'Sarah Knauss',   118.7205, 'Lucile Randon',
     *                               117.7123, 'Nabi Tajima',    117.6301, 'Marie-Louise Meilleur',
     *                               117.5178, 'Violet Brown',   117.3753, 'Emma Morano',
     *                               117.2219, 'Chiyo Miyako',   117.0740, 'Misao Okawa');
     *
     * $valkey_glide->zRevRangeByScore('oldest-people', 122, 119);
     * $valkey_glide->zRevRangeByScore('oldest-people', 'inf', 118);
     * $valkey_glide->zRevRangeByScore('oldest-people', '117.5', '-inf', ['LIMIT' => [0, 1]]);
     */
    public function zRevRangeByScore(string $key, string $max, string $min, array|bool $options = []): ValkeyGlide|array|false;

    /**
    * Retrieve a member of a sorted set by reverse rank.
    *
    * @param string $key      The sorted set to query.
    * @param mixed  $member   The member to look up.
    *
    * @return ValkeyGlide|int|false The reverse rank (the rank if counted high to low) of the member or
    *                         false on failure.
    * @see https://valkey.io/commands/zrevrank
    *
    * @example
    * $valkey_glide->zAdd('ds9-characters', 10, 'Sisko', 9, 'Garak', 8, 'Dax', 7, 'Odo');
    *
    * $valkey_glide->zrevrank('ds9-characters', 'Sisko');
    * $valkey_glide->zrevrank('ds9-characters', 'Garak');
    */
    public function zRevRank(string $key, mixed $member): ValkeyGlide|int|false;

    /**
     * Get the score of a member of a sorted set.
     *
     * @param string $key    The sorted set to query.
     * @param mixed  $member The member we wish to query.
     *
     * @return The score of the requested element or false if it is not found.
     *
     * @see https://valkey.io/commands/zscore
     *
     * @example
     * $valkey_glide->zAdd('telescopes', 11.9, 'LBT', 10.4, 'GTC', 10, 'HET');
     * $valkey_glide->zScore('telescopes', 'LBT');
     */
    public function zScore(string $key, mixed $member): ValkeyGlide|float|false;

    /**
     * Given one or more sorted set key names, return every element that is in the first
     * set but not any of the others.
     *
     * @param array $keys    One or more sorted sets.
     * @param array $options An array which can contain ['WITHSCORES' => true] if you want ValkeyGlide to
     *                       return members and scores.
     *
     * @return ValkeyGlide|array|false An array of members or false on failure.
     *
     * @see https://valkey.io/commands/zdiff
     *
     * @example
     * $valkey_glide->zAdd('primes', 1, 'one', 3, 'three', 5, 'five');
     * $valkey_glide->zAdd('evens', 2, 'two', 4, 'four');
     * $valkey_glide->zAdd('mod3', 3, 'three', 6, 'six');
     *
     * $valkey_glide->zDiff(['primes', 'evens', 'mod3']);
     */
    public function zdiff(array $keys, ?array $options = null): ValkeyGlide|array|false;

    /**
     * Store the difference of one or more sorted sets in a destination sorted set.
     *
     * See {@link ValkeyGlide::zdiff} for a more detailed description of how the diff operation works.
     *
     * @param string $key  The destination set name.
     * @param array  $keys One or more source key names
     *
     * @return ValkeyGlide|int|false The number of elements stored in the destination set or false on
     *                         failure.
     *
     * @see https://valkey.io/commands/zdiff
     * @see ValkeyGlide::zdiff()
     */
    public function zdiffstore(string $dst, array $keys): ValkeyGlide|int|false;

    /**
     * Compute the intersection of one or more sorted sets and return the members
     *
     * @param array $keys    One or more sorted sets.
     * @param array $weights An optional array of weights to be applied to each set when performing
     *                       the intersection.
     * @param array $options Options for how ValkeyGlide should combine duplicate elements when performing the
     *                       intersection.  See ValkeyGlide::zunion() for details.
     *
     * @return ValkeyGlide|array|false All of the members that exist in every set.
     *
     * @see https://valkey.io/commands/zinter
     *
     * @example
     * $valkey_glide->zAdd('TNG', 2, 'Worf', 2.5, 'Data', 4.0, 'Picard');
     * $valkey_glide->zAdd('DS9', 2.5, 'Worf', 3.0, 'Kira', 4.0, 'Sisko');
     *
     * $valkey_glide->zInter(['TNG', 'DS9']);
     * $valkey_glide->zInter(['TNG', 'DS9'], NULL, ['withscores' => true]);
     * $valkey_glide->zInter(['TNG', 'DS9'], NULL, ['withscores' => true, 'aggregate' => 'max']);
     */
    public function zinter(array $keys, ?array $weights = null, ?array $options = null): ValkeyGlide|array|false;

    /**
     * Similar to ZINTER but instead of returning the intersected values, this command returns the
     * cardinality of the intersected set.
     *
     * @see https://valkey.io/commands/zintercard
     * @see https://valkey.io/commands/zinter
     * @see ValkeyGlide::zinter()
     *
     * @param array $keys   One or more sorted set key names.
     * @param int   $limit  An optional upper bound on the returned cardinality.  If set to a value
     *                      greater than zero, ValkeyGlide will stop processing the intersection once the
     *                      resulting cardinality reaches this limit.
     *
     * @return ValkeyGlide|int|false The cardinality of the intersection or false on failure.
     *
     * @example
     * $valkey_glide->zAdd('zs1', 1, 'one', 2, 'two', 3, 'three', 4, 'four');
     * $valkey_glide->zAdd('zs2', 2, 'two', 4, 'four');
     *
     * $valkey_glide->zInterCard(['zs1', 'zs2']);
     */
    public function zintercard(array $keys, int $limit = -1): ValkeyGlide|int|false;

    /**
     * Compute the intersection of one or more sorted sets storing the result in a new sorted set.
     *
     * @param string $dst       The destination sorted set to store the intersected values.
     * @param array  $keys      One or more sorted set key names.
     * @param array  $weights   An optional array of floats to weight each passed input set.
     * @param string $aggregate An optional aggregation method to use.
     *
     *                          'SUM' - Store sum of all intersected members (this is the default).
     *                          'MIN' - Store minimum value for each intersected member.
     *                          'MAX' - Store maximum value for each intersected member.
     *
     * @return ValkeyGlide|int|false  The total number of members writtern to the destination set or false on failure.
     *
     * @see https://valkey.io/commands/zinterstore
     * @see https://valkey.io/commands/zinter
     *
     * @example
     * $valkey_glide->zAdd('zs1', 3, 'apples', 2, 'pears');
     * $valkey_glide->zAdd('zs2', 4, 'pears', 3, 'bananas');
     * $valkey_glide->zAdd('zs3', 2, 'figs', 3, 'pears');
     *
     * $valkey_glide->zInterStore('fruit-sum', ['zs1', 'zs2', 'zs3']);
     * $valkey_glide->zInterStore('fruit-max', ['zs1', 'zs2', 'zs3'], NULL, 'MAX');
     */
    public function zinterstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): ValkeyGlide|int|false;

    /**
     * Scan the members of a sorted set incrementally, using a cursor
     *
     * @param string $key        The sorted set to scan.
     * @param string $iterator   A reference to an iterator that should be initialized to NULL initially, that
     *                           will be updated after each subsequent call to ZSCAN.  Once the iterator
     *                           has returned to "0" the scan is complete
     * @param string|null $pattern An optional glob-style pattern that limits which members are returned during
     *                           the scanning process.
     * @param int    $count      A hint for ValkeyGlide that tells it how many elements it should test before returning
     *                           from the call.  The higher the more work ValkeyGlide may do in any one given call to
     *                           ZSCAN potentially blocking for longer periods of time.
     *
     * @return ValkeyGlide|array|false An array of elements or false on failure.
     *
     * @see https://valkey.io/commands/zscan
     * @see https://valkey.io/commands/scan
     * @see ValkeyGlide::scan()
     *
     * NOTE:  See ValkeyGlide::scan() for detailed example code on how to call SCAN like commands.
     *
     */
    public function zscan(string $key, null|string &$iterator, ?string $pattern = null, int $count = 0): ValkeyGlide|array|false;

    /**
     * Retrieve the union of one or more sorted sets
     *
     * @param array $keys     One or more sorted set key names
     * @param array $weights  An optional array with floating point weights used when performing the union.
     *                        Note that if this argument is passed, it must contain the same number of
     *                        elements as the $keys array.
     * @param array $options  An array that modifies how this command functions.
     *
     *                        <code>
     *                        $options = [
     *                            # By default when members exist in more than one set ValkeyGlide will SUM
     *                            # total score for each match.  Instead, it can return the AVG, MIN,
     *                            # or MAX value based on this option.
     *                            'AGGREGATE' => 'sum' | 'min' | 'max'
     *
     *                            # Whether ValkeyGlide should also return each members aggregated score.
     *                            'WITHSCORES' => true | false
     *                        ]
     *                        </code>
     *
     * @return ValkeyGlide|array|false The union of each sorted set or false on failure
     *
     * @example
     * $valkey_glide->del('store1', 'store2', 'store3');
     * $valkey_glide->zAdd('store1', 1, 'apples', 3, 'pears', 6, 'bananas');
     * $valkey_glide->zAdd('store2', 3, 'apples', 5, 'coconuts', 2, 'bananas');
     * $valkey_glide->zAdd('store3', 2, 'bananas', 6, 'apples', 4, 'figs');
     *
     * $valkey_glide->zUnion(['store1', 'store2', 'store3'], NULL, ['withscores' => true]);
     * $valkey_glide->zUnion(['store1', 'store3'], [2, .5], ['withscores' => true]);
     * $valkey_glide->zUnion(['store1', 'store3'], [2, .5], ['withscores' => true, 'aggregate' => 'MIN']);
     */
    public function zunion(array $keys, ?array $weights = null, ?array $options = null): ValkeyGlide|array|false;

    /**
     * Perform a union on one or more ValkeyGlide sets and store the result in a destination sorted set.
     *
     * @param string $dst       The destination set to store the union.
     * @param array  $keys      One or more input keys on which to perform our union.
     * @param array  $weights   An optional weights array used to weight each input set.
     * @param string $aggregate An optional modifier in how ValkeyGlide will combine duplicate members.
     *                          Valid:  'MIN', 'MAX', 'SUM'.
     *
     * @return ValkeyGlide|int|false The number of members stored in the destination set or false on failure.
     *
     * @see https://valkey.io/commands/zunionstore
     * @see ValkeyGlide::zunion()
     *
     * @example
     * $valkey_glide->zAdd('zs1', 1, 'one', 3, 'three');
     * $valkey_glide->zAdd('zs1', 2, 'two', 4, 'four');
     * $valkey_glide->zadd('zs3', 1, 'one', 7, 'five');
     *
     * $valkey_glide->zUnionStore('dst', ['zs1', 'zs2', 'zs3']);
     */
    public function zunionstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): ValkeyGlide|int|false;
}

class ValkeyGlideException extends RuntimeException
{
}
