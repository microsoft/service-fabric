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

class TestStateMachine_ParallelPhase3Phase4
{
protected:
    void RunAndVerify(wstring const & replicaStr, bool expected, wstring const & tag)
    {
        auto v = Reader::ReadVector<Replica>(L'[', L']', replicaStr);

        holder_ = ReplicaStoreHolder(move(v));

        auto actual = FailoverUnit::ShouldSkipPhase3Deactivate(holder_.ReplicaStoreObj.ConfigurationReplicas);

        Verify::AreEqual(expected, actual, wformatString("Expected {0}. Actual {1}. {2}. {3}", expected, actual, tag, replicaStr));
    }

private:
    ReplicaStoreHolder holder_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_ParallelPhase3Phase4Suite, TestStateMachine_ParallelPhase3Phase4)

BOOST_AUTO_TEST_CASE(Phase3Phase4Test)
{
    RunAndVerify(L"[P/P 1]", true, L"Single replica");
    
    RunAndVerify(L"[P/S 1] [S/P 2]", true, L"Swap 2 replicas");
    RunAndVerify(L"[P/S 1] [S/S 2] [S/P 3]", true, L"Swap 3 replicas");
    RunAndVerify(L"[P/S 1] [S/P 2] [S/S 3] [S/S 4]", true, L"Swap 4 replicas");
    
    RunAndVerify(L"[P/S 1] [S/P 2] [S/S 3]", true, L"Failure MRSS odd");
    RunAndVerify(L"[P/S 1] [S/P 2] [S/S 3] [S/S 4]", true, L"Failure MRSS even");

    RunAndVerify(L"[P/P 1] [S/S 2] [S/S 3]", true, L"Failure MRSS odd no role change");
    RunAndVerify(L"[P/P 1] [S/S 2] [S/S 3] [S/S 4]", true, L"Failure MRSS even no role change");

    RunAndVerify(L"[P/P 1] [I/S 2]", false, L"upshift 1");
    RunAndVerify(L"[P/P 1] [I/S 2] [I/S 3]", false, L"upshift 2");
    RunAndVerify(L"[P/P 1] [I/S 2] [I/S 3] [I/S 4]", false, L"upshift 3");

    RunAndVerify(L"[P/P 1] [S/I 2]", false, L"downshift 1");
    RunAndVerify(L"[P/P 1] [S/I 2] [S/I 3]", false, L"downshift 2");
    RunAndVerify(L"[P/P 1] [S/I 2] [S/I 3] [S/I 4]", false, L"downshift 3");

    RunAndVerify(L"[P/P 1] [S/I 2] [I/S 3]", false, L"upshift+downshift odd");
    RunAndVerify(L"[P/P 1] [S/I 2] [I/S 3] [S/S 4]", false, L"upshift+downshift even");
    RunAndVerify(L"[P/P 1] [S/I 2] [I/S 3] [S/I 4] [I/S 5]", false, L"upshift+downshift 2 odd");
    RunAndVerify(L"[P/P 1] [S/I 2] [I/S 3] [S/I 4] [I/S 5] [S/S 6]", false, L"upshift+downshift 2 odd");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
