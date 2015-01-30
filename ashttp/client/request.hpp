/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 27.01.2015
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
#include "../header.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <vector>

namespace ashttp {

class Header;

namespace client {

class Client;
class ClientSSL;
template <class C>
class ClientBase;

template <class C>
class Request {
  friend class ClientBase<C>;

public:
  using ErrorCode = boost::system::error_code;
  using HeaderCallback = std::function<void(const ErrorCode&, const Header&)>;
  using BodyChunkCallback = std::function<void(const ErrorCode&, std::istream&, std::size_t chunkSize)>;
  using TimeoutCallback = std::function<void()>;

public:
  Request(std::weak_ptr<C> client, std::string resource, Millisec timeout = Millisec{10000});
  ~Request();

  /**
   * @brief onHeader Registers the given callback to be called when header is received.
   * @param callback
   * @return Self.
   */
  Request& onHeader(HeaderCallback callback);

  /**
   * @brief onBodyChunk Registers the given callback to be called when a chunk of the body is received.
   * @param callback
   * @return Self.
   *
   * If transfer-encoding: chunked is used for the response, this function will be called multiple times
   * for each chunk. If the error code is success and chunk size parameter is 0, then it means the received
   *chunk is the last chunk.
   */
  Request& onBodyChunk(BodyChunkCallback callback);

  /**
   * @brief onTimeout Registers the given callback to be called on receive timeout.
   * @param callback
   * @return Self.
   */
  Request& onTimeout(TimeoutCallback callback);


  /**
   * @brief timeout Sets a new timeout.
   * @param timeout
   */
  void timeout(Millisec timeout);


  /**
   * @brief cancel Stops the current operation.
   */
  void cancel();


  /**
   * @brief reset Resets the Request object. Clears out the callbacks.
   */
  void reset();

private:
  /**
    * @brief start Add the request to a queue to be processed by the client.
    * @param timeout The amount to wait to raise a timeout before the body is completely received.
    */
  void start();


  /**
   * @brief headerCompleted Internal method used to take action when header retrieval is completed (successful
   *or not).
   * @param ec
   * @param header
   *
   * All the registered callbacks that are not to live until end of object's lifetime must be in a cleared
   *state after this is called with an error.
   */
  void headerCompleted(const ErrorCode& ec, const Header& header);

  /**
   * @brief bodyChunkCompleted Internal method used to take action when body chunk retrieval is completed
   *(successful or not).
   * @param ec
   * @param chunkSize The length of the part in \p is that holds the chunk.
   *
   * All the registered callbacks that are not to live until end of object's lifetime must be in a cleared
   *state after this is called with an error.
   */
  void bodyChunkCompleted(const ErrorCode& ec, std::size_t chunkSize);

  /**
   * @brief timeoutCompleted Internal method used to take action when request timeouts occurs (successful or
   *not).
   * @param ec
   *
   * All the registered callbacks that are not to live until end of object's lifetime must be in a cleared
   *state after this is called with an error.
   */
  void timeoutCompleted(const ErrorCode& ec);


  /**
   * @brief completeRequest Complete the request with the error code \p ec.
   * @param ec
   */
  void completeRequest(const ErrorCode& ec);


  /**
   * @brief client_ Tries to get the parent client.
   * @return Empty shared_ptr if parent client no longer exists.
   */
  std::shared_ptr<C> client_();


  // internal methods to handle callbacks

  void onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt);
  void onRequestSent_(const ErrorCode& ec, std::size_t bt);
  void onHeaderReceived_(const ErrorCode& ec, std::size_t bt);
  void onBodyReceived_(const ErrorCode& ec, std::size_t bt);

  void onTimeout_(const ErrorCode& ec);

  void onChunkSizeReceived_(const ErrorCode& ec, std::size_t bt);
  void onChunkDataReceived_(const ErrorCode& ec, std::size_t bt, std::size_t chunkSize);

private:
  std::weak_ptr<C> m_client;
  std::string m_resource;

  Header m_header;

  boost::asio::streambuf m_recvBuf;

  HeaderCallback m_headerCallback;
  BodyChunkCallback m_bodyChunkCallback;
  TimeoutCallback m_timeoutCallback;

  bool m_timedOut;
  Millisec m_timeout;
  boost::asio::deadline_timer m_timeoutTimer;

  static const constexpr std::size_t MaxRecvbufSize{20 * 1024 * 1024};
};
}
}

extern template class ashttp::client::Request<ashttp::client::Client>;
extern template class ashttp::client::Request<ashttp::client::ClientSSL>;
