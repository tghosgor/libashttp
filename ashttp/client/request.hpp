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
#include "../connection.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace ashttp {

class Header;

namespace client {

template <Protocol p>
class ClientImpl;
template <Protocol p>
class ClientCRTPBase;

template <Protocol p>
class Request {
  template <Protocol p_>
  friend class ClientCRTPBase;
public:
  using HeaderCallback = std::function<void(const ErrorCode&, const Header&)>;
  using BodyChunkCallback = std::function<void(const ErrorCode&, std::istream&, std::size_t chunkSize)>;
  using TimeoutCallback = std::function<void()>;
  using CompleteCallback = std::function<void (const ErrorCode&)>;

public:
  Request(std::weak_ptr<ClientImpl<p>> client, std::string host, std::string resource, Millisec timeout = Millisec{10000});
  ~Request();

  /**
   * @brief onComplete Registers the given callback to be called on completion of the request.
   * @param callback
   * @return Self.
   *
   * This callback can be used to keep the object alive. The registered \p callback is guaranteed to be called
   *at the end of request processing. The \p callback will be released after being called once.
   */
  Request& onComplete(CompleteCallback callback);


  /**
   * @brief onHeader Registers the given callback to be called when header is received.
   * @param callback
   * @return Self.
   *
   * The given \p callback will live until the lifetime of this object ends. This callback should not be
   *used to keep the object alive as it will cause the object to keep itself alive forever. This callback
   *also is not guaranteed to be called at all. Use Request<C>::onComplete() to track the state
   *of the object.
   */
  Request& onHeader(HeaderCallback callback);

  /**
   * @brief onBodyChunk Registers the given callback to be called when a chunk of the body is received.
   * @param callback
   * @return Self.
   *
   * IMPORTANT: If a callback is registered using this method, the callback must consume
   *all the chunk data (given in chunkSize) from the input stream. There may be more data in the input
   *stream and callback must not read beyond the given chunk size in the parameter.
   *
   * If transfer-encoding: chunked is used for the response, this function will be called multiple times
   * for each chunk. If the error code is success and chunk size parameter is 0, then it means the received
   *chunk is the last chunk.
   *
   * The given \p callback will live until the lifetime of this object ends. This callback should not be
   *used to keep the object alive as it will cause the object to keep itself alive forever. This callback
   *also is not guaranteed to be called at all. Use Request<C>::onComplete() to track the state
   *of the object.
   */
  Request& onBodyChunk(BodyChunkCallback callback);

  /**
   * @brief onTimeout Registers the given callback to be called on receive timeout.
   * @param callback
   * @return Self.
   *
   * The given \p callback will live until the lifetime of this object ends. This callback should not be
   *used to keep the object alive as it will cause the object to keep itself alive forever. This callback
   *also is not guaranteed to be called at all. Use Request<C>::onComplete() to track the state
   *of the object.
   */
  Request& onTimeout(TimeoutCallback callback);


  /**
   * @brief timeout Sets a new timeout.
   * @param timeout
   * @return Self.
   */
  Request& timeout(Millisec timeout);


private:
  /**
    * @brief start Add the request to a queue to be processed by the client.
    * @param timeout The amount to wait to raise a timeout before the body is completely received.
    */
  void start();


  /**
   * @brief addNextRequest Adds a new request to be started when this request finishes.
   * @param req
   */
  void addNextRequest(std::weak_ptr<Request<p>> req);


  /**
   * @brief headerCompleted Internal method used to take action when header retrieval is completed (successful
   *or not).
   * @param ec
   * @param header
   */
  void headerCompleted(const ErrorCode& ec, const Header& header);

  /**
   * @brief bodyChunkCompleted Internal method used to take action when body chunk retrieval is completed
   *(successful or not).
   * @param ec
   * @param chunkSize The length of the part in \p is that holds the chunk.
   */
  void bodyChunkCompleted(const ErrorCode& ec, std::size_t chunkSize);

  /**
   * @brief timeoutCompleted Internal method used to take action when request timeouts occurs (successful or
   *not).
   * @param ec
   */
  void timeoutCompleted(const ErrorCode& ec);


  /**
   * @brief completeRequest Completes the request and notifies its client.
   * @param ec
   *
   * Tries to cancel timeouts and, if successful,
   *does the internal clean-up and and notifies its client with the given error code \p ec.
   */
  void tryCompleteRequest(const ErrorCode& ec);

  /**
   * @brief completeRequest See tryCompleteRequest. This one does not check timeouts.
   * @param ec
   */
  void completeRequest(const ErrorCode& ec);


  /**
   * @brief end Try to finish the request.
   * @param ec
   *
   * Tries to cancel timeouts and, if successful,
   *does the internal clean-up and calls ending callbacks with the given error code \p ec.
   * IMPORTANT: Request may not exist after this method as it clears the complete callback.
   *Members of 'this' must not be accessed after calling this method.
   */
  void tryFinish(const ErrorCode& ec);

  /**
   * @brief finish See tryFinish(). This one does not check timeouts.
   * @param ec
   */
  void finish(const ErrorCode& ec);


  /**
   * @brief cancelTimeouts Tries to cancel all timeouts.
   * @return true if all waiters cancelled, false if not.
   */
  bool cancelTimeouts();


  // internal methods to handle callbacks

  void onRequestSent_(const ErrorCode& ec, std::size_t bt);
  void onHeaderReceived_(const ErrorCode& ec, std::size_t bt);
  void onBodyReceived_(const ErrorCode& ec, std::size_t bt);

  /**
   * @brief onTimeout_
   * @param ec
   *
   * This function must not use 'this' if \p ec is not 0.
   */
  void onTimeout_(const ErrorCode& ec);

  void onChunkSizeReceived_(const ErrorCode& ec, std::size_t bt);
  void onChunkDataReceived_(const ErrorCode& ec, std::size_t bt, std::size_t chunkSize);

private:
  std::weak_ptr<ClientImpl<p>> m_client;
  std::string m_host;
  std::string m_resource;

  Header m_header;

  boost::asio::streambuf m_recvBuf;

  HeaderCallback m_headerCallback;
  BodyChunkCallback m_bodyChunkCallback;
  TimeoutCallback m_timeoutCallback;
  CompleteCallback m_completeCallback;

  bool m_timedOut;
  Millisec m_timeout;
  boost::asio::deadline_timer m_timeoutTimer;

  static const constexpr std::size_t MaxRecvbufSize{20 * 1024 * 1024};
};

}
}

extern template class ashttp::client::Request<ashttp::Protocol::HTTP>;
extern template class ashttp::client::Request<ashttp::Protocol::HTTPS>;
