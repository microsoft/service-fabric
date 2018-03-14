// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ComTestStatefulServicePartition.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"
#include "TestChangeRole.h"

namespace ReplicationUnitTest
{
    static Common::StringLiteral const ChangeRoleTestSource("ChangeRoleTest");

    Common::Guid TestChangeRole::PartitionId = Common::Guid::NewGuid();

    typedef std::shared_ptr<REConfig> REConfigSPtr;

    BOOST_FIXTURE_TEST_SUITE(TestChangeRoleSuite,TestChangeRole)

    BOOST_AUTO_TEST_CASE(TestChangeRolePrimaryDown1)
    {
        TestChangeRolePrimaryDown(true);
    }

    BOOST_AUTO_TEST_CASE(TestChangeRolePrimaryDown2)
    {
        TestChangeRolePrimaryDown(false);
    }

    BOOST_AUTO_TEST_CASE(TestSimpleChangeRole1)
    {
        TestSimpleChangeRole(true);
    }

    BOOST_AUTO_TEST_CASE(TestSimpleChangeRole2)
    {
        TestSimpleChangeRole(false);
    }

    BOOST_AUTO_TEST_CASE(TestChangeRoleUpdateConfigWithPrevious)
    {
        bool hasPersistedState = false;
        ComTestOperation::WriteInfo(
            ChangeRoleTestSource,
            "Start TestChangeRoleUpdateConfigWithPrevious, persisted state {0}",
            hasPersistedState);
                
        ReplicatorSPtr primary;
        ComPointer<ComTestStateProvider> pStateProvider;
        FABRIC_REPLICA_ID pId = 1;

        ReplicatorSPtr secondary0;
        ComPointer<ComTestStateProvider> s0StateProvider;
        FABRIC_REPLICA_ID s0Id = 2;

        ReplicatorSPtr secondary1;
        ComPointer<ComTestStateProvider> s1StateProvider;
        FABRIC_REPLICA_ID s1Id = 3;

        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 23;
        epoch.ConfigurationNumber = 222;
        epoch.Reserved = NULL;

        int64 numberOfCopyOps = 1;
        int64 numberOfCopyContextOps = hasPersistedState ? 1 : -1;
        
        auto transportPrimary = CreateAndOpen(
            pId, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_PRIMARY, hasPersistedState, primary, pStateProvider);
        Replicator & p = *(primary.get());

        auto transportSecondary0 = CreateAndOpen(
            s0Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary0, s0StateProvider);
        Replicator & s0 = *(secondary0.get());

        auto transportSecondary1 = CreateAndOpen(
            s1Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary1, s1StateProvider);
        Replicator & s1 = *(secondary1.get());

        // Create config: P, I1, I2 by building Idle replicas
        vector<ReplicaInformation> replicas;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        Copy(p, replicas);

        // Change role I1 -> S1, I2 -> S2
        ++epoch.ConfigurationNumber;
        ChangeRole(s0, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        ChangeRole(s1, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

        // Update config: 
        // - previous {s0}, but writeQuorum too big so there are not enough replicas for quorum
        // - current {s0, s1}, default quorum
        vector<ReplicaInformation> previousReplicas;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, previousReplicas);
        ULONG previousQuorum = 5;

        vector<ReplicaInformation> currentReplicas;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, currentReplicas);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, currentReplicas);
        ULONG currentQuorum = static_cast<ULONG>((currentReplicas.size() + 1) / 2 + 1);
        
