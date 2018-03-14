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

namespace
{
    const wstring FT = L"SP1";

    namespace ExpectedReplicaUploadState
    {
        enum Enum
        {
            None, // not added
            DeferredUpload,
            ImmediateUpload
        };
    }

    namespace ExpectedFTState
    {
        enum Enum
        {
            Closed,
            Down,
            Up
        };
    }

    namespace UpgradeType
    {
        enum Enum
        {
            None,
            InvalidInstance,
            UpdatedInstance,
            PackageNotFound,
            RandomApplicationWithInvalidInstance,
            RandomApplicationAndUpdatedInstance,
        };
    }
}

class TestStateMachine_NodeUpAckProcessing : public StateMachineTestBase
{
protected:

    void AddOpenFT(bool isDeleted)
    {
        AddFT(L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]", isDeleted);
    }

    void AddClosedFT(bool isDeleted)
    {
        AddFT(L"C None 000/000/411 1:1 -", isDeleted);
    }

    void AddFT(wstring const & state, bool isDeleted)
    {
        Test.AddFT(FT, state);

        auto & ft = Test.GetFT(FT);
        
        ft.Test_SetLocalReplicaDeleted(isDeleted);
        
        if (isDeleted && ft.IsOpen)
        {
            ft.LocalReplica.State = ReconfigurationAgentComponent::ReplicaStates::InDrop;
        }
    }

    void Execute(bool isActivated)
    {
        Execute(isActivated, UpgradeType::None);
    }

    UpgradeDescription CreateUpgradeDescriptionForRandomApp(int instance)
    {
        auto description = StateManagement::Reader::ReadHelper<UpgradeDescription>(wformatString("{0} Rolling false [SP1 2.0] |", instance));
        description.Specification.ApplicationId = ServiceModel::ApplicationIdentifier(L"SomeAppType", 1929);
        return description;
    }

    void Execute(bool isActivated, UpgradeType::Enum e)
    {
        vector<UpgradeDescription> upgrades;

        switch (e)
        {
        case UpgradeType::InvalidInstance:
            upgrades.push_back(StateManagement::Reader::ReadHelper<UpgradeDescription>(L"0 Rolling false |"));
            break;

        case UpgradeType::RandomApplicationAndUpdatedInstance:
            upgrades.push_back(CreateUpgradeDescriptionForRandomApp(0));
            upgrades.push_back(StateManagement::Reader::ReadHelper<UpgradeDescription>(L"2 Rolling false [SP1 2.0] |"));
            break;
            
        case UpgradeType::UpdatedInstance:
            upgrades.push_back(StateManagement::Reader::ReadHelper<UpgradeDescription>(L"2 Rolling false [SP1 2.0] |"));
            break;

        case UpgradeType::PackageNotFound:
            upgrades.push_back(StateManagement::Reader::ReadHelper<UpgradeDescription>(L"2 Rolling false [SP2 2.0] |"));
            break;

        case UpgradeType::RandomApplicationWithInvalidInstance:
            upgrades.push_back(CreateUpgradeDescriptionForRandomApp(0));
            break;

        default:
            break;
        }

        auto & ft = Test.GetFT(FT);
        commitDescriptionUtility_ = Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            FT,
            [&](Infrastructure::EntityExecutionContext & base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.ProcessNodeUpAck(upgrades, isActivated, context);
        });
    }

    void VerifyReplicaUploadState(ExpectedReplicaUploadState::Enum e)
    {
        bool isInSet = e != ExpectedReplicaUploadState::None;
        FMMessageStage::Enum expectedFMMessageStage = e == ExpectedReplicaUploadState::ImmediateUpload ? FMMessageStage::ReplicaUpload : FMMessageStage::None;

        auto & state = Test.GetFT(FT).ReplicaUploadStateObj;
        Verify::AreEqual(isInSet, state.IsUploadPending, L"UploadPending");
        Verify::AreEqual(expectedFMMessageStage, Test.GetFT(FT).FMMessageStateObj.MessageStage, L"Message stage");
    }

    void VerifyFTState(ExpectedFTState::Enum e)
    {
        auto & ft = Test.GetFT(FT);
        switch (e)
        {
        case ExpectedFTState::Closed:
            Verify::IsTrue(ft.IsClosed, wformatString("IsClosed {0}", ft));
            break;

        case ExpectedFTState::Down:
            Verify::IsTrue(ft.IsOpen && !ft.LocalReplica.IsUp, wformatString("Should be down {0}", ft));
            break;

        case ExpectedFTState::Up:
            Verify::IsTrue(ft.IsOpen && ft.LocalReplica.IsUp, wformatString("Should be up {0}", ft));
            break;

        default:
            Verify::Fail(wformatString("Unknown Expected FT State {0}", static_cast<int>(e)));
        }
    }

    void RunClosedFTReplicaUploadTest(bool isDeleted, bool isNodeActivated, ExpectedReplicaUploadState::Enum e)
    {
        AddClosedFT(isDeleted);

        Execute(isNodeActivated);

        VerifyReplicaUploadState(e);
    }

    void RunOpenFTReplicaUploadTest(bool isDeleted, bool isNodeActivated, ExpectedReplicaUploadState::Enum e)
    {
        AddOpenFT(isDeleted);

        Execute(isNodeActivated);

        VerifyReplicaUploadState(e);
    }

    void RunFTReplicaLifecycleTest(bool isClosed, bool isActivated, ExpectedFTState::Enum e)
    {
        if (isClosed)
        {
            AddClosedFT(false);
        }
        else
        {
            AddOpenFT(false);
        }

        Execute(isActivated);

        VerifyFTState(e);
    }

    void VerifyPackageVersionInstance(uint64 instance)
    {
        Verify::AreEqual(instance, Test.GetFT(FT).LocalReplica.PackageVersionInstance.InstanceId, L"Package version instance");
    }

    CommitTypeDescriptionUtility commitDescriptionUtility_;
};

