libashttp
=========

An asynchronous HTTP library using Boost.ASIO as the backend.

This project is licensed under:

[![LGPLv3](http://www.gnu.org/graphics/lgplv3-147x51.png)](http://www.gnu.org/licenses/lgpl-3.0.html)

## Usage
Here is a example usage which is taken from the repository:

```
auto client = ClientSSL::create("www.google.com", ioService);

auto request = client->get("/");

request->onHeader([&ioService](const ErrorCode& ec, Header header) {
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

client->schedule(request);
```

More through examples can be found in the repository.
