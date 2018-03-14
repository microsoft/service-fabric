// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RATestHeaders.h"

using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;
using namespace std;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

class TestReplicaStore
{
protected:
    void AddReplicas(wstring const & s)
    {
        vector<Replica> v;
        Reader r(s, ReadContext()); 
        bool check = r.ReadVector(L'[', L']', v);
        Verify::IsTrueLogOnError(check, L"Failed to read vector");

        holder_ = ReplicaStoreHolder(move(v));
    }

    void AddReplica(int replicaId)
    {
        AddReplica(replicaId, ReplicaRole::Primary, ReplicaRole::Primary);
    }

    void AddReplica(int replicaId, ReplicaRole::Enum pcRole, ReplicaRole::Enum ccRole)
    {
        NodeInstance ni(CreateNodeId(replicaId), 1);
        ReplicaDescription d(ni, replicaId, pcRole, ccRole);
        holder_.ReplicaStoreObj.AddReplica(d, pcRole, ccRole);
    }

    void RemoveReplica(int64 replicaId)
    {
        holder_.ReplicaStoreObj.RemoveReplica(replicaId);
    }

    void Remove(initializer_list<int64> replicas)
    {
        vector<int64> v(replicas.begin(), replicas.end());

        holder_.ReplicaStoreObj.Remove(v);
    }

    void VerifyReplica(int replicaIdExpected, Replica const & actual)
    {
        Verify::AreEqual(replicaIdExpected, actual.ReplicaId, L"ReplicaId");
    }

    void VerifyInvalid(Replica const & actual)
    {
        Verify::IsTrue(actual.IsInvalid, L"Must be invalid");
    }

    NodeId CreateNodeId(int nodeId)
    {
        return NodeId(LargeInteger(0, nodeId));
    }

    void GetAndVerifyExists(int node)
    {
        auto & replica = holder_.ReplicaStoreObj.GetReplica(CreateNodeId(node));
        VerifyReplica(node, replica);
    }

    void GetAndVerifyInvalid(int node)
    {
        auto & replica = holder_.ReplicaStoreObj.GetReplica(CreateNodeId(node));
        VerifyInvalid(replica);
    }

    template<typename T>
    void Verify(initializer_list<int> expectedE, T & t)
    {
        vector<int> expected(expectedE.begin(), expectedE.end());

        vector<int> actual;
        for (auto const & it : t)
        {
            actual.push_back(static_cast<int>(it.ReplicaId));
        }

        Verify::VectorStrict(expected, actual);
    }

    void VerifyConfigurationReplicas(initializer_list<int> expectedE)
    {
        Verify(expectedE, holder_.ReplicaStoreObj.ConfigurationReplicas);
    }

    void VerifyConfigurationRemoteReplicas(initializer_list<int> expectedE)
    {
        Verify(expectedE, holder_.ReplicaStoreObj.ConfigurationRemoteReplicas);
    }

    void VerifyIdleReplicas(initializer_list<int> idleE)
    {
        Verify(idleE, holder_.ReplicaStoreObj.IdleReplicas);
    }

    void VerifyIsLocalReplica(int id, bool expected)
    {
        Verify::AreEqualLogOnError(expected, holder_.ReplicaStoreObj.IsLocalReplica(holder_.ReplicaStoreObj.GetReplica(CreateNodeId(id))), L"IsLocal");
    }

protected:
    ReplicaStoreHolder holder_;
};


BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestReplicaStoreSuite, TestReplicaStore)

BOOST_AUTO_TEST_CASE(GetReplicaByNodeIdReturnsReplica)
{
    AddReplica(1);
    AddReplica(2);
    AddReplica(3);

    auto const & constStore = holder_.ReplicaStoreObj;

    {
        auto & replica = holder_.ReplicaStoreObj.GetReplica(CreateNodeId(1));
        VerifyReplica(1, replica);
    }

    {
        auto & replica = constStore.GetReplica(CreateNodeId(1));
        VerifyReplica(1, replica);
    }

    {
        auto & replica = holder_.ReplicaStoreObj.GetReplica(CreateNodeId(2));
        VerifyReplica(2, replica);
    }

    {
        auto & replica = constStore.GetReplica(CreateNodeId(2));
        VerifyReplica(2, replica);
    }
}

BOOST_AUTO_TEST_CASE(GetReplicaByNodeIdNegativeTest)
{
    auto const & constStore = holder_.ReplicaStoreObj;

    {
        auto & replica = holder_.ReplicaStoreObj.GetReplica(CreateNodeId(1));
        VerifyInvalid(replica);
    }

    {
        auto const & replica = constStore.GetReplica(CreateNodeId(1));
        VerifyInvalid(replica);
    }

    AddReplica(1);

    {
        auto & replica = holder_.ReplicaStoreObj.GetReplica(CreateNodeId(2));
        VerifyInvalid(replica);
    }

    {
        auto const & replica = constStore.GetReplica(CreateNodeId(2));
        VerifyInvalid(replica);
    }
}

BOOST_AUTO_TEST_CASE(RemoveReplicaMakingVectorEmpty)
{
    AddReplica(1);

    RemoveReplica(1);

    GetAndVerifyInvalid(1);
}

