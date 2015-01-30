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

#include "header.hpp"

#include <algorithm>
#include <iterator>
#include <functional>

namespace ashttp {

using namespace std::placeholders;

Header::Header(std::istream& is, std::size_t length) {
  m_data.reserve(length);

  std::copy_n(std::istreambuf_iterator<char>{is}, length, std::back_inserter(m_data));

  // also discard the last element
  is.ignore(1);
}

Header::~Header() {}

boost::optional<const Header::StringRange&> Header::get(const std::string& key) const {
  const auto lowerBound = m_headerCache.lower_bound(key);

  if (lowerBound != m_headerCache.end() && lowerBound->first == key) { // key is already cached
    return boost::optional<const StringRange&>{lowerBound->second};
  } else { // key is not cached
    // key does not care about case
    const auto keyPredicate = [](const char& lhs, const char& rhs) { return tolower(lhs) == rhs; };

    auto field = std::search(m_data.begin(), m_data.end(), key.begin(), key.end(), keyPredicate);

    if (field != m_data.end()) {
      field += key.size();

      auto valBegin = std::find(field, m_data.end(), ':');

      if (valBegin == m_data.end())
        throw std::runtime_error{"Malformed header."};

      // skip the colon
      ++valBegin;

      if (valBegin == m_data.end())
        throw std::runtime_error{"Malformed header."};

      // skip the spaces
      while (*valBegin == ' ') {
        ++valBegin;

        if (valBegin == m_data.end())
          throw std::runtime_error{"Malformed header."};
      }

      const auto valEnd = std::find(valBegin, m_data.end(), '\r');

      if (valEnd == m_data.end())
        throw std::runtime_error{"Malformed header."};

      const auto newIt =
          m_headerCache.emplace_hint(lowerBound, std::piecewise_construct, std::forward_as_tuple(key),
                                     std::forward_as_tuple(StringRange{valBegin, valEnd}));

      return boost::optional<const StringRange&>{newIt->second};
    } else { // key does not exist, put a cache entry
      const auto newIt = m_headerCache.emplace_hint(lowerBound, std::piecewise_construct,
                                                    std::forward_as_tuple(key), std::forward_as_tuple());

      return boost::optional<const StringRange&>{newIt->second};
    }
  }
}

}
