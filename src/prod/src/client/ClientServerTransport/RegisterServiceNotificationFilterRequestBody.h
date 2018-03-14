// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class RegisterServiceNotificationFilterRequestBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        RegisterServiceNotificationFilterRequestBody() : clientId_(), filter_() { }

        RegisterServiceNotificationFilterRequestBody(RegisterServiceNotificationFilterRequestBody && other) 
            : clientId_(std::move(other.clientId_))
            , filter_(std::move(other.filter_)) 
        { }

        RegisterServiceNotificationFilterRequestBody(
            std::wstring const & clientId, 
            Reliability::ServiceNotificationFilterSPtr const & filter) 
            : clientId_(clientId)
            , filter_(filter) 
        { }

        std::wstring && TakeClientId() { return std::move(clientId_); }
        Reliability::ServiceNotificationFilterSPtr && TakeFilter() { return std::move(filter_); }

        FABRIC_FIELDS_02(clientId_, filter_);

    private:
        std::wstring clientId_;
        Reliability::ServiceNotificationFilterSPtr filter_;
    };
}
