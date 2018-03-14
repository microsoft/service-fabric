// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceLocationNotificationRequestBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        ServiceLocationNotificationRequestBody()
        {
        }

        ServiceLocationNotificationRequestBody(std::vector<ServiceLocationNotificationRequestData> requests)
            : requests_(requests)
        {
        }
        
        __declspec(property(get=get_Requests)) std::vector<ServiceLocationNotificationRequestData> const & Requests;
        std::vector<ServiceLocationNotificationRequestData> const & get_Requests() const { return requests_; }

        std::vector<ServiceLocationNotificationRequestData> TakeRequestData() { return std::move(requests_); }

        FABRIC_FIELDS_01(requests_);

    private:
        std::vector<ServiceLocationNotificationRequestData> requests_;
    };
}
