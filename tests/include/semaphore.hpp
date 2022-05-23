/**
 * _________________________________________________
 *
 * @file   semaphore.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

// cppstd
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <chrono>

namespace tdsl { namespace test {
    /**
     * @brief Semaphore class.
     */
    class semaphore {
    private:
        using mtx_t = std::mutex;
        using cvr_t = std::condition_variable;
        using dur_t = std::chrono::milliseconds;

    public:
        semaphore(std::uint32_t count_ = 0) : count(count_) {}

        /**
         * @brief
         *
         */
        void notify() {
            std::unique_lock<mtx_t> lock(mtx);
            count++;
            cv.notify_one();
        }

        void wait() {
            std::unique_lock<mtx_t> lock(mtx);

            while (count == 0) {
                cv.wait(lock);
            }
            count--;
        }

        bool wait_for(const dur_t ms) {
            std::unique_lock<mtx_t> lock(mtx);

            while (count == 0) {
                if (std::cv_status::timeout == cv.wait_for(lock, ms)) {
                    return false;
                }
            }
            count--;
            return true;
        }

        void notify_n(std::uint32_t n) {
            while (n--) {
                notify();
            }
        }

        void wait_n(std::uint32_t n) {
            while (n--) {
                wait();
            }
        }

        bool wait_for_n(const dur_t ms, std::uint32_t n) {
            while (n--) {
                if (not wait_for(ms))
                    return false;
            }
            return true;
        }

    private:
        mutable mtx_t mtx;
        mutable cvr_t cv;
        std::uint32_t count;
    };

}} // namespace tdsl::test