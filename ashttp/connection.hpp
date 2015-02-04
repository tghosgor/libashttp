/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 2.02.2015
*/

#pragma once

#include "type.hpp"
#include "connectionbase.hpp"

#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace ashttp {

template <Protocol protocol>
class ConnectionImpl
    : public ConnectionCRTPBase<ConnectionImpl<protocol>> { };

template <>
class ConnectionImpl<Protocol::HTTP>
    : public ConnectionCRTPBase<ConnectionImpl<Protocol::HTTP>> {
public:
  ConnectionImpl(asio::io_service& is, Millisec noopTimeout);
  ~ConnectionImpl();

  tcp::socket& socket() { return m_socket; }

private:
  tcp::socket m_socket;
};

template <>
class ConnectionImpl<Protocol::HTTPS>
    : public ConnectionCRTPBase<ConnectionImpl<Protocol::HTTPS>> {
  friend class ConnectionCRTPBase<ConnectionImpl<Protocol::HTTPS>>;

public:
  ConnectionImpl(asio::io_service& is, Millisec noopTimeout);
  ~ConnectionImpl();

  /**
   * @brief setHost Sets the hostname to use when verifying the endpoint.
   * @param hostname
   *
   * The connected endpoint will be verified if it matches the given hostname.
   */
  void setHost(const std::string& hostname);

  asio::ssl::stream<tcp::socket>& socket() { return m_socket; }

private:
  // override to handle the https handshake
  void onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt, ConnectCallback callback);

  void onHandshake_(const ErrorCode& ec,
                    const tcp::resolver::iterator& endpointIt,
                    ConnectCallback callback);

private:
  asio::ssl::context m_sslContext;
  asio::ssl::stream<tcp::socket> m_socket;
};

extern template class ConnectionImpl<Protocol::HTTP>;
extern template class ConnectionImpl<Protocol::HTTPS>;

using Connection = ConnectionImpl<Protocol::HTTP>;
using ConnectionHTTPS = ConnectionImpl<Protocol::HTTPS>;

}
