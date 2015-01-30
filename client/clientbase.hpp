/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 28.01.2015
*/

#pragma once

#include "../type.hpp"

#include <boost/asio.hpp>
#include <boost/lockfree/queue.hpp>

#include <atomic>
#include <functional>
#include <deque>

namespace ashttp {
namespace client {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class Client;
class ClientSSL;
template <class C>
class Request;

template <class C>
class ClientBase {
  friend class Request<C>;
public:
  using ResolveCallback = std::function<void (const ErrorCode&, const tcp::resolver::iterator&)>;
  using ConnectCallback = ResolveCallback;
  using RequestCompletedCallback = std::function<void (Request<C>&)>;

public:
  /**
    * @brief create Creates a new client object.
    * @param host The host to connect to.
    * @param is The io_service to use.
    * @param noopTimeout Time in milliseconds to stay inactive before closing the socket.
    * @param resolveTimeout Time in milliseconds to try to resolve the given hostname before timing out.
    * @return A shared_ptr object containing the new client.
    */
  static std::shared_ptr<C> create(std::string host, asio::io_service& is,
                                   Millisec noopTimeout = Millisec{30000},
                                   Millisec resolveTimeout = Millisec{5000});

  ~ClientBase();


  /**
   * @brief host
   * @return The hostname this client handles.
   */
  const std::string& host() const { return m_host; }


  /**
   * @brief requestCount Gets the number of requests being processed.
   * @return Number of requests being processed.
   */
  std::size_t requestCount() const { return m_requestCount; }


  /**
   * @brief get Creates a Response object to retrieve the resource.
   * @param resource
   * @return
   *
   * This method is usually the only thing you need. Response object automatically manages the set-up this
   * Client will need.
   */
  std::unique_ptr<Request<C>> get(std::string resource);


  /**
   * @brief schedule Schedules a request object for processing.
   * @param request Request to schedule.
   *
   * Takes the ownership of the item inside the \p request object.
   */
  void schedule(std::unique_ptr<Request<C>>& request);


  /**
   * @brief connect Registers a callback to call after trying to connect.
   * @param callback
   * @return Self.
   *
   * The callback registered with this method will live until the end of lifetime of this object.
   *This means that if you bind the this client's shared_ptr object to this callback, the object will live forever.
   */
  C& onConnect(ConnectCallback callback);

  /**
   * @brief onRequestCompleted Registers a callback to call on completion of each request (successful or not).
   * @param callback The callback.
   * @return Self.
   *
   * The callback registered with this method will live until the end of lifetime of this object.
   *This means that if you bind the this client's shared_ptr object to this callback, the object will live forever.
   */
  C& onRequestCompleted(RequestCompletedCallback callback);


  /**
   * @brief restartProcessing Tries to start processing the requests.
   *
   * This method is meant to be used to try to restart the request processing in case it is stopped in some way (i.e. connection failure).
   */
  void restartProcessing();


  /**
   * @brief connect Tries to connect to the host.
   * @param callback A callback to call on connect.
   *
   * If any callback is registered with ClientBase::onConnect(), it will also be called.
   */
  void connect(ConnectCallback callback);


  /**
   * @brief resolve Asynchronously resolves the given host.
   * @param callback Callback will be called after the resolving is done.
   *
   * This method caches the result and directly calls the callback if already resolved.
   * Also, this method will be automatically called by other methods if the other method requires resolving.
   */
  void resolve(ResolveCallback callback);


  /**
   * @brief resetNoopTimeout Restarts the no-op timeout with the noopTimeout value that was set in constructor.
   */
  void resetNoopTimeout();


  /**
   * @brief reset Resets the registered handlers.
   */
  void reset();

protected:
  ClientBase(std::string host, std::string service, asio::io_service& is, Millisec noopTimeout,
             Millisec resolveTimeout);

  void onResolve_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt, ResolveCallback callback);
  /**
   * @brief onConnect_ Internal method called after trying to connect.
   * @param ec Error code.
   * @param endpointIt The endpoint that is successfully connected.
   * @param callback Callback to call on connect.
   */
  void onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt, ConnectCallback callback);

  void onNoopTimeout_(const ErrorCode& ec);
  void onResolveTimeout_(const ErrorCode& ec);


  /**
   * @brief connectCompleted Internal method used to take action when the connect action is completed (successful
   *or not).
   * @param ec
   * @param endpointIt
   */
  void connectCompleted(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt);

private:
  /**
   * @brief requestCompleted Called by Request<C> to inform the client that request has completed.
   * @param request
   *
   * Request object is invalid after calling this and must not do any operations
   *because this will delete the request.
   *
   * This method is always called after a request started. If connection failure occurs before the request
   *processing starts, then this is not called. So failed requests will be cleaned-up but not-yet-started
   *requests will remain to be reprocessed using ClientBase<C>::restartProcessing().
   */
  void requestCompleted(Request<C>& request);

private:
  asio::io_service& m_is;
  std::string m_host;
  tcp::resolver m_resolver;
  std::string m_service;
  tcp::resolver::iterator m_endpointIterator;

  ConnectCallback m_connectCallback;
  RequestCompletedCallback m_requestCompletedCallback;

  boost::posix_time::millisec m_resolveTimeout;
  boost::asio::deadline_timer m_resolveTimer;

  boost::posix_time::millisec m_noopTimeout;
  boost::asio::deadline_timer m_noopTimer;

  boost::lockfree::queue<Request<C>*> m_requestQueue;
  std::atomic<std::size_t> m_requestCount;
  Request<C>* m_activeRequest;
};

}
}

extern template class ashttp::client::ClientBase<ashttp::client::Client>;
extern template class ashttp::client::ClientBase<ashttp::client::ClientSSL>;
