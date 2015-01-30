/*
 * Copyright © 2014-2015, Tolga HOŞGÖR.
 *
 * File created on: 27.01.2015
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

#include <boost/optional.hpp>

#include <map>

namespace ashttp {

template <class C>
class Request;

class Header {
  template <class C>
  friend class Request;

  using StringRange = std::pair<std::string::const_iterator, std::string::const_iterator>;

public:
  Header();
  ~Header();

  /**
   * @brief field Sets a key to a value.
   * @param key Key to set.
   * @param value Value to set.
   *
   * Will override if the key already exists.
   *
   * Beware that this method is does not validate that header field keys are unique.
   */
  void field(const std::string& key, const std::string& value);

  /**
   * @brief field Gets the whole header section.
   * @return The whole header section.
   */
  const std::string& field() const { return m_data; }

  /**
   * @brief field Gets the value of a header field.
   * @param key Key of the header field to get the value of. Must be all-lowercase.
   * @return An object containing two iterators to the beginning and ending of the header field. Empty if no
   * header field exists with the given key.
   *
   * Always use an all-lowercase key.
   *
   * Calls to this method caches to a std::map in the background for the further queries with the same key.
   */
  boost::optional<const StringRange&> field(const std::string& key) const;

  /**
   * @brief reset Resets the header state.
   */
  void reset();

public://private:
  /**
   * @brief load Loads the header data from the given istream \p is with the \p length limit.
   * @param is The stream to read from.
   * @param length Length of the header.
   */
  void load(std::istream& is, std::size_t length);

private:
  std::string m_query;
  std::string m_data;

  mutable std::map<std::string, boost::optional<StringRange>> m_headerCache;
};

}
