// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceDirectMessagingHeader 
        : public Transport::ForwardMessageHeader
        , protected Transport::MessageHeader<Transport::MessageHeaderId::ServiceDirectMessaging>
    {
    public:
        using Transport::MessageHeader<Transport::MessageHeaderId::ServiceDirectMessaging>::Id;

        ServiceDirectMessagingHeader() : ForwardMessageHeader() { }

        ServiceDirectMessagingHeader(Transport::Actor::Enum const actor, std::wstring const & action)
            : ForwardMessageHeader(actor, action)
        {
        }
    };
}
