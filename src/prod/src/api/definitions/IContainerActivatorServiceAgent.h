// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IContainerActivatorServiceAgent
    {
    public:
        virtual ~IContainerActivatorServiceAgent() {};
        
        virtual void Release() = 0;

        virtual void ProcessContainerEvents(
            Hosting2::ContainerEventNotification && notification) = 0;

        virtual Common::ErrorCode RegisterContainerActivatorService(
            IContainerActivatorServicePtr const & activatorService) = 0;
    };
}
