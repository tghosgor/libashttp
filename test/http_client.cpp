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

#include "../ashttp/header.hpp"
#include "../ashttp/client/client.hpp"
#include "../ashttp/client/request.hpp"
#include "../ashttp/type.hpp"

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <iostream>
#include <thread>

using namespace ashttp;
using namespace ashttp::client;

ashttp::asio::io_service* pIoService;

void sigintHandler(int);

int main(int argc, char* argv[]) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::severity_level::trace);

  ashttp::asio::io_service ioService;
  pIoService = &ioService;

  std::signal(SIGTERM, sigintHandler);

  {
    const auto client = ClientHTTPS::create("www.google.com.tr", ioService);

    client->onConnect([](const ErrorCode& ec) {
      std::cerr << "\tclient onconnect " << ec << std::endl;
    });

    {
      auto request = client->get("/a");

      request->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequest body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, request](const ErrorCode& ec) {
        std::cerr << "\trequest oncomplete" << ec << std::endl;
      });

      client->schedule(request);
    }

    {
      auto request2 = client->get("/");

      request2->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequest2 body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, request2](const ErrorCode& ec) {
        std::cerr << "\trequest2 oncomplete" << ec << std::endl;
      });

      client->schedule(request2);
    }

    {
      auto requestX = client->get("/");

      requestX->onHeader([](const ErrorCode& ec, const Header& header) {
        std::cerr << "\trequestX onheader " << ec << std::endl;

        std::cout << std::endl << "Header received; ec: " << ec << std::endl;

        std::cout << header.field() << std::endl;
      })
      .onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestX body chunk; size: " << chunkSize << std::endl;

        if (!ec) {
          std::cerr << "\tBody chunk received; ec: " << ec << std::endl;

          std::copy_n(std::istreambuf_iterator<char>{is}, chunkSize, std::ostreambuf_iterator<char>{std::cout});
          // copy_n does not consume the last byte
          is.ignore(1);

          std::cout << std::endl;
        }
      })
      .onTimeout([]() {
        ;
      }).onComplete([client, requestX](const ErrorCode& ec) {
        std::cerr << "\trequestX oncomplete" << ec << std::endl;
      });

      client->schedule(requestX);
    }

    {
      auto requestY = client->get("/");

      requestY->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestY body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, requestY](const ErrorCode& ec) {
        std::cerr << "\trequestY oncomplete" << ec << std::endl;
      });

      client->schedule(requestY);
    }

    {
      auto requestR = client->get("/");

      requestR->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestR body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, requestR](const ErrorCode& ec) {
        std::cerr << "\trequestR oncomplete" << ec << std::endl;
      });

      client->schedule(requestR);
    }

    {
      auto requestR = client->get("/");

      requestR->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestR body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, requestR](const ErrorCode& ec) {
        std::cerr << "\trequestR oncomplete" << ec << std::endl;
      });

      client->schedule(requestR);
    }

    {
      auto requestR = client->get("/");

      requestR->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestR body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, requestR](const ErrorCode& ec) {
        std::cerr << "\trequestR oncomplete" << ec << std::endl;
      });

      client->schedule(requestR);
    }

    {
      auto requestR = client->get("/");

      requestR->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestR body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, requestR](const ErrorCode& ec) {
        std::cerr << "\trequestR oncomplete" << ec << std::endl;
      });

      client->schedule(requestR);
    }

    {
      auto requestR = client->get("/");

      requestR->onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
        std::cerr << "\trequestR body chunk; size: " << chunkSize << std::endl;

        is.ignore(chunkSize);
      })
      .onComplete([client, requestR](const ErrorCode& ec) {
        std::cerr << "\trequestR oncomplete" << ec << std::endl;
      });

      client->schedule(requestR);
    }
  }

  std::thread t{[&ioService]() { ioService.run(); }};

  std::cerr << "\tJoining io_service thread." << std::endl;

  t.join();

  std::cerr << "\tJoined io_service thread." << std::endl;

  return 0;
}

void sigintHandler(int) {
  std::cerr << "\tClose requested." << std::endl;

  pIoService->stop();
}
