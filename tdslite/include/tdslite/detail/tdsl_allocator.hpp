/**
 * ____________________________________________________
 *
 * @file   tdsl_allocator.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   29.09.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_ALLOCATOR_HPP
#define TDSL_DETAIL_TDSL_ALLOCATOR_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/detail/tdsl_default_allocator.hpp>

#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {
    struct placement_new_tag {};
} // namespace tdsl

/**
 * tdslite placement new
 *
 * Some platforms do not have <new> header in their standard libraries,
 * hence, we're providing a placement new overload of our own.
 * We're `overloading` the placement new because the standard prohibits
 * overriding the original placement new functions.
 *
 * @param [in] p Object
 * @return p
 */
inline void * operator new(tdsl::size_t, void * p, tdsl::placement_new_tag) noexcept {
    return p;
}

namespace tdsl {

    using malloc_fn_type = void * (*) (unsigned long);
    using free_fn_type   = void (*)(void * ptr);

    struct mfctx {
        malloc_fn_type a = tdsl::tdsl_default_malloc;
        free_fn_type f   = tdsl::tdsl_default_free;
    };

    // --------------------------------------------------------------------------------

    /**
     * Set-get malloc & free functions for tdslite
     *
     * Function is a setter when @p mfn and @p ffn are non-null.
     *
     * @param mfn Malloc function (default nullptr)
     * @param ffn Free function (default nullptr)
     *
     * @return mfctx Object containing current malloc-free functions
     */
    inline auto tdslite_malloc_free(malloc_fn_type mfn = nullptr,
                                    free_fn_type ffn   = nullptr) noexcept -> const mfctx & {
        TDSL_ASSERT_MSG(not(!(mfn) != !(ffn)),
                        "malloc and free functions can either be both null or non-null.");

        // Since we're a header-only library we cannot use a global
        // static variable or static inline since we're targeting
        // C++11. We use a static variable inside non-static,
        // inline function to leverage the following fact from the
        // C++ standard: 7.1.2/4 - C++98/C++14 (n3797):
        //
        // "A static local variable in an extern inline function
        // always refers to the same object."

        static mfctx mf = {};
        if (mfn && ffn) {
            mf.a = mfn;
            mf.f = ffn;
        }
        return mf;
    }

    // --------------------------------------------------------------------------------

    inline TDSL_NODISCARD auto tdslite_malloc(unsigned long n_bytes) noexcept -> void * {
        return tdslite_malloc_free().a(n_bytes);
    }

    // --------------------------------------------------------------------------------

    inline auto tdslite_free(void * p, unsigned long n_bytes) noexcept -> void {
        (void) n_bytes;
        tdslite_malloc_free().f(p);
    }

    // TODO: create, destroy, create_n should be noexcept if
    // type T's corresponding constructor & destructor is noexcept
    template <typename T>
    struct tds_allocator {
        // --------------------------------------------------------------------------------

        static TDSL_NODISCARD auto allocate(tdsl::uint32_t n_elems) noexcept -> T * {
            void * mem = tdslite_malloc(n_elems * sizeof(T));
            if (nullptr == mem) {
                return nullptr;
            }

            return static_cast<T *>(mem);
        }

        // --------------------------------------------------------------------------------

        static inline auto deallocate(T * p, tdsl::uint32_t n_elems) noexcept -> void {
            // call destructor if class type
            tdslite_free(p, sizeof(T) * n_elems);
        }

        // --------------------------------------------------------------------------------

        template <typename... Args>
        static TDSL_NODISCARD auto create(Args &&... args) -> T * {
            void * mem = tdslite_malloc(sizeof(T));
            if (nullptr == mem) {
                return nullptr;
            }

            // Invoke placement new for each element
            T * storage = static_cast<T *>(mem);

            construct(storage, 1, TDSL_FORWARD(args)...);

            return storage;
        }

        // --------------------------------------------------------------------------------

        static auto destroy(T * p) -> void {
            destruct(p, 1);
            deallocate(p, /*n_elems=*/1);
        }

        // --------------------------------------------------------------------------------

        template <typename... Args>
        static TDSL_NODISCARD auto create_n(tdsl::uint32_t n_elems, Args &&... args) -> T * {
            void * mem = tdslite_malloc(sizeof(T) * n_elems);
            if (nullptr == mem) {
                return nullptr;
            }

            T * storage = static_cast<T *>(mem);

            // Invoke placement new for each element
            construct(storage, n_elems, TDSL_FORWARD(args)...);

            return storage;
        }

        // --------------------------------------------------------------------------------

        static auto destroy_n(T * p, tdsl::uint32_t n_elems) -> void {
            destruct(p, n_elems);
            deallocate(p, /*n_elems=*/n_elems);
        }

    private:
        // --------------------------------------------------------------------------------

        template <typename Q = T, traits::enable_when::class_type<Q> = true, typename... Args>
        static void construct(Q * storage, tdsl::uint32_t n_elems, Args &&... args) {
            for (tdsl::uint32_t i = 0; i < n_elems; i++) {
                // placement new
                new (storage + i, placement_new_tag{}) Q(TDSL_FORWARD(args)...);
            }
        }

        // --------------------------------------------------------------------------------

        template <typename Q = T, traits::enable_when::class_type<Q> = true>
        static void destruct(Q * storage, tdsl::uint32_t n_elems) {

            for (tdsl::uint32_t i = 0; i < n_elems; i++) {
                storage [i].~Q();
            }
        }

        // --------------------------------------------------------------------------------

        // noop
        template <typename Q, traits::enable_when::non_class_type<Q> = true>
        static void construct(Q *, tdsl::uint32_t) {}

        // --------------------------------------------------------------------------------

        // noop
        template <typename Q, traits::enable_when::non_class_type<Q> = true>
        static void destruct(Q *, tdsl::uint32_t) {}
    };
} // namespace tdsl

#endif