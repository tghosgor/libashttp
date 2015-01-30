/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 28.01.2015
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
