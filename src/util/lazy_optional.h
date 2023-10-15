// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Original implementation here: https://github.com/whoshuu/cpp_range/blob/cd6897e40176c3031ad1e4c8069d672b2e544996/include/range.h under MIT software license

#ifndef BITCOIN_UTIL_LAZY_OPTIONAL_H
#define BITCOIN_UTIL_LAZY_OPTIONAL_H

#include <optional>

template<typename ValueType, typename FallbackFunctionType>
ValueType lazy_value_or(const std::optional<ValueType>& opt, FallbackFunctionType&& fn) {
    if (opt) return *opt;
    return std::forward<FallbackFunctionType>(fn)();
}

#endif //BITCOIN_UTIL_LAZY_OPTIONAL_H
