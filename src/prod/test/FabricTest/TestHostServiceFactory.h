// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestHostSession;

    class TestHostServiceFactory : public Common::ComponentRoot
    {
          DENY_COPY(TestHostServiceFactory);

    public:
        TestHostServiceFactory(
            std::wstring const& traceId, 
            std::wstring const& codePackageId, 
            std::wstring const& appId, 
            std::wstring const& packageName, 
            std::wstring const& workingDir,
            Transport::SecuritySettings const& clientSecuritySettings);

        virtual ~TestHostServiceFactory();

        CalculatorServiceSPtr CreateStatelessService(FABRIC_PARTITION_ID partitionId, std::wstring const& serviceName, std::wstring const& serviceType, std::wstring const& initDataStr);
        TestStoreServiceSPtr CreateStatefulService(FABRIC_PARTITION_ID partitionId, std::wstring const& serviceName, std::wstring const& serviceType, std::wstring const& initDataStr);
        TestPersistedStoreServiceSPtr CreatePersistedService(
            FABRIC_PARTITION_ID, 
            FABRIC_REPLICA_ID, 
            std::wstring const& serviceName, 
            std::wstring const& serviceType,
            std::wstring const& initDataStr);

        bool TryFindStoreService(std::wstring const& serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec (property(get=get_ActiveServicesCopy)) std::map<std::wstring, TestServiceInfo> ActiveServices;
        std::map<std::wstring, TestServiceInfo> get_ActiveServicesCopy() const 
        {
            Common::AcquireWriteLock grab(lock_);
            return activeServices_;
        }

    private:
        void InitializeSharedLogSettings();
        bool ContainsServiceInfo(std::wstring const& location) const;
        bool TryGetServiceInfo(std::wstring const& location, TestServiceInfo & testServiceInfo) const;
        void ProcessOpenCallback(TestServiceInfo const & serviceInfo, Common::ComponentRootSPtr const& service, bool hasPersistedState);
        void ProcessCloseCallback(std::wstring const& location);
        void ProcessChangeRoleCallback(std::wstring const& serviceLocation, FABRIC_REPLICA_ROLE replicaRole);
        
        std::wstring traceId_;
        std::wstring codePackageId_;
        std::wstring appId_;
        std::wstring packageName_;
        std::wstring workingDir_;

        std::map<std::wstring, TestServiceInfo> activeServices_;
        std::vector<ITestStoreServiceSPtr> storeServices_;
        mutable Common::RwLock lock_;

        Api::IClientFactoryPtr clientFactory_;
    };

    typedef std::shared_ptr<TestHostServiceFactory> TestHostServiceFactorySPtr;
};
