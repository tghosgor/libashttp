libashttp
=========

An asynchronous HTTP library using Boost.ASIO as the backend.

This project is licensed under:

[![LGPLv3](http://www.gnu.org/graphics/lgplv3-147x51.png)](http://www.gnu.org/licenses/lgpl-3.0.html)

## Usage
Here is a example usage which is taken from the repository:

```
auto client = ClientHTTPS::create("www.google.com", ioService);

{
  auto request = client->get("/");

  {
    request->onHeader([](const ErrorCode& ec, const Header& header) {
      std::cout << "request onheader " << ec << std::endl;

      std::cout << std::endl << "Header received; ec: " << ec << std::endl;

      std::cout << header.field() << std::endl;
    }).onBodyChunk([](const ErrorCode& ec, std::istream& is, std::size_t chunkSize) {
      std::cout << "request onbodychunk chunksize: " << chunkSize << ", ec: " << ec << std::endl;

      if (!ec) {
        std::cout << std::endl << "Body chunk received; ec: " << ec << std::endl;

        std::copy_n(std::istreambuf_iterator<char>{is}, chunkSize, std::ostreambuf_iterator<char>{std::cout});

        std::cout << std::endl;

        if (chunkSize == 0)// this is the last chunk
          std::cout << "all chunks received" << std::endl;
      }
    }).onTimeout([]() {
      std::cout << "timeout" << ec << std::endl;
    }).onComplete([client, request](const ErrorCode& ec) {
      std::cout << "oncomplete" << ec << std::endl;

      ioService.stop();
    });

    client->schedule(request);
  }
}
```

More through examples can be found in the repository.
