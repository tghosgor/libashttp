/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 28.01.2015
*/

#include "external/ashttp/ashttp/header.hpp"
#include "external/ashttp/ashttp/client/client.hpp"
#include "external/ashttp/ashttp/client/request.hpp"
#include "external/ashttp/ashttp/type.hpp"

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
  });

  auto request = client->get("/a");
  request->onBodyChunk([&ioService](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
    std::cerr << "request2 body chunk; size: " << chunkSize << std::endl;
  });
  client->schedule(request);

  auto request2 = client->get("/");

  request2->onHeader([&ioService](const ErrorCode& ec, Header header) {
    BOOST_LOG_TRIVIAL(debug) << "request onheader " << ec;

    std::cout << std::endl << "Header received; ec: " << ec << std::endl;

    std::cout << header.get() << std::endl;
  }).onBodyChunk([&ioService](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
    BOOST_LOG_TRIVIAL(debug) << "request onbodychunk " << ec;

    if (!ec) {
      std::cout << std::endl << "Body chunk received; ec: " << ec << std::endl;

      std::copy_n(std::istreambuf_iterator<char>{is}, chunkSize, std::ostreambuf_iterator<char>{std::cout});

      std::cout << std::endl;

      if (chunkSize == 0)// this is the last chunk
        ioService.stop();
    } else {
      ioService.stop();
    }
  }).onTimeout([&ioService]() {
    ioService.stop();
  });

  client->schedule(request2);

  std::cout << "Joining io_service thread." << std::endl;

  t.join();
}