BOOST_AUTO_TEST_CASE(RemoveMiddleReplica)
{
    AddReplica(1);
    AddReplica(2);
    AddReplica(3);

    RemoveReplica(2);

    GetAndVerifyExists(1);
    GetAndVerifyInvalid(2);
    GetAndVerifyExists(3);
}

BOOST_AUTO_TEST_CASE(RemoveManyReplicas)
{
    AddReplicas(L"[P/P 1] [I/S 2] [N/I 3]");

    Remove({ 2, 3 });
    GetAndVerifyExists(1);
    GetAndVerifyInvalid(2);
    GetAndVerifyInvalid(3);
}

BOOST_AUTO_TEST_CASE(IsLocalReplica)
{
    AddReplicas(L"[P/P 1] [I/S 2] [N/I 3]");

    VerifyIsLocalReplica(1, true);
    VerifyIsLocalReplica(2, false);
    VerifyIsLocalReplica(3, false);
    VerifyIsLocalReplica(4, false);
}

BOOST_AUTO_TEST_CASE(ConfigurationReplicas_IS)
{
    AddReplicas(L"[P/P 1] [I/S 2] [S/S 3] [N/I 4]");

    VerifyConfigurationReplicas({ 1, 2, 3 });
    VerifyConfigurationRemoteReplicas({ 2, 3 });
}

BOOST_AUTO_TEST_CASE(ConfigurationReplicas_SN)
{
    AddReplicas(L"[P/P 1] [I/S 2] [S/N 3] [N/I 4]");

    VerifyConfigurationReplicas({ 1, 2, 3 });
    VerifyConfigurationRemoteReplicas({ 2, 3 });
}

BOOST_AUTO_TEST_CASE(ConfigurationReplicas_Swap)
{
    AddReplicas(L"[P/S 1] [S/P 2]");

    VerifyConfigurationReplicas({ 1, 2});
    VerifyConfigurationRemoteReplicas({ 2});
}

BOOST_AUTO_TEST_CASE(ConfigurationReplicas_IP)
{
    AddReplicas(L"[I/P 1] [S/S 2] [P/S 3]");

    VerifyConfigurationReplicas({ 1, 2, 3 });
    VerifyConfigurationRemoteReplicas({ 2, 3 });
}

BOOST_AUTO_TEST_CASE(ConfigurationReplicas_SI)
{
    AddReplicas(L"[S/P 1] [S/S 2] [S/I 3]");

    VerifyConfigurationReplicas({ 1, 2, 3 });
    VerifyConfigurationRemoteReplicas({ 2, 3 });
}

BOOST_AUTO_TEST_CASE(ConfigurationReplicas_PI)
{
    AddReplicas(L"[S/P 1] [S/S 2] [P/I 3] [I/S 4]");

    VerifyConfigurationReplicas({ 1, 2, 3, 4 });
    VerifyConfigurationRemoteReplicas({ 2, 3, 4 });
}

BOOST_AUTO_TEST_CASE(IdleReplica_Empty)
{
    VerifyIdleReplicas({});
}

BOOST_AUTO_TEST_CASE(IdleReplica_LocalReplicaIsIdle)
{
    AddReplicas(L"[N/I 1]");
    VerifyIdleReplicas({});
}

BOOST_AUTO_TEST_CASE(IdleReplica_LocalReplicaIsP)
{
    AddReplicas(L"[N/P 1]");
    VerifyIdleReplicas({});
}

BOOST_AUTO_TEST_CASE(IdleReplica_LocalReplicaWIthIdle)
{
    AddReplicas(L"[N/P 1] [N/I 2]");
    VerifyIdleReplicas({2});
}

BOOST_AUTO_TEST_CASE(IdleReplica_LocalReplicaWIthMultipleIdle)
{
    AddReplicas(L"[N/P 1] [N/I 2] [N/I 3]");
    VerifyIdleReplicas({ 2, 3 });
}

BOOST_AUTO_TEST_CASE(IdleReplica_LocalReplicaWIthConfiguration)
{
    AddReplicas(L"[N/P 1] [N/S 2] [N/I 3]");
    VerifyIdleReplicas({ 3 });
}

BOOST_AUTO_TEST_CASE(IdleReplica_LocalReplicaDuringReconfiguration)
{
    AddReplicas(L"[P/P 1] [S/S 2] [S/I 3] [S/N 4] [I/S 5] [N/I 6]");
    VerifyIdleReplicas({ 6 });
}

BOOST_AUTO_TEST_CASE(Enumeration)
{
    AddReplicas(L"[P/P 1] [S/S 2]");

    Verify({ 1, 2 }, holder_.ReplicaStoreObj);

    auto const & store = holder_.ReplicaStoreObj;
    Verify({ 1, 2 }, store);
}

BOOST_AUTO_TEST_CASE(ClearTest)
{
    AddReplicas(L"[P/P 1] [S/S 2]");

    holder_.ReplicaStoreObj.Clear();

    Verify({}, holder_.ReplicaStoreObj);
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
