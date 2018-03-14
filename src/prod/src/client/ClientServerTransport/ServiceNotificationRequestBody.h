// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceNotificationRequestBody : public ServiceModel::ClientServerMessageBody
    {
        DEFAULT_COPY_ASSIGNMENT(ServiceNotificationRequestBody)
        DECLARE_SERIALIZATION_OVERHEAD_ESTIMATE()

    public:
        ServiceNotificationRequestBody() 
            : notification_()
        { 
        }

        ServiceNotificationRequestBody(ServiceNotificationRequestBody && other)
            : notification_(std::move(other.notification_))
        {
        }

        ServiceNotificationRequestBody(ServiceNotificationRequestBody const & other)
            : notification_(other.notification_)
        {
        }

        ServiceNotificationRequestBody(
            ServiceNotificationSPtr && notification)
            : notification_(std::move(notification))
        {
        }

        ServiceNotificationSPtr && TakeNotification() { return std::move(notification_); }

        FABRIC_FIELDS_01(notification_);
        
    private:
        ServiceNotificationSPtr notification_;
    };

    typedef std::shared_ptr<ServiceNotificationRequestBody> ServiceNotificationRequestBodySPtr;
}
