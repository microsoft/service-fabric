// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceTableEntryNotification 
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(ServiceTableEntryNotification)

    public:
        ServiceTableEntryNotification()
            : ste_()
            , matchedPrimaryOnly_(false)
        {
        }

        ServiceTableEntryNotification(
            ServiceTableEntrySPtr const & ste,
            bool matchedPrimaryOnly)
            : ste_(ste)
            , matchedPrimaryOnly_(matchedPrimaryOnly)
        {
        }

        ServiceTableEntrySPtr const & GetPartition() { return ste_; }
        bool IsMatchedPrimaryOnly() { return matchedPrimaryOnly_; }

        FABRIC_FIELDS_02(ste_, matchedPrimaryOnly_);
        
        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(ste_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ServiceTableEntrySPtr ste_;
        bool matchedPrimaryOnly_;
    };

    typedef std::shared_ptr<ServiceTableEntryNotification> ServiceTableEntryNotificationSPtr;
}
