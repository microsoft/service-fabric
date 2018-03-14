// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    template<MessageHeaderId::Enum id>
    struct MessageHeader
    {
        static const MessageHeaderId::Enum Id = id;
    };

#ifdef PLATFORM_UNIX
    // Need to keep compiler happy on Linux
    template <MessageHeaderId::Enum id> const MessageHeaderId::Enum MessageHeader<id>::Id;
#endif
}
