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

#include "request.hpp"

#include "client.hpp"

#include <boost/chrono.hpp>

#include <array>

namespace ashttp {
namespace client {

using namespace std::placeholders;

template <Protocol p>
Request<p>::Request(std::weak_ptr<ClientImpl<p>> client, std::string host, std::string resource, Millisec timeout)
    : m_client{std::move(client)}
    , m_host{std::move(host)}
    , m_resource{std::move(resource)}
    , m_timeout{timeout}
    , m_timeoutTimer{m_client.lock()->connection().socket().get_io_service()} {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request";
}

template <Protocol p>
Request<p>::~Request() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " ~Request";
}

template <Protocol p>
Request<p>& Request<p>::onComplete(CompleteCallback callback) {
  m_completeCallback = std::move(callback);

  return *this;
}

template <Protocol p>
Request<p>& Request<p>::timeout(Millisec timeout) {
  m_timeout = std::move(timeout);

  return *this;
}

template <Protocol p>
Request<p>& Request<p>::onHeader(HeaderCallback callback) {
  m_headerCallback = std::move(callback);

  return *this;
}

template <Protocol p>
Request<p>& Request<p>::onBodyChunk(BodyChunkCallback callback) {
  m_bodyChunkCallback = std::move(callback);

  return *this;
}

template <Protocol p>
Request<p>& Request<p>::onTimeout(TimeoutCallback callback) {
  m_timeoutCallback = std::move(callback);

  return *this;
}

template <Protocol p>
void Request<p>::start() {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::start";

  // start the timeout
  m_timedOut = false;
  m_timeoutTimer.expires_from_now(m_timeout);
  m_timeoutTimer.async_wait(std::bind(&Request<p>::onTimeout_, this, _1));

  std::ostream os(&m_recvBuf);

  os << "GET " << m_resource << " HTTP/1.1\r\n"
     << "Host: " << m_host << "\r\n"
                                       //         << "Connection: close\r\n"
                                       "\r\n";

  if (const auto client = m_client.lock()) {
    async_write(client->connection().socket(), m_recvBuf, std::bind(&Request<p>::onRequestSent_, this, _1, _2));
  } else {
    tryCompleteRequest(error::canceled);
  }
}

template <Protocol p>
void Request<p>::headerCompleted(const ErrorCode& ec, const Header& header) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::headerCompleted ec: " << ec;

  if (!ec) {
    if (m_headerCallback) {
      m_headerCallback(ec, header);
//      m_headerCallback = nullptr;
    }
  } else {// clear out other callbacks if there is an error (they will not be called in that case)
    tryCompleteRequest(ec);
  }
}

template <Protocol p>
void Request<p>::bodyChunkCompleted(const ErrorCode& ec, std::size_t chunkSize) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::bodyChunkCompleted ec: " << ec
                           << ", chunk size: " << chunkSize;

  if (!ec) {// no error

    // call the chunk callback if it exists
    if (m_bodyChunkCallback) {
      std::istream is{&m_recvBuf};

      m_bodyChunkCallback(ec, is, chunkSize);
    } else {
      m_recvBuf.consume(chunkSize);
    }

    if (chunkSize == 0) { // it was last chunk
//      m_bodyChunkCallback = nullptr;

      tryCompleteRequest(ec);
    }

  } else { // error
//    m_bodyChunkCallback = nullptr;

    tryCompleteRequest(ec);
  }
}

template <Protocol p>
void Request<p>::timeoutCompleted(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::timeoutCompleted ec: " << ec;

  if (!ec) { // if timeout actually occurred
    if (m_timeoutCallback) {
      m_timeoutCallback();

//      m_timeoutCallback = nullptr;
    }

    completeRequest(error::timeout);
  }
}

template <Protocol p>
void Request<p>::tryCompleteRequest(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::tryCompleteRequest ec: " << ec;

  // no timeout callback should access members of 'this' in case of error (e.g. cancel)
  // therefore we must be sure we successfully cancelled the handlers
  // or else let the timeout happen

  if (cancelTimeouts()) {
    completeRequest(ec);
  }
}

