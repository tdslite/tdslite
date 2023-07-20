/**
 * ____________________________________________________
 * High level type that integrates sub-level impl
 * to provide a TDS driver.
 *
 * @file   tdsl_driver.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_DRIVER_HPP
#define TDSL_DETAIL_DRIVER_HPP

#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>

namespace tdsl { namespace detail {

    /**
     * tdslite main TDS driver
     *
     * @tparam NetImpl The networking implementation
     */
    template <typename NetImpl>
    struct tdsl_driver {
        using tds_context_type      = detail::tds_context<NetImpl>;
        using info_callback_type    = typename tds_context_type::info_callback_type::function_type;
        using login_context_type    = detail::login_context<NetImpl>;
        using login_parameters_type = typename login_context_type::login_parameters;
        using pmem_login_parameters_type = typename login_context_type::pmem_login_parameters;
        using wlogin_parameters_type     = typename login_context_type::wlogin_parameters;
        using sql_command_type           = detail::command_context<NetImpl>;
        using sql_command_options_type   = typename sql_command_type::command_options;
        using sql_command_rpc_mode       = e_rpc_mode;
        using sql_command_rpc_result     = typename sql_command_type::execute_rpc_result;
        using sql_command_row_callback   = typename sql_command_type::row_callback_fn_t;
        using sql_command_query_result   = typename sql_command_type::query_result;

        // --------------------------------------------------------------------------------

        enum class e_driver_error_code
        {
            success,
            connection_failed,
            login_failed,
            connection_param_server_name_empty,
            connection_param_packet_size_invalid
        };

        // --------------------------------------------------------------------------------

        template <typename T>
        struct connection_parameters_base : public T {
            tdsl::uint16_t port = {1433};

            /**
             * Check whether connection parameters have
             * sane values.
             *
             * @returns e_driver_error_code::success if all parameters are OK
             * @returns e_driver_error_code::connection_param_server_name_empty
             *          if the server_name is empty
             * @returns e_driver_error_code::connection_param_packet_size_invalid
             *          if packet_size is smaller or larger than allowed
             *          (must be >=512 && <=32767)
             */
            inline auto validate() const noexcept -> e_driver_error_code {

                // Server name cannot be empty
                if (not this->server_name) {
                    return e_driver_error_code::connection_param_server_name_empty;
                }

                // TDS packet size must be in >= 512 && <= 32767 range
                if (this->packet_size < 512 || this->packet_size > 32767) {
                    return e_driver_error_code::connection_param_packet_size_invalid;
                }

                return e_driver_error_code::success;
            }
        };

        using connection_parameters  = connection_parameters_base<login_parameters_type>;
        using wconnection_parameters = connection_parameters_base<wlogin_parameters_type>;
        using progmem_connection_parameters =
            connection_parameters_base<pmem_login_parameters_type>;

        /**
         * Construct a new tdsl driver object
         *
         * (forwarding constructor)
         */
        template <typename... Args>
        inline tdsl_driver(Args &&... args) noexcept : tds_ctx(TDSL_FORWARD(args)...) {}

        // --------------------------------------------------------------------------------

        /**
         * Try to connect to the SQL server with details
         * specified in @p p.
         *
         * @param [in] p Connection parameters
         *
         * @returns e_driver_error_code::success if connected & logged in
         * @returns e_driver_error_code::connection_failed if all
         *          connection attempts are failed
         * @returns e_driver_error_code::login_failed if connection succeeded,
         *          but login failed
         * @returns e_driver_error_code::connection_param_server_name_empty
         *          if the server_name is empty
         * @returns e_driver_error_code::connection_param_packet_size_invalid
         *          if packet_size is smaller or larger than allowed
         *          (must be >=512 && <=32767)
         */
        template <typename T>
        inline auto connect(const connection_parameters_base<T> & p) noexcept
            -> e_driver_error_code {

            // Validate the parameters first
            auto pvr = p.validate();
            if (not(e_driver_error_code::success == pvr)) {
                return pvr;
            }

            if (not tds_ctx.connect(p.server_name, p.port)) {
                return e_driver_error_code::connection_failed;
            }

            if (not(login_context_type::e_login_status::success ==
                    login_context_type{tds_ctx}.do_login(p))) {
                return e_driver_error_code::login_failed;
            }

            return e_driver_error_code::success;
        }

        // --------------------------------------------------------------------------------

        /**
         * Set callback function for INFO/ERROR messages.
         *
         * This callback function will be called when server sends
         * INFO/ERROR tokens. Useful for diagnostics.
         *
         * @param [in] callback Pointer to the function to call whenever
         *             an INFO token is received
         * @param [in] user_ptr (optional) User-supplied first argument to pass to the
         *             callback function. `nullptr` by default (unused).
         */
        inline auto set_info_callback(info_callback_type callback,
                                      void * user_ptr = nullptr) noexcept -> void {
            tds_ctx.callbacks.info = {callback, user_ptr};
        }

        // --------------------------------------------------------------------------------

        /**
         * Send a query to the server
         *
         * @param [in] command SQL command to execute
         * @param [in] uptr User supplied pointer, will be passed to row_callback as first
         * argument on every invocation
         * @param [in] row_callback Callback to invoke for each row received as
         *             a result to the query. (optional)
         *
         * @return tdsl::uint32_t Amount of rows affected from the query
         */
        template <typename T>
        inline auto execute_query(
            T command,
            sql_command_row_callback row_callback = +[](void *, const tds_colmetadata_token &,
                                                        const tdsl_row &) -> void {},
            void * uptr                           = nullptr) noexcept -> sql_command_query_result {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return sql_command_type{tds_ctx, command_options}.execute_query(command, row_callback,
                                                                            uptr);
        }

        // --------------------------------------------------------------------------------

        /**
         * Send a query to the server
         * (const char array overload)
         *
         * @param [in] command SQL command to execute
         * @param [in] uptr User supplied pointer, will be passed to row_callback as first
         * argument on every invocation
         * @param [in] row_callback Callback to invoke for each row received as
         *             a result to the query. (optional)
         *
         * @return tdsl::uint32_t Amount of rows affected from the query
         */
        template <tdsl::uint32_t N>
        inline auto execute_query(
            const char (&command) [N],
            sql_command_row_callback row_callback = +[](void *, const tds_colmetadata_token &,
                                                        const tdsl_row &) -> void {},
            void * uptr                           = nullptr) noexcept -> sql_command_query_result {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return execute_query(tdsl::string_view{command}, row_callback, uptr);
        }

        // --------------------------------------------------------------------------------

        /**
         * Perform a remote procedure call (e.g. execute a stored procedure or
         * a parameterized query)
         *
         * @tparam T String view type
         *
         * @param [in] command Command to execute
         * @param [in] params Parameters of the command, if any
         * @param [in] mode RPC execution mode
         * @param [in] row_callback Row callback function (optional)
         * @param [in] rcb_uptr Row callback user pointer (optional)
         *
         * The result set returned by query @p command can be read by providing
         * a row callback function
         *
         * @returns execute_rpc_result::unexpected(e_rpc_error_code::invalid_mode) if @p mode
         *          value is invalid
         * @returns rows_affected if successful
         */
        template <typename T, traits::enable_when::same_any_of<T, string_view, wstring_view,
                                                               struct progmem_string_view> = true>
        inline sql_command_rpc_result execute_rpc(
            T command, tdsl::span<sql_parameter_binding> params = {},
            sql_command_rpc_mode mode             = {sql_command_rpc_mode::executesql},

            sql_command_row_callback row_callback = +[](void *, const tds_colmetadata_token &,
                                                        const tdsl_row &) -> void {},
            void * rcb_uptr                       = nullptr) noexcept {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return sql_command_type{tds_ctx, command_options}.execute_rpc(command, params, mode,
                                                                          row_callback, rcb_uptr);
        }

        // --------------------------------------------------------------------------------

        /**
         * Enable/disable column name reading for the result
         * set returned from the commands
         *
         * @param [in] value The value
         */
        inline void option_set_read_column_names(bool value) noexcept {
            command_options.flags.read_colnames = value;
        }

    private:
        /**
         * Driver's TDS context. All TDS related
         * operations are routed through this object.
         */
        tds_context_type tds_ctx;

        /**
         * Command options
         */
        sql_command_options_type command_options{};
    };
}} // namespace tdsl::detail

#endif