// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceNotification
    {
    public:
        virtual ~IServiceNotification() {};

        virtual void GetNotification(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_NOTIFICATION & notification) const = 0;

        virtual Common::RootedObjectPointer<IServiceEndpointsVersion> GetVersion() const = 0;

        virtual Reliability::ServiceTableEntry const & GetServiceTableEntry() const = 0;
    };
}