template <Protocol p>
void Request<p>::completeRequest(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::completeRequest ec: " << ec;

  if (const auto client = m_client.lock()) {
    // notify the client request has completed
    client->requestCompleted(ec);
  }

  finish(ec);
  // no more operations below this point as clearing complete callback may have removed the request
}

template <Protocol p>
void Request<p>::tryFinish(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::tryFinish ec: " << ec;

  // no timeout callback should access members of 'this' in case of error (e.g. cancel)
  // therefore we must be sure we successfully cancelled the handlers
  // or else let the timeout happen

  if (cancelTimeouts()) {
    finish(ec);
    // no more operations below this point as clearing complete callback may have removed the request
  }
}

template <Protocol p>
void Request<p>::finish(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::finish ec: " << ec;

  if (m_completeCallback) {
    m_completeCallback(ec);

    // NOTE! assigning nullptr here causes double destruction of the callback object
    // because removal of the callback may trigger object destruction again
    // clear the callback
    m_completeCallback = [](const ErrorCode& ec){};

    // no more operations below this point as clearing complete callback may have removed the request
  }
}

template <Protocol p>
bool Request<p>::cancelTimeouts() {
  const auto waitersCancelled = m_timeoutTimer.cancel();

  assert(waitersCancelled <= 1); // there can be at most one waiter

  return waitersCancelled > 0;
}

template <Protocol p>
void Request<p>::onRequestSent_(const ErrorCode& ec, std::size_t bt) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onRequestSent_ ec: " << ec;

  if (!ec) {
    if (const auto client = m_client.lock()) {
      async_read_until(client->connection().socket(), m_recvBuf, "\r\n\r\n",
                       std::bind(&Request<p>::onHeaderReceived_, this, _1, _2));
    } else {
      tryCompleteRequest(error::canceled);
    }
  } else {
    tryCompleteRequest(ec);
  }
}

template <Protocol p>
void Request<p>::onHeaderReceived_(const ErrorCode& ec, std::size_t bt) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onHeaderReceived_ ec: " << ec;

  std::istream is{&m_recvBuf};

  try {
    if (!ec) {
      const auto headerSize = bt - 2; // do not include header end

      m_header.reset();
      m_header.load(is, headerSize);

      // consume the extra \r\n
      m_recvBuf.consume(2);

      if (const auto transferEncoding = m_header.field("transfer-encoding")) { // check if chunked

        // read the chunk size
        if (const auto client = m_client.lock()) {
          async_read_until(client->connection().socket(), m_recvBuf, "\r\n",
                           std::bind(&Request<p>::onChunkSizeReceived_, this, _1, _2));

          headerCompleted(ec, m_header);
        } else {
          headerCompleted(error::canceled, m_header);
        }

      } else if (const auto contentLengthIt = m_header.field("content-length")) { // if content-length exists

        const auto contentLengthLength = std::distance(contentLengthIt->first, contentLengthIt->second);

        if (contentLengthLength <= MaxRecvbufSize) { // check content-length header
          std::array<char, std::numeric_limits<unsigned long long>::digits10 + 1> contentLengthStr;
          std::copy(contentLengthIt->first, contentLengthIt->second, contentLengthStr.begin());
          contentLengthStr[contentLengthLength] = 0;

          const std::size_t contentLength = std::stoul(contentLengthStr.data());

          const auto bytesLeftToReceive = contentLength - std::min(contentLength, m_recvBuf.size());

          TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onHeaderReceived_ bytesLeftToReceive: " << bytesLeftToReceive;

          if (const auto client = m_client.lock()) {
            async_read(client->connection().socket(), m_recvBuf.prepare(bytesLeftToReceive),
                       std::bind(&Request<p>::onBodyReceived_, this, _1, _2));

            headerCompleted(ec, m_header);
          } else {// no client any more
            headerCompleted(error::canceled, m_header);
          }
        } else { // too big content-length
          headerCompleted(error::fileTooLarge, m_header);
        }

      } else { // no content-length field in header

        // assume connection close is body end
        if (const auto client = m_client.lock()) {
          async_read(client->connection().socket(), m_recvBuf, std::bind(&Request<p>::onBodyReceived_, this, _1, _2));

          headerCompleted(ec, m_header);
        } else {// no client any more
          headerCompleted(error::canceled, m_header);
        }

      }
    } else { // ec exists

      headerCompleted(ec, m_header);

    }
  } catch (const std::exception& e) {

    headerCompleted(error::headerParse, m_header);

  }
}

