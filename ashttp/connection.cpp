/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 2.02.2015
*/

#include "connection.hpp"

namespace ashttp {

using namespace std::placeholders;

ConnectionImpl<Protocol::HTTP>::ConnectionImpl(asio::io_service& is, Millisec noopTimeout)
    : ConnectionCRTPBase<ConnectionImpl<Protocol::HTTP>>{is, std::move(noopTimeout)}
    , m_socket{is} {

}

ConnectionImpl<Protocol::HTTPS>::ConnectionImpl(asio::io_service& is, Millisec noopTimeout)
    : ConnectionCRTPBase<ConnectionImpl<Protocol::HTTPS>>{is, std::move(noopTimeout)}
    , m_sslContext{asio::ssl::context::tlsv12_client}
    , m_socket{is, m_sslContext} {
  m_sslContext.set_default_verify_paths();

  m_socket.set_verify_mode(asio::ssl::verify_peer);
}

ConnectionImpl<Protocol::HTTP>::~ConnectionImpl() {

}

ConnectionImpl<Protocol::HTTPS>::~ConnectionImpl() {

}

void ConnectionImpl<Protocol::HTTPS>::setHost(const std::string& hostname) {
  m_socket.set_verify_callback(asio::ssl::rfc2818_verification{hostname});
}

void ConnectionImpl<Protocol::HTTPS>::onConnect_(const ErrorCode& ec,
                                                 const tcp::resolver::iterator& endpointIt,
                                                 ConnectCallback callback) {
  if (!ec)
    m_socket.async_handshake(
        asio::ssl::stream<tcp::socket>::client,
        std::bind(&ConnectionImpl<Protocol::HTTPS>::onHandshake_, this, _1, endpointIt, std::move(callback)));
  else
    callback(ec);
}

void ConnectionImpl<Protocol::HTTPS>::onHandshake_(const ErrorCode& ec,
                                                   const tcp::resolver::iterator& endpointIt,
                                                   ConnectCallback callback) {
  ConnectionCRTPBase<ConnectionImpl<Protocol::HTTPS>>::onConnect_(ec, endpointIt, std::move(callback));
}

template class ConnectionImpl<Protocol::HTTP>;
template class ConnectionImpl<Protocol::HTTPS>;

}
