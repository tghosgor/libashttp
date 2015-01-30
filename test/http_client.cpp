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

void sigintHandler(int) {
  std::cout << "Close requested." << std::endl;

  pIoService->stop();
};

int main(int argc, char* argv[]) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::severity_level::trace);

  ashttp::asio::io_service ioService;
  pIoService = &ioService;

  std::signal(SIGTERM, sigintHandler);

  // post an endless work
  ashttp::asio::io_service::work work{ioService};

  std::thread t{[&ioService]() { ioService.run(); }};


  auto client = ClientSSL::create("www.google.com.tr", ioService);

  client->onConnect([&ioService](const ErrorCode& ec, const tcp::resolver::iterator& endpointIt) {
    BOOST_LOG_TRIVIAL(debug) << "client onconnect " << ec;

    if (ec)
      ioService.stop();
  })
  .onRequestCompleted([&ioService](const ErrorCode& ec) {
    std::cout << "request completed ec: " << ec << std::endl;

    if (ec)
      ioService.stop();
  });

  auto request = client->get("/a");

  request->onBodyChunk([&ioService, request](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
    std::cout << "request body chunk; size: " << chunkSize << std::endl;
  });

  client->schedule(request);

  auto request2 = client->get("/");

  request2->onBodyChunk([&ioService, request2](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
    std::cout << "request2 body chunk; size: " << chunkSize << std::endl;
  });

  client->schedule(request2);

  auto requestX = client->get("/");

  requestX->onHeader([&ioService, requestX](const ErrorCode& ec, const Header& header) {
    std::cout << "requestX onheader " << ec << std::endl;

    std::cout << std::endl << "Header received; ec: " << ec << std::endl;

    std::cout << header.field() << std::endl;
  }).onBodyChunk([&ioService, requestX](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
    std::cout << "requestX onbodychunk " << ec << std::endl;

    if (!ec) {
      std::cout << std::endl << "Body chunk received; ec: " << ec << std::endl;

      std::copy_n(std::istreambuf_iterator<char>{is}, chunkSize, std::ostreambuf_iterator<char>{std::cout});

      std::cout << std::endl;

      if (chunkSize == 0)// this is the last chunk
        ioService.stop();
    } else {
      ioService.stop();
    }
  }).onTimeout([&ioService, requestX]() {
    ioService.stop();
  });

  client->schedule(requestX);


  std::cout << "Joining io_service thread." << std::endl;

  t.join();
}

