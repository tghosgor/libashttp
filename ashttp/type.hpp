/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 28.01.2015
*/

#pragma once

#include <boost/system/error_code.hpp>
#include <boost/date_time.hpp>

namespace boost {
namespace asio {
namespace ip {
class tcp;
}
}
}

namespace ashttp {

using ErrorCode = boost::system::error_code;
using Millisec = boost::posix_time::millisec;

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace error {

extern const ErrorCode success;
extern const ErrorCode fileTooLarge;
extern const ErrorCode headerParse;
extern const ErrorCode timeout;

}

}