        ErrorCode error = p.UpdateCatchUpReplicaSetConfiguration(
            previousReplicas, previousQuorum, currentReplicas, currentQuorum);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "UpdateCatchUpReplicaSetConfiguration completed with {0}", error);
        
        // Since the previous config doesn't have quorum,
        // replicate should be stuck, but catchup should complete fine
        ManualResetEvent replicateDoneEvent;
        DateTime replicateDoneTime = DateTime::Zero;
        StartReplicate(p, replicateDoneEvent, replicateDoneTime, 1);
        Catchup(p);
        DateTime beforeConfigCompletedTime = DateTime::Now();
        vector<ReplicaInformation> emptyReplicas;
        error = p.UpdateCatchUpReplicaSetConfiguration(
            emptyReplicas, 0 /*previousQuorum*/, currentReplicas, currentQuorum);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "UpdateCatchUpReplicaSetConfiguration - done reconfig completed with {0}", error);
        error = p.UpdateCurrentReplicaSetConfiguration(currentReplicas, currentQuorum);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "UpdateCurrentReplicaSetConfiguration - done reconfig completed with {0}", error);
        
        EndReplicate(replicateDoneEvent, replicateDoneTime, beforeConfigCompletedTime);

        // Close s0 and update config with s0 in previous config but correct quorum
        // (so no quorum can be achieved for replicate because no ACKs are received from s0)
        // and only s1 in current config
        Close(s0);

        vector<ReplicaInformation> newCurrentReplicas;
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, newCurrentReplicas);
        previousQuorum = static_cast<ULONG>((previousReplicas.size() + 1) / 2 + 1);
        currentQuorum = static_cast<ULONG>((newCurrentReplicas.size() + 1) / 2 + 1);
        error = p.UpdateCatchUpReplicaSetConfiguration(
            previousReplicas, previousQuorum, newCurrentReplicas, currentQuorum);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "UpdateCatchUpReplicaSetConfiguration completed with {0}", error);
        
        replicateDoneEvent.Reset();
        StartReplicate(p, replicateDoneEvent, replicateDoneTime, 2);
        Catchup(p); // should return, since current config is ok
        beforeConfigCompletedTime = DateTime::Now();

        error = p.UpdateCatchUpReplicaSetConfiguration(
            emptyReplicas, 0  /*previousQuorum*/, newCurrentReplicas, currentQuorum);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "UpdateCatchUpReplicaSetConfiguration - done reconfig completed with {0}", error);
        error = p.UpdateCurrentReplicaSetConfiguration(newCurrentReplicas, currentQuorum);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "UpdateCurrentReplicaSetConfiguration - done reconfig completed with {0}", error);
        
        EndReplicate(replicateDoneEvent, replicateDoneTime, beforeConfigCompletedTime);

        Close(p);
        Close(s1);

        transportPrimary->Stop();
        transportSecondary0->Stop();
        transportSecondary1->Stop();
    }

    BOOST_AUTO_TEST_SUITE_END()

    REConfigSPtr TestChangeRole::CreateGenericConfig()
    {
        auto config = std::make_shared<REConfig>();
        config->BatchAcknowledgementInterval = TimeSpan::FromMilliseconds(100);
        config->QueueHealthMonitoringInterval = TimeSpan::Zero;
        return config;
    }

    Common::ComponentRoot const & TestChangeRole::GetRoot()
    {
        static std::shared_ptr<DummyRoot> root = std::make_shared<DummyRoot>();
        return *root;
    }

    bool TestChangeRole::Setup()
    {
        return TRUE;
    }

    void TestChangeRole::TestSimpleChangeRole(bool hasPersistedState)
    {
        ComTestOperation::WriteInfo(
            ChangeRoleTestSource,
            "Start TestSimpleChangeRole, persisted state {0}",
            hasPersistedState);

        ReplicatorSPtr primary;
        ComPointer<ComTestStateProvider> pStateProvider;
        FABRIC_REPLICA_ID pId = 1;

        ReplicatorSPtr secondary0;
        ComPointer<ComTestStateProvider> s0StateProvider;
        FABRIC_REPLICA_ID s0Id = 2;

        ReplicatorSPtr secondary1;
        ComPointer<ComTestStateProvider> s1StateProvider;
        FABRIC_REPLICA_ID s1Id = 3;

        ReplicatorSPtr secondary2;
        ComPointer<ComTestStateProvider> s2StateProvider;
        FABRIC_REPLICA_ID s2Id = 4;

        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 1;
        epoch.ConfigurationNumber = 122;
        epoch.Reserved = NULL;

        int64 numberOfCopyOps = 2;
        int64 numberOfCopyContextOps = hasPersistedState ? 1 : -1;
        auto transportPrimary = CreateAndOpen(
            pId, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_PRIMARY, hasPersistedState, primary, pStateProvider);
        Replicator & p = *(primary.get());

        auto transportSecondary0 = CreateAndOpen(
            s0Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary0, s0StateProvider);
        Replicator & s0 = *(secondary0.get());

        auto transportSecondary1 = CreateAndOpen(
            s1Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary1, s1StateProvider);
        Replicator & s1 = *(secondary1.get());

        auto transportSecondary2 = CreateAndOpen(
            s2Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary2, s2StateProvider);
        Replicator & s2 = *(secondary2.get());

        // Create config: P, I1, I2, I3 by building Idle replicas
        vector<ReplicaInformation> replicas;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        AddReplicaInfo(
            s2Id, s2.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        Copy(p, replicas);

        // Remove the last idle replica
        ErrorCode errorRemoveIdleReplica = p.RemoveReplica(s2Id);
        if (!errorRemoveIdleReplica.IsSuccess())
        {
            VERIFY_FAIL(L"RemoveReplica should have succeeded");
        }
        
        // Change role I1 -> S1, I2 -> S2
        ChangeRole(s0, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        ChangeRole(s1, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        vector<ReplicaInformation> replicas2;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas2);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas2);
        
        UpdateCatchUpReplicaSetConfiguration(p, replicas2);
        
        // Replicate operation 1; 
        // the operation will be replicated in epoch 122 (the primary initial epoch)
        Replicate(p, 1); 

        // Swap primary: S0 becomes new Primary
        Catchup(p);

        ++epoch.ConfigurationNumber;
        ReplicatorUpdateEpoch(p, epoch); // Update Primary's epoch mimicking RAP's call
        ReplicatorUpdateEpoch(s1, epoch);
        ReplicatorUpdateEpoch(s2, epoch);
        ChangeRole(p, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        
        // Since the replica is already built,
        // we could start the replication pump directly. 
        // However, test that starting the copy pump will start the replication one
        StartCopyOperationPump(p);
        
        ChangeRole(s0, epoch, FABRIC_REPLICA_ROLE_PRIMARY);
        vector<ReplicaInformation> replicas3;
        AddReplicaInfo(
            pId, p.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas3);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas3);

        UpdateCatchUpReplicaSetConfiguration(s0, replicas3);
                
        // 2 is generated in epoch 123
        Replicate(s0, 2);
        Catchup(s0);

        // Add the last replica to the list of replicas as a secondary
        ChangeRole(s2, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        AddReplicaInfo(
            s2Id, s2.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas3, 2);
        UpdateCatchUpReplicaSetConfiguration(s0, replicas3);
        
        // Remove old primary
        vector<ReplicaInformation> replicas4;
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas4);
        AddReplicaInfo(
            s2Id, s2.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas4);
        UpdateCatchUpReplicaSetConfiguration(s0, replicas4);
         
        ++epoch.ConfigurationNumber;
        ReplicatorUpdateEpoch(s0, epoch); // Update Primary's epoch mimicking RAP's call to check for the output value of previous epoch LSN

        CheckProgressVector(pStateProvider, L"0.0:0;1.122:1;1.123:-1;");
        Close(p);
        CheckProgressVector(s0StateProvider, L"0.0:1;1.123:2;1.124:-1;");
        Close(s0);
        CheckProgressVector(s1StateProvider, L"0.0:1;1.123:-1;");
        Close(s1);
        CheckProgressVector(s2StateProvider, L"0.0:-1;");
        Close(s2);

        transportPrimary->Stop();
        transportSecondary0->Stop();
        transportSecondary1->Stop();
        transportSecondary2->Stop();
    }

    void TestChangeRole::TestChangeRolePrimaryDown(bool hasPersistedState)
    {
        ComTestOperation::WriteInfo(
            ChangeRoleTestSource,
            "Start TestChangeRolePrimaryDown, persisted state {0}",
            hasPersistedState);
                
        ReplicatorSPtr primary;
        ComPointer<ComTestStateProvider> pStateProvider;
        FABRIC_REPLICA_ID pId = 1;

        ReplicatorSPtr secondary0;
        ComPointer<ComTestStateProvider> s0StateProvider;
        FABRIC_REPLICA_ID s0Id = 2;

        ReplicatorSPtr secondary1;
        ComPointer<ComTestStateProvider> s1StateProvider;
        FABRIC_REPLICA_ID s1Id = 3;

        FABRIC_EPOCH epoch;
        epoch.DataLossNumber = 21;
        epoch.ConfigurationNumber = 122;
        epoch.Reserved = NULL;

        int64 numberOfCopyOps = 1;
        int64 numberOfCopyContextOps = hasPersistedState ? 1 : -1;

        auto transportPrimary = CreateAndOpen(
            pId, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_PRIMARY, hasPersistedState, primary, pStateProvider);
        Replicator & p = *(primary.get());

        auto transportSecondary0 = CreateAndOpen(
            s0Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary0, s0StateProvider);
        Replicator & s0 = *(secondary0.get());

        auto transportSecondary1 = CreateAndOpen(
            s1Id, numberOfCopyOps, numberOfCopyContextOps, epoch, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, hasPersistedState, secondary1, s1StateProvider);
        Replicator & s1 = *(secondary1.get());

        // Create config: P, I1, I2 by building Idle replicas
        vector<ReplicaInformation> replicas;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_IDLE_SECONDARY, replicas);
        Copy(p, replicas);

        // Change role I1 -> S1, I2 -> S2
        ++epoch.ConfigurationNumber;
        ChangeRole(s0, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        ChangeRole(s1, epoch, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        vector<ReplicaInformation> replicas2;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas2);
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas2);
        UpdateCatchUpReplicaSetConfiguration(p, replicas2);

        Replicate(p, 1);

        // Remove the last replica, so the primary can't send it data
        ++epoch.ConfigurationNumber;
        vector<ReplicaInformation> replicas3;
        AddReplicaInfo(
            s0Id, s0.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas3);
        UpdateCatchUpReplicaSetConfiguration(p, replicas3);

        // S0 should receive 2, S1 shouldn't
        Replicate(p, 2);
        Catchup(p);

        // Close primary and make S0 primary, with one replica, S1
        Close(p);
        ++epoch.ConfigurationNumber;
        ChangeRole(s0, epoch, FABRIC_REPLICA_ROLE_PRIMARY);
        ReplicatorUpdateEpoch(s1, epoch);
        
        FABRIC_SEQUENCE_NUMBER last;
        VERIFY_IS_TRUE(s1.GetCurrentProgress(last).IsSuccess(), L"GetCurrentProgress succeeds");
        VERIFY_ARE_EQUAL(last, 1, L"s1 last progress is correct");

        vector<ReplicaInformation> replicas4;
        AddReplicaInfo(
            s1Id, s1.ReplicationEndpoint, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, replicas4, last);
        UpdateCatchUpReplicaSetConfiguration(s0, replicas4);
        
        // Wait for s0 to catch up S1 (send it 2)
        Catchup(s0);

        Close(s0);
        Close(s1);

        transportPrimary->Stop();
        transportSecondary0->Stop();
        transportSecondary1->Stop();
    }


    ReplicationTransportSPtr TestChangeRole::CreateAndOpen(
        FABRIC_REPLICA_ID replicaId,
        int64 numberOfCopyOps,
        int64 numberOfCopyContextOps,
        FABRIC_EPOCH const & epoch,
        FABRIC_REPLICA_ROLE role,
        bool hasPersistedState,
        __out ReplicatorSPtr & replicator,
        __out ComPointer<ComTestStateProvider> & stateProvider)
    {
        REConfigSPtr config = CreateGenericConfig();
        ComponentRoot const & root = GetRoot();
        ReplicationTransportSPtr transport = ReplicationTransport::Create(config->ReplicatorListenAddress);
        
        Common::Guid partitionId = PartitionId;

        VERIFY_IS_TRUE(transport->Start(L"localhost:0").IsSuccess());

        ComProxyStatefulServicePartition ra(
            make_com<ComTestStatefulServicePartition,IFabricStatefulServicePartition>(partitionId));

        ComProxyStateProvider sp(
            make_com<ComTestStateProvider,IFabricStateProvider>(
                numberOfCopyOps, 
                numberOfCopyContextOps, 
                root));

        auto transportCopy = transport;
        stateProvider.SetAndAddRef(static_cast<ComTestStateProvider*>(sp.InnerStateProvider));
        IReplicatorHealthClientSPtr temp;
        replicator = make_shared<Replicator>(
                REInternalSettings::Create(nullptr, config), 
                replicaId,
                partitionId,
                hasPersistedState,
                move(ra), 
                move(sp),
                Replicator::V1,
                move(transportCopy),
                move(temp));
        
        Replicator & repl = *(replicator.get());
        Open(repl);
        ChangeRole(repl, epoch, role);

        return transport;
    }

    void TestChangeRole::AddReplicaInfo(
        FABRIC_REPLICA_ID replicaId,
        wstring const & endpoint,
        FABRIC_REPLICA_ROLE role,
        __in vector<ReplicaInformation> & replicas,
        FABRIC_SEQUENCE_NUMBER progressLSN)
    {
        replicas.push_back(
            ReplicaInformation(
                replicaId,
                role,
                endpoint, 
                false,
                progressLSN,
                Constants::InvalidLSN));
    }

    void TestChangeRole::Copy(
        __in Replicator & repl,
        vector<ReplicaInformation> const & replicas)
    {
        ManualResetEvent copyDoneEvent;
        volatile size_t count = replicas.size();

        for(size_t i = 0; i < replicas.size(); ++i)
        {
            FABRIC_REPLICA_ID replicaId = replicas[i].Id;
            repl.BeginBuildReplica(replicas[i],
                [&repl, replicaId, &count, &copyDoneEvent](AsyncOperationSPtr const& operation) -> void
                {
                    ErrorCode error = repl.EndBuildReplica(operation);
                    if (!error.IsSuccess())
                    {
                        VERIFY_FAIL_FMT("Copy failed with error {0}.", error);
                    }

                    Trace.WriteInfo(ChangeRoleTestSource, "Copy done for replica {0}.", replicaId);
                    if (0 == InterlockedDecrement64((volatile LONGLONG *)&count))
                    {
                        copyDoneEvent.Set();
                    }

                }, AsyncOperationSPtr());
        }
    
        copyDoneEvent.WaitOne();  
    }
    
    void TestChangeRole::Catchup(__in Replicator & repl)
    {
        ManualResetEvent completeEvent;
        repl.BeginWaitForCatchUpQuorum(
            FABRIC_REPLICA_SET_QUORUM_ALL,
            [&repl, &completeEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.EndWaitForCatchUpQuorum(operation);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Wait for full progress completed successfully.");
                completeEvent.Set();
            }, AsyncOperationSPtr());

        completeEvent.WaitOne();
    }

    void TestChangeRole::ChangeRole(
        __in Replicator & repl, 
        FABRIC_EPOCH const & epoch,
        FABRIC_REPLICA_ROLE newRole)
    {
        ManualResetEvent completeEvent;
        repl.BeginChangeRole(
            epoch,
            newRole,
            [&repl, &completeEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.EndChangeRole(operation);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Replication engine ChangeRole completed successfully.");
                completeEvent.Set();
            }, AsyncOperationSPtr());

        completeEvent.WaitOne();
        if(newRole == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
        {
            // Start copy operation pump;
            // when all copy operations processed, the callback will
            // automatically switch to replication pump
            StartCopyOperationPump(repl);
        }
    }

    void TestChangeRole::StartCopyOperationPump(__in Replicator & repl)
    {
        OperationStreamSPtr stream;
        ErrorCode error = repl.GetCopyStream(stream);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "{0}: GetCopyStream returned error {1}", repl.ReplicaId, error);
        StartGetOperation(repl, stream);
    }

    void TestChangeRole::StartReplicationOperationPump(__in Replicator & repl)
    {
        OperationStreamSPtr stream;
        ErrorCode error = repl.GetReplicationStream(stream);
        VERIFY_IS_TRUE_FMT(error.IsSuccess(), "{0}: GetReplicationStream returned error {1}", repl.ReplicaId, error);
        StartGetOperation(repl, stream);
    }

    void TestChangeRole::StartGetOperation(__in Replicator & repl, OperationStreamSPtr const & stream)
    {
        for(;;)
        {
            auto op = stream->BeginGetOperation(
                [stream, &repl](AsyncOperationSPtr const& asyncOp)->void 
                { 
                    if(!asyncOp->CompletedSynchronously && 
                       TestChangeRole::EndGetOperation(repl, asyncOp, stream))
                    {
                        TestChangeRole::StartGetOperation(repl, stream);
                    }
                }, 
                nullptr);

            if(!op->CompletedSynchronously || (!EndGetOperation(repl, op, stream)))
            {
                break;
            }
        }
    }

    bool TestChangeRole::EndGetOperation(
        __in Replicator & repl,
        AsyncOperationSPtr const & asyncOp, 
        OperationStreamSPtr const & stream)
    {
        IFabricOperation * operation = nullptr;
        ErrorCode error = stream->EndGetOperation(asyncOp, operation); 
        VERIFY_IS_TRUE(error.IsSuccess(), L"GetOperation completed successfully.");
        if (operation != nullptr && operation->get_Metadata()->Type != FABRIC_OPERATION_TYPE_END_OF_STREAM)
        {
            FABRIC_OPERATION_METADATA const *opMetadata = operation->get_Metadata();
            ComTestOperation::WriteInfo(
                ChangeRoleTestSource, 
                "{0}: GetOperation returned: type {1}, sequence number {2}.", 
                repl.ReplicaId,
                opMetadata->Type, 
                opMetadata->SequenceNumber);

            ULONG bufferCount = 0;
            FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

            VERIFY_SUCCEEDED(operation->GetData(&bufferCount, &replicaBuffers));

            if (opMetadata->SequenceNumber <= 0)
            {
                VERIFY_FAIL(L"EndGetOperation returned non-null op with invalid lsn");
            }

            // ACK and release the operation
            operation->Acknowledge();
            operation->Release();
            return true;
        }
        else
        {
            if (operation)
            {
                ComTestOperation::WriteInfo(ChangeRoleTestSource, "{0}: Received sentinel operation.", stream->Purpose);
                operation->Acknowledge();
            }

            ComTestOperation::WriteInfo(ChangeRoleTestSource, "{0}: Received null operation.", stream->Purpose);
            if (stream->Purpose == Constants::CopyOperationTrace)
            {
                StartReplicationOperationPump(repl);
            }
            return false;
        }
    }

    void TestChangeRole::Open(
        __in Replicator & repl)
    {
        ManualResetEvent openCompleteEvent;
        repl.BeginOpen(
            [&repl, &openCompleteEvent](AsyncOperationSPtr const& operation) -> void
            {
                wstring endpoint;
                ErrorCode error = repl.EndOpen(operation, endpoint);
                if (error.IsSuccess())
                {
                    Trace.WriteInfo(ChangeRoleTestSource, "Replication engine opened successfully at {0}.", endpoint);
                }
                else
                {
                    VERIFY_FAIL(L"Error opening replication engine");
                }
                openCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(openCompleteEvent.WaitOne(10000), L"Open should succeed");
    }

    void TestChangeRole::Close(__in Replicator & repl)
    {
        ManualResetEvent closeCompleteEvent;
        repl.BeginClose(
            [&repl, &closeCompleteEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.EndClose(operation);
                if (!error.IsSuccess())
                {
                    VERIFY_FAIL(L"Error closing replication engine");
                }
                closeCompleteEvent.Set();
            }, AsyncOperationSPtr());

        VERIFY_IS_TRUE(closeCompleteEvent.WaitOne(10000), L"Close should succeed");
    }

    void TestChangeRole::Replicate(
        __in Replicator & repl, 
        FABRIC_SEQUENCE_NUMBER expectedSequenceNumber)
    {
        ManualResetEvent replicateDoneEvent;
        auto operation = make_com<ComTestOperation,IFabricOperationData>(L"ReplicatorChangeRole - test replicate operation");
        FABRIC_SEQUENCE_NUMBER sequenceNumber;
        repl.BeginReplicate(
            move(operation), 
            sequenceNumber,
            [&repl, &replicateDoneEvent](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.EndReplicate(operation);
                if (!error.IsSuccess())
                {
                    VERIFY_FAIL_FMT("Replication failed with error {0}.", error);
                }

                replicateDoneEvent.Set();
            }, AsyncOperationSPtr());

        Trace.WriteInfo(ChangeRoleTestSource, "Replicate operation returned {0} sequence number.", sequenceNumber);
        VERIFY_ARE_EQUAL(expectedSequenceNumber, sequenceNumber, 
            L"Replicate operation returned expected sequence number");
                
        replicateDoneEvent.WaitOne();            
    }

    void TestChangeRole::Replicate(
        __in Replicator & repl,
        const ErrorCode expectedError)
    {
        auto operation = make_com<ComTestOperation, IFabricOperationData>(L"ReplicatorChangeRole - test replicate operation");
        FABRIC_SEQUENCE_NUMBER sequenceNumber;
        auto ptr = repl.BeginReplicate(
            move(operation),
            sequenceNumber,
            [&repl](AsyncOperationSPtr const&) -> void
        {
            VERIFY_FAIL(L"User callback must not be called.");
        }, AsyncOperationSPtr());

        VERIFY_ARE_EQUAL(ptr->Error.ReadValue(), expectedError.ReadValue(), L"Unexpected Error code.")

        Trace.WriteInfo(ChangeRoleTestSource, "Replicate operation returned {0} sequence number.", sequenceNumber);
        VERIFY_ARE_EQUAL(-1, sequenceNumber,
            L"Replicate operation returned expected sequence number");
    }

    void TestChangeRole::StartReplicate(
        __in Replicator & repl,
        __in ManualResetEvent & replicateDoneEvent,
        __in DateTime & replicateDoneTime,
        FABRIC_SEQUENCE_NUMBER expectedSequenceNumber)
    {
        auto operation = make_com<ComTestOperation,IFabricOperationData>(L"ReplicatorChangeRole - test replicate operation");
        FABRIC_SEQUENCE_NUMBER sequenceNumber;
        repl.BeginReplicate(
            move(operation), 
            sequenceNumber,
            [&repl, &replicateDoneEvent, &replicateDoneTime](AsyncOperationSPtr const& operation) -> void
            {
                ErrorCode error = repl.EndReplicate(operation);
                if (!error.IsSuccess())
                {
                    VERIFY_FAIL_FMT("Replication failed with error {0}.", error);
                }

                replicateDoneTime = DateTime::Now();
                replicateDoneEvent.Set();
            }, AsyncOperationSPtr());

        Trace.WriteInfo(ChangeRoleTestSource, "Replicate operation returned {0} sequence number.", sequenceNumber);
        VERIFY_ARE_EQUAL(expectedSequenceNumber, sequenceNumber, 
            L"Replicate operation returned expected sequence number");
               
        // Do not wait for replicate to finish, it's probably stuck
    }

    void TestChangeRole::EndReplicate(
        __in ManualResetEvent & replicateDoneEvent,
        DateTime const & replicateDoneTime,
        DateTime const & compareTime)
    {
        replicateDoneEvent.WaitOne();
        VERIFY_IS_TRUE_FMT(replicateDoneTime >= compareTime,
                "Replicate completed at {0}, compare time is {0}", 
                replicateDoneTime, 
                compareTime);
    }

    void TestChangeRole::UpdateCatchUpReplicaSetConfiguration(
        __in Replicator & repl, 
        vector<ReplicaInformation> const & replicas)
    {
        ULONG quorum = static_cast<ULONG>((replicas.size() + 1) / 2 + 1);
        ULONG previousQuorum = 0;
        vector<ReplicaInformation> previousReplicas;
        ErrorCode error = repl.UpdateCatchUpReplicaSetConfiguration(previousReplicas, previousQuorum, replicas, quorum);
        if (!error.IsSuccess())
        {
            VERIFY_FAIL_FMT("UpdateCatchUpReplicaSetConfiguration failed with {0}", error);
        }
        error = repl.UpdateCurrentReplicaSetConfiguration(replicas, quorum);
        if (!error.IsSuccess())
        {
            VERIFY_FAIL_FMT("UpdateCurrentReplicaSetConfiguration failed with {0}", error);
        }
    }

    void TestChangeRole::CheckProgressVector(
        __in ComPointer<ComTestStateProvider> & sp, 
        wstring const & expectedProgressVector)
    {
        wstring progressVector = sp->GetProgressVectorString();
        Trace.WriteInfo(ChangeRoleTestSource, "Expected progress vector: \"{0}\", received: \"{1}\"", expectedProgressVector, progressVector);
        VERIFY_ARE_EQUAL(expectedProgressVector, progressVector);
    }

    void TestChangeRole::ReplicatorUpdateEpoch(
        Replicator & replicator,
        FABRIC_EPOCH const & newEpoch)
    {
        ManualResetEvent updateEpochCompleteEvent;
        replicator.BeginUpdateEpoch(
            newEpoch,
            [&updateEpochCompleteEvent, &replicator](AsyncOperationSPtr const & asyncOperation)
            {
                ErrorCode error = replicator.EndUpdateEpoch(asyncOperation);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Replication engine UpdateEpoch successfully.");
                updateEpochCompleteEvent.Set();
            },
            AsyncOperationSPtr());
        VERIFY_IS_TRUE(updateEpochCompleteEvent.WaitOne(10000), L"UpdateEpoch should succeed");
    }
}
