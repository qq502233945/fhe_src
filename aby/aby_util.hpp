//*****************************************************************************
// Copyright 2018-2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#pragma once

#include <ENCRYPTO_utils/crypto/crypto.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "abycore/aby/abyparty.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/share.h"
#include "abycore/sharing/sharing.h"
#include "logging/ngraph_he_log.hpp"
#include "ngraph/type/element_type.hpp"
#include "seal/he_seal_backend.hpp"

namespace ngraph::runtime::aby {
template <typename T>
void print_argument(const std::vector<T>& values, const std::string& name) {
  size_t print_size = std::min(values.size(), 200UL);

  if (values.size() > 1) {
    if (values[0] == 0 && values[1] == 0) {
      return;
    }
  }

  for (size_t i = 0; i < print_size; ++i) {
    NGRAPH_HE_LOG(5) << "\t" << name << "[" << i << "] = " << values[i];
  }
}

template <typename T>
void check_argument_range(const std::vector<T>& values, const T min_val,
                          const T max_val) {
  for (size_t i = 0; i < values.size(); ++i) {
    NGRAPH_CHECK(values[i] >= min_val, "Values[", i, "] (", values[i],
                 ") too small (minimum ", min_val, ")");
    NGRAPH_CHECK(values[i] <= max_val, "Values[", i, "] (", values[i],
                 ") too large (maximum ", max_val, ")");
  }
}

// Maps numbers from (0, q) to (-q/(2 * scale), q/(2*scale))
inline double uint64_to_double(uint64_t i, uint64_t q, double scale) {
  if (i >= q) {
    NGRAPH_WARN << "i " << i << " is too large for q " << q;
    // throw ngraph_error("i is too large");
  }
  if (i > q / 2) {
    return (i - q / 2) / scale;
  } else {
    return (q / 2 - i) / (-scale);
  }
}

// Reduces d to range (-q/2, q/2) by adding / subtracting q
inline double mod_reduce_zero_centered(double d, const double q) {
  NGRAPH_CHECK(q >= 0, "q should be positive");
  if (d < -q / 2) {
    d += ceil(-1 / 2. - d / q) * q;
  } else if (d > q / 2) {
    d -= ceil(d / q - 1 / 2.) * q;
  }
  // Avoid boundary cases due to floating-point inaccuracies
  while (d < -q / 2) {
    d += q;
  }
  while (d > q / 2) {
    d -= q;
  }

  NGRAPH_CHECK(d <= q / 2 && d >= -q / 2, "d ", d, " outside valid range [",
               -q / 2, ", ", q / 2, "]");
  return d;
}

// if (x > mod), let x = x - mod
// else, keep x = x
inline share* reduce_mod(BooleanCircuit& circ, share* x, share* mod) {
  share* sel_share = circ.PutGTGate(mod, x);
  x = circ.PutMUXGate(x, circ.PutSUBGate(x, mod), sel_share);
  return x;
}

// Splits a vector into num_splits nearly-equal-sized subvectors
// Returns vector of [start_idx, end_idx) for each split
inline std::vector<std::pair<size_t, size_t>> split_vector(size_t vector_size,
                                                           size_t num_splits) {
  std::vector<std::pair<size_t, size_t>> splits{num_splits};

  size_t split_size = std::floor(vector_size / num_splits);
  size_t splits_extra = vector_size - num_splits * split_size;
  std::vector<std::pair<size_t, size_t>> party_data_start_end_idx{
      num_splits, std::make_pair(0, 0)};
  for (size_t split_idx = 0; split_idx < num_splits; ++split_idx) {
    size_t start_idx = (split_idx > 0) ? splits[split_idx - 1].second : 0;
    size_t end_idx = start_idx + split_size;
    if (split_idx < splits_extra) {
      end_idx++;
    }

    if (split_idx == num_splits - 1) {
      end_idx = vector_size;
    }

    splits[split_idx] = std::make_pair(start_idx, end_idx);
  }

  return splits;
}

}  // namespace ngraph::runtime::aby
