// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    typedef std::function<void(TestServiceInfo const&, Common::ComponentRootSPtr const&, bool hasPersistedState)> OpenCallback;
    typedef std::function<void(std::wstring const&)> CloseCallback;

    class FabricTestSession;

    class CalculatorService : public Common::ComponentRoot
    {
        DENY_COPY(CalculatorService);

    public:
        explicit CalculatorService(
            Federation::NodeId nodeId,
            FABRIC_PARTITION_ID partitionId,
            std::wstring const& serviceName,
            std::wstring const& initDataStr,
            std::wstring const& serviceType = CalculatorService::DefaultServiceType,
            std::wstring const& appId = L"");

        virtual ~CalculatorService();

        Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatelessServicePartition3> const& partition);

        Common::ErrorCode OnClose();
        void OnAbort();

        OpenCallback OnOpenCallback;
        CloseCallback OnCloseCallback;

        virtual std::wstring const & GetServiceName() const { return serviceName_; }
        virtual Federation::NodeId const & GetNodeId() const { return nodeId_; }
        Common::ComPointer<IFabricStatelessServicePartition3> const& GetPartition() const { return partition_; }

        virtual void ReportFault(::FABRIC_FAULT_TYPE faultType) const 
        { 
            partitionWrapper_.ReportFault(faultType);
        }
        
        virtual Common::ErrorCode ReportPartitionHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            return  partitionWrapper_.ReportPartitionHealth(healthInfo, sendOptions);
        }

        virtual Common::ErrorCode ReportInstanceHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            return partitionWrapper_.ReportInstanceHealth(healthInfo, sendOptions);
        }

        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const
        {
            return partitionWrapper_.GetWriteStatus();
        }

        virtual FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const
        {
            return partitionWrapper_.GetReadStatus();
        }

        virtual void ReportLoad(ULONG metricCount, ::FABRIC_LOAD_METRIC const *metrics) const
        { 
            return partitionWrapper_.ReportLoad(metricCount, metrics);
        }

        virtual void ReportMoveCost(FABRIC_MOVE_COST moveCost)
        {
            return partitionWrapper_.ReportMoveCost(moveCost);
        }

        __declspec (property(get=getServiceLocation)) std::wstring const& ServiceLocation;
        std::wstring const& getServiceLocation() { return serviceLocation_; }

        bool ShouldFailOn(ApiFaultHelper::ComponentName compName, std::wstring operationName, ApiFaultHelper::FaultType faultType=ApiFaultHelper::Failure)
        {
            return ApiFaultHelper::Get().ShouldFailOn(nodeId_, serviceName_, compName, operationName, faultType);
        }

        static std::wstring const DefaultServiceType;
    private:
        std::wstring serviceName_;
        std::wstring partitionId_;
        std::wstring serviceLocation_;
        bool isOpen_;
        StatelessPartitionWrapper<IFabricStatelessServicePartition3> partitionWrapper_;
        Common::ComPointer<IFabricStatelessServicePartition3> partition_;
        std::wstring serviceType_;
        std::wstring appId_;
        Federation::NodeId nodeId_;
        std::wstring initDataStr_;
    };

    typedef std::shared_ptr<CalculatorService> CalculatorServiceSPtr;
};
