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

namespace ashttp {
namespace client {

using namespace std::placeholders;

template <Protocol p>
ClientCRTPBase<p>::ClientCRTPBase(std::string host, std::string service, asio::io_service& is, Millisec resolveTimeout)
    : m_is{is}
    , m_host{std::move(host)}
    , m_resolver{m_is}
    , m_service{std::move(service)}
    , m_resolveTimeout{std::move(resolveTimeout)}
    , m_resolveTimer{m_is} {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>";
}

template <Protocol p>
std::shared_ptr<ClientImpl<p>> ClientCRTPBase<p>::create(std::string host, asio::io_service& is, Millisec noopTimeout,
                                                            Millisec resolveTimeout) {
  return std::shared_ptr<ClientImpl<p>>(new ClientImpl<p>{std::move(host), is, noopTimeout, resolveTimeout});
}

template <Protocol p>
ClientCRTPBase<p>::~ClientCRTPBase() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ~ClientCRTPBase<p>";

  // note that this does not guarantee that all the requests are destructed,
  // it will skip the active requests and let them handle themselves

  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  clearRequestQueue(error::canceled);
}

template <Protocol p>
void ClientCRTPBase<p>::connect(ConnectCallback callback) {
  auto onResolve = [ self = static_cast<ClientImpl<p>*>(this)->shared_from_this(), callback = std::move(callback) ](
      const ErrorCode& ec, const tcp::resolver::iterator& it) {
    TEMPLOG_DEVLOG(templog::sev_debug) << self.get() << " ClientCRTPBase<p>::connect onResolve ec: " << ec;

    if (!ec) {
      auto& connection = self->connection();

      connection.connect(it, std::bind(&ClientImpl<p>::onConnect_, self.get(), _1, std::move(callback)));
    } else {
      self->connectCompleted(ec);
    }
  };

  resolve(onResolve);
}

template <Protocol p>
std::shared_ptr<Request<p>> ClientCRTPBase<p>::get(std::string resource) {
  auto request = std::make_shared<Request<p>>(static_cast<ClientImpl<p>*>(this)->shared_from_this(), m_host, std::move(resource));

  return std::move(request);
}

template <Protocol p>
void ClientCRTPBase<p>::schedule(std::weak_ptr<Request<p>> request) {
  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::schedule";

  m_requestQueue.push_back(std::move(request));

  if (m_requestActive == false) {
    m_requestActive = true;

    auto onConnect = [this](const ErrorCode& ec) {
      TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::schedule onConnect";

      std::lock_guard<std::mutex> l{m_requestQueueMtx};

      if (!ec) {
        if (const auto request = m_requestQueue.front().lock())
          request->start();
        else
          m_requestActive = false;
      }
      else
        m_requestActive = false;
    };

    connect(std::move(onConnect));
  }
}

template <Protocol p>
ClientImpl<p>& ClientCRTPBase<p>::onConnect(ConnectCallback callback) {
  m_connectCallback = std::move(callback);

  return *static_cast<ClientImpl<p>*>(this);
}

template <Protocol p>
std::size_t ClientCRTPBase<p>::requestCount() const {
  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  return m_requestQueue.size() + m_requestActive;
}

template <Protocol p>
void ClientCRTPBase<p>::resolve(ResolveCallback callback) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::resolve";

  if (m_endpointIterator == tcp::resolver::iterator{}) { // if endpoint is not yet resolved
    TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::resolve endpoint not resolved";

    m_resolveTimer.expires_from_now(m_resolveTimeout);
    m_resolveTimer.async_wait(std::bind(&ClientCRTPBase<p>::onResolveTimeout_, this, _1));

    tcp::resolver::query query{m_host, m_service};

    m_resolver.async_resolve(query, std::bind(&ClientCRTPBase<p>::onResolve_, this, _1, _2, std::move(callback)));
  } else {
    TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::resolve endpoint already resolved";

    callback(error::success, m_endpointIterator);
  }
}

template <Protocol p>
void ClientCRTPBase<p>::onResolve_(const ErrorCode& ec, tcp::resolver::iterator endpointIt,
                               ResolveCallback callback) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::onResolve_ ec: " << ec;

  m_resolveTimer.cancel();

  if (!ec)
    m_endpointIterator = std::move(endpointIt);

  callback(ec, m_endpointIterator);
}

template <Protocol p>
void ClientCRTPBase<p>::onConnect_(const ErrorCode& ec, ConnectCallback callback) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::onConnect_ " << ec;

  connectCompleted(ec);

  callback(ec);
}

template <Protocol p>
void ClientCRTPBase<p>::onResolveTimeout_(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::onResolveTimeout_ " << ec;

  if (!ec)
    m_resolver.cancel();
}

template <Protocol p>
void ClientCRTPBase<p>::onNoopTimeout_() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientBase<C>::onNoopTimeout_ ";

  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  clearRequestQueue(error::timeout);
}

template <Protocol p>
void ClientCRTPBase<p>::connectCompleted(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::connectCompleted " << ec;

  if (m_connectCallback) {
    m_connectCallback(ec);
  }

  if (ec) {
    std::lock_guard<std::mutex> l{m_requestQueueMtx};

    clearRequestQueue(ec);
  }
}

template <Protocol p>
void ClientCRTPBase<p>::requestCompleted(const ErrorCode& ec) {
  std::lock_guard<std::mutex> l{m_requestQueueMtx};

  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ClientCRTPBase<p>::requestCompleted ec: " << ec << ", requestCount: " << m_requestQueue.size();
  std::cout.flush();

  assert(m_requestQueue.size() > 0);

  // pop the processed request
  m_requestQueue.pop_front();
  m_requestActive = false;

  if (!ec) {
TRY_NEXT_REQUEST:
    if (m_requestQueue.size() > 0) {

      if (const auto request = m_requestQueue.front().lock()) {
        m_requestActive = true;

        request->start();
      } else {// the request no longer exists
        m_requestQueue.pop_front();

        goto TRY_NEXT_REQUEST;
      }
    }
  } else {// if error happened
    // TODO: is this behavior correct? maybe propogate the same error?
    clearRequestQueue(error::canceled);
  }
}

template <Protocol p>
void ClientCRTPBase<p>::clearRequestQueue(const ErrorCode& ec) {
  // leave the active request in queue
  const std::size_t requestsToLeave = m_requestActive;

  while (m_requestQueue.size() > requestsToLeave) {
    auto requestIt = m_requestQueue.begin();

    // skip the active request
    std::advance(requestIt, requestsToLeave);

    if (const auto request = requestIt->lock()) {
      request->finish(ec);
    }

    m_requestQueue.erase(requestIt);
  }
}

template class ClientCRTPBase<Protocol::HTTP>;
template class ClientCRTPBase<Protocol::HTTPS>;

}
}
