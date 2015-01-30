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

#include "type.hpp"

#include <boost/asio/error.hpp>

namespace ashttp {

namespace error {

const ErrorCode success{0, boost::system::system_category()};
const ErrorCode fileTooLarge{
    boost::system::error_code{boost::asio::error::no_buffer_space, boost::asio::error::get_misc_category()}};
const ErrorCode headerParse{
    boost::system::error_code{boost::asio::error::fault, boost::asio::error::get_misc_category()}};
const ErrorCode timeout{
    boost::system::error_code{boost::asio::error::timed_out, boost::asio::error::get_misc_category()}};

}

}
