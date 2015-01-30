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

Client::Client(std::string host, asio::io_service& is, boost::posix_time::millisec noopTimeout, Millisec resolveTimeout)
    : ClientBase<Client>{host, "http", is, noopTimeout, resolveTimeout}
    , m_socket{is} {

}

Client::~Client() {

}

ClientSSL::ClientSSL(std::string host, asio::io_service& is, boost::posix_time::millisec noopTimeout, Millisec resolveTimeout)
  : ClientBase<ClientSSL>{host, "https", is, noopTimeout, resolveTimeout}
  , m_sslContext{boost::asio::ssl::context::sslv23}
  , m_socket{is, m_sslContext} {

}

ClientSSL::~ClientSSL() {

}

void ClientSSL::onConnect_(const ErrorCode& ec, tcp::resolver::iterator endpointIt, ConnectCallback callback) {
  const auto onBaseConnect = [this, callback = std::move(callback)](const ErrorCode& ec, tcp::resolver::iterator it) {
    if (!ec) {
      socket().async_handshake(boost::asio::ssl::stream_base::client,
                               std::bind(&ClientSSL::onHandshake_, this, _1, std::move(it), std::move(callback)));
    } else {
      callback(ec, it);
    }
  };

  ClientBase<ClientSSL>::onConnect_(ec, endpointIt, onBaseConnect);
}

void ClientSSL::onHandshake_(const ErrorCode& ec, tcp::resolver::iterator it, ConnectCallback callback) {
  callback(ec, std::move(it));
}

}
}
