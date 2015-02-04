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

#pragma once

#include "../type.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <deque>
#include <mutex>

namespace ashttp {
namespace client {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

template <Protocol p>
class ClientImpl;
template <Protocol p>
class Request;

template <Protocol p>
class ClientCRTPBase
    : public std::enable_shared_from_this<ClientImpl<p>> {
  friend class Request<p>;
public:
  using ResolveCallback = std::function<void (const ErrorCode&, const tcp::resolver::iterator&)>;
  using ConnectCallback = std::function<void (const ErrorCode&)>;

public:
  /**
    * @brief create Creates a new client object.
    * @param host The host to connect to.
    * @param is The io_service to use.
    * @param noopTimeout Time in milliseconds to stay inactive before closing the socket.
    * @param resolveTimeout Time in milliseconds to try to resolve the given hostname before timing out.
    * @return A shared_ptr object containing the new client.
    */
  static std::shared_ptr<ClientImpl<p>> create(std::string host, asio::io_service& is, Millisec noopTimeout = Millisec{30000},
                                               Millisec resolveTimeout = Millisec{10000});

  ~ClientCRTPBase();


  /**
   * @brief connect Tries to connect to the client's host.
   * @param callback Callback to call with the connect result.
   */
  void connect(ConnectCallback callback);


  /**
   * @brief host
   * @return The hostname this client handles.
   */
  const std::string& host() const { return m_host; }


  /**
   * @brief get Creates a Response object to retrieve the resource.
   * @param resource
   * @return
   *
   * This method is usually the only thing you need. Response object automatically manages the set-up this
   * Client will need.
   */
  std::shared_ptr<Request<p>> get(std::string resource);


  /**
   * @brief schedule Schedules a request object for processing.
   * @param request Request to schedule.
   */
  void schedule(std::weak_ptr<Request<p>> request);


  /**
   * @brief connect Registers a callback to call after trying to connect.
   * @param callback
   * @return Self.
   *
   * The callback registered with this method will live until the end of lifetime of this object.
   *This means that if you bind the this client's shared_ptr object to this callback, the object will live forever.
   */
  ClientImpl<p>& onConnect(ConnectCallback callback);


  /**
   * @brief requestCount Gets the number of requests being processed.
   * @return Number of requests being processed.
   */
  std::size_t requestCount() const;


  /**
   * @brief resolve Asynchronously resolves the given host.
   * @param callback Callback will be called after the resolving is done.
   *
   * This method caches the result and directly calls the callback if already resolved.
   * Also, this method will be automatically called by other methods if the other method requires resolving.
   */
  void resolve(ResolveCallback callback);


protected:
  ClientCRTPBase(std::string host, std::string service, asio::io_service& is, Millisec resolveTimeout);

  void onResolve_(const ErrorCode& ec, tcp::resolver::iterator endpointIt, ResolveCallback callback);
  /**
   * @brief onConnect_ Internal method called after trying to connect.
   * @param ec Error code.
   * @param endpointIt The endpoint that is successfully connected.
   * @param callback Callback to call on connect.
   */
  void onConnect_(const ErrorCode& ec, ConnectCallback callback);

  void onNoopTimeout_();
  void onResolveTimeout_(const ErrorCode& ec);


private:
  /**
   * @brief connectCompleted
   * @param ec
   * @param endpointIt
   */
  void connectCompleted(const ErrorCode& ec);


  /**
   * @brief requestCompleted Called by Request<p> to inform the client that request has completed.
   * @param ec
   *
   * Request object is invalid after calling this and must not do any operations
   *because this will delete the request.
   *
   * This method is always called after a request started. If connection failure occurs before the request
   *processing starts, then this is not called. So failed requests will be cleaned-up but not-yet-started
   *requests will remain to be reprocessed using ClientBase<C>::restartProcessing().
   */
  void requestCompleted(const ErrorCode& ec);


  /**
   * @brief clearRequestQueue
   * @param ec Error code to notify the requests in queue.
   */
  void clearRequestQueue(const ErrorCode& ec);


private:
  asio::io_service& m_is;
  std::string m_host;
  tcp::resolver m_resolver;
  std::string m_service;
  tcp::resolver::iterator m_endpointIterator;

  ConnectCallback m_connectCallback;

  boost::posix_time::millisec m_resolveTimeout;
  boost::asio::deadline_timer m_resolveTimer;

  mutable std::mutex m_requestQueueMtx;
  std::deque<std::weak_ptr<Request<p>>> m_requestQueue;
  bool m_requestActive;
};

extern template class ClientCRTPBase<Protocol::HTTP>;
extern template class ClientCRTPBase<Protocol::HTTPS>;

}
}

