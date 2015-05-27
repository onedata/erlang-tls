/**
 * @file tlsApplication.cpp
 * @author Konrad Zemek
 * @copyright (C) 2015 ACK CYFRONET AGH
 * @copyright This software is released under the MIT license cited in
 * 'LICENSE.txt'
 */

#include "tlsApplication.hpp"

#include <algorithm>

namespace one {
namespace etls {

TLSApplication::TLSApplication()
{
    const auto threadsNo = std::thread::hardware_concurrency();
    m_threads.reserve(threadsNo);
    std::generate_n(std::back_inserter(m_threads), threadsNo, [this] {
        return std::thread{[this] {
            try {
                m_ioService.run();
            }
            catch (...) {
            }
        }};
    });
}

TLSApplication::~TLSApplication()
{
    m_ioService.stop();
    for (auto &thread : m_threads)
        thread.join();
}

boost::asio::io_service &TLSApplication::ioService() { return m_ioService; }

} // namespace etls
} // namespace one
