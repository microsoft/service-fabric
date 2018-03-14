// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class FabricTestNode : public std::enable_shared_from_this<FabricTestNode>
    {
        class CodePackageData
        {
        public:
            CodePackageData() : toBeKilled_(false){}

            __declspec (property(get=get_Context, put=put_Context)) TestCodePackageContext const& Context;
            TestCodePackageContext const& get_Context() const {return context_;}
            void put_Context(TestCodePackageContext const& value) { context_ = value; }

            __declspec (property(get=get_Description, put=put_Description)) ServiceModel::ServiceManifestDescription const& Description;
            ServiceModel::ServiceManifestDescription const& get_Description() const {return description_;}
            void put_Description(ServiceModel::ServiceManifestDescription const& value) { description_ = value; }

            __declspec(property(get=get_PlacedServices, put=put_PlacedServices)) std::map<std::wstring, TestServiceInfo> const& PlacedServices;
            std::map<std::wstring, TestServiceInfo> const & get_PlacedServices() const { return placedServices_; };
            void put_PlacedServices(std::map<std::wstring, TestServiceInfo> const& value) { placedServices_ = value; }

            __declspec(property(get=get_StoreServices, put=put_StoreServices)) map<wstring, ITestStoreServiceSPtr> const& StoreServices;
            map<wstring, ITestStoreServiceSPtr> const& get_StoreServices() const { return storeServices_; };
            void put_StoreServices(map<wstring, ITestStoreServiceSPtr> && value) { storeServices_ = move(value); }

            __declspec (property(get=get_ToBeKilled, put=put_ToBeKilled)) bool ToBeKilled;
            bool get_ToBeKilled() const {return toBeKilled_;}
            void put_ToBeKilled(bool value) { toBeKilled_ = value; }

        private:

            TestCodePackageContext context_;
            std::map<std::wstring, TestServiceInfo> placedServices_;
            ServiceModel::ServiceManifestDescription description_;
            map<wstring, ITestStoreServiceSPtr> storeServices_;
            bool toBeKilled_;
        };

        DENY_COPY(FabricTestNode)
            
    public:

        FabricTestNode(
            Federation::NodeId nodeId, 
            FabricNodeConfigOverride const& configOverride,
            Common::FabricNodeConfig::NodePropertyCollectionMap const & nodeProperties,
            Common::FabricNodeConfig::NodeCapacityCollectionMap const & nodeCapacities,
            std::vector<std::wstring> const & nodeCredentials,
            std::vector<std::wstring> const & clusterCredentials,
            bool checkCert);

        ~FabricTestNode();

        __declspec (property(get=get_NodeWrapper)) std::shared_ptr<INodeWrapper> const& Node;
        std::shared_ptr<INodeWrapper> const& get_NodeWrapper() const
        { 
            return nodeWrapper_;
        }

        __declspec (property(get=get_RuntimeManager)) std::unique_ptr<FabricTestRuntimeManager> const& RuntimeManager;
        std::unique_ptr<FabricTestRuntimeManager> const& get_RuntimeManager() const
        { 
            return nodeWrapper_->RuntimeManager;
        }

        __declspec (property(get=get_Id)) Federation::NodeId Id;
        Federation::NodeId get_Id() const
        { 
            return nodeWrapper_->Id;
        }

        __declspec (property(get=get_ActiveCodePackagesCopy)) std::map<std::wstring, CodePackageData> ActiveCodePackagesCopy;
        std::map<std::wstring, CodePackageData> get_ActiveCodePackagesCopy() const
        { 
            Common::AcquireWriteLock grab(activeHostsLock_);
            return activeCodePackages_;
        }

        __declspec(property(get=get_IsActivated,put=set_IsActivated)) bool IsActivated;
        bool get_IsActivated() const { return isActivated_; }
        void set_IsActivated(bool value) { isActivated_ = value; }

        void FailFastOnAllCodePackages();

        void ProcessServicePlacementData(ServicePlacementData const&);
        void ProcessUpdateServiceManifest(TestCodePackageData const& codePackageData);
        
        void ProcessCodePackageHostInit(TestCodePackageContext const& context);
        void ProcessCodePackageHostFailure(TestCodePackageContext const& context);

        void ProcessMultiCodePackageHostInit(TestMultiPackageHostContext const& context);
        void ProcessMultiCodePackageHostFailure(TestMultiPackageHostContext const& context);

        bool KillCodePackage(std::wstring const& codePackageId);
        bool KillHostAt(std::wstring const& codePackageId);
        bool MarkKillPending(std::wstring const& codePackageId);

        void Open(
            Common::TimeSpan, 
            bool expectNodeFailure, 
            bool addRuntime, 
            Common::ErrorCodeValue::Enum openErrorCode,
            std::vector<Common::ErrorCodeValue::Enum> const& retryErrors);       
        void Close(bool removeData = false);
        void Abort();

        void PrintActiveServices() const;

        void RegisterServiceForVerification(TestServiceInfo const&);
        bool VerifyAllServices() const;
        void ClearServicePlacementList();

        bool TryFindStoreService(std::wstring const & serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr);
        bool TryFindStoreService(std::wstring const & serviceName, Federation::NodeId const & nodeId, ITestStoreServiceSPtr & storeServiceSPtr);
        bool TryFindCalculatorService(std::wstring const & serviceName, Federation::NodeId const & nodeId, CalculatorServiceSPtr & calculatorServiceSPtr);
        bool TryFindScriptableService(__in std::wstring const & serviceLocation, __out ITestScriptableServiceSPtr & scriptableServiceSPtr);
        bool TryFindScriptableService(__in std::wstring const & serviceName, Federation::NodeId const & nodeId, __out ITestScriptableServiceSPtr & scriptableServiceSPtr);

        bool TryGetFabricNodeWrapper(std::shared_ptr<FabricNodeWrapper> & fabricNodeWrapper) const;

		void VerifyImageCacheFiles(std::set<wstring> const & imageStoreFiles);
		void VerifyApplicationInstanceAndSharedPackageFiles(std::set<wstring> const & imageStoreFiles);

        ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentProxyTestHelperUPtr GetProxyForStatefulHost();

        void GetResourceUsageFromDeployedPackages(double & cpuUsage, double & memoryUsage);

    private:

        mutable std::vector<TestServiceInfo> servicesFromGFUM_;

        std::shared_ptr<INodeWrapper> nodeWrapper_;
        
        mutable Common::ExclusiveLock activeHostsLock_;
        std::map<std::wstring, CodePackageData> activeCodePackages_;

        class MultiCodePackageHostData;
        std::map<std::wstring, MultiCodePackageHostData> activeMultiCodePackageHosts_;

        bool isActivated_;

		void VerifyDeployedServicePackages(std::vector<Hosting2::ServicePackage2SPtr> deployedServicePackages, std::set<wstring> const & imageStoreFiles);

		void VerifyPackage(
			ServiceModel::ApplicationIdentifier const & appId,
			std::wstring const & serviceManifestName,
			std::wstring const & packageName, 
			std::wstring const & packageVersion, 
			bool const isShared,
			std::set<wstring> const & imageStoreFiles);

        bool VerifyNamingServicePlacement(
            std::vector<TestServiceInfo> & activeGFUMServices, 
            Naming::ActiveServiceMap const &) const;

        bool VerifyServicePlacement(
            std::vector<TestServiceInfo> & activeGFUMServices, 
            std::map<std::wstring, TestServiceInfo> const& placementData) const;

        void ProcessCodePackageHostFailureInternal(TestCodePackageContext const& context);        
    };

    typedef std::shared_ptr<FabricTestNode> FabricTestNodeSPtr;
};
