/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 26.01.2015
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
