// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability::ReplicationComponent;

namespace FabricTest
{
    class SGStatelessService;
    typedef std::shared_ptr<SGStatelessService> SGStatelessServiceSPtr;

    class SGStatelessService : 
        public Common::ComponentRoot
    {
        DENY_COPY(SGStatelessService)

    public:
        //
        // Constructor/Destructor.
        //
        SGStatelessService(
            __in NodeId nodeId,
            __in LPCWSTR serviceType,
            __in LPCWSTR serviceName,
            __in ULONG initializationDataLength,
            __in const byte* initializationData,
            __in FABRIC_INSTANCE_ID instanceId);
        virtual ~SGStatelessService();
        
        HRESULT OnOpen(IFabricStatelessServicePartition *servicePartition);
        HRESULT OnClose();
        void OnAbort();
        bool ShouldFailOn(ApiFaultHelper::ComponentName compName, std::wstring operationName, ApiFaultHelper::FaultType faultType=ApiFaultHelper::Failure);

        //
        // Properties
        //
        __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
        std::wstring const & get_ServiceName() { return serviceName_; }

        __declspec(property(get=get_Partition)) ComPointer<IFabricStatelessServicePartition> const & Partition;
        ComPointer<IFabricStatelessServicePartition> const & get_Partition() { return partition_; }

        static std::wstring const StatelessServiceType;

    private:

        void StopWorkLoad();
        void ReportMetricsRandom();

        static void CALLBACK ThreadPoolWorkItemReportMetrics(
            __inout PTP_CALLBACK_INSTANCE Instance,
            __inout_opt PVOID Context,
            __inout PTP_WORK Work);

        PTP_WORK workItemReportMetrics_;
        Common::ComponentRootSPtr selfReportMetrics_;

        NodeId nodeId_;
        std::wstring serviceName_;
        std::wstring serviceType_;
        FABRIC_INSTANCE_ID instanceId_;
        
        Common::ComPointer<IFabricStatelessServicePartition> partition_;

        static Common::StringLiteral const TraceSource;

        Common::Random random_;
        std::unique_ptr<TestCommon::CommandLineParser> settings_;
    };
}
