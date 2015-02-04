/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 2.02.2015
*/

#pragma once

#include "type.hpp"

#include <boost/asio.hpp>

#include <functional>

namespace ashttp {

namespace asio = boost::asio;

template <Protocol p>
class ConnectionImpl;

template <class T>
class ConnectionCRTPBase {
public:
  using ConnectCallback = std::function<void (const ErrorCode&)>;
  using TimeoutCallback = std::function<void ()>;

public:
  ConnectionCRTPBase(asio::io_service& is, Millisec noopTimeout);
  ~ConnectionCRTPBase();


  /**
   * @brief connect Tries to connect to the host.
   * @param iterator An iterator to a series of endpoints to try to connect to.
   * @param callback A callback to call on connect.
   */
  void connect(tcp::resolver::iterator iterator, ConnectCallback callback);


  /**
   * @brief onNoopTimeout Registers a \p callback to call on timeout completion.
   * @param callback The callback to call.
   *
   * The given callback will live until this object is destroyed.
   */
  void onNoopTimeout(TimeoutCallback callback);

  /**
   * @brief startNoopTimer
   */
  void startNoopTimer();

  /**
   * @brief stopNoopTimer
   * @return true if timer successfully cancelled, false if not (handlers are already called or about to be called).
   */
  bool stopNoopTimer();


  void close() { static_cast<T*>(this)->socket().lowest_layer().close(); }


protected:
  void onConnect_(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt, ConnectCallback callback);


  /**
   * @brief onNoopTimeout_ Internal callback.
   * @param ec
   */
  void onNoopTimeout_(const ErrorCode& ec);

private:
  /**
   * @brief connectCompleted Internal method used to take action when the connect action is completed (successful
   *or not).
   * @param ec
   * @param endpointIt
   */
  void connectCompleted(const ErrorCode& ec, const tcp::resolver::iterator& endpointIt);

private:
  boost::posix_time::millisec m_noopTimeout;
  boost::asio::deadline_timer m_noopTimer;

  TimeoutCallback m_noopCallback;
};

extern template class ConnectionCRTPBase<ConnectionImpl<Protocol::HTTP>>;
extern template class ConnectionCRTPBase<ConnectionImpl<Protocol::HTTPS>>;

}
