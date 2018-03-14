// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef TESTCHANGEROLE_H
#define TESTCHANGEROLE_H

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ComTestStatefulServicePartition.h"
#include "ComTestStateProvider.h"
#include "ComTestOperation.h"

namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability::ReplicationComponent;

    typedef std::shared_ptr<REConfig> REConfigSPtr;

    class TestChangeRole
    {
    public:
        static ReplicationTransportSPtr CreateAndOpen(
            FABRIC_REPLICA_ID replicaId,
            int64 numberOfCopyOps,
            int64 numberOfCopyContextOps,
            FABRIC_EPOCH const & epoch,
            FABRIC_REPLICA_ROLE role,
            bool hasPersistedState,
            __out ReplicatorSPtr & replicator,
            __out ComPointer<ComTestStateProvider> & stateProvider);

        static void ChangeRole(__in Replicator & repl, FABRIC_EPOCH const & epoch, FABRIC_REPLICA_ROLE newRole);

        static void Catchup(__in Replicator & repl);

        static void Open(__in Replicator & repl);

        static void Close(__in Replicator & repl);

    protected:        
        TestChangeRole() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup)

        static void StartCopyOperationPump(__in Replicator & repl);

        static void StartReplicationOperationPump(__in Replicator & repl);

        static void StartGetOperation(__in Replicator & repl, OperationStreamSPtr const & stream);

        static bool EndGetOperation(
            __in Replicator & repl,
            AsyncOperationSPtr const & asyncOp,
            OperationStreamSPtr const & stream);

        static REConfigSPtr CreateGenericConfig();

        static void AddReplicaInfo(
            FABRIC_REPLICA_ID replicaId,
            wstring const & endpoint,
            FABRIC_REPLICA_ROLE role,
            __in vector<ReplicaInformation> & replicas,
            FABRIC_SEQUENCE_NUMBER progressLSN = Constants::InvalidLSN);

        static void Copy(__in Replicator & repl, vector<ReplicaInformation> const & replicas);

        static void Replicate(__in Replicator & repl, FABRIC_SEQUENCE_NUMBER expectedSequenceNumber);
        static void Replicate(__in Replicator & repl, const ErrorCode expectedErrorCode);

        static void StartReplicate(
            __in Replicator & repl,
            __in ManualResetEvent & replicateDoneEvent,
            __in DateTime & replicateDoneTime,
            FABRIC_SEQUENCE_NUMBER expectedSequenceNumber);

        static void EndReplicate(
            __in ManualResetEvent & replicateDoneEvent,
            DateTime const & replicateDoneTime,
            DateTime const & compareTime);

        static void UpdateCatchUpReplicaSetConfiguration(
            __in Replicator & repl,
            vector<ReplicaInformation> const & replicas);

        static void CheckProgressVector(
            __in ComPointer<ComTestStateProvider> & sp,
            wstring const & expectedProgressVector);

        static void ReplicatorUpdateEpoch(
            Replicator & replicator,
            FABRIC_EPOCH const & newEpoch);

        static void TestChangeRolePrimaryDown(bool hasPersistedState);
        static void TestSimpleChangeRole(bool hasPersistedState);
        static void TestReplicateCallbackNotCalledIfSynchronouslyFail(bool hasPersistedState);

        static Common::Guid PartitionId;

        class DummyRoot : public Common::ComponentRoot
        {
        };

        static Common::ComponentRoot const & GetRoot();
    };
}

#endif
