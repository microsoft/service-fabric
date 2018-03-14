// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class FabricNodeConfigOverride
    {
        DENY_COPY(FabricNodeConfigOverride);

    public:
        FabricNodeConfigOverride(
            std::wstring && nodeUpgradeDomainId,
            Common::Uri && nodeFaultDomainId,
            std::wstring && systemServiceInitializationTimeout,
            std::wstring && systemServiceInitializationRetryInterval);

        std::wstring NodeUpgradeDomainId;
        Common::Uri NodeFaultDomainId;
        std::wstring SystemServiceInitializationTimeout;
        std::wstring SystemServiceInitializationRetryInterval;
    };

    class FabricNodeWrapper 
        : public INodeWrapper
        , public std::enable_shared_from_this<FabricNodeWrapper>
    {
        DENY_COPY(FabricNodeWrapper)

    public:

        FabricNodeWrapper(
            FabricVersionInstance const& fabVerInst,
            Federation::NodeId nodeId, 
            FabricNodeConfigOverride const& configOverride,
            Common::FabricNodeConfig::NodePropertyCollectionMap const & nodeProperties,
            Common::FabricNodeConfig::NodeCapacityCollectionMap const & nodeCapacities,
            std::vector<std::wstring> const & nodeCredentials,
            std::vector<std::wstring> const & clusterCredentials,
            bool checkCert);

        virtual ~FabricNodeWrapper();
        
        void GetRuntimeManagerUnderLock(std::function<void(Hosting2::FabricRuntimeManager &)> processor) const
        {
            Common::AcquireReadLock lock(fabricNodeLock_);
            TestSession::FailTestIfNot(fabricNode_ != nullptr, "FabricNode {0} should be opened before calling this GetRuntimeManagerUnderLock", nodeId_);

            // The fabric node is constructed with the TestHostingSubsystem which is a wrapper around HostingSubsystem
            processor(*(GetHostingSubsystem().FabricRuntimeManagerObj));
        }

        TestHostingSubsystem & GetTestHostingSubsystem() const
        {
            return dynamic_cast<TestHostingSubsystem&>(fabricNode_->Test_Hosting);
        }

        Hosting2::HostingSubsystem & GetHostingSubsystem() const
        {
            return GetTestHostingSubsystem().InnerHosting;
        }

        void GetReliabilitySubsystemUnderLock(std::function<void(ReliabilityTestApi::ReliabilityTestHelper &)> processor) const;

        ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFailoverManager() const;

        ReliabilityTestApi::FailoverManagerComponentTestApi::FailoverManagerTestHelperUPtr GetFailoverManagerMaster() const;

        std::shared_ptr<Management::ClusterManager::ClusterManagerReplica> GetCM();

        Management::HealthManager::HealthManagerReplicaSPtr GetHM();

        __declspec (property(get=get_IsOpened)) bool IsOpened;
        bool get_IsOpened() const
        { 
            Common::AcquireReadLock lock(fabricNodeLock_);
            return fabricNode_ == nullptr ? false : (fabricNode_->State.Value == FabricComponentState::Opened);
        }

        __declspec (property(get=get_IsOpening)) bool IsOpening;
        bool get_IsOpening() const
        { 
            Common::AcquireReadLock lock(fabricNodeLock_);
            return fabricNode_ == nullptr ? false : (fabricNode_->State.Value == FabricComponentState::Opening);
        }

        __declspec (property(get=get_Instance)) Federation::NodeInstance Instance;
        Federation::NodeInstance get_Instance() const
        { 
            Common::AcquireReadLock lock(fabricNodeLock_);
            return fabricNode_ == nullptr ? Federation::NodeInstance() :fabricNode_->Instance;
        }

        __declspec (property(get=get_Id)) Federation::NodeId Id;
        Federation::NodeId get_Id() const
        { 
            return nodeId_;
        }

        __declspec (property(get=get_RuntimeManager)) std::unique_ptr<FabricTestRuntimeManager> const& RuntimeManager;
        std::unique_ptr<FabricTestRuntimeManager> const& get_RuntimeManager() const
        { 
            return runtimeManager_;
        }

        __declspec (property(get=get_Node)) Fabric::FabricNodeSPtr const & Node;
        Fabric::FabricNodeSPtr const & get_Node() const { return fabricNode_; }
        
        __declspec(property(get=get_NodeOpenTimestamp)) Common::StopwatchTime NodeOpenTimestamp;
        Common::StopwatchTime get_NodeOpenTimestamp() const { return nodeOpenTimestamp_; }

        void Open(
            Common::TimeSpan, 
            bool expectNodeFailure, 
            bool addRuntime, 
            Common::ErrorCodeValue::Enum openErrorCode,
            std::vector<Common::ErrorCodeValue::Enum> const& retryErrors);

        void Close(bool removeData = false) override;
        void Abort();

        __declspec (property(get=get_NodePropertyMap)) std::map<std::wstring, std::wstring> const& NodeProperties;
        std::map<std::wstring, std::wstring> const& get_NodePropertyMap() const { return nodeProperties_; }

        __declspec (property(get=get_FabricNodeConfig)) Common::FabricNodeConfigSPtr const & Config;
        Common::FabricNodeConfigSPtr const & get_FabricNodeConfig() const { return fabricNodeConfigSPtr_; }

        __declspec (property(get=get_MutableFabricNodeConfig)) Common::FabricNodeConfigSPtr & MutableConfig;
        Common::FabricNodeConfigSPtr & get_MutableFabricNodeConfig() { return fabricNodeConfigSPtr_; }

        void SetFabricNodeConfigEntry(std::wstring configEntryName, std::wstring configEntryValue, bool updateStore = false);     
        template <typename TKey, typename TValue>
        void SetConfigSection(ConfigSection & configSection, Common::ConfigCollection<TKey, TValue> sectionContents);          
        void SetNodeDomainIdsConfigSection(std::wstring nodeUpgradeDomainIdKey, std::wstring nodeUpgradeDomainId, Common::FabricNodeConfig::NodeFaultDomainIdCollection faultDomainIds);

    private:
        void SetSharedLogEnvironmentVariables();

        void OpenInternal(
            Common::TimeSpan, 
            bool expectNodeFailure, 
            bool addRuntime, 
            Common::ErrorCodeValue::Enum openErrorCode,
            std::vector<Common::ErrorCodeValue::Enum> const& retryErrors);

        void AbortInternal();

        std::wstring GetFabricNodeConfigEntryValue(std::wstring configEntryName);

        static void ClientClaimsHandler(std::wstring const & claims, Transport::SecurityContextSPtr const & context);
        mutable Common::RwLock fabricNodeLock_;
        mutable Fabric::FabricNodeSPtr fabricNode_;
        std::map<std::wstring, std::wstring> nodeProperties_;

        bool isFirstCreate_;
        ConfigSection fabricNodeConfigSection_;
        ConfigSection nodePropertiesSection_;
        ConfigSection nodeCapacitySection_;
        ConfigSection nodeFaultDomainIdsSection_;
        ConfigSettings configSettings_;   
        std::shared_ptr<ConfigSettingsConfigStore> configStoreSPtr_;
        FabricNodeConfigSPtr fabricNodeConfigSPtr_;

        Common::ErrorCode fabricNodeCreateError_;

        Common::StopwatchTime nodeOpenTimestamp_;

        Federation::NodeId nodeId_;
        int retryCount_;

        mutable Common::RwLock nodeRebootLock_;
        mutable Common::ManualResetEvent nodeOpenCompletedEvent_;        
        friend class TestFabricUpgradeHostingImpl;

        std::unique_ptr<FabricTestRuntimeManager> runtimeManager_;
    };
};
