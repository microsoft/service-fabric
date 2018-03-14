// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Transport
{
    MessageHeadersCollection::MessageHeadersCollection(size_t size)
            : headers_(make_unique<vector<unique_ptr<MessageHeaders>>>(size))
    {
    }

    MessageHeadersCollection::MessageHeadersCollection(MessageHeadersCollection&& other)
        : headers_(std::move(other.headers_))
    {
    }

    MessageHeadersCollection::~MessageHeadersCollection()
    {
    }

    size_t MessageHeadersCollection::size()
    {
        return headers_->size();
    }
        
    MessageHeaders& MessageHeadersCollection::operator[](size_t index)
    {
        if ((*headers_)[index].get() == 0)
        {
            (*headers_)[index] = make_unique<MessageHeaders>();
        }

        return *(*headers_)[index].get();
    }
}
