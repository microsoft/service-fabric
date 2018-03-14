// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class UnregisterServiceNotificationFilterRequestBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        UnregisterServiceNotificationFilterRequestBody() : clientId_(), filterId_() { }

        UnregisterServiceNotificationFilterRequestBody(UnregisterServiceNotificationFilterRequestBody && other) 
            : clientId_(std::move(other.clientId_))
            , filterId_(std::move(other.filterId_)) 
        { }

        UnregisterServiceNotificationFilterRequestBody(
            std::wstring const & clientId, 
            uint64 filterId) 
            : clientId_(clientId)
            , filterId_(filterId) 
        { }

        std::wstring && TakeClientId() { return std::move(clientId_); }
        uint64 GetFilterId() { return filterId_; }

        FABRIC_FIELDS_02(clientId_, filterId_);

    private:
        std::wstring clientId_;
        uint64 filterId_;
    };
}
