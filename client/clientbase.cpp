/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 28.01.2015
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
    , m_noopTimer{m_is}
    , m_requestQueue{0}
    , m_requestCount{0}
    , m_activeRequest{nullptr} {}

template <class C>
std::shared_ptr<C> ClientBase<C>::create(std::string host, asio::io_service& is, Millisec noopTimeout,
                                         Millisec resolveTimeout) {
  return std::shared_ptr<C>(new C{std::move(host), is, noopTimeout, resolveTimeout});
}

template <class C>
ClientBase<C>::~ClientBase() {
  delete m_activeRequest;

  while (m_requestQueue.pop(m_activeRequest))
    delete m_activeRequest;
}

template <class C>
std::unique_ptr<Request<C>> ClientBase<C>::get(std::string resource) {
  auto request = std::make_unique<Request<C>>(*static_cast<C*>(this), std::move(resource));

  return std::move(request);
}

template <class C>
void ClientBase<C>::schedule(std::unique_ptr<Request<C>>& request) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::schedule";

  const auto requestPtr = request.release();

  if (!m_requestCount++) {
    m_activeRequest = requestPtr;

    m_activeRequest->start();
  } else {
    m_requestQueue.push(requestPtr);
  }
}

template <class C>
C& ClientBase<C>::onConnect(ConnectCallback callback) {
  m_connectCallback = std::move(callback);

  return *static_cast<C*>(this);
}

template <class C>
C& ClientBase<C>::onRequestCompleted(RequestCompletedCallback callback) {
  m_requestCompletedCallback = std::move(callback);

  return *static_cast<C*>(this);
}

template <class C>
void ClientBase<C>::restartProcessing() {
  if (m_activeRequest != nullptr) {
    m_activeRequest->start();
  } else {
    if (m_requestQueue.pop(m_activeRequest)) { // if there are queued request
      m_activeRequest->start();
    }
  }
}

template <class C>
void ClientBase<C>::connect(ConnectCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::connect";

  auto onResolved = [ this, callback = std::move(callback) ](const ErrorCode& ec,
                                                             const tcp::resolver::iterator& endpointIt) {
    BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::connect onResolved " << ec;

    if (!ec) {
      async_connect(static_cast<C*>(this)->socket().lowest_layer(), endpointIt,
                    std::bind(&C::onConnect_, static_cast<C*>(this), _1, _2, std::move(callback)));
    } else {
      connectCompleted(ec, endpointIt);
    }
  };

  resolve(onResolved);
}

template <class C>
void ClientBase<C>::resolve(ResolveCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::resolve";

  if (m_endpointIterator == tcp::resolver::iterator{}) { // if endpoint is not yet resolved
    BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::resolve endpoint not resolved";

    tcp::resolver::query query{m_host, m_service};

    m_resolver.async_resolve(query, std::bind(&ClientBase<C>::onResolve_, this, _1, _2, std::move(callback)));

    m_resolveTimer.expires_from_now(m_resolveTimeout);
    m_resolveTimer.async_wait(std::bind(&ClientBase<C>::onResolveTimeout_, this, _1));
  } else {
    BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::resolve endpoint already resolved";

    callback(error::success, m_endpointIterator);
  }
}

template <class C>
void ClientBase<C>::resetNoopTimeout() {
  m_noopTimer.expires_from_now(m_noopTimeout);
  m_noopTimer.async_wait(std::bind(&ClientBase<C>::onNoopTimeout_, this, _1));
}

template <class C>
void ClientBase<C>::reset() {
  m_connectCallback = nullptr;
  m_requestCompletedCallback = nullptr;
}

template <class C>
void ClientBase<C>::onResolve_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt,
                               ResolveCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::onResolve_ ec: " << ec;

  m_resolveTimer.cancel();

  if (!ec)
    m_endpointIterator = endpointIt;

  callback(ec, m_endpointIterator);
}

template <class C>
void ClientBase<C>::onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt,
                               ConnectCallback callback) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::onConnect_ " << ec;

  m_noopTimer.expires_from_now(m_noopTimeout);
  m_noopTimer.async_wait(std::bind(&ClientBase<C>::onNoopTimeout_, this, _1));

  connectCompleted(ec, endpointIt);

  callback(ec, endpointIt);
}

template <class C>
void ClientBase<C>::onNoopTimeout_(const ErrorCode& ec) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::onResolveTimeout_ " << ec;

  if (!ec)
    static_cast<C*>(this)->socket().lowest_layer().close();
}

template <class C>
void ClientBase<C>::onResolveTimeout_(const ErrorCode& ec) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::onResolveTimeout_ " << ec;

  if (!ec)
    m_resolver.cancel();
}

template <class C>
void ClientBase<C>::connectCompleted(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::connectCompleted " << ec;

  if (m_connectCallback) {
    m_connectCallback(ec, endpointIt);
  }
}

template <class C>
void ClientBase<C>::requestCompleted(Request<C>& request) {
  BOOST_LOG_TRIVIAL(trace) << "ClientBase<C>::requestCompleted requestCount: " << m_requestCount;

  assert(m_activeRequest == &request);

  if (m_requestCompletedCallback)
    m_requestCompletedCallback(request);

  delete &request;

  m_activeRequest = nullptr;

  if (--m_requestCount) {
    const auto popped = m_requestQueue.pop(m_activeRequest);

    assert(popped);

    m_activeRequest->start();
  }
}
}
}

template class ashttp::client::ClientBase<ashttp::client::Client>;
template class ashttp::client::ClientBase<ashttp::client::ClientSSL>;