BOOST_AUTO_TEST_SUITE(StateMachine)

BOOST_FIXTURE_TEST_SUITE(TestStateMachine_NodeUpAckProcessingSuite, TestStateMachine_NodeUpAckProcessing)

BOOST_AUTO_TEST_CASE(ReplicaUpload_ClosedFT_IsAdded)
{
    RunClosedFTReplicaUploadTest(false, true, ExpectedReplicaUploadState::ImmediateUpload);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_DeletedClosedFT_IsNotAdded)
{
    RunClosedFTReplicaUploadTest(true, true, ExpectedReplicaUploadState::None);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_ClosedFT_IsAdded_NodeDeactivated)
{
    RunClosedFTReplicaUploadTest(false, false, ExpectedReplicaUploadState::ImmediateUpload);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_DeletedClosedFT_IsNotAdded_NodeDeactivated)
{
    RunClosedFTReplicaUploadTest(true, false, ExpectedReplicaUploadState::None);
}

BOOST_AUTO_TEST_CASE(LifeCycle_ClosedFTStaysClosed)
{
    RunFTReplicaLifecycleTest(true, true, ExpectedFTState::Closed);
}

BOOST_AUTO_TEST_CASE(LifeCycle_ClosedFTStaysClosed_NodeDeactivated)
{
    RunFTReplicaLifecycleTest(true, false, ExpectedFTState::Closed);
}

BOOST_AUTO_TEST_CASE(LifeCycle_OpenFT_IsUp_Activated)
{
    RunFTReplicaLifecycleTest(false, true, ExpectedFTState::Up);
}

BOOST_AUTO_TEST_CASE(LifeCycle_OpenFT_IsDown_Deactivated)
{
    RunFTReplicaLifecycleTest(false, false, ExpectedFTState::Down);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_OpenFT_IsAdded)
{
    RunOpenFTReplicaUploadTest(false, true, ExpectedReplicaUploadState::DeferredUpload);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_DeletedOpenFT_IsNotAdded)
{
    RunOpenFTReplicaUploadTest(true, true, ExpectedReplicaUploadState::None);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_OpenFT_IsAdded_Deactivated)
{
    RunOpenFTReplicaUploadTest(false, false, ExpectedReplicaUploadState::ImmediateUpload);
}

BOOST_AUTO_TEST_CASE(ReplicaUpload_DeletedOpenFT_IsNotAdded_Deactivated)
{
    RunOpenFTReplicaUploadTest(true, false, ExpectedReplicaUploadState::None);
}

BOOST_AUTO_TEST_CASE(Upgrade_VersionIsUpgraded)
{
    AddOpenFT(false);

    Execute(true, UpgradeType::UpdatedInstance);

    VerifyPackageVersionInstance(2);
}

BOOST_AUTO_TEST_CASE(Upgrade_DifferentPackage_IsNoOp)
{
    AddOpenFT(false);

    Execute(true, UpgradeType::PackageNotFound);

    VerifyPackageVersionInstance(1);
}

BOOST_AUTO_TEST_CASE(Upgrade_DifferentApp_IsNoOp)
{
    AddOpenFT(false);
    
    Execute(true, UpgradeType::RandomApplicationWithInvalidInstance);

    VerifyFTState(ExpectedFTState::Up);
}

BOOST_AUTO_TEST_CASE(Upgrade_AppIdIsConsideredForInvalidInstanceDrop)
{
    AddOpenFT(false);

    Execute(true, UpgradeType::RandomApplicationAndUpdatedInstance);

    VerifyFTState(ExpectedFTState::Up);

    VerifyPackageVersionInstance(2);
}

BOOST_AUTO_TEST_CASE(Upgrade_InvalidInstanceCauses_Drop)
{
    AddOpenFT(false);

    Execute(true, UpgradeType::InvalidInstance);

    VerifyFTState(ExpectedFTState::Closed);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
