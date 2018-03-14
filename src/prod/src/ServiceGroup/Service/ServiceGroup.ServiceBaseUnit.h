// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricClient.h"

#include "ServiceGroup.ServiceBase.h"

namespace ServiceGroup
{
    //
    // Base class for stateless and stateful service group units.
    //
    class CBaseServiceUnit : 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupCommon>
    {
    
        DENY_COPY(CBaseServiceUnit)
        
    public:
        //
        // Constructor/Destructor.
        //
        CBaseServiceUnit(__in FABRIC_PARTITION_ID partitionId);
        virtual ~CBaseServiceUnit(void);

        //
        // Properties.
        //
        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        inline std::wstring const & get_ServiceName() { return serviceName_; }

    protected:
        //
        // Concurrency control.
        //
        Common::RwLock lock_;
        //
        // The service partition id. System provided.
        //
        Common::Guid partitionId_;
        //
        // Service name.
        //
        std::wstring serviceName_; 
        //
        // Flags that the service replica or instance has been successfully opened.
        //
        BOOLEAN opened_;
        //
        // Internal service group load metric reporting.
        //
        IServiceGroupReport* outerServiceGroupReport_;
        //
        // Set during report fault permanent. Guards the operation streams against being reconstructed.
        //
        BOOLEAN isFaulted_;
    };
}
