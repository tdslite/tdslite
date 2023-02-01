/**
 * ____________________________________________________
 * noncopyable base class implementation
 *
 * @file   tdsl_noncopyable.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   17.08.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_NONCOPYABLE_HPP
#define TDSL_UTIL_NONCOPYABLE_HPP

namespace tdsl { namespace util {

    /**
     * A base class to make derived class non-copyable.
     *
     * Inherit from this class to in order to make derived class noncopyable.
     * By non-copyable, it means the following operations are explicitly removed from the derived;
     *
     * Copy-construction from mutable lvalue reference
     * Copy-construction from immutable lvalue reference
     * Copy-assignment from mutable lvalue reference
     * Copy-assignment from immutable lvalue reference
     */
    class noncopyable {
    private:
        /**
         * @brief Deleted copy constructor (mutable)
         */
        noncopyable(noncopyable &)                   = delete;

        // --------------------------------------------------------------------------------

        /**
         * @brief Deleted copy constructor (immutable)
         */
        noncopyable(const noncopyable &)             = delete;

        // --------------------------------------------------------------------------------

        /**
         * @brief Deleted copy assingment operator (mutable)
         */
        noncopyable & operator=(noncopyable &)       = delete;

        // --------------------------------------------------------------------------------

        /**
         * @brief Deleted copy assignment operator (immutable)
         */
        noncopyable & operator=(const noncopyable &) = delete;

    public:
        /**
         * @brief Default constructor
         */
        noncopyable()                           = default;

        // --------------------------------------------------------------------------------

        /**
         * @brief Default destructor
         */
        ~noncopyable()                          = default;

        // --------------------------------------------------------------------------------

        /**
         * @brief Default move constructor
         */
        noncopyable(noncopyable &&)             = default;

        // --------------------------------------------------------------------------------

        /**
         * @brief Default move assignment operator
         */
        noncopyable & operator=(noncopyable &&) = default;
    }; // class noncopyable
}}     // namespace tdsl::util

#endif