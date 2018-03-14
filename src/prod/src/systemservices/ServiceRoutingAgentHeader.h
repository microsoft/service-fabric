// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class ServiceRoutingAgentHeader 
        : public Transport::ForwardMessageHeader
        , protected Transport::MessageHeader<Transport::MessageHeaderId::ServiceRoutingAgent>
    {
    public:
        using Transport::MessageHeader<Transport::MessageHeaderId::ServiceRoutingAgent>::Id;

        ServiceRoutingAgentHeader() : ForwardMessageHeader(), typeId_() { }

        ServiceRoutingAgentHeader(Transport::Actor::Enum const actor, std::wstring const & action, ServiceModel::ServiceTypeIdentifier const & typeId)
            : ForwardMessageHeader(actor, action)
            , typeId_(typeId)
        {
        }

        __declspec(property(get=get_TypeId)) ServiceModel::ServiceTypeIdentifier const & TypeId;
        ServiceModel::ServiceTypeIdentifier const & get_TypeId() const { return typeId_; };

        FABRIC_FIELDS_01(typeId_);

    private:
        ServiceModel::ServiceTypeIdentifier typeId_;
    };
}
