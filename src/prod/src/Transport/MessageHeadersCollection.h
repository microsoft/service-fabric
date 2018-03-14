// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    //
    //
    // MessageHeadersCollection type represents an immutable collection of MessageHeaders. 
    // This type is used to specify target specific MessageHeaders in multicast communication.
    //
    // TODO: implementation of iterator pattern?
    class MessageHeadersCollection
    {
        DENY_COPY(MessageHeadersCollection)

    public:
        MessageHeadersCollection(size_t size);
        MessageHeadersCollection(MessageHeadersCollection&& other);
        virtual ~MessageHeadersCollection();

        size_t size();
        MessageHeaders& operator[](size_t index);

    private:
        std::unique_ptr<std::vector<std::unique_ptr<MessageHeaders>>> headers_;
    };
}