template <Protocol p>
void Request<p>::onBodyReceived_(const ErrorCode& ec, std::size_t bt) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onBodyReceived_ ec: " << ec;

  m_recvBuf.commit(bt);

  if (!ec) {
    bodyChunkCompleted(ec, bt);
    bodyChunkCompleted(ec, 0);
  } else {
    bodyChunkCompleted(ec, bt);
  }
}

template <Protocol p>
void Request<p>::onTimeout_(const ErrorCode& ec) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onTimeout_ ec: " << ec;

  if (!ec) {
    m_timedOut = true;
  }

  timeoutCompleted(ec);
}

template <Protocol p>
void Request<p>::onChunkSizeReceived_(const ErrorCode& ec, std::size_t bt) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onChunkSizeReceived_ ec: " << ec;

  if (!ec) {
    // move the chunk size data to input sequence
    std::istream is{&m_recvBuf};

    // read the chunk size
    std::size_t chunkSize;
    is >> std::hex >> chunkSize;

    // skip \r\n
    is.ignore(2);

    const auto totalChunkSize = chunkSize + 2; // assume chunk size includes the \r\n

    const auto alreadyInBuffer = m_recvBuf.size();

    // maximum receive buf size is exceeded (a chunk is held at a time here)
    if (alreadyInBuffer + chunkSize <= MaxRecvbufSize) {
      const auto chunkBytesLeftToReceive =
          totalChunkSize -
          std::min(totalChunkSize,
                   alreadyInBuffer); /* extract the already fetched part (by async_read_until before coming here) */

      TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onChunkSizeReceived_ " << alreadyInBuffer << '-'
                                         << totalChunkSize << '-' << chunkBytesLeftToReceive;

      if (const auto client = m_client.lock()) {
        async_read(client->connection().socket(), m_recvBuf.prepare(chunkBytesLeftToReceive),
                   std::bind(&Request<p>::onChunkDataReceived_, this, _1, _2, chunkSize));
      } else {
        tryCompleteRequest(error::canceled);
      }

    } else { // max recvbuf size exceeded
      tryCompleteRequest(error::fileTooLarge);
    }
  } else {
    tryCompleteRequest(ec);
  }
}

template <Protocol p>
void Request<p>::onChunkDataReceived_(const ErrorCode& ec, std::size_t bt, std::size_t chunkSize) {
  TEMPLOG_DEVLOG(templog::sev_debug) << this << " Request<p>::onChunkDataReceived_ chunk size: " << chunkSize
                           << ", ec: " << ec;

  if (!ec) {
    // reset the noop timeout on chunk data received
    if (const auto client = m_client.lock()) {
      if (client->connection().stopNoopTimer()) {
        client->connection().startNoopTimer();

        // read the next chunk size
        if (chunkSize > 0) {
          m_recvBuf.commit(bt);

          bodyChunkCompleted(ec, chunkSize);

          m_recvBuf.consume(2);

          // see if there are more chunks
          async_read_until(client->connection().socket(), m_recvBuf, "\r\n",
                           std::bind(&Request<p>::onChunkSizeReceived_, this, _1, _2));
        } else { // this is the last chunk (empty chunk)
          bodyChunkCompleted(ec, chunkSize);
        }
      }// if not reset, let the timeout happen
    } else {
      tryCompleteRequest(error::canceled);
    }
  } else {
    tryCompleteRequest(ec);
  }
}

}
}

template class ashttp::client::Request<ashttp::Protocol::HTTP>;
template class ashttp::client::Request<ashttp::Protocol::HTTPS>;
