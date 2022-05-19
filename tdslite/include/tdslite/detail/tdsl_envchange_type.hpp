/**
 * _________________________________________________
 *
 * @file   tdsl_envchange_type.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl { namespace detail {

    enum class e_tds_envchange_type : tdsl::uint8_t
    {
        database                          = 1,
        language                          = 2,
        charset                           = 3,
        packet_size                       = 4,
        unicode_datasort_localid          = 5,
        unicode_datasort_comparison_flags = 6,
        sql_collation                     = 7,
        begin_transaction                 = 8,
        commit_transaction                = 9,
        rollback_transaction              = 10,
        enlist_dtc_transaction            = 11,
        defect_transaction                = 12,
        database_mirroring_partner        = 13,
        promote_transaction               = 15,
        transaction_manager_address       = 16,
        transaction_ended                 = 17,
        reset_completion_ack              = 18,
        login_user_instance_info          = 19,
        routing                           = 20
    };

}} // namespace tdsl::detail
