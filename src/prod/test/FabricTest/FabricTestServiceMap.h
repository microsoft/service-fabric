// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestServiceMap
    {
        DENY_COPY(FabricTestServiceMap)

    public:     
        FabricTestServiceMap() 
        {
        }

        void AddPartitionedServiceDescriptor(std::wstring const& name, Naming::PartitionedServiceDescriptor && partitionedServiceDescriptor, bool overWriteExisting = false);

        void UpdatePartitionedServiceDescriptor(std::wstring const& serviceName, Reliability::ServiceDescription const &);

        void UpdatePartitionedServiceDescriptor(std::wstring const& serviceName, Naming::PartitionedServiceDescriptor &&);

        void MarkServiceAsDeleted(std::wstring const& name, bool verifyExists = true);

        bool IsStatefulService(std::wstring const& name);

        bool IsCreatedService(std::wstring const& name);

        bool IsDeletedService(std::wstring const& name);

        int GetTargetReplicaForService(std::wstring const& name);

        int GetMinReplicaSetSizeForService(std::wstring const& serviceName);

        void GetAdhocStatefulServices(std::vector<std::wstring> & services);

        size_t GetBlockListSize(std::wstring const& serviceName);

        void GetCreatedServiceNames(std::vector<std::wstring> & names);
        void GetDeletedServiceNames(std::vector<std::wstring> & names);

        void GetCreatedServicesDescription(std::vector<Reliability::ServiceDescription> & serviceDescriptions);

        Naming::PartitionedServiceDescriptor GetServiceDescriptor(std::wstring const& name);

        Common::Guid GetFUIdAt(std::wstring const& serviceName, int index);

        ServiceModel::ServiceTypeIdentifier GetServiceType(std::wstring const & appName, std::wstring const & typeName);

        void MarkAppAsDeleted(std::wstring const& appName);

        bool IsBlockedNode(std::wstring const& serviceName, Federation::NodeId nodeId);

        bool HasPersistedState(std::wstring const& name);

        Common::Guid ResolveFuidForKey(std::wstring const& serviceName, __int64 key);

        void RecalculateBlockList();

        std::vector<std::wstring> GetReportMetricNames(std::wstring const& serviceName);

        void GetAllFUs(std::vector<Common::Guid> & fus);
        void GetAllFUsForService(std::wstring const& serviceName, std::vector<Common::Guid> & fus);

    private:

        class TestServiceData;
        typedef std::shared_ptr<TestServiceData> TestServiceDataSPtr;
        std::map<std::wstring, TestServiceDataSPtr> serviceDataMap_;
        Common::RwLock serviceDataMapLock_;
    };
};
