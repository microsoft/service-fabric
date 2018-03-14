// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Serialization.h"

namespace Common
{
    struct VectorStream : public ByteStream<VectorStream>
    {
        VectorStream(std::vector<byte> & vector)
            : vector_(vector),
              iterator_(vector.begin()),
              isReadonly_(false)
        {
        }

        VectorStream(std::vector<byte> const & v)
            : vector_(*(const_cast<std::vector<byte>*>(&v))),
              iterator_(vector_.begin()),
              isReadonly_(true)
        {
        }

        void WriteBytes(void const * buf, size_t size);

        void ReadBytes(void * buf, size_t size);

        bool empty();

    private:
        DENY_COPY_ASSIGNMENT(VectorStream);

        std::vector<byte> & vector_;
        std::vector<byte>::iterator iterator_;
        bool isReadonly_;
    };
}
