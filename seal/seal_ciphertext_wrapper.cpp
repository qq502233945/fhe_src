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

#include "seal/seal_ciphertext_wrapper.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "logging/ngraph_he_log.hpp"
#include "ngraph/check.hpp"
#include "protos/message.pb.h"
#include "seal/seal.h"

namespace ngraph::runtime::he {

SealCiphertextWrapper::SealCiphertextWrapper() = default;

void SealCiphertextWrapper::save(pb::HEType& he_type) const {
  size_t cipher_size = ciphertext_size(m_ciphertext);
  std::string cipher_str;
  cipher_str.resize(cipher_size);

  size_t save_size = ngraph::runtime::he::save(
      m_ciphertext, reinterpret_cast<std::byte*>(cipher_str.data()));

  NGRAPH_CHECK(save_size == cipher_size, "Save size != cipher size");

  he_type.set_ciphertext(std::move(cipher_str));
}

void SealCiphertextWrapper::load(SealCiphertextWrapper& dst,
                                 const pb::HEType& pb_he_type,
                                 std::shared_ptr<seal::SEALContext> context) {
  NGRAPH_CHECK(!pb_he_type.is_plaintext(),
               "Cannot load ciphertext from plaintext HEType");

  const std::string& cipher_str = pb_he_type.ciphertext();
  ngraph::runtime::he::load(
      dst.ciphertext(), std::move(context),
      reinterpret_cast<const std::byte*>(cipher_str.data()), cipher_str.size());
}

}  // namespace ngraph::runtime::he
