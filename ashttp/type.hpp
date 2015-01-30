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
