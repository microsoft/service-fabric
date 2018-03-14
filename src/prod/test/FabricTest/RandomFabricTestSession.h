// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class RandomFabricTestSession: public FabricTestSession
    {
        DENY_COPY(RandomFabricTestSession)
    public:

        static void CreateSingleton(int maxIterations, int timeout, std::wstring const& label, bool autoMode, std::wstring const& serverListenAddress = FederationTestCommon::AddressHelper::GetServerListenAddress());
        std::wstring GetInput();

        static std::wstring GenerateAppName();

    private:

        RandomFabricTestSession(int maxIterations, int timeout, std::wstring const& label, bool autoMode, std::wstring const& serverListenAddress);

        void AssignNodeIds();
        void SetupConfiguration();
        void SetupRing();
        void SetupUnreliableTransport();
        void SetupForServicesAndApplications();
        void EnsureAllServicesAndApplicationsAreCreated();
        void SetupWatchDogs();

        bool TryRebuild();
        void CauseFullRebuild();
        bool TryUpgrade();
        void UpgradeApplication();
        void RemoveDeletedApplicationFolders();

        void ExecuteDynamismStage();
        void InduceSlowCopyInPersistedTestStoreService();
        void GenerateNodeRGCapacities();
        void AddNode();
        void AddNode(Common::LargeInteger const& node);
        void ActivateNode(Common::LargeInteger const& node);
        void RemoveNode();
        bool RemoveNode(Common::LargeInteger const& node);
        bool IsOneNodeUpPerUpgradeDomain();
        bool ContainsNode(std::vector<Common::LargeInteger> const&, Common::LargeInteger const&);
        bool ContainsName(std::vector<std::wstring> const&, std::wstring const&);
        bool IsSeedNode(Common::LargeInteger const& node);
        void CloseAllNodes();

        void ActivateDeactivateNodes();
        void CreateDeleteServices();
        void UpdateService();
        void CreateService(std::wstring serviceName, std::wstring serviceType);
        void CreateServiceInner(std::wstring serviceName, std::wstring serviceType, std::wstring initializationData, Common::TimeSpan replicaRestartWaitDuration);
        void CreateServiceWithSmallReplicationQueue(std::wstring serviceName, std::wstring serviceType);
        void DeleteService(std::wstring serviceName);
        void SafeDeleteService(std::wstring serviceNames);
        void SafeDeleteService(std::vector<std::wstring> serviceNames);
        void RecoverFromQuorumLoss(std::wstring serviceNames, bool doVerify);
        void RecoverFromQuorumLoss(std::vector<std::wstring> serviceNames, bool doVerify);
        void RecoverFromQuorumLoss(std::vector<std::wstring> serviceNames, bool doVerify, bool ignoreNotCreatedServices);
        std::wstring GetServiceType(std::wstring const& serviceName);

        void CheckMemory();

        static Common::GlobalWString CalculatorServicePrefix;
        static Common::GlobalWString TestStoreServicePrefix;
        static Common::GlobalWString TestPersistedStoreServicePrefix;
        static Common::GlobalWString TXRServicePrefix;
        static Common::GlobalWString AppNamePrefix;

        const int maxIterations_;
        std::map<std::wstring, int> createdapps_;
        std::wstring upgradedApp_;
        std::vector<std::wstring> servicesInUpgradedApp_;
        std::vector<std::wstring> newStatefulServicesAfterUpgrade_;
        Common::DateTime upgradeUntil_;
        std::shared_ptr<RandomSessionConfig> config_;
        std::vector<Common::LargeInteger> nodeHistory_;
        std::map<Common::LargeInteger, std::wstring> allNodes_;
        std::wstring nodeRGCapacities_;
        std::map<std::wstring, int> liveNodesPerUpgradeDomain_;
        std::vector<Common::LargeInteger> liveSeedNodes_;
        std::vector<std::wstring> createdServices_;
        std::vector<std::wstring> deletedServices_;
        int iterations_;
        Common::TimeoutHelper timeout_;
        Common::Random random_;
        bool initialized_;
        Common::Stopwatch watch_;
        int quorum_;
        Common::StopwatchTime highMemoryStartTime_;
        std::vector<Common::LargeInteger> seedNodes_;
        RandomTestApplicationHelper applicationHelper_;
        std::vector<std::wstring> deletedAppIds_;

        class PeriodicTracer
        {
        public:
            void AddString(std::wstring const & string)
            {
                periodicTraceString_ += string;
            }

            void OnIteration(int iteration, RandomFabricTestSession & session)
            {
                if (iteration % IterationsBetweenPeriodicTrace == 0)
                {
                    session.AddInput(Common::wformatString("#Periodic Trace:\r\n{0}", periodicTraceString_));
                }
            }

        private:
            static const int IterationsBetweenPeriodicTrace = 5;
            std::wstring periodicTraceString_;
        };

        PeriodicTracer periodicTracer_;
    };
};
