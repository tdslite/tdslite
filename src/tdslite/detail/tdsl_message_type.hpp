/**
 * ____________________________________________________
 *
 * @file   tds_message_type.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   17.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDS_MESSAGE_TYPE_HPP
#define TDSL_DETAIL_TDS_MESSAGE_TYPE_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl { namespace detail {
    /**
     * TDS protocol mesasge type codes
     *
     * Type defines the type of the message.
     *
     * If an unknown Type is specified, the message receiver SHOULD
     * disconnect the connection.
     */
    enum class e_tds_message_type : tdsl::uint8_t
    {
        /**
         * Message types:
         * - SQL batch (Client)
         */
        sql_batch               = 1,
        /**
         * Message types:
         * - Pre-login (Client)
         */
        pre_tds7_login          = 2,
        /**
         * Message types:
         * - RPC (Client)
         */
        rpc                     = 3,
        /**
         * Message types:
         * - FeatureExtAck (Server)
         * - Pre-Login Response (Server)
         * - Federated Authentication Information (Server)
         * - Row Data (Server)
         * - Return Status (Server)
         * - Return Parameters (Server)
         * - Response Completion (Server)
         * - Session State (Server)
         * - Error and Info (Server)
         * - Attention Acknowledgement (Server)
         */
        tabular_result          = 4,
        attention_signal        = 6,
        /**
         * Message types:
         * - Bulk load data (Client)
         */
        bulk_load_data          = 7,
        /**
         * Message types:
         * - Federated Authentication Token (Client)
         */
        federated_auth_token    = 8,
        /**
         * Message types:
         * - Transaction Manager Request (Client)
         */
        transaction_manager_req = 14,
        /**
         * Message types:
         * - Login (Client)
         */
        login                   = 16,
        /**
         * Message types:
         * - Login (Client)
         */
        sspi                    = 17,
        /**
         * Message types:
         * - Pre-login (Client)
         */
        pre_login               = 18
    };
}} // namespace tdsl::detail

#endif