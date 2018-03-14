// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FederationTest
{
    class RandomFederationTestSession: public FederationTestSession
    {
        DENY_COPY(RandomFederationTestSession)
    public:
        static void CreateSingleton(int iterations, int timeout, std::wstring const& label, bool autoMode, int ringCount);
        std::wstring GetInput();

    private:
        class RingContext
        {
            DENY_COPY(RingContext);

        public:
            RingContext(RandomFederationTestSession & session, std::wstring const & ringName, std::vector<Common::LargeInteger> const & allNodes, int baseLeaseAgentPort);

            std::wstring const & GetRingName() { return ringName_; }

            bool IsSeedNode(Common::LargeInteger const& node);
            Common::LargeInteger SelectLiveNode();

            void InitializeFederationStage();

            void UpdateNodes(std::set<Federation::NodeId> const & nodes);

            std::wstring GetSeedNodes();

            void AddSeedNodes();
            void AddOrRemoveNode();
            void AddNode(bool seedNodeOnly);
            void RemoveNode(bool seedNodeOnly);
            void ExecuteArbitrationStage();
            void TestRouting();
            void TestBroadcast();
            void TestVoterStore();

            static bool ContainsNode(std::vector<Common::LargeInteger> const&, Common::LargeInteger const&);

        private:
            int GetLeaseAgentPort(Common::LargeInteger const node) const;
            int GetArbitratorCount();

            RandomFederationTestSession & session_;
            std::wstring ringName_;
            std::vector<Common::LargeInteger> nodeHistory_;
            std::vector<Common::LargeInteger> allNodes_;
            std::vector<Common::LargeInteger> liveNodes_;
            std::map<Common::LargeInteger, Common::StopwatchTime> liveSeedNodes_;
            int64 storeValues_[10];
            size_t seedCount_;
            size_t quorum_;
            int baseLeaseAgentPort_;
            Common::StopwatchTime belowQuorumStartTime_;
            Common::StopwatchTime lastArbitrationTime_;
        };

        typedef std::unique_ptr<RingContext> RingContextUPtr;

        RandomFederationTestSession(int iterations, int timeout, std::wstring const& label, bool autoMode, int ringCount);

        void InitializeSeedNodes();
        void ExecuteFederationStage();
        void ExecuteArbitrationStage();
        void ExecuteRoutingStage();
        void ExecuteBroadcastStage();
        void ExecuteVoterStoreStage();
        void CloseAllNodes();
        bool CheckMemory();

        void ChangeRing(size_t index);
        void SelectRing();

        void AddUnreliableTransportBehavior(std::wstring const & command, bool userMode);
        void ClearUnreliableTransportBehavior();

        static int const DefaultPingIntervalInSeconds;

        RandomFederationTestConfig const & config_;
        int testStage_;
        int iterations_;
        int initalizeStage_;
        Common::TimeoutHelper timeout_;
        Common::Random random_;
        Common::Stopwatch watch_;
        std::wstring waitTime_;
        Common::StopwatchTime highMemoryStartTime_;
        int ringCount_;
        size_t currentRing_;

        std::vector<RingContextUPtr> rings_;
        std::vector<std::wstring> unreliableTransportCommands_;
    };
};
