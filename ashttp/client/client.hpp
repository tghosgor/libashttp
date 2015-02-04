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

#pragma once

#include "clientbase.hpp"
#include "../connection.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <functional>

namespace ashttp {
namespace client {

template <Protocol p>
class ClientImpl { };

template <>
class ClientImpl<Protocol::HTTP>
    : public ClientCRTPBase<Protocol::HTTP> {
public:
  /**
   * @brief Client
   * @param host A string that holds the host to connect to. It should not contain any protocol (i.e. http://).
   * @param is An io_service object to use.
   * @param noopTimeout Time to stay in a idle state before closing socket.
   * @param resolveTimeout Time to wait bofore timing out while trying to resolve host.
   */
  ClientImpl(std::string host, asio::io_service& is, Millisec noopTimeout, Millisec resolveTimeout);
  ~ClientImpl();

  Connection& connection() { return m_conn; }

private:
  Connection m_conn;
};

template <>
class ClientImpl<Protocol::HTTPS>
    : public ClientCRTPBase<Protocol::HTTPS> {
  friend class ClientCRTPBase<Protocol::HTTPS>;
public:
  /**
   * @brief ClientSSL
   * @param host A string that holds the host to connect to. It should not contain any protocol (i.e. http://).
   * @param is An io_service object to use.
   * @param noopTimeout Time to stay in a idle state before closing socket.
   * @param resolveTimeout Time to wait bofore timing out while trying to resolve host.
   */
  ClientImpl(std::string host, asio::io_service& is, Millisec noopTimeout, Millisec resolveTimeout);
  ~ClientImpl();

  ConnectionHTTPS& connection() { return m_conn; }

private:
  ConnectionHTTPS m_conn;
};

template<Protocol p>
using Client = ClientImpl<p>;

using ClientHTTP = Client<Protocol::HTTP>;
using ClientHTTPS = Client<Protocol::HTTPS>;

}
}
