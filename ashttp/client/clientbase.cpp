/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 28.01.2015
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

#include "clientbase.hpp"

#include "client.hpp"
#include "request.hpp"

#include <boost/log/trivial.hpp>

namespace ashttp {
namespace client {

using namespace std::placeholders;

template <class C>
ClientBase<C>::ClientBase(std::string host, std::string service, asio::io_service& is, Millisec noopTimeout,
                          Millisec resolveTimeout)
    : m_is{is}
    , m_host{std::move(host)}
    , m_resolver{m_is}
    , m_service{std::move(service)}
    , m_resolveTimeout{std::move(resolveTimeout)}
    , m_resolveTimer{m_is}
    , m_noopTimeout{std::move(noopTimeout)}
    , m_noopTimer{m_is} {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>";
}

template <class C>
std::shared_ptr<C> ClientBase<C>::create(std::string host, asio::io_service& is, Millisec noopTimeout,
                                         Millisec resolveTimeout) {
  return std::shared_ptr<C>(new C{std::move(host), is, noopTimeout, resolveTimeout});
}

template <class C>
ClientBase<C>::~ClientBase() {
  BOOST_LOG_TRIVIAL(trace) << this << " ~ClientBase<C>";

  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  clearRequests();
}

template <class C>
std::unique_ptr<Request<C> > ClientBase<C>::get(std::string resource) {
  auto request = std::make_unique<Request<C>>(*static_cast<C*>(this), std::move(resource));

  return std::move(request);
}

template <class C>
void ClientBase<C>::schedule(std::unique_ptr<Request<C>>& request) {
  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::schedule";

  m_requestQueue.push_back(std::move(request));

  if (m_requestActive == false) {
    m_requestActive = true;

    m_requestQueue.front()->start();
  }
}

template <class C>
C& ClientBase<C>::onConnect(ConnectCallback callback) {
  m_connectCallback = std::move(callback);

  return *static_cast<C*>(this);
}

template <class C>
std::size_t ClientBase<C>::requestCount() const {
  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  return m_requestQueue.size() + m_requestActive;
}

template <class C>
void ClientBase<C>::connect(ConnectCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::connect";

  auto& socket = static_cast<C*>(this)->socket().lowest_layer();

  if (!socket.is_open()) {
    auto onResolved = [ this, callback = std::move(callback) ](const ErrorCode& ec,
                                                               const tcp::resolver::iterator& endpointIt) {
      BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::connect onResolved " << ec;

      if (!ec) {
        async_connect(static_cast<C*>(this)->socket().lowest_layer(), endpointIt,
                      std::bind(&C::onConnect_, static_cast<C*>(this), _1, _2, std::move(callback)));
      } else {
        connectCompleted(ec, endpointIt);
      }
    };

    resolve(onResolved);
  } else {
    assert(m_endpointIterator != tcp::resolver::iterator{});

    callback(error::success, m_endpointIterator);
  }
}

template <class C>
void ClientBase<C>::resolve(ResolveCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::resolve";

  if (m_endpointIterator == tcp::resolver::iterator{}) { // if endpoint is not yet resolved
    BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::resolve endpoint not resolved";

    tcp::resolver::query query{m_host, m_service};

    m_resolver.async_resolve(query, std::bind(&ClientBase<C>::onResolve_, this, _1, _2, std::move(callback)));

    m_resolveTimer.expires_from_now(m_resolveTimeout);
    m_resolveTimer.async_wait(std::bind(&ClientBase<C>::onResolveTimeout_, this, _1));
  } else {
    BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::resolve endpoint already resolved";

    callback(error::success, m_endpointIterator);
  }
}

template <class C>
void ClientBase<C>::resetNoopTimeout() {
  m_noopTimer.expires_from_now(m_noopTimeout);
  m_noopTimer.async_wait(std::bind(&ClientBase<C>::onNoopTimeout_, this, _1));
}

template <class C>
void ClientBase<C>::onResolve_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt,
                               ResolveCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::onResolve_ ec: " << ec;

  m_resolveTimer.cancel();

  if (!ec)
    m_endpointIterator = endpointIt;

  callback(ec, m_endpointIterator);
}

template <class C>
void ClientBase<C>::onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt,
                               ConnectCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::onConnect_ " << ec;

  m_noopTimer.expires_from_now(m_noopTimeout);
  m_noopTimer.async_wait(std::bind(&ClientBase<C>::onNoopTimeout_, this, _1));

  connectCompleted(ec, endpointIt);

  callback(ec, endpointIt);
}

template <class C>
void ClientBase<C>::onNoopTimeout_(const ErrorCode& ec) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::onResolveTimeout_ " << ec;

  if (!ec)
    static_cast<C*>(this)->socket().lowest_layer().close();
}

template <class C>
void ClientBase<C>::onResolveTimeout_(const ErrorCode& ec) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::onResolveTimeout_ " << ec;

  if (!ec)
    m_resolver.cancel();
}

template <class C>
void ClientBase<C>::connectCompleted(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt) {
  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::connectCompleted " << ec;

  if (m_connectCallback) {
    m_connectCallback(ec, endpointIt);
  }
}

template <class C>
void ClientBase<C>::requestCompleted(const ErrorCode& ec) {
  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  BOOST_LOG_TRIVIAL(trace) << this << " ClientBase<C>::requestCompleted ec: " << ec << ", requestCount: " << m_requestQueue.size();

  assert(m_requestQueue.size() > 0);

  // pop the processed request
  m_requestQueue.pop_front();

  if (!ec) {
    if (m_requestQueue.size() > 0) {
      m_requestQueue.front()->start();
    } else {
      m_requestActive = false;
    }
  } else {
    static_cast<C*>(this)->socket().lowest_layer().close();

    // clear all the requests
    clearRequests();
  }
}

template <class C>
void ClientBase<C>::clearRequests() {
  while (m_requestQueue.size() > 0) {
    // notify the request completion handlers
    m_requestQueue.front()->finish(error::canceled);

    // pop the request item
    m_requestQueue.pop_front();
  }
}

}
}

template class ashttp::client::ClientBase<ashttp::client::Client>;
template class ashttp::client::ClientBase<ashttp::client::ClientSSL>;
