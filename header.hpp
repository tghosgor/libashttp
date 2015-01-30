/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 27.01.2015
*/

#pragma once

#include <boost/optional.hpp>

#include <map>

namespace ashttp {

class Header {
  using StringRange = std::pair<std::string::const_iterator, std::string::const_iterator>;

public:
  /**
   * @brief Header Constructs a header object using the given stream as data source.
   * @param is Stream to read the header from.
   * @param length Length of the header in the stream.
   *
   * This constructor creates a header object using the \p is and \p length as header data.
   */
  Header(std::istream& is, std::size_t length);
  ~Header();

  /**
   * @brief get Gets the whole header section.
   * @return The whole header section.
   */
  const std::string& get() const { return m_data; }

  /**
   * @brief get Gets the value of a header field.
   * @param key Key of the header field to get the value of. Must be all-lowercase.
   * @return An object containing two iterators to the beginning and ending of the header field. Empty if no
   * header field exists with the given key.
   *
   * Always use an all-lowercase key.
   *
   * Calls to this method caches to a std::map in the background for the further queries with the same key.
   */
  boost::optional<const StringRange&> get(const std::string& key) const;

private:
  std::string m_data;

  mutable std::map<std::string, boost::optional<StringRange>> m_headerCache;
};

}
