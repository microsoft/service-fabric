// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class TestServiceFactory : public Common::ComponentRoot
    {
        DENY_COPY(TestServiceFactory);

    public:
        TestServiceFactory(
            Federation::NodeId nodeId,
            Api::IClientFactoryPtr const &);

        virtual ~TestServiceFactory();

        __declspec (property(get=getSupportedStatelessServiceTypes)) std::vector<std::wstring> const & SupportedStatelessServiceTypes;
        __declspec (property(get=getSupportedStatefulServiceTypes)) std::vector<std::wstring> const & SupportedStatefulServiceTypes;
        std::vector<std::wstring> const & getSupportedStatelessServiceTypes() { return supportedStatelessServiceTypes_; }
        std::vector<std::wstring> const & getSupportedStatefulServiceTypes() { return supportedStatefulServiceTypes_; }

        CalculatorServiceSPtr CreateCalculatorService(FABRIC_PARTITION_ID paritionId, std::wstring serviceName, std::wstring initDataStr);
        TestStoreServiceSPtr CreateTestStoreService(FABRIC_PARTITION_ID paritionId, std::wstring serviceName, std::wstring initDataStr);
        TestPersistedStoreServiceSPtr CreateTestPersistedStoreService(FABRIC_PARTITION_ID, FABRIC_REPLICA_ID, std::wstring serviceName, std::wstring initDataStr);
        TXRServiceSPtr CreateTXRService(FABRIC_PARTITION_ID, FABRIC_REPLICA_ID, std::wstring serviceName, std::wstring initDataStr);
        TStoreServiceSPtr CreateTStoreService(FABRIC_PARTITION_ID, FABRIC_REPLICA_ID, std::wstring serviceName, std::wstring initDataStr);

        bool TryFindStoreService(std::wstring const& serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr);
        bool TryFindStoreService(std::wstring const & serviceName, Federation::NodeId const & nodeId, ITestStoreServiceSPtr & storeServiceSPtr);
        bool TryFindCalculatorService(std::wstring const & serviceName, Federation::NodeId const & nodeId, CalculatorServiceSPtr & calculatorServiceSPtr);

        bool TryFindScriptableService(__in std::wstring const & serviceLocation, __out ITestScriptableServiceSPtr & scriptableServiceSPtr);
        bool TryFindScriptableService(__in std::wstring const & serviceName, Federation::NodeId const & nodeId, __out ITestScriptableServiceSPtr & scriptableServiceSPtr);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec (property(get=get_ActiveServicesCopy)) std::map<std::wstring, TestServiceInfo> ActiveServicesCopy;
        std::map<std::wstring, TestServiceInfo> get_ActiveServicesCopy() const 
        {
            Common::AcquireWriteLock grab(lock_);
            return activeServices_;
        }

    private:
        bool ContainsServiceInfo(std::wstring const& location) const;
        bool TryGetServiceInfo(std::wstring const& location, TestServiceInfo & testServiceInfo) const;
        void ProcessOpenCallback(TestServiceInfo const & serviceInfo, Common::ComponentRootSPtr const& service, bool hasPersistedState);
        void ProcessCloseCallback(std::wstring const& location);
        void ProcessChangeRoleCallback(std::wstring const& oldLocation, std::wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole);

        //Called under lock
        void DeleteCalculatorService(std::wstring const& location);
        void DeleteTestStoreService(std::wstring const& location);
        void DeleteAtomicGroupService(std::wstring const& location);

        Federation::NodeId nodeId_;
        Api::IClientFactoryPtr clientFactory_;

        std::map<std::wstring, TestServiceInfo> activeServices_;
        std::vector<ITestStoreServiceSPtr> storeServices_;
        std::vector<CalculatorServiceSPtr> calculatorServices_;
        std::vector<ITestScriptableServiceSPtr> scriptableServices_;
        //Single shared lock for all three structures. 
        mutable Common::RwLock lock_;

        std::vector<std::wstring> supportedStatelessServiceTypes_;
        std::vector<std::wstring> supportedStatefulServiceTypes_;

    };
};
