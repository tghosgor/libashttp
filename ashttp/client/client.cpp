/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 26.01.2015
*/

/*
  This file is part of libashttp.

  libashttp is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  libashttp is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with libashttp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "client.hpp"

namespace ashttp {
namespace client {

using namespace std::placeholders;
namespace ssl = boost::asio::ssl;

ClientImpl<Protocol::HTTP>::ClientImpl(std::string host, asio::io_service& is, boost::posix_time::millisec noopTimeout,
                                       Millisec resolveTimeout)
    : ClientCRTPBase<Protocol::HTTP>{host, "http", is, resolveTimeout}
    , m_conn{is, noopTimeout} {
//  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientImpl<Protocol::HTTP>";

  m_conn.onNoopTimeout(std::bind(&ClientImpl<Protocol::HTTP>::onNoopTimeout_, this));
}

ClientImpl<Protocol::HTTP>::~ClientImpl() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ~ClientImpl<Protocol::HTTP>";
}

ClientImpl<Protocol::HTTPS>::ClientImpl(std::string host, asio::io_service& is, boost::posix_time::millisec noopTimeout,
                                        Millisec resolveTimeout)
    : ClientCRTPBase<Protocol::HTTPS>{host, "https", is, resolveTimeout}
    , m_conn{is, noopTimeout} {
//  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientImpl<Protocol::HTTPS>";

  m_conn.onNoopTimeout(std::bind(&ClientImpl<Protocol::HTTPS>::onNoopTimeout_, this));
}

ClientImpl<Protocol::HTTPS>::~ClientImpl() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ~ClientImpl<Protocol::HTTPS>";
}

}
}
