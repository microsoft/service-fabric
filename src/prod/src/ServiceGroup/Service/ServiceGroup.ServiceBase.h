// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricClient.h"

namespace ServiceGroup
{
    //
    // Internal load reporting metric interface.
    //
    static const GUID IID_IServiceGroupReport = {0xf168cf35,0xec9d,0x4662,{0xb5,0x79,0x46,0x47,0x1b,0xe2,0xc5,0x27}};
    struct IServiceGroupReport : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE ReportLoad(__in FABRIC_PARTITION_ID partitionId, __in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics) = 0;
        virtual HRESULT STDMETHODCALLTYPE ReportFault(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_FAULT_TYPE faultType) = 0;
        virtual HRESULT STDMETHODCALLTYPE ReportMoveCost(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_MOVE_COST MoveCost) = 0;
    };
    //
    // Base class for stateless and stateful service groups.
    //
    class CBaseServiceGroup : 
        public IServiceGroupReport,
        public Common::ComUnknownBase, 
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupCommon>
    {
        DENY_COPY(CBaseServiceGroup)
    
    public:
        //
        // Constructor/Destructor.
        //
        CBaseServiceGroup(void);
        virtual ~CBaseServiceGroup(void);

    protected:
        //
        // Refreshes load metric information based on new data.
        //
        HRESULT UpdateLoadMetrics(
            __in FABRIC_PARTITION_ID partitionId, 
            __in ULONG metricCount, 
            __in const FABRIC_LOAD_METRIC* metrics,
            __out FABRIC_LOAD_METRIC** computedMetrics);

    protected:
        //
        // Concurrency control.
        //
        Common::RwLock lock_;
        //
        // Partition id of this service group. This is provided by the system partition.
        //
        Common::Guid partitionId_;
        //
        // Partition id of this service group. This is provided by the system partition.
        //
        std::wstring partitionIdString_;
        //
        // Map of service instances to service ids.
        //
        std::map<std::wstring, Common::Guid> serviceInstanceNames_;
        //
        // Map of service units to map of metrics.
        //
        std::map<Common::Guid, std::map<std::wstring, ULONG>> loadReporting_;
        //
        // Common typedefs.
        //
        typedef std::pair<std::map<std::wstring, Common::Guid>::iterator, bool> ServiceInstanceName_ServiceId_Insert;
        typedef std::pair<std::wstring, Common::Guid> ServiceInstanceName_ServiceId_Pair;
        typedef std::pair<Common::Guid, std::map<std::wstring, ULONG>> Service_Metric_Pair;
        typedef std::pair<std::wstring, ULONG> MetricName_MetricValue_Pair;
    };
}
