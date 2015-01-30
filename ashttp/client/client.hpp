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

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <functional>

namespace ashttp {
namespace client {

class Client
    : public ClientBase<Client> {
public:
  /**
   * @brief Client
   * @param host A string that holds the host to connect to. It should not contain any protocol (i.e. http://).
   * @param is An io_service object to use.
   * @param noopTimeout Time to stay in a idle state before closing socket.
   * @param resolveTimeout Time to wait bofore timing out while trying to resolve host.
   */
  Client(std::string host, asio::io_service& is, Millisec noopTimeout, Millisec resolveTimeout);
  ~Client();

  /**
   * @brief socket
   * @return The underyling socket.
   */
  tcp::socket& socket() { return m_socket; }

private:
  tcp::socket m_socket;
};

class ClientSSL
    : public ClientBase<ClientSSL> {
  friend class ClientBase<ClientSSL>;
public:
  /**
   * @brief ClientSSL
   * @param host A string that holds the host to connect to. It should not contain any protocol (i.e. http://).
   * @param is An io_service object to use.
   * @param noopTimeout Time to stay in a idle state before closing socket.
   * @param resolveTimeout Time to wait bofore timing out while trying to resolve host.
   */
  ClientSSL(std::string host, asio::io_service& is, Millisec noopTimeout, Millisec resolveTimeout);
  ~ClientSSL();

  /**
   * @brief socket
   * @return The underyling socket.
   */
  asio::ssl::stream<tcp::socket>& socket() { return m_socket; }

private:
  /**
   * This override is needed because SSL requires handshake after socket connection.
   * See ClientBase::onConnect_() for more information.
   */
  void onConnect_(const ErrorCode& ec, tcp::resolver::iterator endpointIt, ConnectCallback callback);

  void onHandshake_(const ErrorCode& ec, tcp::resolver::iterator it, ConnectCallback callback);

private:
  asio::ssl::context m_sslContext;
  asio::ssl::stream<tcp::socket> m_socket;
};

}
}
