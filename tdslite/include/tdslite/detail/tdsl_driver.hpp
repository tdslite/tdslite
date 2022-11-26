/**
 * _________________________________________________
 *
 * @file   tdsl_driver.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_DRIVER_HPP
#define TDSL_DETAIL_DRIVER_HPP

#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>

namespace tdsl { namespace detail {

    /**
     * tdslite TDS driver
     *
     * @tparam NetImpl The networking implementation
     */
    template <typename NetImpl>
    struct tdsl_driver {
        using tds_context_type       = detail::tds_context<NetImpl>;
        using login_context_type     = detail::login_context<NetImpl>;
        using login_parameters_type  = typename login_context_type::login_parameters;
        using wlogin_parameters_type = typename login_context_type::wlogin_parameters;
        using sql_command_type       = detail::command_context<NetImpl>;

        enum class e_driver_error_code
        {
            success,
            connection_failed,
            login_failed,
            connection_param_server_name_empty,
            connection_param_packet_size_invalid
        };

        /**
         * Connection parameters
         */
        struct connection_parameters : public login_parameters_type {
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
        inline auto connect(const connection_parameters & p) noexcept -> e_driver_error_code {

            // Validate the parameters first
            auto pvr = p.validate();
            if (not(e_driver_error_code::success == pvr)) {
                return pvr;
            }

            if (not(0 == tds_ctx.connect(p.server_name, p.port))) {
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
         * @param [in] user_ptr User-supplied first argument to pass to the
         *             callback function. Can be nullptr if unused.
         * @param [in] callback Pointer to the function to call whenever
         *             an INFO token is received
         */
        void set_info_callback(
            void * user_ptr,
            typename tds_context_type::info_callback_type::function_type callback) noexcept {
            tds_ctx.callbacks.info = {user_ptr, callback};
        }

        // --------------------------------------------------------------------------------

        /**
         * Send a query to the server
         *
         * @param [in] command SQL command to execute
         * @param [in] uptr User supplied pointer, will be passed to row_callback as first argument
         *             on every invocation
         * @param [in] row_callback Callback to invoke for each row received as
         *             a result to the query. (optional)
         *
         * @return tdsl::uint32_t Amount of rows affected from the query
         */
        template <typename T>
        inline auto execute_query(
            T command, void * uptr = nullptr,
            typename sql_command_type::row_callback_fn_t row_callback =
                +[](void *, const tds_colmetadata_token &, const tdsl_row &) -> void {}) noexcept
            -> tdsl::uint32_t {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return sql_command_type{tds_ctx}.execute_query(command, uptr, row_callback);
        }

        // --------------------------------------------------------------------------------

        /**
         * Send a query to the server
         * (const char array overload)
         *
         * @param [in] command SQL command to execute
         * @param [in] uptr User supplied pointer, will be passed to row_callback as first argument
         *             on every invocation
         * @param [in] row_callback Callback to invoke for each row received as
         *             a result to the query. (optional)
         *
         * @return tdsl::uint32_t Amount of rows affected from the query
         */
        template <tdsl::uint32_t N>
        inline auto execute_query(
            const char (&command) [N], void * uptr = nullptr,
            typename sql_command_type::row_callback_fn_t row_callback =
                +[](void *, const tds_colmetadata_token &, const tdsl_row &) -> void {}) noexcept
            -> tdsl::uint32_t {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return execute_query(tdsl::string_view{command}, uptr, row_callback);
        }

    private:
        /**
         * Driver's TDS context. All TDS related
         * operations are routed through this object.
         */
        tds_context_type tds_ctx;
    };

}} // namespace tdsl::detail

#endif