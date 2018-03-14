// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"
#include "ComTestStateProvider.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    static StringLiteral const TestReplicaManagerSource("ReplicaManagerSourceTest");

    typedef std::shared_ptr<REConfig> REConfigSPtr;
    class ReplicaManagerWrapper;
    typedef std::pair<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> UpdateConfigEntry;
    typedef std::map<FABRIC_REPLICA_ID, FABRIC_SEQUENCE_NUMBER> UpdateConfigMap;

    class TestReplicaManager
    {
    protected:
        TestReplicaManager() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup)
                
    };

    static FABRIC_EPOCH const ReplicaManagerEpoch = {120, 37643, NULL};
    class ReplicaManagerWrapper : public ComponentRoot
    {
    public:
        ReplicaManagerWrapper() :
            ComponentRoot(),
            guid_(Guid::NewGuid()),
            oldConfig_(CreateGenericConfig()),
            perfCounters_(),
            endpointUniqueId_(guid_, 1),
            transport_(CreateTransport(oldConfig_)),
            stateProvider_(ComTestStateProvider::GetProvider(1, -1, GetRoot())),
            replicaManagerSPtr_()
        {
        }
     
        ~ReplicaManagerWrapper() 
        {
        }

        void CreateAndOpenReplicaManager()
        {
            perfCounters_ = REPerformanceCounters::CreateInstance(PartitionId,1);
            IReplicatorHealthClientSPtr temp;
            ApiMonitoringWrapperSPtr temp2;
            config_ = REInternalSettings::Create(nullptr,oldConfig_),
            replicaManagerSPtr_ = std::shared_ptr<ReplicaManager>(new ReplicaManager(
                config_,
                false,
                perfCounters_,
                1, /* ReplicaId*/
                false, /* requireserviceACK */
                endpointUniqueId_, 
                transport_,
                0, /* initialProgress */
                ReplicaManagerEpoch, 
                stateProvider_,
                partition_,
                temp,
                temp2,
                PartitionId));
            VERIFY_IS_TRUE(replicaManagerSPtr_->Open().IsSuccess(), L"Replica manager opened");
        }

        void CloseReplicaManager()
        {
            VERIFY_IS_TRUE(replicaManagerSPtr_->Close(false).IsSuccess(), L"Replica manager closed");
        }

        void UpdateConfiguration(
            wstring const & previousReplicasDescription,
            wstring const & currentReplicasDescription);

        void UpdateConfiguration(
            wstring const & previousReplicasDescription,
            ULONG previousQuorum,
            wstring const & currentReplicasDescription,
            ULONG currentQuorum);

        void UpdateConfiguration(
            wstring const & replicasDescription,
            ULONG quorum);

        void UpdateConfiguration(
            wstring const & replicasDescription);

        void CheckState(
            wstring const & expectedActiveReplicas,
            wstring const & expectedPreviousActiveReplicas,
            wstring const & expectedIdleReplicas,
            bool expectedQuorumAchieved,
            FABRIC_SEQUENCE_NUMBER expectedCommittedLSN,
            FABRIC_SEQUENCE_NUMBER expectedCompletedLSN);

        void AddIdles(wstring const & idleReplica);

    private:
        static REConfigSPtr CreateGenericConfig();
        static ReplicationTransportSPtr CreateTransport(REConfigSPtr const & config);
        static void AddIncarnationIdIfNecessary(std::wstring & replica);

        class DummyRoot : public Common::ComponentRoot
        {
        public:
            ~DummyRoot() {}
        };
        static Common::ComponentRoot const & GetRoot();
        static bool CompareStrings(wstring const & s1, wstring const & s2, bool doVerify);

        void GenerateActiveReplicas(
            std::wstring const & replicasDescription,
            __out size_t & replicaCount,
            __out std::vector<ReplicaInformation> & replicas);

        void UpdateConfiguration(
            ReplicaInformationVector const & previousActiveReplicas,
            ULONG previousQuorum,
            ReplicaInformationVector const & currentActiveReplicas,
            ULONG currentQuorum);

        static wstring PartitionId;

        Guid guid_;
        REConfigSPtr oldConfig_;
        REInternalSettingsSPtr config_;
        REPerformanceCountersSPtr perfCounters_;
        ReplicationEndpointId endpointUniqueId_;
        ReplicationTransportSPtr transport_; 
        ComProxyStateProvider stateProvider_;
        std::shared_ptr<ReplicaManager> replicaManagerSPtr_;
        ComProxyStatefulServicePartition partition_;
    };

    wstring ReplicaManagerWrapper::PartitionId(Common::Guid::NewGuid().ToString());

    /***********************************
    * TestReplicaManager methods
    **********************************/
    BOOST_FIXTURE_TEST_SUITE(TestReplicaManagerSuite,TestReplicaManager)

    BOOST_AUTO_TEST_CASE(TestUpdateConfigurationWithQuorumIgnorePreviousConfig)
    {
        // UpdateConfiguration params: 
        //      - current active replicas and quorum
        //      - previous active replicas and quorum
        //      - expected committed LSN
        //      - expected completed LSN

        // Test with default quorum, ignoring the previous config entry
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();

        wstring cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-100:2";
        Sleep(1222);

        wstring ccState = L"100:2";
        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 2, 2);

        // Quorum = 3/2 + 1 = 2
        // We need just one ACK from an active replica, so expected committed is 6
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-200:5;27f0e7b6-8caf-4f30-9f49-193fd31ee870-201:6";
        ccState = L"200:5;201:6";
        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 6, 5);

        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:2;27f0e7b6-8caf-4f30-9f49-193fd31ee870-301:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-302:5";
        ccState = L"300:2;301:3;302:5";

        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 3, 2);

        // Quorum = 6/2 + 1 = 4
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:2;27f0e7b6-8caf-4f30-9f49-193fd31ee870-401:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-402:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-403:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-302:5";
        ccState = L"300:2;401:3;402:4;403:4;302:5";

        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 4, 2);

        // {2, 3, 3, 4, 6, 10}; Quorum = 7/2 + 1 = 4
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:2;27f0e7b6-8caf-4f30-9f49-193fd31ee870-401:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-502:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-503:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-504:6;27f0e7b6-8caf-4f30-9f49-193fd31ee870-505:10";
        ccState = L"400:2;401:3;502:3;503:4;504:6;505:10";

        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 4, 2);
        wrapper.CloseReplicaManager();
    }

    BOOST_AUTO_TEST_CASE(TestUpdateConfigurationNoQuorumIgnorePreviousConfig)
    {
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();
        // Test quorum higher than expected per number of replicas
        // {1, 4, 7, 8, 9, 11};  expected Quorum = 7/2 + 1 = 4; forced quorum 5
        wstring cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-100:1;27f0e7b6-8caf-4f30-9f49-193fd31ee870-101:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-102:7;27f0e7b6-8caf-4f30-9f49-193fd31ee870-103:8;27f0e7b6-8caf-4f30-9f49-193fd31ee870-104:9;27f0e7b6-8caf-4f30-9f49-193fd31ee870-105:11";
        wstring ccState = L"100:1;101:4;102:7;103:8;104:9;105:11";
        wrapper.UpdateConfiguration(cc, 5);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 7, 1);

        // {3, 6, 6, 8, 19}; expected Quorum = 6/2 + 1 = 4
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-200:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-201:6;27f0e7b6-8caf-4f30-9f49-193fd31ee870-202:6;27f0e7b6-8caf-4f30-9f49-193fd31ee870-203:8;27f0e7b6-8caf-4f30-9f49-193fd31ee870-204:19";
        ccState = L"200:3;201:6;202:6;203:8;204:19";
        wrapper.UpdateConfiguration(cc, 7);
        wrapper.CheckState(ccState, L"", L"", false /*quorumAchieved*/, 0, 0);

        wrapper.UpdateConfiguration(L"", 2);
        wrapper.CheckState(L"", L"", L"", false /*quorumAchieved*/, 0, 0);
        wrapper.CloseReplicaManager();
    }

    BOOST_AUTO_TEST_CASE(TestUpdateConfigurationWithQuorumWithPreviousConfig)
    {
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();

        wrapper.AddIdles(L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-200:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:3");
        wrapper.CheckState(L"", L"", L"200:4;300:3;", true /*quorumAchieved*/, -1, 3);

        // PC: 100, 101, 200
        // CC: 200, 300, 400
        wstring pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-100:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-101:1;27f0e7b6-8caf-4f30-9f49-193fd31ee870-200:4";
        wstring pcState = L"100:3;101:1;200:4";
        wstring cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-200:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5";
        wstring ccState = L"200:4;300:3;400:5";

        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState, pcState, L"", true /*quorumAchieved*/, 3, 1);
        // 100 should be closed
        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 4, 3);

        // PC: 300, 400 (200 is removed, so it should be closed)
        // CC: 400, 500, 600
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5;27f0e7b6-8caf-4f30-9f49-193fd31ee870-500:7;27f0e7b6-8caf-4f30-9f49-193fd31ee870-600:4";
        pcState = L"300:3;400:5";
        ccState = L"400:5;500:7;600:4"; 

        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState, pcState, L"", true /*quorumAchieved*/, 5, 3);
        // 300 should be closed
        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", L"", true /*quorumAchieved*/, 5, 4);

        wstring idles = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-555:4;27f0e7b6-8caf-4f30-9f49-193fd31ee870-666:3";
        wstring idlesState = L"555:4;666:3";
        wrapper.AddIdles(idles);
        wrapper.CheckState(ccState, L"", idlesState, true /*quorumAchieved*/, 5, 3);

        // PC: 400, 500, 600
        // CC: 700, 800
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5;27f0e7b6-8caf-4f30-9f49-193fd31ee870-500:7;27f0e7b6-8caf-4f30-9f49-193fd31ee870-600:4";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-700:8;27f0e7b6-8caf-4f30-9f49-193fd31ee870-800:9";
        pcState = L"400:5;500:7;600:4";
        ccState = L"700:8;800:9";

        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState, pcState, idlesState,  true /*quorumAchieved*/, 5, 3);

        // Update config again, on top of the pending one
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5;27f0e7b6-8caf-4f30-9f49-193fd31ee870-500:7";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-700:8;27f0e7b6-8caf-4f30-9f49-193fd31ee870-800:9;27f0e7b6-8caf-4f30-9f49-193fd31ee870-600:4";
        pcState = L"400:5;500:7";
        ccState = L"700:8;800:9;600:4";
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState, pcState, idlesState,  true /*quorumAchieved*/, 7, 3);

        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", idlesState, true /*quorumAchieved*/, 8, 3);

        // Remove all active replicas, then update config again with different previous replicas
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-700:8;27f0e7b6-8caf-4f30-9f49-193fd31ee870-800:9";
        pcState = L"700:8;800:9";
        cc = L"";
        ccState = L"";
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState, pcState, idlesState,  true /*quorumAchieved*/, 9, 3);

        // 700 should be closed, 800 should be moved to current
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-800:9;27f0e7b6-8caf-4f30-9f49-193fd31ee870-900:4";
        ccState = L"800:9;900:4";
        wrapper.UpdateConfiguration(L"", cc);
        wrapper.CheckState(ccState, L"", idlesState,  true /*quorumAchieved*/, 9, 3);
        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState, L"", idlesState,  true /*quorumAchieved*/, 9, 3);
        wrapper.CloseReplicaManager();
    }

        BOOST_AUTO_TEST_CASE(TestUpdateConfigurationNoQuorumWithPreviousConfig)
    {
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();

        // PC: 100, 200 (not enough replicas for quorum)
        // CC: 300, 400, 500 (enough replicas for quorum)
        wstring pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-100:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-200:4";
        wstring cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5;27f0e7b6-8caf-4f30-9f49-193fd31ee870-500:4";
        wstring pcState = L"100:3;200:4";
        wstring ccState = L"300:3;400:5;500:4";
        wrapper.UpdateConfiguration(
            pc, 4,
            cc, 3);
        wrapper.CheckState(ccState, pcState, L"", false, -1, -1);
        wrapper.UpdateConfiguration(cc, 3);
        wrapper.CheckState(ccState, L"", L"", true, 4, 3);

        // PC: 300, 400, 500 (enough replicas for quorum)
        // CC: 500 (not enough replicas for quorum)
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-300:3;27f0e7b6-8caf-4f30-9f49-193fd31ee870-400:5;27f0e7b6-8caf-4f30-9f49-193fd31ee870-500:4";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-500:4";
        pcState = L"300:3;400:5;500:4";
        ccState = L"500:4";
        wrapper.UpdateConfiguration(
            pc, 3,
            cc, 3);
        wrapper.CheckState(ccState, pcState, L"", false, -1, -1);
        wrapper.UpdateConfiguration(cc, 3);
        wrapper.CheckState(L"500:4;", L"", L"", false, -1, -1);
        wrapper.CloseReplicaManager();
    }

    BOOST_AUTO_TEST_CASE(TestUpdateConfigurationWithSToIReadyIdleReplica1)
    {
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();

        // PC : empty
        // CC : 999
        wstring cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0";
        wstring ccState = L"999:0";

        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState,L"",L"",true,0,0);
        
        // PC : 999
        // CC : 999
        wstring pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0";
        wstring pcState = L"999:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0";
        ccState = L"999:0";
        
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"",true,0,0);
        
        // Idle : 9999
        wrapper.AddIdles(L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-9999:-1");
        wrapper.CheckState(ccState,pcState,L"9999:0",true,0,0);

        // PC : 999, 9999
        // CC : 999
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-9999:0";
        // Verify that 9999 is in both PC and IDle list
        pcState = L"999:0;9999:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0";
        ccState = L"999:0";
        
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"9999:0",true,0,0);

        // PC : empty
        // CC : 999
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0";
        ccState = L"999:0";
        
        wrapper.UpdateConfiguration(cc);
        // Verify the idle is still present
        wrapper.CheckState(ccState,L"",L"9999:0",true,0,0);

        // PC : 999
        // CC : 999 , 9999 
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0";
        pcState = L"999:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-999:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-9999:0";
        ccState = L"999:0;9999:0";
        
        wrapper.UpdateConfiguration(pc, cc);
        // Verify the idle has been promoted to active
        wrapper.CheckState(ccState,pcState,L"",true,0,0);

        wrapper.CloseReplicaManager();
    }

    BOOST_AUTO_TEST_CASE(TestUpdateConfigurationWithSToIReadyIdleReplica2)
    {
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();

        // PC : NULL
        // CC : 111,222,444
        wstring cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-444:0";
        wstring ccState = L"111:0;222:0;444:0";
        wstring pc = L"";
        wstring pcState = L"";

        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState,pcState,L"",true,0,0);
         
        // Idle : 333
        wrapper.AddIdles(L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-333:-1");
        wrapper.CheckState(ccState,pcState,L"333:0",true,0,0);

       
        // PC : 111,222,444
        // CC : 111,222,444
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-444:0";
        pcState = L"111:0;222:0;444:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-444:0";
        ccState = L"111:0;222:0;444:0";
        
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"333:0;",true,0,0);

        // PC : 111,222
        // CC : 111,222
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0";
        pcState = L"111:0;222:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0";
        ccState = L"111:0;222:0";
        // 444 is removed twice
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"333:0;",true,0,0);

        // PC : 111,222,333
        // CC : 111,222
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-333:0";
        // Verify 333 is in idle and PC list
        pcState = L"111:0;222:0;333:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0";
        ccState = L"111:0;222:0";
        // verify idle is not removed
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"333:0;",true,0,0);
        
        // Repeat step above again
        // PC : 111,222,333
        // CC : 111,222
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-333:0";
        // Verify 333 is in idle and PC list
        pcState = L"111:0;222:0;333:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0";
        ccState = L"111:0;222:0";
        // verify idle is not removed
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"333:0;",true,0,0);

        // PC : empty
        // CC : 111,222
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0";
        ccState = L"111:0;222:0";
        wrapper.UpdateConfiguration(cc);
        wrapper.CheckState(ccState,L"",L"333:0;",true,0,0);
        
        // PC : 111,222
        // CC : 111,222,333
        // Verify 333 is promoted to Active
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0";
        pcState = L"111:0;222:0";
        cc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:0;27f0e7b6-8caf-4f30-9f49-193fd31ee870-333:0";
        ccState = L"111:0;222:0;333:0";
        // verify idle is promoted 
        wrapper.UpdateConfiguration(pc, cc);
        wrapper.CheckState(ccState,pcState,L"",true,0,0);

        // verify idle is promoted
        wrapper.CloseReplicaManager();
    }

    BOOST_AUTO_TEST_CASE(TestUpdateConfigurationWithPrimaryAbsentInPC)
    {
        ReplicaManagerWrapper wrapper;
        wrapper.CreateAndOpenReplicaManager();
 
        // PC : 111:10, 222:8
        // CC : empty
        wstring pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:10;27f0e7b6-8caf-4f30-9f49-193fd31ee870-222:8;";
        wstring pcState = L"111:10;222:8";
        wstring cc = L"";
        wstring ccState = L"";

        wrapper.UpdateConfiguration(pc, 2, cc, 1);
        // VERIFY completed LSN is 8
        wrapper.CheckState(ccState, pcState, L"", true, 10, 8);

       
        // PC : 111:10
        // CC : empty
        pc = L"27f0e7b6-8caf-4f30-9f49-193fd31ee870-111:10;";
        pcState = L"111:10;";
        cc = L"";
        ccState = L"";
        
        wrapper.UpdateConfiguration(pc, 1, cc, 1);
        // VERIFY completed LSN is 10 and NOT -1
        wrapper.CheckState(ccState,pcState,L"",true,10,10);

        wrapper.CloseReplicaManager();
    }

    BOOST_AUTO_TEST_SUITE_END()


    bool TestReplicaManager::Setup()
    {
        return true;
    }
 
    /***********************************
    * ReplicaManagerWrapper methods
    **********************************/
    void ReplicaManagerWrapper::UpdateConfiguration(
        wstring const & currentReplicasDescription)
    {
        ReplicaInformationVector previousActiveReplicas;
        ULONG previousQuorum = 0;

        ReplicaInformationVector currentActiveReplicas;
        size_t currentReplicaCount;
        GenerateActiveReplicas(currentReplicasDescription, currentReplicaCount, currentActiveReplicas);
        
        ULONG currentQuorum = static_cast<ULONG>((currentReplicaCount + 1) / 2 + 1);
        Trace.WriteInfo(TestReplicaManagerSource,
            "*********** UpdateConfiguration with current replicas: {0}, quorum {1}, no previous config",
            currentReplicasDescription,
            currentQuorum);

        auto error = replicaManagerSPtr_->UpdateConfiguration(
            previousActiveReplicas,
            previousQuorum,
            currentActiveReplicas, 
            currentQuorum, 
            false,
            true);
        VERIFY_IS_TRUE(error.IsSuccess(), L"UpdateConfiguration (catchup replicas set) should succeed");
    }

    void ReplicaManagerWrapper::UpdateConfiguration(
        wstring const & currentReplicasDescription,
        ULONG currentQuorum)
    {
        ReplicaInformationVector previousActiveReplicas;
        ULONG previousQuorum = 0;

        ReplicaInformationVector currentActiveReplicas;
        size_t currentReplicaCount;
        GenerateActiveReplicas(currentReplicasDescription, currentReplicaCount, currentActiveReplicas);
        
        Trace.WriteInfo(TestReplicaManagerSource,
            "*********** UpdateConfiguration with current replicas: {0}, quorum {1}, no previous config", 
            currentReplicasDescription,
            currentQuorum);

        auto error = replicaManagerSPtr_->UpdateConfiguration(
            previousActiveReplicas,
            previousQuorum,
            currentActiveReplicas, 
            currentQuorum, 
            false,
            true);
        VERIFY_IS_TRUE(error.IsSuccess(), L"UpdateConfiguration (catchup replicas set) should succeed");
    }
    
    void ReplicaManagerWrapper::UpdateConfiguration(
        wstring const & previousReplicasDescription,
        wstring const & currentReplicasDescription)
    {
        ReplicaInformationVector previousActiveReplicas;
        size_t previousReplicaCount;
        GenerateActiveReplicas(previousReplicasDescription, previousReplicaCount, previousActiveReplicas);
        ULONG previousQuorum = static_cast<ULONG>((previousReplicaCount + 1) / 2 + 1);
        
        ReplicaInformationVector currentActiveReplicas;
        size_t currentReplicaCount;
        GenerateActiveReplicas(currentReplicasDescription, currentReplicaCount, currentActiveReplicas);
        ULONG currentQuorum = static_cast<ULONG>((currentReplicaCount + 1) / 2 + 1);
        
        Trace.WriteInfo(TestReplicaManagerSource,
            "*********** UpdateConfiguration with current replicas: {0}, quorum {1}, previous: {2}, quorum {3}", 
            currentReplicasDescription,
            currentQuorum,
            previousReplicasDescription,
            previousQuorum);

        auto error = replicaManagerSPtr_->UpdateConfiguration(
            previousActiveReplicas,
            previousQuorum,
            currentActiveReplicas, 
            currentQuorum, 
            true,
            true);
        VERIFY_IS_TRUE(error.IsSuccess(), L"UpdateConfiguration (catchup replicas set) should succeed");
    }

    void ReplicaManagerWrapper::UpdateConfiguration(
        wstring const & previousReplicasDescription,
        ULONG previousQuorum,
        wstring const & currentReplicasDescription,
        ULONG currentQuorum)
    {
        ReplicaInformationVector previousActiveReplicas;
        size_t previousReplicaCount;
        GenerateActiveReplicas(previousReplicasDescription, previousReplicaCount, previousActiveReplicas);

        ReplicaInformationVector currentActiveReplicas;
        size_t currentReplicaCount;
        GenerateActiveReplicas(currentReplicasDescription, currentReplicaCount, currentActiveReplicas);
        
        Trace.WriteInfo(TestReplicaManagerSource,
            "*********** UpdateConfiguration with current replicas: {0}, quorum {1}, previous: {2}, quorum {3}", 
            currentReplicasDescription,
            currentQuorum,
            previousReplicasDescription,
            previousQuorum);

        auto error = replicaManagerSPtr_->UpdateConfiguration(
            previousActiveReplicas,
            previousQuorum,
            currentActiveReplicas, 
            currentQuorum, 
            true,
            true);
        VERIFY_IS_TRUE(error.IsSuccess(), L"UpdateConfiguration (catchup replicas set) should succeed");
    }

    void ReplicaManagerWrapper::UpdateConfiguration(
        ReplicaInformationVector const & previousActiveReplicas,
        ULONG previousQuorum,
        ReplicaInformationVector const & currentActiveReplicas,
        ULONG currentQuorum)
    {
        auto error = replicaManagerSPtr_->UpdateConfiguration(
            previousActiveReplicas,
            previousQuorum,
            currentActiveReplicas, 
            currentQuorum, 
            true,
            true);
        VERIFY_IS_TRUE(error.IsSuccess(), L"UpdateConfiguration (catchup replicas set) should succeed");
    }

    void ReplicaManagerWrapper::CheckState(
        wstring const & expectedActiveReplicas,
        wstring const & expectedPreviousActiveReplicas,
        wstring const & expectedIdleReplicas,
        bool expectedQuorumAchieved,
        FABRIC_SEQUENCE_NUMBER expectedCommittedLSN,
        FABRIC_SEQUENCE_NUMBER expectedCompletedLSN)
    {
        wstring activeReplicas;
        wstring previousActiveReplicas;
        wstring idleReplicas;
        FABRIC_SEQUENCE_NUMBER committedLSN;
        FABRIC_SEQUENCE_NUMBER completedLSN;
        bool quorumAchieved;

        for (int count = 1; ; count++)
        {
            bool replicationAckProcessingInProgress = false;
            quorumAchieved = replicaManagerSPtr_->Test_TryGetProgress(
                activeReplicas, 
                previousActiveReplicas, 
                idleReplicas,
                committedLSN, 
                completedLSN,
                replicationAckProcessingInProgress);
            
            if (!replicationAckProcessingInProgress)
            {
                bool reachedDesiredState = CompareStrings(activeReplicas, expectedActiveReplicas, false);
                reachedDesiredState &= CompareStrings(previousActiveReplicas, expectedPreviousActiveReplicas, false);
                reachedDesiredState &= CompareStrings(idleReplicas, expectedIdleReplicas, false);

                if (!reachedDesiredState)
                {
                    Sleep(100);
                }
                else if (count >= 100)
                {
                    break;
                }
            }
            else
            {
                Sleep(100);
            }
        }

        VERIFY_ARE_EQUAL_FMT(
            quorumAchieved,
            expectedQuorumAchieved,
            // wait for the ack to be completely processed
                "Expected quorum status = {0}, achieved = {1}", expectedQuorumAchieved, quorumAchieved);

        if (!expectedQuorumAchieved)
        {
            return;
        }

        VERIFY_ARE_EQUAL_FMT(
            expectedCommittedLSN, 
            committedLSN, 
            "Expected committedLSN {0} = committedLSN {1}", expectedCommittedLSN, committedLSN);

        VERIFY_ARE_EQUAL_FMT(
            expectedCompletedLSN, 
            completedLSN, 
            "Expected completedLSN {0} = completedLSN {1}", expectedCompletedLSN, completedLSN);
    }

    bool ReplicaManagerWrapper::CompareStrings(
        wstring const & s1, 
        wstring const & s2,
        bool doVerify)
    {
        vector<wstring> s1Tokens;
        vector<wstring> s2Tokens;
        
        if (!s1.empty() && s1[s1.size() - 1] != L';')
        {
            StringUtility::Split<wstring>(s1 + L";", s1Tokens, L";");
        }
        else
        {
            StringUtility::Split<wstring>(s1, s1Tokens, L";");
        }

        if (!s2.empty() && s2[s2.size() - 2] != L';')
        {
            StringUtility::Split<wstring>(s2 + L";", s2Tokens, L";");
        }
        else
        {
            StringUtility::Split<wstring>(s2, s2Tokens, L";");
        }

        std::sort(s1Tokens.begin(), s1Tokens.end());
        std::sort(s2Tokens.begin(), s2Tokens.end());

        if (doVerify)
        {
            VERIFY_ARE_EQUAL_FMT(
                s1Tokens,
                s2Tokens,
                "Expected = \"{0}\", actual = \"{1}\"", s2, s1);
        }
        return (s1Tokens == s2Tokens);
    }

    void ReplicaManagerWrapper::AddIncarnationIdIfNecessary(std::wstring & replica)
    {
        if (replica.find(L'@') == std::wstring::npos)
        {
            auto dex = replica.find(L':');
            wstring r;
            StringWriter w(r);
            w << replica.substr(0, dex) << Constants::ReplicaIncarnationDelimiter << Common::Guid::NewGuid().ToString() << ':' << replica.substr(dex+1);
            replica = r;
        }
    }

    void ReplicaManagerWrapper::AddIdles(wstring const & idleReplicas)
    {
        Trace.WriteInfo(TestReplicaManagerSource, "** AddIdles {0}", idleReplicas);

        ReplicaInformationVector idles;
        vector<wstring> replicas;
        StringUtility::Split<wstring>(idleReplicas, replicas, L";");
        size_t replicaCount = replicas.size();
        
        FABRIC_SEQUENCE_NUMBER replLSN;
        for(size_t i = 0; i < replicaCount; ++i)
        {
            vector<wstring> replicaDetails;
            AddIncarnationIdIfNecessary(replicas[i]);
            StringUtility::Split<wstring>(replicas[i], replicaDetails, L":");

            ReplicationEndpointId endpointUniqueId = ReplicationEndpointId::FromString(replicaDetails[0]);
            wstring endpoint;

            transport_->GeneratePublishEndpoint(endpointUniqueId, endpoint);

            ReplicaInformation replica(
                endpointUniqueId.ReplicaId,
                ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                endpoint,
                false,
                -1,
                -1);
            
            ReplicationSessionSPtr session;
            ErrorCode error = replicaManagerSPtr_->TryAddIdleReplica(replica, session, replLSN);
            VERIFY_IS_TRUE_FMT(error.IsSuccess(), "Added idle \"{0}\"", endpointUniqueId);

            int64 lsn = Int64_Parse(replicaDetails[1]);
            session->UpdateAckProgress(lsn, lsn, L"empty", nullptr);
            for (int count = 1; count < 100; count++)
            {
                if (session->Test_IsReplicationAckProcessingInProgress())
                {
                    Sleep(100);
                }
                else
                {
                    break;
                }
            }
        }
    }

    void ReplicaManagerWrapper::GenerateActiveReplicas(
        std::wstring const & replicasDescription,
        __out size_t & replicaCount,
        __out std::vector<ReplicaInformation> & replicaInfos)
    {
        vector<wstring> replicas;
        StringUtility::Split<wstring>(replicasDescription, replicas, L";");
        replicaCount = replicas.size();
        for(size_t i = 0; i < replicaCount; ++i)
        {
            vector<wstring> replicaDetails;
            AddIncarnationIdIfNecessary(replicas[i]);
            StringUtility::Split<wstring>(replicas[i], replicaDetails, L":");

            ReplicationEndpointId endpointUniqueId = ReplicationEndpointId::FromString(replicaDetails[0]);
            int64 lsn = Int64_Parse(replicaDetails[1]);
            wstring endpoint;
            transport_->GeneratePublishEndpoint(endpointUniqueId, endpoint);
            ReplicaInformation replica(
                endpointUniqueId.ReplicaId,
                ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,
                endpoint,
                false,
                lsn,
                lsn);

            replicaInfos.push_back(std::move(replica));
        }
    }

    REConfigSPtr ReplicaManagerWrapper::CreateGenericConfig()
    {
        REConfigSPtr config = std::make_shared<REConfig>();
        config->BatchAcknowledgementInterval = TimeSpan::FromMilliseconds(300);
        config->SecondaryClearAcknowledgedOperations = (Random().NextDouble() > 0.5)? true : false;
        config->QueueHealthMonitoringInterval = TimeSpan::Zero;
        return config;
    }

    ReplicationTransportSPtr ReplicaManagerWrapper::CreateTransport(REConfigSPtr const & config)
    {
        auto transport = ReplicationTransport::Create(config->ReplicatorListenAddress);

        auto errorCode = transport->Start(L"localhost:0");
        ASSERT_IFNOT(errorCode.IsSuccess(), "Failed to start: {0}", errorCode);

        return transport;
    }

    Common::ComponentRoot const & ReplicaManagerWrapper::GetRoot()
    {
        static std::shared_ptr<DummyRoot> root = std::make_shared<DummyRoot>();
        return *root;
    }
}
