/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 2.02.2015
*/

#include "connectionbase.hpp"

#include "connection.hpp"

namespace ashttp {

using namespace std::placeholders;

template <class C>
ConnectionCRTPBase<C>::ConnectionCRTPBase(asio::io_service& is, Millisec noopTimeout)
    : m_noopTimeout{noopTimeout}
    , m_noopTimer{is} {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ConnectionCRTPBase<C>";
}

template <class C>
ConnectionCRTPBase<C>::~ConnectionCRTPBase() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ~ConnectionCRTPBase<C>";

  std::cout.flush();
}

template <class C>
void ConnectionCRTPBase<C>::connect(tcp::resolver::iterator iterator, ConnectCallback callback) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ConnectionCRTPBase<C>::connect";

  auto& socket = static_cast<C*>(this)->socket().lowest_layer();

  if (!socket.is_open()) {
    async_connect(static_cast<C*>(this)->socket().lowest_layer(), iterator,
                  std::bind(&C::onConnect_, static_cast<C*>(this), _1, _2, std::move(callback)));
  } else {
    callback(error::success);
  }
}

template <class C>
void ConnectionCRTPBase<C>::onNoopTimeout(TimeoutCallback callback) {
  m_noopCallback = std::move(callback);
}

template <class C>
void ConnectionCRTPBase<C>::startNoopTimer() {
  m_noopTimer.expires_from_now(m_noopTimeout);
  m_noopTimer.async_wait(std::bind(&ConnectionCRTPBase<C>::onNoopTimeout_, this, _1));
}

template <class C>
bool ConnectionCRTPBase<C>::stopNoopTimer() {
  const auto cancelledOps = m_noopTimer.cancel();

  // there can only be at most 1 waiters for now
  assert(cancelledOps <= 1);

  return cancelledOps > 0;
}

template <class C>
void ConnectionCRTPBase<C>::connectCompleted(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ConnectionCRTPBase<C>::connectCompleted " << ec;

  if (!ec) {
    startNoopTimer();
  }
}

template <class C>
void ConnectionCRTPBase<C>::onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt,
                                       ConnectCallback callback) {
  connectCompleted(ec, endpointIt);

  callback(ec);
}

template <class C>
void ConnectionCRTPBase<C>::onNoopTimeout_(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ConnectionCRTPBase<C>::onNoopTimeout_ " << ec;

  if (!ec) {
    if (m_noopCallback)
      m_noopCallback();

    static_cast<C*>(this)->socket().lowest_layer().shutdown(tcp::socket::shutdown_both);
    static_cast<C*>(this)->socket().lowest_layer().close();
  }
}

template class ConnectionCRTPBase<ConnectionImpl<Protocol::HTTP>>;
template class ConnectionCRTPBase<ConnectionImpl<Protocol::HTTPS>>;

}
