/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 27.01.2015
*/

#pragma once

#include "../type.hpp"

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
  using HeaderCallback = std::function<void(const ErrorCode&, Header)>;
  using BodyChunkCallback = std::function<void(const ErrorCode&, std::istream&, std::size_t chunkSize)>;
  using TimeoutCallback = std::function<void()>;

public:
  Request(C& client, std::string resource, Millisec timeout = Millisec{10000});
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
  void headerCompleted(const ErrorCode& ec, Header header);

  /**
   * @brief bodyChunkCompleted Internal method used to take action when body chunk retrieval is completed
   *(successful or not).
   * @param ec
   * @param is
   * @param chunkSize
   * @return
   *
   * All the registered callbacks that are not to live until end of object's lifetime must be in a cleared
   *state after this is called with an error.
   */
  void bodyChunkCompleted(const ErrorCode& ec, std::istream& is, std::size_t chunkSize);

  /**
   * @brief timeoutCompleted Internal method used to take action when request timeouts occurs (successful or
   *not).
   * @param ec
   *
   * All the registered callbacks that are not to live until end of object's lifetime must be in a cleared
   *state after this is called with an error.
   */
  void timeoutCompleted(const ErrorCode& ec);


  // internal methods to handle callbacks

  void onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt);
  void onRequestSent_(const ErrorCode& ec, std::size_t bt);
  void onHeaderReceived_(const ErrorCode& ec, std::size_t bt);
  void onBodyReceived_(const ErrorCode& ec, std::size_t bt);

  void onTimeout_(const ErrorCode& ec);

  void onChunkSizeReceived_(const ErrorCode& ec, std::size_t bt);
  void onChunkDataReceived_(const ErrorCode& ec, std::size_t bt, std::size_t chunkSize);

private:
  C& m_client;
  std::string m_resource;
  boost::asio::streambuf m_recvBuf;

  HeaderCallback m_headerCallback;
  BodyChunkCallback m_bodyChunkCallback;
  TimeoutCallback m_timeoutCallback;

  bool m_timedOut;
  Millisec m_timeout;
  boost::asio::deadline_timer m_timeoutTimer;

  static const constexpr std::size_t MaxContentSize{20 * 1024 * 1024};
};
}
}

extern template class ashttp::client::Request<ashttp::client::Client>;
extern template class ashttp::client::Request<ashttp::client::ClientSSL>;
