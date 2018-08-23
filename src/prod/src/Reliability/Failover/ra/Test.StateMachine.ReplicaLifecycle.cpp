// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReplicaUp;
using namespace Infrastructure;
using namespace std;
using namespace ReconfigurationAgentComponent::Communication;

using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateItemHelper;

#pragma region On App Upgrade
namespace OnApplicationUpgrade
{
    wstring const DownFT = L"O None 000/000/411 {411:1} 1:1 - [N/N/P SB D N F 1:1]";
    wstring const OpenFT = L"O None 000/000/411 {411:1} 1:1 CM [N/N/P RD U N F 1:1]";
    wstring const FT = L"SP1";

    class TestLifeCycle_OnApplicationUpgrade : public StateMachineTestBase
    {
    protected:
        void RunTest(wstring const& initial, wstring const & upgradeDescriptionStr)
        {
            Test.AddFT(FT, initial);

            auto & ft = Test.GetFT(FT);
            original_ = ft.LocalReplica.PackageVersionInstance;
            
            auto description = Reader::ReadHelper<UpgradeDescription>(upgradeDescriptionStr);
            commit_ = Test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
                              FT,
                              [&](EntityExecutionContext & base)
                              {
                                  auto & context = base.As<FailoverUnitEntityExecutionContext>();
                                  ft.OnApplicationCodeUpgrade(description.Specification, context);
                              });
        }

        void RunServiceDescriptionAndLocalReplicaVersionEqualityTest(wstring const & initial)
        {
            wstring upgradeDescriptionStr = L"2 Rolling false [SP1 2.0] |";
            wstring newVersionStr = L"2.0:2.0:2";

            ServiceModel::ServicePackageVersionInstance newVersion = Reader::ReadHelper<ServiceModel::ServicePackageVersionInstance>(newVersionStr);

            Recreate();

            RunTest(initial, upgradeDescriptionStr);

            FailoverUnit& ft = Test.GetFT(FT);

            Verify::AreEqual(newVersion, ft.LocalReplica.PackageVersionInstance, L"Local replica package version");
            Verify::AreEqual(newVersion, ft.ServiceDescription.PackageVersionInstance, L"Service Description package version");
        }

        void VerifyOriginalVersion()
        {
            Verify::AreEqual(original_, Test.GetFT(FT).LocalReplica.PackageVersionInstance, L"Package version instance");
        }

        ServiceModel::ServicePackageVersionInstance original_;
        CommitTypeDescriptionUtility commit_;
    };

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_OnApplicationUpgradeSuite, TestLifeCycle_OnApplicationUpgrade)

    BOOST_AUTO_TEST_CASE(ApplicationUpgradeFixesVersionInServiceDescriptionAndLocalReplica)
    {
        RunServiceDescriptionAndLocalReplicaVersionEqualityTest(DownFT);
        RunServiceDescriptionAndLocalReplicaVersionEqualityTest(OpenFT);
    }

    BOOST_AUTO_TEST_CASE(ServicePackageNotBeingUpgradedDoesNotChangeTheVersion)
    {
        RunTest(OpenFT, L"2 Rolling false [SP2 2.0] |");

        commit_.VerifyNoOp();

        VerifyOriginalVersion();
    }

    BOOST_AUTO_TEST_CASE(LowerInstanceDoesNotChangeTheVersion)
    {
        RunTest(OpenFT, L"0 Rolling false [SP1 2.0] |");

        commit_.VerifyNoOp();

        VerifyOriginalVersion();
    }

    BOOST_AUTO_TEST_CASE(UpdatedVersionCausesDiskCommit)
    {
        RunTest(OpenFT, L"3 Rolling false [SP1 2.0] |");

        commit_.VerifyDiskCommit();
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}
#pragma endregion

#pragma region On Code Version Upgraded
namespace OnCodeVersionUpgrade
{
    wstring const ClosedFT = L"C None 000/000/411 1:1 -";
    wstring const DownFT = L"O None 000/000/411 {411:1} 1:1 CM [N/N/P SB D N F 1:1]";
    wstring const OpenFT = L"O None 000/000/411 {411:1} 1:1 CM [N/N/P RD U N F 1:1]";

    namespace DeactivationInfoExpectation
    {
        enum Enum
        {
            Valid,
            Dropped,
            Invalid
        };
    }

    class TestLifeCycle_OnCodeVersionUpgrade
    {
    protected:
        TestLifeCycle_OnCodeVersionUpgrade() { BOOST_REQUIRE(TestSetup()); }
        TEST_METHOD_SETUP(TestSetup);
        ~TestLifeCycle_OnCodeVersionUpgrade() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_CLEANUP(TestCleanup);

        void RunTest(wstring const & initial, bool isDeactivationInfoSupported, DeactivationInfoExpectation::Enum expected);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestLifeCycle_OnCodeVersionUpgrade::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_OnCodeVersionUpgrade::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    void TestLifeCycle_OnCodeVersionUpgrade::RunTest(wstring const & initial, bool isDeactivationInfoSupported, DeactivationInfoExpectation::Enum expected)
    {
        auto & test = holder_->Recreate();

        test.AddFT(L"SP1", initial);

        auto & ft = test.GetFT(L"SP1");

        Upgrade::FabricCodeVersionProperties properties;
        properties.IsDeactivationInfoSupported = isDeactivationInfoSupported;

        ft.OnFabricCodeUpgrade(properties);

        auto msg = wformatString("IsValid for deactivation info supported {0} FT {1}", isDeactivationInfoSupported, ft);
        switch (expected)
        {
        case DeactivationInfoExpectation::Invalid:
            Verify::IsTrue(ft.DeactivationInfo.IsInvalid, msg);
            break;

        case DeactivationInfoExpectation::Valid:
            Verify::IsTrue(ft.DeactivationInfo.IsValid, msg);
            break;

        case DeactivationInfoExpectation::Dropped:
            Verify::IsTrue(ft.DeactivationInfo.IsDropped, msg);
            break;
        }
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_OnCodeVersionUpgradeSuite, TestLifeCycle_OnCodeVersionUpgrade)

    BOOST_AUTO_TEST_CASE(CodeUpgradeResetsDeactivationInfoTest)
    {
            RunTest(ClosedFT, true, DeactivationInfoExpectation::Dropped);
            RunTest(OpenFT, true, DeactivationInfoExpectation::Valid);
            RunTest(DownFT, true, DeactivationInfoExpectation::Valid);

            RunTest(ClosedFT, false, DeactivationInfoExpectation::Invalid);
            RunTest(OpenFT, false, DeactivationInfoExpectation::Invalid);
            RunTest(DownFT, false, DeactivationInfoExpectation::Invalid);
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}

#pragma endregion

#pragma region Close

namespace Close
{
    class HealthReportVerifier
    {
    public:
        HealthReportVerifier(bool healthReportExpected) : healthReportExpected_(healthReportExpected)
        {
        }

        void Verify(ScenarioTest & test, std::wstring const & ftShortName, FailoverUnit & ft) const
        {
            if (healthReportExpected_)
            {
                test.ValidateReplicaHealthEvent(Health::ReplicaHealthEvent::Close, ftShortName, ft.LocalReplicaId, ft.LocalReplicaInstanceId);
            }
            else
            {
                test.ValidateNoReplicaHealthEvent();
            }
        }

    private:
        bool healthReportExpected_;
    };

    namespace TerminateActionExpectation
    {
        enum Enum
        {
            DoNotVerify,
            NotCalled,
            Called
        };
    }

    namespace CommitExpectation
    {
        enum Enum
        {
            DoNotVerify,
            None,
            InMemory,
            Persisted
        };
    }

    class StateExpectation
    {
    public:
        StateExpectation(wstring const & state, bool healthReportExpected) :
        state_(state),
        healthReportVerifier_(healthReportExpected),
        terminateActionExpectation_(TerminateActionExpectation::DoNotVerify),
        commitExpectation_(CommitExpectation::DoNotVerify)
        {
        }

        StateExpectation(wstring const & state, bool healthReportExpected, TerminateActionExpectation::Enum terminateActionExpectation) :
        state_(state),
        healthReportVerifier_(healthReportExpected),
        terminateActionExpectation_(terminateActionExpectation),
        commitExpectation_(CommitExpectation::DoNotVerify)
        {
        }

        StateExpectation(wstring const & state, bool healthReportExpected, TerminateActionExpectation::Enum terminateActionExpectation, CommitExpectation::Enum commitExpectation) :
        state_(state),
        healthReportVerifier_(healthReportExpected),
        terminateActionExpectation_(terminateActionExpectation),
        commitExpectation_(commitExpectation)
        {
        }

        StateExpectation(wstring const & state, bool healthReportExpected, CommitExpectation::Enum commitExpectation) :
        state_(state),
        healthReportVerifier_(healthReportExpected),
        terminateActionExpectation_(TerminateActionExpectation::DoNotVerify),
        commitExpectation_(commitExpectation)
        {
        }

        StateExpectation(wstring const & state, CommitExpectation::Enum commitExpectation) :
        state_(state),
        healthReportVerifier_(false),
        terminateActionExpectation_(TerminateActionExpectation::DoNotVerify),
        commitExpectation_(commitExpectation)
        {
        }

        void Verify(ScenarioTest & test, std::wstring const & ftShortName, CommitTypeDescriptionUtility * commitTypeDescription) const
        {
            test.ValidateFT(ftShortName, state_);

            auto & ft = test.GetFT(ftShortName);
            healthReportVerifier_.Verify(test, ftShortName, ft);

            VerifyTerminateAction(test, ftShortName);

            VerifyCommitType(commitTypeDescription);
        }

    private:
        void VerifyCommitType(CommitTypeDescriptionUtility * utility) const
        {
            if (commitExpectation_ == CommitExpectation::DoNotVerify)
            {
                return;
            }

            Verify::IsTrueLogOnError(utility != nullptr, L"commit type description cant be null");

            switch (commitExpectation_)
            {
            case CommitExpectation::None: 
                utility->VerifyNoOp();
                break;

            case CommitExpectation::InMemory: 
                utility->VerifyInMemoryCommit();
                break;

            case CommitExpectation::Persisted: 
                utility->VerifyDiskCommit();
                break;

            default: 
                Verify::Fail(L"Unknown type");
                break;
            }
        }

        void VerifyTerminateAction(ScenarioTest & test, std::wstring const & shortName) const
        {
            if (terminateActionExpectation_ == TerminateActionExpectation::DoNotVerify)
            {
                return;
            }
            else if (terminateActionExpectation_ == TerminateActionExpectation::NotCalled)
            {
                test.ValidateNoTerminateCalls();
            }
            else
            {
                test.ValidateTerminateCall(shortName);
            }
        }

        wstring state_;
        HealthReportVerifier healthReportVerifier_;
        TerminateActionExpectation::Enum terminateActionExpectation_;
        CommitExpectation::Enum commitExpectation_;
    };

    wstring const FT = L"SP1";
    NodeInstance const SenderNode = CreateNodeInstanceEx(2, 1);
    namespace State
    {
        // Closed FT
        wstring const Closed = L"C None 000/000/411 1:1 -";

        // Down FT
        wstring const Down = L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]";
        wstring const DownInDrop = L"O None 000/000/411 1:1 - [N/N/P ID D N F 1:1]";
        wstring const DownDeleted = L"O None 000/000/411 1:1 L [N/N/P ID D N F 1:1]";

        // Initial replica create without service type registration
        // No open has been sent
        wstring const InitialCreateWithoutSTR = L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]";
        wstring const InitialCreateWithoutSTRClosing_SV = L"C None 000/000/411 1:1 I";
        wstring const InitialCreateWithoutSTRClosing_SP = L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]";
        wstring const InitialCreateWithoutSTRDropping = L"C None 000/000/411 1:1 G";

        // Initial replica create with STR
        // ReplicaOpen may have been sent
        wstring const InitialCreateWithSTR = L"O None 000/000/411 1:1 SM [N/N/P IC U N F 1:1]";
        wstring const InitialCreateWithSTRClosing = L"O None 000/000/411 1:1 MHc [N/N/P IC U N F 1:1]";
        wstring const InitialCreateWithSTRDropping = L"O None 000/000/411 1:1 MHd [N/N/P ID U N F 1:1]";
        wstring const InitialCreateWithSTRObliterating = L"O None 000/000/411 1:1 MHo [N/N/P ID U N F 1:1]";

        // Change Role on an existing replica
        // The replica has state on the node
        wstring const ChangeRoleOnExistingReplica = L"O None 000/000/411 1:1 CMS [N/N/P IC U N F 1:1]";
        wstring const ChangeRoleOnExistingReplicaClosing = L"O None 000/000/411 1:1 CMHc [N/N/P IC U N F 1:1]";
        wstring const ChangeRoleOnExistingReplicaDropping = L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]";
        wstring const ChangeRoleOnExistingReplicaObliterating = L"O None 000/000/411 1:1 CMHo [N/N/P ID U N F 1:1]";

        // Ready replica
        wstring const ReadyReplica = L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]";
        wstring const MessagePendingFlagsAreReset = L"O None 000/000/411 1:1 CMTN [N/N/P RD U N F 1:1]";
        wstring const ReadyReplicaClosing = L"O None 000/000/411 1:1 CMHc [N/N/P RD U N F 1:1]";
        wstring const ReadyReplicaClosingReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 ICMHc [N/N/P RD U N F 1:1]";
        wstring const ReadyReplicaDropping = L"O None 000/000/411 1:1 CMHd [N/N/P ID U N F 1:1]";
        wstring const ReadyReplicaDroppingReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 ICMHd [N/N/P ID U N F 1:1]";
        wstring const ReadyReplicaObliterating = L"O None 000/000/411 1:1 CMHo [N/N/P ID U N F 1:1]";
        wstring const ReadyReplicaDeleting = L"O None 000/000/411 1:1 CHlLM [N/N/P ID U N F 1:1]";
        wstring const ReadyReplicaDropped = L"C None 000/000/411 1:1 G";
        wstring const ReadyReplicaDown_SP = L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]";
        wstring const ReadyReplicaDown_SV = L"C None 000/000/411 1:1 I";
        wstring const ReadyReplicaClosed = L"C None 000/000/411 1:1 -";
        wstring const ReadyReplicaDeleted = L"C None 000/000/411 1:1 PL";
        wstring const ReadyReplicaEndpointUpdatePending = L"O Phase2_Catchup 411/000/422 1:1 CME [S/N/P RD U N F 1:1] [P/N/S DD D N F 2:1]";

        // Reopening replica
        wstring const OpeningStandbyReplicaWithoutSTR = L"O None 000/000/411 1:1 S [N/N/S SB U N F 1:1]";
        wstring const OpeningStandbyReplicaWithoutSTRClosing = L"O None 000/000/411 1:1 1 I [N/N/S SB D N F 1:1]";
        wstring const OpeningStandbyReplicaWithoutSTRDropping = L"O None 000/000/411 1:1 Hd [N/N/S ID U N F 1:1]";

        // Opening SB replica with STR
        wstring const OpeningStandbyReplicaWithSTR = L"O None 000/000/411 1:1 MS [N/N/S SB U N F 1:1]";
        wstring const OpeningStandbyReplicaWithSTRClosing = L"O None 000/000/411 1:1 MHc [N/N/S SB U N F 1:1]";
        wstring const OpeningStandbyReplicaWithSTRClosingReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 IMHc [N/N/S SB U N F 1:1]";
        wstring const OpeningStandbyReplicaWithSTRDropping = L"O None 000/000/411 1:1 MHd [N/N/S ID U N F 1:1]";
        wstring const OpeningStandbyReplicaWithSTRDroppingReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 IMHd [N/N/S ID U N F 1:1]";
        wstring const OpeningStandbyReplicaWithSTRObliterating = L"O None 000/000/411 1:1 MHo [N/N/S ID U N F 1:1]";

        // Open SB Replica
        wstring const OpenStandbyReplica = L"O None 000/000/411 1:1 CM [N/N/S SB U N F 1:1]";
        wstring const OpenStandbyReplicaClosing = L"O None 000/000/411 1:1 CMHc [N/N/S SB U N F 1:1]";
        wstring const OpenStandbyReplicaClosingReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 ICMHc [N/N/S SB U N F 1:1]";
        wstring const OpenStandbyReplicaDropping = L"O None 000/000/411 1:1 CMHd [N/N/S ID U N F 1:1]";
        wstring const OpenStandbyReplicaDroppingReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 ICMHd [N/N/S ID U N F 1:1]";
        wstring const OpenStandbyReplicaWithReplicaUpPending = L"O None 000/000/411 1:1 CMKU [N/N/S SB U N F 1:1]";
        wstring const OpenStandbyReplicaObliterating = L"O None 000/000/411 1:1 CMHo [N/N/S ID U N F 1:1]";

        // Aborting replica
        wstring const AbortingReplica = L"O None 000/000/411 1:1 CMHa [N/N/S ID U N F 1:1]";
        wstring const AbortingReplicaReadWriteStatusRevoked = L"O None 000/000/411 1:1 1 ICMHa [N/N/S ID U N F 1:1]";

        wstring const DroppingReplicaWithoutSTR = L"O None 000/000/411 1:1 Hd [N/N/P ID U N F 1:1]";
        wstring const DeletingReplicaWithoutSTR = L"O None 000/000/411 1:1 LHd [N/N/P ID U N F 1:1]";

    }

    namespace OperationResult
    {
        enum Enum
        {
            Completed = 0,
            NotStarted = 1,
            Invalid = 2,
            CannotProcessButComplete = 3,
        };
    }

    class TestLifeCycle_Close
    {
    protected:
        TestLifeCycle_Close() { BOOST_REQUIRE(TestSetup()); }
        ~TestLifeCycle_Close() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_SETUP(TestSetup);
        TEST_METHOD_CLEANUP(TestCleanup);

        void FlagStateUpdatedTestHelper(ReplicaCloseMode mode);

        void SendCloseMessageTestHelper(
            ReplicaCloseMode closeMode,
            wstring const & messageExpected);

        ScenarioTest &AddFTAndInvokeRetryTimer(
            wstring const & initialState);

        void ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode closeMode,
            StateExpectation const & expected,
            bool isCloseCompleted);

        void ProcessReplicaCloseReplyTestHelper(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode,
            StateExpectation const & expected,
            bool isCloseCompleted);

        void ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode closeMode,
            StateExpectation const & volatileState,
            StateExpectation const & persistedState,
            bool isCloseCompleted);

        void ExecuteReplicaCloseReplyAndVerify(
            ScenarioTest & test,
            wstring const & ftShortName,
            ReplicaCloseMode closeMode,
            StateExpectation const & expected,
            bool isCompleted);

        void ExecuteReadWriteStatusRevokedNotification(
            ReplicaCloseMode closeMode,
            bool shouldBeProcessed);

        void ExecuteReadWriteStatusRevokedNotification(
            wstring const & tag,
            wstring const & ftShortName,
            wstring const & initial,
            StateExpectation expected);

        ScenarioTest & StartCloseCompletionTest(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode,
            wstring const & state);

        ScenarioTest & CloseCompletionTestHelper(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode,
            wstring const & state);

        ScenarioTest & CloseCompletionTestHelper(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode);

        ScenarioTest & ExecuteCloseDropAndObliterateTest(
            wstring const & ftShortName,
            wstring const & initial,
            StateExpectation const & closing,
            StateExpectation const & dropping,
            StateExpectation const & obliterating);

        ScenarioTest & ExecuteCloseDropAndObliterateTest(
            wstring const & initial,
            StateExpectation const & closing,
            StateExpectation const & dropping,
            StateExpectation const & obliterating);

        ScenarioTest & ExecuteTest(
            wstring const & ft,
            wstring const & state,
            ReplicaCloseMode closeMode);

        ScenarioTest & ExecuteTestAndVerifyEx(
            wstring const & ft,
            wstring const & state,
            ReplicaCloseMode closeMode,
            function<void(ScenarioTest&, FailoverUnit&)> verifier);

        ScenarioTest & ExecuteTestAndVerify(
            wstring const & ft,
            wstring const & state,
            ReplicaCloseMode closeMode,
            function<void(FailoverUnit&)> verifier);

        ScenarioTest & ExecuteTest(
            wstring const & ft,
            wstring const & initialState,
            StateExpectation const & finalState,
            ReplicaCloseMode closeMode);

        ScenarioTest & ExecuteForceScenarioTest(
            wstring const & initial,
            bool healthReportExpected);

        ScenarioTest & ExecuteObliterateScenarioWithReplicaDropped(
            wstring const & initial,
            bool healthReportExpected);

        void OperationResultTestHelper(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & state,
            ReplicaCloseMode closeMode,
            OperationResult::Enum expected);

        void OperationResultTestHelper(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & state,
            OperationResult::Enum expected);

        void VerifyIsComplete(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode,
            bool isCompleteExpected);

        void InvokeStartCloseOnFT(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode);

        void InvokeStartCloseOnRA(
            wstring const & ftShortName,
            ReplicaCloseMode closeMode);

        void InvokeReadWriteStatusRevokedOnFT(
            ScenarioTest & test,
            wstring const & ftShortName);

        void ValidateReplicaDroppedMessage(
            ScenarioTest & test,
            wstring const & ftShortName);

        // void InvokeCloseAndVerifyProcessed(wstring const & ft, wstring const & state, ReplicaCloseMode closeMode)
        ScenarioTestHolderUPtr holder_;
        vector<ReplicaCloseMode> allCloseModes_;
        bool isAssertDebugBreakDisabled_; // used for testing obliterate of FM
        CommitTypeDescriptionUtility lastCommit_;
    };

    bool TestLifeCycle_Close::TestSetup()
    {
        isAssertDebugBreakDisabled_ = false;
        allCloseModes_.push_back(ReplicaCloseMode::Abort);
        allCloseModes_.push_back(ReplicaCloseMode::Close);
        allCloseModes_.push_back(ReplicaCloseMode::DeactivateNode);
        allCloseModes_.push_back(ReplicaCloseMode::Restart);
        allCloseModes_.push_back(ReplicaCloseMode::Drop);
        allCloseModes_.push_back(ReplicaCloseMode::Delete);
        allCloseModes_.push_back(ReplicaCloseMode::Deactivate);
        allCloseModes_.push_back(ReplicaCloseMode::ForceAbort);
        allCloseModes_.push_back(ReplicaCloseMode::ForceDelete);
        allCloseModes_.push_back(ReplicaCloseMode::AppHostDown);
        allCloseModes_.push_back(ReplicaCloseMode::Obliterate);
        allCloseModes_.push_back(ReplicaCloseMode::QueuedDelete);

        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_Close::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    void TestLifeCycle_Close::OperationResultTestHelper(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & state,
        ReplicaCloseMode closeMode,
        OperationResult::Enum expected)
    {
        auto & test = holder_->Recreate();
        test.LogStep(log);
        test.AddFT(ftShortName, state);

        auto & ft = test.GetFT(ftShortName);

        bool shouldRunLocalReplicaClosed = false;
        bool expectedCanProcessResult = false;
        bool expectedIsCompleteResult = false;

        switch (expected)
        {
        case OperationResult::Completed:
            shouldRunLocalReplicaClosed = true;
            expectedCanProcessResult = true;
            expectedIsCompleteResult = true;
            break;

        case OperationResult::NotStarted:
            shouldRunLocalReplicaClosed = true;
            expectedCanProcessResult = true;
            expectedIsCompleteResult = false;
            break;

        case OperationResult::Invalid:
            shouldRunLocalReplicaClosed = false;
            expectedCanProcessResult = false;
            expectedIsCompleteResult = false;
            break;

        case OperationResult::CannotProcessButComplete:
            shouldRunLocalReplicaClosed = true;
            expectedCanProcessResult = false;
            expectedIsCompleteResult = true;
            break;
        };

        auto actualCanProcessResult = ft.CanCloseLocalReplica(closeMode);
        Verify::AreEqualLogOnError(expectedCanProcessResult, actualCanProcessResult, wformatString("CanProcess {0} {1}", closeMode, ft));

        if (shouldRunLocalReplicaClosed)
        {
            auto actual = ft.IsLocalReplicaClosed(closeMode);
            Verify::AreEqualLogOnError(expectedIsCompleteResult, actual, wformatString("IsLocalReplicaClosed {0} {1}", closeMode, ft));
        }
    }

    void TestLifeCycle_Close::OperationResultTestHelper(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & state,
        OperationResult::Enum expected)
    {
        for (auto i : allCloseModes_)
        {
            OperationResultTestHelper(log, ftShortName, state, i, expected);
        }
    }

    void TestLifeCycle_Close::InvokeStartCloseOnFT(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode)
    {
        auto & test = holder_->ScenarioTestObj;
        
        lastCommit_ =  test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            ftShortName,
            [&](EntityExecutionContext & base)
        {            
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            NodeInstance nodeInstance = closeMode == ReplicaCloseMode::Deactivate ? SenderNode : ReconfigurationAgent::InvalidNode;            
            test.GetFT(ftShortName).StartCloseLocalReplica(closeMode, nodeInstance, context, ActivityDescription::Empty);
        });
    }

    void TestLifeCycle_Close::InvokeStartCloseOnRA(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode)
    {
        auto & test = holder_->ScenarioTestObj;
        test.ExecuteJobItemHandler<JobItemContextBase>(
            ftShortName,
            [&](HandlerParameters & handlerParameters, JobItemContextBase &)
        {
            NodeInstance nodeInstance = closeMode == ReplicaCloseMode::Deactivate ? SenderNode : ReconfigurationAgent::InvalidNode;
            handlerParameters.RA.CloseLocalReplica(handlerParameters, closeMode, nodeInstance, ActivityDescription::Empty);
            return true;
        });

        test.DrainJobQueues();
    }
    
    void TestLifeCycle_Close::InvokeReadWriteStatusRevokedOnFT(
        ScenarioTest & test,
        wstring const & ftShortName)
    {
        auto & ft = test.GetFT(ftShortName);

        lastCommit_ = test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            ftShortName,
            [&](EntityExecutionContext & base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.Test_ReadWriteStatusRevokedNotification(context);
        });
    }

    void TestLifeCycle_Close::VerifyIsComplete(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode,
        bool isCompleteExpected)
    {
        auto & ft = holder_->ScenarioTestObj.GetFT(ftShortName);
        Verify::AreEqualLogOnError(isCompleteExpected, ft.IsLocalReplicaClosed(closeMode), wformatString(ft));
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteTest(
        wstring const & ftShortName,
        wstring const & state,
        ReplicaCloseMode closeMode)
    {
        auto & test = holder_->Recreate();
        unique_ptr<Assert::DisableDebugBreakInThisScope> disabler;

        if (isAssertDebugBreakDisabled_)
        {
            disabler = make_unique<Assert::DisableDebugBreakInThisScope>();
        }

        test.LogStep(wformatString("Executing {0}", closeMode));
        test.AddFT(ftShortName, state);

        InvokeStartCloseOnFT(ftShortName, closeMode);

        return test;
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteTest(
        wstring const & ftShortName,
        wstring const & initialState,
        StateExpectation const & finalState,
        ReplicaCloseMode closeMode)
    {
        auto & test = ExecuteTest(ftShortName, initialState, closeMode);
        finalState.Verify(test, ftShortName, &lastCommit_);
        return test;
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteTestAndVerify(
        wstring const & ftShortName,
        wstring const & state,
        ReplicaCloseMode closeMode,
        function<void(FailoverUnit&)> verifier)
    {
        return ExecuteTestAndVerifyEx(
            ftShortName,
            state,
            closeMode,
            [&](ScenarioTest &, FailoverUnit & ft) { verifier(ft); });
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteTestAndVerifyEx(
        wstring const & ftShortName,
        wstring const & state,
        ReplicaCloseMode closeMode,
        function<void(ScenarioTest &, FailoverUnit&)> verifier)
    {
        auto & test = ExecuteTest(ftShortName, state, closeMode);
        auto & ft = test.GetFT(ftShortName);
        verifier(test, ft);
        return test;
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteCloseDropAndObliterateTest(
        wstring const & initial,
        StateExpectation const & closing,
        StateExpectation const & dropping,
        StateExpectation const & obliterating)
    {
        return ExecuteCloseDropAndObliterateTest(FT, initial, closing, dropping, obliterating);
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteCloseDropAndObliterateTest(
        wstring const & ftShortName,
        wstring const & initial,
        StateExpectation const & closing,
        StateExpectation const & dropping,
        StateExpectation const & obliterating)
    {
        ExecuteTest(ftShortName, initial, closing, ReplicaCloseMode::Close);
        ExecuteTest(ftShortName, initial, dropping, ReplicaCloseMode::Drop);
        return ExecuteTest(ftShortName, initial, obliterating, ReplicaCloseMode::Obliterate);
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteForceScenarioTest(
        wstring const & initial,
        bool healthReportExpected)
    {
        ExecuteTest(
            FT,
            initial,
            StateExpectation(State::ReadyReplicaDropped, healthReportExpected),
            ReplicaCloseMode::ForceAbort);

        return ExecuteTest(
            FT,
            initial,
            StateExpectation(State::ReadyReplicaDeleted, healthReportExpected),
            ReplicaCloseMode::ForceDelete);
    }

    ScenarioTest & TestLifeCycle_Close::ExecuteObliterateScenarioWithReplicaDropped(
        wstring const & initial,
        bool healthReportExpected)
    {
        return ExecuteTest(
            FT,
            initial,
            StateExpectation(State::ReadyReplicaDropped, healthReportExpected),
            ReplicaCloseMode::Obliterate);
    }

    void TestLifeCycle_Close::ValidateReplicaDroppedMessage(
        ScenarioTest & test,
        wstring const & ftShortName)
    {
        auto msg = L"false false | {" + ftShortName + L" 000/000/411 [N/N/N DD D 1:1 n] } ";
        test.ValidateFMMessage<MessageType::ReplicaUp>(msg);
    }

    void TestLifeCycle_Close::ExecuteReadWriteStatusRevokedNotification(
        wstring const & tag,
        wstring const & ftShortName,
        wstring const & initial,
        StateExpectation expected)
    {
        TestLog::WriteInfo(tag);
        auto & test = holder_->Recreate();

        test.AddFT(ftShortName, initial);

        InvokeReadWriteStatusRevokedOnFT(test, ftShortName);

        expected.Verify(test, ftShortName, &lastCommit_);
    }

    void TestLifeCycle_Close::ExecuteReadWriteStatusRevokedNotification(
        ReplicaCloseMode closeMode,
        bool shouldBeProcessed)
    {
        TestLog::WriteInfo(wformatString(closeMode));

        auto & test = holder_->Recreate();

        test.AddFT(FT, State::ReadyReplica);

        InvokeStartCloseOnFT(FT, closeMode);

        auto & ft = test.GetFT(FT);

        lastCommit_ = test.EntityExecutionContextTestUtilityObj.ExecuteEntityProcessor(
            FT,
            [&](EntityExecutionContext & base)
        {
            auto & context = base.As<FailoverUnitEntityExecutionContext>();
            ft.Test_ReadWriteStatusRevokedNotification(context);
        });

        Verify::AreEqualLogOnError(shouldBeProcessed, lastCommit_.WasProcessed, L"Was processed");
    }



    ScenarioTest & TestLifeCycle_Close::CloseCompletionTestHelper(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode)
    {
        return CloseCompletionTestHelper(ftShortName, closeMode, State::ReadyReplica);
    }

    ScenarioTest & TestLifeCycle_Close::CloseCompletionTestHelper(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode,
        wstring const & state)
    {
        auto & test = StartCloseCompletionTest(ftShortName, closeMode, state);

        test.ResetAll();

        test.ProcessRAPMessageAndDrain<MessageType::ReplicaCloseReply>(
            ftShortName,
            L"000/411 [N/P RD U 1:1] Success -");

        return test;
    }

    void TestLifeCycle_Close::FlagStateUpdatedTestHelper(ReplicaCloseMode mode)
    {
        ExecuteTestAndVerify(
            FT,
            State::ReadyReplica,
            mode,
            [=](FailoverUnit & ft)
        {
            if (mode == ReplicaCloseMode::AppHostDown)
            {
                Verify::AreEqualLogOnError(ReplicaCloseMode::None, ft.CloseMode, wformatString("ReplicaCloseMode is not set {0}", ft));
            }
            else
            {
                Verify::AreEqualLogOnError(mode, ft.CloseMode, wformatString("ReplicaCloseMode is not set {0}", ft));
            }
        });
    }

    void TestLifeCycle_Close::SendCloseMessageTestHelper(
        ReplicaCloseMode closeMode,
        wstring const & messageExpected)
    {
        auto & test = holder_->Recreate();
        test.AddFT(FT, State::ReadyReplica);
        test.LogStep(wformatString(closeMode));

        InvokeStartCloseOnRA(FT, closeMode);

        if (messageExpected.empty())
        {
            test.ValidateNoRAPMessages();
        }
        else
        {
            test.ValidateRAPMessage<MessageType::ReplicaClose>(FT, messageExpected);
        }
    }

    ScenarioTest & TestLifeCycle_Close::AddFTAndInvokeRetryTimer(
        wstring const & initialState)
    {
        auto & test = holder_->Recreate();

        test.AddFT(FT, initialState);
        test.RequestWorkAndDrain(test.StateItemHelpers.ReplicaCloseRetryHelper);

        return test;
    }

    void TestLifeCycle_Close::ExecuteReplicaCloseReplyAndVerify(
        ScenarioTest & test,
        wstring const & ftShortName,
        ReplicaCloseMode closeMode,
        StateExpectation const & expected,
        bool isCloseCompleted)
    {
        test.ExecuteUnderLock(
            ftShortName,
            [&test](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent & ra)
        {
            ft.EnableUpdate();
            FailoverUnitEntityExecutionContext context = Infrastructure::EntityTraits<FailoverUnit>::CreateEntityExecutionContext(ra, queue, ft.UpdateContextObj, nullptr);
            ft->RetryableErrorStateObj.EnterState(RetryableErrorStateName::ReplicaClose);
            ft->RetryableErrorStateObj.Test_SetThresholds(RetryableErrorStateName::ReplicaClose, 1, 1, INT_MAX, INT_MAX);
            ft->ProcessReplicaCloseReply(TransitionErrorCode(ProxyErrorCode::Success()), context);
        });

        expected.Verify(test, ftShortName, &lastCommit_);

        Verify::AreEqualLogOnError(isCloseCompleted, test.GetFT(ftShortName).IsLocalReplicaClosed(closeMode), L"IsClosed");
    }

    void TestLifeCycle_Close::ProcessReplicaCloseReplyTestHelper(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode,
        StateExpectation const & expected,
        bool isCloseCompleted)
    {
        auto & test = holder_->Recreate();

        test.LogStep(wformatString(closeMode));
        test.AddFT(ftShortName, State::ReadyReplica);

        InvokeStartCloseOnRA(ftShortName, closeMode);
        test.ResetAll();

        ExecuteReplicaCloseReplyAndVerify(test, ftShortName, closeMode, expected, isCloseCompleted);
    }

    void TestLifeCycle_Close::ProcessReplicaCloseReplyTestHelper(
        ReplicaCloseMode closeMode,
        StateExpectation const & volatileState,
        StateExpectation const & persistedState,
        bool isCloseCompleted)
    {
        ProcessReplicaCloseReplyTestHelper(FT, closeMode, persistedState, isCloseCompleted);
        ProcessReplicaCloseReplyTestHelper(L"SV1", closeMode, volatileState, isCloseCompleted);
    }

    void TestLifeCycle_Close::ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(        
        ReplicaCloseMode closeMode,
        StateExpectation const & expected,
        bool isCloseCompleted)
    {
        wstring ftShortName = FT;

        auto & test = holder_->Recreate();

        test.LogStep(wformatString(closeMode));
        test.AddFT(ftShortName, State::ReadyReplica);

        InvokeStartCloseOnRA(ftShortName, closeMode);
        test.ResetAll();

        InvokeReadWriteStatusRevokedOnFT(test, ftShortName);

        ExecuteReplicaCloseReplyAndVerify(test, ftShortName, closeMode, expected, isCloseCompleted);
    }

    ScenarioTest & TestLifeCycle_Close::StartCloseCompletionTest(
        wstring const & ftShortName,
        ReplicaCloseMode closeMode,
        wstring const & state)
    {
        auto & test = holder_->ScenarioTestObj;
        test.AddFT(ftShortName, state);

        InvokeStartCloseOnRA(ftShortName, closeMode);

        return test;
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_CloseSuite, TestLifeCycle_Close)

    BOOST_AUTO_TEST_CASE(IsLocalReplicaClosedTest)
    {
        wstring tc;
        wstring const ft = L"SP1";

        // Closed FT
        // A closed FT cannot be restarted
        // A closed FT is not deleted
        tc = L"Closed";
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Abort, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Close, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Deactivate, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::DeactivateNode, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Drop, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::ForceAbort, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::QueuedDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::AppHostDown, OperationResult::CannotProcessButComplete);
        OperationResultTestHelper(tc, FT, State::Closed, ReplicaCloseMode::Obliterate, OperationResult::Completed);

        // Closed and Deleted FT
        // A Closed FT cannot be restarted
        tc = L"Deleted";
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Abort, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Close, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Deactivate, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::DeactivateNode, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Delete, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Drop, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::ForceAbort, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::ForceDelete, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::QueuedDelete, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::AppHostDown, OperationResult::CannotProcessButComplete);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaDeleted, ReplicaCloseMode::Obliterate, OperationResult::Completed);


        // Down FT
        // Cannot be Aborted, Deleted, Dropped
        // Can be forced
        tc = L"Down";
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Abort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Close, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Deactivate, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::DeactivateNode, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Delete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Drop, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::QueuedDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::AppHostDown, OperationResult::CannotProcessButComplete);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"DownID";
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Abort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Close, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Deactivate, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::DeactivateNode, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Delete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Drop, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::QueuedDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::Down, ReplicaCloseMode::AppHostDown, OperationResult::CannotProcessButComplete);
        OperationResultTestHelper(tc, FT, State::DownInDrop, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"DownDeleted";
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Abort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Close, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Deactivate, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::DeactivateNode, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Delete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Drop, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::QueuedDelete, OperationResult::Completed);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::AppHostDown, OperationResult::CannotProcessButComplete);
        OperationResultTestHelper(tc, FT, State::DownDeleted, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        // Initial replica create without service type registration
        // No open has been sent
        // Everything can be performed including restart
        tc = L"InitialCreateWithoutSTR";
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Abort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Close, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Deactivate, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::DeactivateNode, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Drop, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Restart, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithoutSTR, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"InitialCreateWithSTR";
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Abort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Close, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Deactivate, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::DeactivateNode, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Drop, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Restart, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::InitialCreateWithSTR, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"ChangeRoleOnExistingReplica";
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Abort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Close, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Deactivate, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::DeactivateNode, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Drop, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Restart, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ChangeRoleOnExistingReplica, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"ReadyReplica";
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Abort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Close, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Deactivate, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::DeactivateNode, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Drop, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Restart, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplica, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        // No service type registration has been found
        tc = L"OpeningStandbyReplicaWithoutSTR";
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Abort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Close, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Deactivate, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::DeactivateNode, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Drop, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Restart, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithoutSTR, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"OpeningStandbyReplicaWithSTR";
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Abort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Close, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Deactivate, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::DeactivateNode, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Delete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Drop, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Restart, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::OpeningStandbyReplicaWithSTR, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"AbortingReplica";
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Abort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Close, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Deactivate, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::DeactivateNode, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Delete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Drop, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::ForceAbort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::ForceDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::AbortingReplica, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"DroppingReplicaWithoutSTR";
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Abort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Close, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Deactivate, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::DeactivateNode, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Delete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Drop, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::ForceAbort, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::ForceDelete, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::DroppingReplicaWithoutSTR, ReplicaCloseMode::Obliterate, OperationResult::NotStarted);

        tc = L"Obliterating";
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Abort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Close, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Deactivate, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::DeactivateNode, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Delete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Drop, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Restart, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::ForceAbort, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::ForceDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::QueuedDelete, OperationResult::Invalid);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::AppHostDown, OperationResult::NotStarted);
        OperationResultTestHelper(tc, FT, State::ReadyReplicaObliterating, ReplicaCloseMode::Obliterate, OperationResult::Invalid);
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_SynchronousCloseSendsMessage)
    {
        auto & test = holder_->Recreate();
        test.AddFT(FT, State::InitialCreateWithoutSTR);
        InvokeStartCloseOnRA(FT, ReplicaCloseMode::Abort);
        test.ValidateFMMessage<MessageType::ReplicaUp>();
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_AppHostDown_DeletedReplicaSendsDeleteReplicaReply)
    {
        auto & test = holder_->Recreate();
        test.AddFT(L"SV1", State::ReadyReplicaDeleting);
        InvokeStartCloseOnRA(L"SV1", ReplicaCloseMode::AppHostDown);
        test.ValidateNoFMMessages();
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_SynchronousCloseSendsMessageAppHostDown)
    {
        auto & test = holder_->Recreate();
        test.AddFT(FT, State::ReadyReplica);
        InvokeStartCloseOnRA(FT, ReplicaCloseMode::AppHostDown);
        test.DrainJobQueues();
        test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1 ]} |");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_SynchronousCloseWithoutSTRFindsSTR)
    {
        auto & test = holder_->Recreate();
        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForDrop);
        test.AddFT(FT, State::OpeningStandbyReplicaWithoutSTR);
        InvokeStartCloseOnRA(FT, ReplicaCloseMode::Abort);
        Verify::IsTrueLogOnError(test.GetFT(FT).LocalReplicaClosePending.IsSet, L"ClosePending");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_SynchronousCloseSendsMessageObliterate)
    {
        auto & test = holder_->Recreate();
        test.AddFT(FT, State::Down);
        InvokeStartCloseOnRA(FT, ReplicaCloseMode::Obliterate);
        test.DrainJobQueues();
        test.ValidateFMMessage<MessageType::ReplicaUp>();
        test.ValidateNoTerminateCalls();
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_SynchronousCloseQueuedDeleteIsNoOp)
    {
        auto & test = holder_->Recreate();
        test.AddFT(FT, State::Down);
        InvokeStartCloseOnRA(FT, ReplicaCloseMode::QueuedDelete);
        test.DrainJobQueues();
        test.ValidateNoFMMessages();
    }

    BOOST_AUTO_TEST_CASE(InitialCreateWithoutSTRTest)
    {
        ExecuteCloseDropAndObliterateTest(
            State::InitialCreateWithoutSTR,
            StateExpectation(State::InitialCreateWithoutSTRClosing_SP, true, CommitExpectation::Persisted),
            StateExpectation(State::InitialCreateWithoutSTRDropping, true, CommitExpectation::Persisted),
            StateExpectation(State::ReadyReplicaDropped, true, CommitExpectation::Persisted));

        ExecuteCloseDropAndObliterateTest(
            L"SV1",
            State::InitialCreateWithoutSTR,
            StateExpectation(State::InitialCreateWithoutSTRClosing_SV, true, CommitExpectation::Persisted),
            StateExpectation(State::InitialCreateWithoutSTRDropping, true, CommitExpectation::Persisted),
            StateExpectation(State::ReadyReplicaDropped, true, CommitExpectation::Persisted));

        ExecuteForceScenarioTest(State::InitialCreateWithoutSTR, true);
    }

    BOOST_AUTO_TEST_CASE(DownReplicaTest)
    {
        ExecuteForceScenarioTest(State::Down, false);

        ExecuteObliterateScenarioWithReplicaDropped(State::Down, false);

        ExecuteTest(
            FT, 
            State::Down, 
            StateExpectation(State::DownDeleted, false, CommitExpectation::Persisted), 
            ReplicaCloseMode::QueuedDelete);

        ExecuteTest(
            FT, 
            State::DownInDrop, 
            StateExpectation(State::DownDeleted, false, CommitExpectation::Persisted), 
            ReplicaCloseMode::QueuedDelete);

        ExecuteTest(
            L"SV1", 
            State::ReadyReplicaDeleting, 
            StateExpectation(State::ReadyReplicaDeleted, true, CommitExpectation::Persisted), 
            ReplicaCloseMode::AppHostDown);

        ExecuteTest(
            FT, 
            State::ReadyReplicaDeleting, 
            StateExpectation(State::DownDeleted, true, CommitExpectation::Persisted), 
            ReplicaCloseMode::AppHostDown);
    }

    BOOST_AUTO_TEST_CASE(SBWithoutSTRTest)
    {
        ExecuteCloseDropAndObliterateTest(
            State::OpeningStandbyReplicaWithoutSTR,
            StateExpectation(State::OpeningStandbyReplicaWithoutSTRClosing, true, CommitExpectation::Persisted),
            StateExpectation(State::OpeningStandbyReplicaWithoutSTRDropping, false, CommitExpectation::Persisted),
            StateExpectation(State::ReadyReplicaDropped, true, CommitExpectation::Persisted));

        ExecuteTest(
            FT,
            State::OpeningStandbyReplicaWithoutSTR,
            StateExpectation(State::ReadyReplicaDropped, true, CommitExpectation::Persisted),
            ReplicaCloseMode::ForceAbort);

        ExecuteForceScenarioTest(State::OpeningStandbyReplicaWithoutSTR, true);
    }

    BOOST_AUTO_TEST_CASE(DroppingReplicaTest)
    {
        ExecuteForceScenarioTest(State::DroppingReplicaWithoutSTR, true);

        ExecuteTest(
            FT,
            State::DroppingReplicaWithoutSTR,
            StateExpectation(State::ReadyReplicaDropped, true, CommitExpectation::Persisted),
            ReplicaCloseMode::Obliterate);
    }

    BOOST_AUTO_TEST_CASE(ClosedTest)
    {
        ExecuteTest(
            FT,
            State::Closed,
            StateExpectation(State::ReadyReplicaDeleted, false, CommitExpectation::Persisted),
            ReplicaCloseMode::Delete);

        ExecuteTest(
            FT,
            State::Closed,
            StateExpectation(State::ReadyReplicaDeleted, false, CommitExpectation::Persisted),
            ReplicaCloseMode::QueuedDelete);
    }

    BOOST_AUTO_TEST_CASE(InitialCreateWithSTRTest)
    {
        ExecuteCloseDropAndObliterateTest(
            State::InitialCreateWithSTR,
            StateExpectation(State::InitialCreateWithSTRClosing, false, CommitExpectation::InMemory),
            StateExpectation(State::InitialCreateWithSTRDropping, false, CommitExpectation::Persisted),
            StateExpectation(State::InitialCreateWithSTRObliterating, false, CommitExpectation::Persisted));
    }

    BOOST_AUTO_TEST_CASE(ChangeRoleOnExistingReplicaTest)
    {
        ExecuteCloseDropAndObliterateTest(
            State::ChangeRoleOnExistingReplica,
            StateExpectation(State::ChangeRoleOnExistingReplicaClosing, false, CommitExpectation::InMemory),
            StateExpectation(State::ChangeRoleOnExistingReplicaDropping, false, CommitExpectation::Persisted),
            StateExpectation(State::ChangeRoleOnExistingReplicaObliterating, false, CommitExpectation::Persisted));
    }

    BOOST_AUTO_TEST_CASE(ReadyReplica)
    {
        ExecuteCloseDropAndObliterateTest(
            State::ReadyReplica,
            StateExpectation(State::ReadyReplicaClosing, false, CommitExpectation::InMemory),
            StateExpectation(State::ReadyReplicaDropping, false, CommitExpectation::Persisted),
            StateExpectation(State::ReadyReplicaObliterating, false, CommitExpectation::Persisted));

            isAssertDebugBreakDisabled_ = true; // instead of passing a parameter everywhere

        ExecuteTest(
            L"FM",
            State::ReadyReplica,
            StateExpectation(State::ReadyReplicaDropped, true),
            ReplicaCloseMode::Obliterate);
    }

    BOOST_AUTO_TEST_CASE(OpeningStandbyReplicaWithSTR)
    {
        ExecuteCloseDropAndObliterateTest(
            State::OpeningStandbyReplicaWithSTR,
            StateExpectation(State::OpeningStandbyReplicaWithSTRClosing, false, CommitExpectation::InMemory),
            StateExpectation(State::OpeningStandbyReplicaWithSTRDropping, false, CommitExpectation::Persisted),
            StateExpectation(State::OpeningStandbyReplicaWithSTRObliterating, false, CommitExpectation::Persisted));
    }

    BOOST_AUTO_TEST_CASE(OpenStandbyReplica)
    {
        ExecuteCloseDropAndObliterateTest(
            State::OpenStandbyReplica,
            StateExpectation(State::OpenStandbyReplicaClosing, false, CommitExpectation::InMemory),
            StateExpectation(State::OpenStandbyReplicaDropping, false, CommitExpectation::Persisted),
            StateExpectation(State::OpenStandbyReplicaObliterating, false, CommitExpectation::Persisted));
    }

    BOOST_AUTO_TEST_CASE(EndpointUpdateIsResetTest)
    {
        function<void(FailoverUnit&)> verifier = [](FailoverUnit& ft)
        {
            Verify::IsTrueLogOnError(!ft.IsEndpointPublishPending, wformatString("Expected IsEndpointAvailable {0}", ft));
        };

        for (auto const & it : allCloseModes_)
        {
            ExecuteTestAndVerify(
                FT,
                State::ReadyReplicaEndpointUpdatePending,
                it,
                verifier);
        }
    }

    BOOST_AUTO_TEST_CASE(MessageRetryIsCleared)
    {
        function<void(FailoverUnit&)> verifier = [](FailoverUnit& ft)
        {
            Verify::IsTrueLogOnError(!ft.IsEndpointPublishPending, wformatString("Expected IsEndpointAvailable {0}", ft));
        };

        for (auto const & it : allCloseModes_)
        {
            ExecuteTestAndVerify(
                FT,
                State::ReadyReplicaEndpointUpdatePending,
                it,
                verifier);
        }
    }

    BOOST_AUTO_TEST_CASE(ReadWriteStatusRevokedNotificationTest)
    {
        ExecuteReadWriteStatusRevokedNotification(
            L"Ready",
            FT,
            State::ReadyReplica,
            StateExpectation(State::ReadyReplica, CommitExpectation::None));

        ExecuteReadWriteStatusRevokedNotification(
            L"ReadyClosing",
            FT,
            State::ReadyReplicaClosing,
            StateExpectation(State::ReadyReplicaClosingReadWriteStatusRevoked, CommitExpectation::InMemory));

        ExecuteReadWriteStatusRevokedNotification(
            L"ReadyDropping",
            FT,
            State::ReadyReplicaDropping,
            StateExpectation(State::ReadyReplicaDroppingReadWriteStatusRevoked, false, CommitExpectation::InMemory));

        ExecuteReadWriteStatusRevokedNotification(
            L"ReadyDeleting",
            FT,
            State::ReadyReplicaDeleting,
            StateExpectation(State::ReadyReplicaDeleting, CommitExpectation::None));

        ExecuteReadWriteStatusRevokedNotification(
            L"ReadyObliterating",
            FT,
            State::ReadyReplicaObliterating,
            StateExpectation(State::ReadyReplicaObliterating, CommitExpectation::None));

        ExecuteReadWriteStatusRevokedNotification(
            L"OpeningSBWithSTRClosing",
            FT,
            State::OpeningStandbyReplicaWithSTRClosing,
            StateExpectation(State::OpeningStandbyReplicaWithSTRClosingReadWriteStatusRevoked, CommitExpectation::InMemory));

        ExecuteReadWriteStatusRevokedNotification(
            L"OpeningSBWithSTRDropping",
            FT,
            State::OpeningStandbyReplicaWithSTRDropping,
            StateExpectation(State::OpeningStandbyReplicaWithSTRDroppingReadWriteStatusRevoked, CommitExpectation::InMemory));

        ExecuteReadWriteStatusRevokedNotification(
            L"OpenStandByClosing",
            FT,
            State::OpenStandbyReplicaClosing,
            StateExpectation(State::OpenStandbyReplicaClosingReadWriteStatusRevoked, CommitExpectation::InMemory));

        ExecuteReadWriteStatusRevokedNotification(
            L"OpenStandByDropping",
            FT,
            State::OpenStandbyReplicaDropping,
            StateExpectation(State::OpenStandbyReplicaDroppingReadWriteStatusRevoked, CommitExpectation::InMemory));

        ExecuteReadWriteStatusRevokedNotification(
            L"AbortingReplica",
            FT,
            State::AbortingReplica,
            StateExpectation(State::AbortingReplicaReadWriteStatusRevoked, CommitExpectation::InMemory));
    }

    BOOST_AUTO_TEST_CASE(ExecuteReadWriteStatusRevokedNotificationIsProcessed)
    {
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Close, true);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Drop, true);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::DeactivateNode, true);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Abort, true);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Restart, true);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Delete, false);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Deactivate, true);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::ForceAbort, false);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::ForceDelete, false);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::QueuedDelete, false);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::AppHostDown, false);
        ExecuteReadWriteStatusRevokedNotification(ReplicaCloseMode::Obliterate, false);
    }

    BOOST_AUTO_TEST_CASE(DeleteTest)
    {
        function<void(FailoverUnit&)> LocalReplicaDeletedFlagSetVerifier = [](FailoverUnit& ft)
        {
            Verify::IsTrueLogOnError(ft.LocalReplicaDeleted, wformatString("Expected LocalReplicaDeleted {0}", ft));
        };

        function<void(FailoverUnit&)> FMMessageStateClearedVerifier = [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(ft.FMMessageStage == FMMessageStage::None, wformatString("Expected no fm message to be pending {0}", ft));
        };

        function<void(FailoverUnit&)> LastReplicaUpClearedVerifier = [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(!ft.Test_GetReplicaUploadState().IsUploadPending, wformatString("Expected last replica up to not be pending {0}", ft));
        };

        ExecuteTestAndVerify(
            FT,
            State::Closed,
            ReplicaCloseMode::Delete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::Closed,
            ReplicaCloseMode::QueuedDelete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::Closed,
            ReplicaCloseMode::ForceDelete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::ReadyReplica,
            ReplicaCloseMode::Delete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::ReadyReplica,
            ReplicaCloseMode::ForceDelete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::Down,
            ReplicaCloseMode::QueuedDelete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::DownDeleted,
            ReplicaCloseMode::QueuedDelete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::DownInDrop,
            ReplicaCloseMode::QueuedDelete,
            LocalReplicaDeletedFlagSetVerifier);

        ExecuteTestAndVerify(
            FT,
            State::ReadyReplicaDown_SP,
            ReplicaCloseMode::QueuedDelete,
            FMMessageStateClearedVerifier);

        ExecuteTestAndVerify(
            L"SV1",
            State::ReadyReplicaDown_SV,
            ReplicaCloseMode::Delete,
            FMMessageStateClearedVerifier);

        ExecuteTestAndVerify(
            FT,
            State::OpenStandbyReplicaWithReplicaUpPending,
            ReplicaCloseMode::ForceDelete,
            LastReplicaUpClearedVerifier);
    }

    BOOST_AUTO_TEST_CASE(DeactivateTest)
    {
        ExecuteTestAndVerify(
            FT,
            State::ReadyReplica,
            ReplicaCloseMode::Deactivate,
            [](FailoverUnit & ft) { Verify::AreEqualLogOnError(SenderNode, ft.SenderNode, wformatString("Sender Node is not set {0}", ft)); });
    }

    BOOST_AUTO_TEST_CASE(UpdateServiceDescriptionIsResetTest)
    {
        ExecuteTestAndVerify(
            FT,
            State::MessagePendingFlagsAreReset,
            ReplicaCloseMode::Deactivate,
            [](FailoverUnit & ft)
            {
                Verify::AreEqualLogOnError(false, ft.LocalReplicaServiceDescriptionUpdatePending.IsSet, wformatString("SD Update pending should not be set {0}", ft));
                Verify::AreEqualLogOnError(false, ft.MessageRetryActiveFlag.IsSet, wformatString("Reconfig message pending should not be set {0}", ft));
            });
    }

    BOOST_AUTO_TEST_CASE(ReplicaUpIsResetTest)
    {
        ExecuteTestAndVerify(
            FT,
            State::OpenStandbyReplicaWithReplicaUpPending,
            ReplicaCloseMode::Delete,
            [](FailoverUnit & ft) { Verify::IsTrueLogOnError(ft.FMMessageStage == FMMessageStage::None, wformatString("FM message stage is reset {0}", ft)); });
    }

    BOOST_AUTO_TEST_CASE(AppHostDownForObliteratingCompletesObliterate)
    {
        ExecuteTest(
            FT,
            State::ReadyReplicaObliterating,
            StateExpectation(State::ReadyReplicaDropped, true),
            ReplicaCloseMode::AppHostDown);
    }

    BOOST_AUTO_TEST_CASE(ObliterateCallsTerminateIfTheHostIsUp)
    {
        auto & test = holder_->Recreate();
        test.AddFT(FT, State::ReadyReplica);

        InvokeStartCloseOnFT(FT, ReplicaCloseMode::Obliterate);

        test.ValidateTerminateCall(FT);
    }

    BOOST_AUTO_TEST_CASE(FlagStateUpdatedTest)
    {
        for (auto i : allCloseModes_)
        {
            if (i == ReplicaCloseMode::QueuedDelete)
            {
                continue;
            }

            FlagStateUpdatedTestHelper(i);
        }
    }

    BOOST_AUTO_TEST_CASE(SendCloseMessageTest)
    {
        wstring const AbortMessage = L"000/411 [N/N RD U 1:1 n] A s";
        wstring const DropMessage = L"000/411 [N/N RD U 1:1 n] D s";
        wstring const CloseMessage = L"000/411 [N/N RD U 1:1 n] - s";

        SendCloseMessageTestHelper(ReplicaCloseMode::Abort, AbortMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::Drop, DropMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::Close, CloseMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::DeactivateNode, CloseMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::Restart, CloseMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::Delete, DropMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::Deactivate, DropMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::ForceAbort, AbortMessage);
        SendCloseMessageTestHelper(ReplicaCloseMode::ForceDelete, AbortMessage);
    }

    BOOST_AUTO_TEST_CASE(ObliterateDoesNotSendCloseMessage)
    {
        SendCloseMessageTestHelper(ReplicaCloseMode::Obliterate, L"");
    }

    BOOST_AUTO_TEST_CASE(RetryTimerAddsServiceTypeRegistrationAndSendsMessage)
    {
        auto & test = holder_->Recreate();

        test.AddFT(FT, L"O None 000/000/411 1:1 Hd [N/N/P ID U N F 1:1]");
        test.GetFT(FT).RetryableErrorStateObj.EnterState(RetryableErrorStateName::ReplicaClose);
        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::Success);
        test.RequestWorkAndDrain(test.StateItemHelpers.ReplicaCloseRetryHelper);

        test.ValidateFT(FT, L"O None 000/000/411 1:1 MHd [N/N/P ID U N F 1:1]");
        test.ValidateRAPMessage<MessageType::ReplicaClose>(FT);
    }

    BOOST_AUTO_TEST_CASE(RetryTimerAbortsReplicaAndSendsReplicaDropped)
    {
        auto & test = holder_->Recreate();

        test.AddFT(FT, L"O None 000/000/411 1:1 Hd [N/N/P ID U N F 1:1]");
        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::FatalErrorForDrop);
        test.GetFT(FT).RetryableErrorStateObj.EnterState(RetryableErrorStateName::ReplicaClose);
        test.RequestWorkAndDrain(test.StateItemHelpers.ReplicaCloseRetryHelper);

        // FT should be closed but not deleted
        auto & ft = test.GetFT(FT);
        Verify::IsTrueLogOnError(ft.IsClosed, L"IsCLosed");
        Verify::IsTrueLogOnError(!ft.LocalReplicaDeleted, L"IsDeleted");
    }

    BOOST_AUTO_TEST_CASE(RetryTimerSendsMessageIfSTRExists)
    {
        auto & test = AddFTAndInvokeRetryTimer(State::ReadyReplicaDropping);
        test.ValidateRAPMessage<MessageType::ReplicaClose>(FT);
    }

    BOOST_AUTO_TEST_CASE(RetryTimerDoesNotSendMessageForObliterate)
    {
        auto & test = AddFTAndInvokeRetryTimer(State::ReadyReplicaObliterating);
        test.ValidateNoRAPMessages();
    }

    BOOST_AUTO_TEST_CASE(RetryTimerDoesNotTerminateHostAgain)
    {
        auto & test = AddFTAndInvokeRetryTimer(State::ReadyReplicaObliterating);
        test.ValidateNoTerminateCalls();
    }

    BOOST_AUTO_TEST_CASE(ProcessReplicaCloseReplyTest)
    {
        ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode::Abort,
            StateExpectation(State::ReadyReplicaDropped, true),
            StateExpectation(State::ReadyReplicaDropped, true),
            true);

        ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode::Close,
            StateExpectation(State::ReadyReplicaDown_SV, true),
            StateExpectation(State::ReadyReplicaDown_SP, true),
            true);

        ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode::Deactivate,
            StateExpectation(State::ReadyReplicaDropped, true),
            StateExpectation(State::ReadyReplicaDropped, true),
            true);

        ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode::DeactivateNode,
            StateExpectation(State::ReadyReplicaDown_SV, true),
            StateExpectation(State::ReadyReplicaDown_SP, true),
            true);

        ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode::Delete,
            StateExpectation(State::ReadyReplicaDeleted, true),
            StateExpectation(State::ReadyReplicaDeleted, true),
            true);

        ProcessReplicaCloseReplyTestHelper(
            L"SP1",
            ReplicaCloseMode::Restart,
            StateExpectation(State::ReadyReplicaDown_SP, false),
            true);

        ProcessReplicaCloseReplyTestHelper(
            ReplicaCloseMode::Obliterate,
            StateExpectation(State::ReadyReplicaObliterating, false),
            StateExpectation(State::ReadyReplicaObliterating, false),
            false); // should be ignored
    }

    BOOST_AUTO_TEST_CASE(ProcessReplicaCloseReplyAfterRevoked)
    {
        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::Close,
            StateExpectation(State::ReadyReplicaDown_SP, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::Drop,
            StateExpectation(State::ReadyReplicaDropped, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::DeactivateNode,
            StateExpectation(State::ReadyReplicaDown_SP, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::Abort,
            StateExpectation(State::ReadyReplicaDropped, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::Restart,
            StateExpectation(State::ReadyReplicaDown_SP, false),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::Deactivate,
            StateExpectation(State::ReadyReplicaDropped, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::ForceAbort,
            StateExpectation(State::ReadyReplicaDropped, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::ForceDelete,
            StateExpectation(State::ReadyReplicaDeleted, true),
            true);

        ProcessReplicaCloseReplyAfterReadWriteStatusRevokedTestHelper(
            ReplicaCloseMode::Obliterate,
            StateExpectation(State::ReadyReplicaObliterating, false),
            false);
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DownReplicaSendsReplicaDown)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::Close);
        test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DroppedReplicaSendsReplicaDropped)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::Drop);
        ValidateReplicaDroppedMessage(test, FT);
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DeletedReplicaSendsDeleteReplicaReply)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::Delete);
        test.ValidateFMMessage<MessageType::DeleteReplicaReply>(L"SP1", L"000/411 [N/N DD D 1:1 n] Success");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DeletedReplicaSendsRemoveInstanceReply)
    {
        auto & test = CloseCompletionTestHelper(L"SL1", ReplicaCloseMode::Delete, L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
        test.ValidateFMMessage<MessageType::RemoveInstanceReply>(L"SL1", L"000/411 [N/N DD D 1:1 n] Success");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_ForceAbortedReplicaSendsReplicaDropped)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::ForceAbort);
        ValidateReplicaDroppedMessage(test, FT);
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_ForceDeletedReplicaIsNoOp)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::ForceDelete);
        test.ValidateNoFMMessages();
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DeactivatedNodeSendsReplicaDown)
    {
        auto & test = holder_->ScenarioTestObj;
        test.SetNodeActivationState(false, 2);

        CloseCompletionTestHelper(L"SL1", ReplicaCloseMode::DeactivateNode, L"O None 000/000/411 1:1 CM [N/N/N RD U N F 1:1]");
        test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false  | {SL1 000/000/411 [N/N/N DD D 1:1 n]}");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DeactivatedNodeOpensReplicaIfNodeIsActivated)
    {
        // By default node is activated
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::DeactivateNode);
        test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");

        // Verify that the FT is opening now - i.e. Open Down Replica is called
        test.ValidateFT(FT, L"O None 000/000/411 1:2 1 IS [N/N/P SB U N F 1:2]");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_DeactivateSendsDeactivateReply)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::Deactivate);
        test.ValidateRAMessage<MessageType::DeactivateReply>(SenderNode.Id.IdValue.Low, FT, L"000/411 [N/N DD D 1:1 n] Success");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_RestartOpensDownReplica)
    {
        auto & test = CloseCompletionTestHelper(FT, ReplicaCloseMode::Restart);
        test.ValidateFMMessage<MessageType::ReplicaUp>(L"false false {SP1 000/000/411 [N/N/P SB D 1:1]} |");
        test.ValidateFT(FT, L"O None 000/000/411 1:2 1 IS [N/N/P SB U N F 1:2]");
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_ObliteratedWithAppHostDownSendsReplicaDropped)
    {
        auto & test = StartCloseCompletionTest(FT, ReplicaCloseMode::Obliterate, State::ReadyReplica);
        test.ProcessAppHostClosedAndDrain(FT);
        ValidateReplicaDroppedMessage(test, FT);
    }

    BOOST_AUTO_TEST_CASE(CloseCompletion_ObliteratedDirectlySendsReplicaDropped)
    {
        auto & test = StartCloseCompletionTest(FT, ReplicaCloseMode::Obliterate, State::Down);
        ValidateReplicaDroppedMessage(test, FT);
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}

#pragma endregion

#pragma region ResetLocalState

namespace ResetLocalState
{
    class TestLifeCycle_ResetLocalState
    {
    protected:
        TestLifeCycle_ResetLocalState() { BOOST_REQUIRE(TestSetup()); }
        ~TestLifeCycle_ResetLocalState() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_SETUP(TestSetup);
        TEST_METHOD_CLEANUP(TestCleanup);

        ScenarioTest & Execute(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & initialState,
            function<void(FailoverUnit&)> setter);

        ScenarioTest & ExecuteAndVerify(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & initialState,
            function<void(FailoverUnit&)> verifier);

        ScenarioTest & ExecuteAndVerify(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & initialState,
            function<void(FailoverUnit&)> setter,
            function<void(FailoverUnit&)> verifier);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestLifeCycle_ResetLocalState::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_ResetLocalState::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    ScenarioTest & TestLifeCycle_ResetLocalState::Execute(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialState,
        function<void(FailoverUnit&)> setter)
    {
        auto & test = holder_->Recreate();
        test.LogStep(log);
        test.AddFT(ftShortName, initialState);

        if (setter != nullptr)
        {
            setter(test.GetFT(ftShortName));
        }

        test.ExecuteUnderLock(
            ftShortName,
            [&](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent &)
        {
            ft.EnableUpdate();
            ft->Test_ResetLocalState(queue);
        });

        return test;
    }

    ScenarioTest & TestLifeCycle_ResetLocalState::ExecuteAndVerify(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialState,
        function<void(FailoverUnit&)> verifier)
    {
        return ExecuteAndVerify(
            log,
            ftShortName,
            initialState,
            nullptr,
            verifier);
    }

    ScenarioTest & TestLifeCycle_ResetLocalState::ExecuteAndVerify(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialState,
        function<void(FailoverUnit&)> setter,
        function<void(FailoverUnit&)> verifier)
    {
        auto & test = Execute(log, ftShortName, initialState, setter);

        auto & ft = test.GetFT(ftShortName);
        verifier(ft);

        return test;
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_ResetLocalStateSuite, TestLifeCycle_ResetLocalState)

    BOOST_AUTO_TEST_CASE(OtherFlags)
    {
        ExecuteAndVerify(
            L"Reconfiguration",
            L"SP1",
            L"O Phase1_GetLSN 411/000/422 1:1 CMN [P/N/P SB U RAP F 1:1]",
            [](FailoverUnit & ft)
        {
            Verify::AreEqualLogOnError(FailoverUnitReconfigurationStage::None, ft.ReconfigurationStage, L"Reconifg stage");
            Verify::IsTrueLogOnError(!ft.MessageRetryActiveFlag.IsSet, L"Message Retry ");
        });

        ExecuteAndVerify(
            L"DeleteIsUnchanged",
            L"SP1",
            L"O None 000/000/411 1:1 CMLHl [N/N/P ID U N F 1:1]",
            [](FailoverUnit & ft) { Verify::IsTrueLogOnError(ft.LocalReplicaDeleted, L"Deleted"); });

        ExecuteAndVerify(
            L"Replicator Flags",
            L"SP1",
            L"O None 000/000/411 1:1 CMB [N/N/P RD U N F 1:1]",
            [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(!ft.IsUpdateReplicatorConfiguration, L"UpdateReplicatorConfig");
        });

        ExecuteAndVerify(
            L"LocalReplicaOpen",
            L"SP1",
            L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
            [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(!ft.LocalReplicaOpen, L"LocalReplicaopen");
        });

        ExecuteAndVerify(
            L"LocalReplicaOpenPending",
            L"SP1",
            L"O None 000/000/411 1:1 MS [N/N/P IC U N F 1:1]",
            [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(!ft.LocalReplicaOpenPending.IsSet, L"LocalReplicaOpenPending");
        });

        ExecuteAndVerify(
            L"LocalReplicaClosePending",
            L"SP1",
            L"O None 000/000/411 1:1 AHcCM [N/N/P RD U N F 1:1]",
            [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(!ft.LocalReplicaClosePending.IsSet, L"LocalReplicaClosePending");
        });

        ExecuteAndVerify(
            L"Proxy Update Service Description ",
            L"SP1",
            L"O None 000/000/411 1:1 CMT [N/N/P RD U N F 1:1]",
            [](FailoverUnit & ft)
        {
            Verify::IsTrueLogOnError(!ft.LocalReplicaServiceDescriptionUpdatePending.IsSet, L"LocalReplicaServiceDescriptionUpdatePending");
        });

        ExecuteAndVerify(
            L"LastUpdated",
            L"SP1",
            L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
            [](FailoverUnit & ft) { ft.Test_SetLastUpdatedTime(DateTime::Zero); },
            [](FailoverUnit & ft)
        {
            Verify::IsTrue(ft.LastUpdatedTime != DateTime::Zero, L"DateTime must be set");
        });
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}

#pragma endregion

#pragma region Down
namespace Down
{
    wstring const FT = L"SP1";

    namespace State
    {
        wstring const Closed = L"C None 000/000/411 1:1 -";

        wstring const DownReplyPendingForClosed = L"C None 000/000/411 1:1 I";

        wstring const DownReplyPendingForOpeningSB = L"O None 000/000/411 1:2 1 IS [N/N/P SB U N F 1:2]";

        wstring const DownReplyPendingForDown = L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]";

        wstring const DownReplyPendingForIDDown = L"O None 000/000/411 1:1 1 I [N/N/P ID D N F 1:1]";

        wstring const DownReplyPendingForOpeningID = L"O None 000/000/411 1:2 1 IS [N/N/P ID U N F 1:2]";
    }

    class TestLifeCycle_ReplicaDownSender
    {
    protected:
        TestLifeCycle_ReplicaDownSender() { BOOST_REQUIRE(TestSetup()); }
        ~TestLifeCycle_ReplicaDownSender() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_SETUP(TestSetup);
        TEST_METHOD_CLEANUP(TestCleanup);

        void ReplicaDownTestHelper(
            wstring const & log,
            wstring const & state,
            wstring const & expectedReplicaDescription);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestLifeCycle_ReplicaDownSender::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_ReplicaDownSender::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    void TestLifeCycle_ReplicaDownSender::ReplicaDownTestHelper(
        wstring const & log,
        wstring const & state,
        wstring const & expectedReplicaDescription)
    {
        auto & test = holder_->Recreate();
        test.LogStep(log);
        test.AddFT(FT, state);

        FMMessageData fmMessage;
        bool result = test.GetFT(FT).TryComposeFMMessage(Default::GetInstance().NodeInstance, Stopwatch::Now(), fmMessage);

        if (expectedReplicaDescription.empty())
        {
            Verify::AreEqualLogOnError(false, result, L"TryGet... Result");
        }
        else
        {
            Verify::AreEqual(FMMessageStage::ReplicaDown, fmMessage.FMMessageStage, L"MessageStage");
            Verify::IsTrueLogOnError(result, L"TryGet... Result");
            auto actual = fmMessage.TakeReplicaUp().first.LocalReplica;
            Verify::Compare(expectedReplicaDescription, actual, Default::GetInstance().SP1_FTContext, wformatString("State {0}, Replica Description. Expected = {1}. Actual = {2}", log, expectedReplicaDescription, actual));
        }
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_ReplicaDownSenderSuite, TestLifeCycle_ReplicaDownSender)

    BOOST_AUTO_TEST_CASE(ReplicaDownTest)
    {
        ReplicaDownTestHelper(
            L"Closed",
            State::Closed,
            L"");

        ReplicaDownTestHelper(
            L"DownReplyPendingForClosed",
            State::DownReplyPendingForClosed,
            L"N/N DD D 1:1 n");

        ReplicaDownTestHelper(
            L"DownReplyPendingForOpeningSB",
            State::DownReplyPendingForOpeningSB,
            L"N/P SB D 1:1 -1 -1 1.0:1.0:1");

        ReplicaDownTestHelper(
            L"DownReplyPendingForDown",
            State::DownReplyPendingForDown,
            L"N/P SB D 1:1 -1 -1 1.0:1.0:1");

        ReplicaDownTestHelper(
            L"DownReplyPendingForIDDown",
            State::DownReplyPendingForIDDown,
            L"N/N DD D 1:1 n");
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()

    class TestLifeCycle_Down
    {
    protected:
        TestLifeCycle_Down() { BOOST_REQUIRE(TestSetup()); }
        ~TestLifeCycle_Down() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_SETUP(TestSetup);
        TEST_METHOD_CLEANUP(TestCleanup);

        ScenarioTest & Execute(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & initialState);

        ScenarioTest & ExecuteAndVerifyState(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & initialState,
            wstring const & finalState);

        ScenarioTest & ExecuteAndVerify(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & initialState,
            function<void(FailoverUnit&)> verifier);

        ScenarioTestHolderUPtr holder_;
    };

    bool TestLifeCycle_Down::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_Down::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    ScenarioTest & TestLifeCycle_Down::Execute(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialState)
    {
        auto & test = holder_->Recreate();
        test.LogStep(log);
        test.AddFT(ftShortName, initialState);

        test.ExecuteUnderLock(
            ftShortName,
            [&](LockedFailoverUnitPtr & ft, StateMachineActionQueue & queue, ReconfigurationAgent & ra)
        {
            ft.EnableUpdate();
            ft->Test_UpdateStateOnLocalReplicaDown(ra.HostingAdapterObj, queue);
        });

        return test;
    }

    ScenarioTest & TestLifeCycle_Down::ExecuteAndVerifyState(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialState,
        wstring const & finalState)
    {
        auto & test = Execute(log, ftShortName, initialState);

        test.ValidateFT(ftShortName, finalState);

        return test;
    }

    ScenarioTest & TestLifeCycle_Down::ExecuteAndVerify(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & initialState,
        function<void(FailoverUnit&)> verifier)
    {
        auto & test = Execute(log, ftShortName, initialState);

        auto & ft = test.GetFT(ftShortName);
        verifier(ft);

        return test;
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_DownSuite, TestLifeCycle_Down)

    BOOST_AUTO_TEST_CASE(ReplicaLifeStatesTest)
    {
        ExecuteAndVerifyState(
            L"ID Persisted Replica",
            L"SP1",
            L"O None 000/000/411 1:1 CHaM [N/N/P ID U N F 1:1]",
            L"O None 000/000/411 1:1 - [N/N/P ID D N F 1:1]");

        ExecuteAndVerifyState(
            L"Non ID Persisted Replica",
            L"SP1",
            L"O None 000/000/411 1:1 CM [N/N/P IC U N F 1:1]",
            L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");

        ExecuteAndVerifyState(
            L"Ready Replica Persisted",
            L"SP1",
            L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
            L"O None 000/000/411 1:1 - [N/N/P SB D N F 1:1]");
    }

    BOOST_AUTO_TEST_CASE(ServiceTypeRegistrationTest)
    {
        ExecuteAndVerify(
            L"SP Registration Is Cleared",
            L"SP1",
            L"O None 000/000/411 1:1 CM [N/N/P RD U N F 1:1]",
            [](FailoverUnit &ft)
        {
            Verify::IsTrueLogOnError(ft.ServiceTypeRegistration == nullptr, L"ST must be released");
            Verify::IsTrueLogOnError(ft.IsServicePackageInUse, L"SP is in use");
        });
    }

    BOOST_AUTO_TEST_CASE(ConfigurationCleanupTest)
    {
        ExecuteAndVerify(
            L"Other replicas are removed",
            L"SP1",
            L"O None 000/000/411 1:1 CMK [N/N/P RD U N F 1:1] [N/N/I IB U N F 2:1]",
            [](FailoverUnit & ft) { Verify::AreEqualLogOnError(1, ft.Replicas.size(), L"Replica Size"); });

        ExecuteAndVerify(
            L"Configuration Replicas are not removed",
            L"SP1",
            L"O None 000/000/411 1:1 CMK [N/N/P RD U N F 1:1] [N/N/S IB U N F 2:1]",
            [](FailoverUnit & ft) { Verify::AreEqualLogOnError(2, ft.Replicas.size(), L"Replica Size"); });
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}
#pragma endregion

#pragma region Reopen Down Replica
namespace ReopenDownReplica
{
    wstring const FT = L"SP1";

    namespace State
    {
        wstring const Closed = L"C None 000/000/411 1:1 -";

        wstring const InDropDown = L"O None 000/000/411 1:1 1 I [N/N/P ID D N F 1:1]";
        wstring const InDropUp = L"O None 000/000/411 1:2 1 IHd [N/N/P ID U N F 1:2]";
        wstring const InDropUpWithSTR = L"O None 000/000/411 1:2 1 IMHd [N/N/P ID U N F 1:2]";

        wstring const InDropDownDeleted = L"O None 000/000/411 1:1 L [N/N/P ID D N F 1:1]";
        wstring const InDropUpDeleted = L"O None 000/000/411 1:2 LHf [N/N/P ID U N F 1:2]";
        wstring const InDropUpWithSTRDeleted = L"O None 000/000/411 1:2 LMHf [N/N/P ID U N F 1:2]";

        wstring const StandByDown = L"O None 000/000/411 1:1 1 I [N/N/P SB D N F 1:1]";
        wstring const StandByUp = L"O None 000/000/411 1:2 1 IS [N/N/P SB U N F 1:2]";

        wstring const StandByDownNoReplicaDown = L"O None 000/000/411 1:1 1 - [N/N/P SB D N F 1:1]";
        wstring const StandByUpNoReplicaDown = L"O None 000/000/411 1:2 1 S [N/N/P SB U N F 1:2]";
        wstring const StandByUpNoReplicaWithSTR = L"O None 000/000/411 1:2 1 SM [N/N/P SB U N F 1:2]";

        wstring const Up = L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]";
    }

    class TestLifeCycle_ReopenDownReplica
    {
    protected:
        TestLifeCycle_ReopenDownReplica() { BOOST_REQUIRE(TestSetup()); }
        ~TestLifeCycle_ReopenDownReplica() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_SETUP(TestSetup);
        TEST_METHOD_CLEANUP(TestCleanup);

        void CanProcessFalseTestHelper(
            wstring const & log,
            wstring const & ftShortName,
            wstring const & ftState);

        ScenarioTest & InitializeFTWithState(
            wstring const & log,
            wstring const & initial);

        ScenarioTest & ExecuteTestOnFT(
            wstring const & log,
            wstring const & initial,
            wstring const & finalState,
            RetryableErrorStateName::Enum expectedRES);

        ScenarioTest & InvokeOnRA();

        ScenarioTestHolderUPtr holder_;
    };

    bool TestLifeCycle_ReopenDownReplica::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_ReopenDownReplica::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    ScenarioTest & TestLifeCycle_ReopenDownReplica::InitializeFTWithState(
        wstring const & log,
        wstring const & initial)
    {
        auto & test = holder_->Recreate();
        test.LogStep(log);
        test.AddFT(FT, initial);
        return test;
    }

    ScenarioTest & TestLifeCycle_ReopenDownReplica::ExecuteTestOnFT(
        wstring const & log,
        wstring const & initial,
        wstring const & finalState,
        RetryableErrorStateName::Enum expectedRES)
    {
        auto & test = InitializeFTWithState(log, initial);

        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::RetryableErrorForOpenAndReopenAndDrop);
        test.ExecuteJobItemHandler<JobItemContextBase>(
            FT,
            [](HandlerParameters & handlerParameters, JobItemContextBase & )
        {
            auto & ft = handlerParameters.FailoverUnit;
            ft.EnableUpdate();
            Verify::IsTrueLogOnError(ft->CanReopenDownReplica(), L"CanReopenDownReplica must be true");
            ft->ReopenDownReplica(handlerParameters.ExecutionContext);
            return true;
        });

        test.ValidateFT(FT, finalState);
        Verify::AreEqualLogOnError(expectedRES, test.GetFT(FT).RetryableErrorStateObj.CurrentState, L"RetryableErrorState");
        return test;
    }

    ScenarioTest & TestLifeCycle_ReopenDownReplica::InvokeOnRA()
    {
        auto & test = holder_->ScenarioTestObj;

        test.ExecuteJobItemHandler<JobItemContextBase>(
            FT,
            [&](HandlerParameters & handlerParameters, JobItemContextBase &)
        {
            handlerParameters.RA.ReopenDownReplica(handlerParameters);
            return true;
        });

        return test;
    }

    void TestLifeCycle_ReopenDownReplica::CanProcessFalseTestHelper(
        wstring const & log,
        wstring const & ftShortName,
        wstring const & ftState)
    {
        auto & test = holder_->Recreate();

        test.LogStep(log);

        test.AddFT(ftShortName, ftState);

        Verify::IsTrueLogOnError(!test.GetFT(ftShortName).CanReopenDownReplica(), wformatString("{0} {1}", log, ftState));
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_ReopenDownReplicaSuite, TestLifeCycle_ReopenDownReplica)

    BOOST_AUTO_TEST_CASE(CanProcessFalseTest)
    {
        CanProcessFalseTestHelper(L"Closed", FT, State::Closed);
        CanProcessFalseTestHelper(L"Up", FT, State::Up);
        CanProcessFalseTestHelper(L"Closed+Volatile", L"SL1", State::Closed);
    }

    BOOST_AUTO_TEST_CASE(ReopenTests)
    {
        ExecuteTestOnFT(L"InDropDown", State::InDropDown, State::InDropUp, RetryableErrorStateName::ReplicaClose);
        ExecuteTestOnFT(L"InDropDownDeleted", State::InDropDownDeleted, State::InDropUpDeleted, RetryableErrorStateName::ReplicaDelete);
        ExecuteTestOnFT(L"StandByDown", State::StandByDown, State::StandByUp, RetryableErrorStateName::ReplicaReopen);
        ExecuteTestOnFT(L"StandBydownNoReplicaDown", State::StandByDownNoReplicaDown, State::StandByUpNoReplicaDown, RetryableErrorStateName::ReplicaReopen);
    }

    BOOST_AUTO_TEST_CASE(ReopenOnRADuringFabricUpgrade)
    {
        auto & test = InitializeFTWithState(L"ReopenOnRADuringFabricUpgrade", State::StandByDown);

        test.UTContext.RA.StateObj.OnFabricCodeUpgradeStart();

        InvokeOnRA();

        test.ValidateFT(FT, State::StandByDown);
        test.ValidateNoMessages();
    }

    BOOST_AUTO_TEST_CASE(ReopenOnRACanProcessFalseTest)
    {
        auto & test = InitializeFTWithState(L"ReopenOnRACanProcessFalseTest", State::Closed);

        InvokeOnRA();

        test.ValidateNoRAPMessages();
        test.ValidateNoFMMessages();

        test.ValidateFT(FT, State::Closed);
    }

    BOOST_AUTO_TEST_CASE(ReopenOnRATestIDReplica)
    {
        auto & test = InitializeFTWithState(L"ReopenOnRATestIDReplica", State::InDropDown);

        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::Success);
        InvokeOnRA();

        test.ValidateFT(FT, State::InDropUpWithSTR);
        test.ValidateRAPMessage<MessageType::ReplicaClose>(FT);
    }

    BOOST_AUTO_TEST_CASE(ReopenOnRATestIDDeletedReplica)
    {
        auto & test = InitializeFTWithState(L"ReopenOnRATestIDDeletedReplica", State::InDropDownDeleted);

        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::Success);
        InvokeOnRA();

        test.ValidateFT(FT, State::InDropUpWithSTRDeleted);
        test.ValidateRAPMessage<MessageType::ReplicaClose>(FT);
    }

    BOOST_AUTO_TEST_CASE(ReopenOnRATestSBReplica)
    {
        auto & test = InitializeFTWithState(L"ReopenOnRATestSBReplica", State::StandByDown);

        InvokeOnRA();

        test.ValidateFT(FT, State::StandByUp);
    }

    BOOST_AUTO_TEST_CASE(StandBySuccessSTRTest)
    {
        auto & test = InitializeFTWithState(L"ReopenOnRACanProcessFalseTest", State::StandByDownNoReplicaDown);

        test.SetFindServiceTypeRegistrationSuccess(FT);
        InvokeOnRA();

        test.ValidateFT(FT, State::StandByUpNoReplicaWithSTR);
        test.ValidateRAPMessage<MessageType::StatefulServiceReopen>(FT);
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}

#pragma endregion

#pragma region Opening
namespace Opening
{
    wstring const FT = L"SP1";

    namespace State
    {
        wstring const OpeningSBReplicaWithoutSTR = L"O None 000/000/411 1:2 1 IS [N/N/P SB U N F 1:2]";
        wstring const OpeningSBReplicaWithSTR = L"O None 000/000/411 1:2 1 IMS [N/N/P SB U N F 1:2]";
        wstring const ChangeRoleSBReplicaWithSTR = L"O None 000/000/411 1:1 CMS [N/N/P IC U N F 1:1]";
        wstring const OpeningICReplicaWithoutSTR = L"O None 000/000/411 1:1 S [N/N/P IC U N F 1:1]";
        wstring const OpeningICReplicaWithSTR = L"O None 000/000/411 1:1 SM [N/N/P IC U N F 1:1]";
    }

    class TestLifeCycle_Opening
    {
    protected:
        TestLifeCycle_Opening() { BOOST_REQUIRE(TestSetup()); }
        ~TestLifeCycle_Opening() { BOOST_REQUIRE(TestCleanup()); }
        TEST_METHOD_SETUP(TestSetup);
        TEST_METHOD_CLEANUP(TestCleanup);

        template<MessageType::Enum T>
        void ValidateMessageScenario(
            wstring const & initial,
            RetryableErrorStateName::Enum retryableErrorState,
            wstring const & finalState,
            wstring const & message)
        {
            auto & test = InitializeFTWithState(FT, initial);

            test.SetFindServiceTypeRegistrationSuccess(FT);

            test.GetFT(FT).RetryableErrorStateObj.EnterState(retryableErrorState);

            ExecuteRetry();

            test.ValidateRAPMessage<T>(FT, message);
            test.ValidateFT(FT, finalState);
        }

        ScenarioTest & InitializeFTWithState(
            wstring const & log,
            wstring const & initial);

        ScenarioTest & ExecuteRetry();

        ScenarioTestHolderUPtr holder_;
    };

    bool TestLifeCycle_Opening::TestSetup()
    {
        holder_ = ScenarioTestHolder::Create();
        return true;
    }

    bool TestLifeCycle_Opening::TestCleanup()
    {
        holder_.reset();
        return true;
    }

    ScenarioTest & TestLifeCycle_Opening::InitializeFTWithState(
        wstring const & log,
        wstring const & initial)
    {
        auto & test = holder_->Recreate();

        test.LogStep(log);
        test.AddFT(FT, initial);

        return test;
    }

    ScenarioTest & TestLifeCycle_Opening::ExecuteRetry()
    {
        auto & test = holder_->ScenarioTestObj;
        test.RequestWorkAndDrain(test.StateItemHelpers.ReplicaOpenRetryHelper);
        return test;
    }

    BOOST_AUTO_TEST_SUITE(StateMachine)

    BOOST_FIXTURE_TEST_SUITE(TestLifeCycle_OpeningSuite, TestLifeCycle_Opening)

    BOOST_AUTO_TEST_CASE(FailureInTryGetAndAddDoesNotSendMessage)
    {
        auto & test = InitializeFTWithState(L"Failure1", State::OpeningSBReplicaWithoutSTR);
        test.GetFT(FT).RetryableErrorStateObj.EnterState(RetryableErrorStateName::ReplicaReopen);
        test.SetFindServiceTypeRegistrationState(FT, FindServiceTypeRegistrationError::CategoryName::FatalErrorForOpen);

        ExecuteRetry();

        test.ValidateNoRAPMessages();
    }

    BOOST_AUTO_TEST_CASE(FSRSuccessAtOpenICReplicaSendsMessage)
    {
        ValidateMessageScenario<MessageType::ReplicaOpen>(
            State::OpeningICReplicaWithoutSTR,
            RetryableErrorStateName::FindServiceRegistrationAtOpen,
            State::OpeningICReplicaWithSTR,
            L"000/411 [N/P IB U 1:1] - s");
    }

    BOOST_AUTO_TEST_CASE(FSRSuccessAtOpenSBReplicaSendsMessage)
    {
        ValidateMessageScenario<MessageType::StatefulServiceReopen>(
            State::OpeningSBReplicaWithoutSTR,
            RetryableErrorStateName::FindServiceRegistrationAtReopen,
            State::OpeningSBReplicaWithSTR,
            L"000/411 [N/P SB U 1:2] - s");
    }

    BOOST_AUTO_TEST_CASE(ChangingRoleReplicaSendsMessage)
    {
        ValidateMessageScenario<MessageType::ReplicaOpen>(
            State::ChangeRoleSBReplicaWithSTR,
            RetryableErrorStateName::ReplicaReopen,
            State::ChangeRoleSBReplicaWithSTR,
            L"000/411 [N/P IB U 1:1] - s");
    }

    BOOST_AUTO_TEST_CASE(OpeningICReplicaSendsMessage)
    {
        ValidateMessageScenario<MessageType::ReplicaOpen>(
            State::OpeningICReplicaWithSTR,
            RetryableErrorStateName::ReplicaOpen,
            State::OpeningICReplicaWithSTR,
            L"000/411 [N/P IB U 1:1] - s");
    }

    BOOST_AUTO_TEST_CASE(OpeningSBReplicaSendsMessage)
    {
        ValidateMessageScenario<MessageType::StatefulServiceReopen>(
            State::OpeningSBReplicaWithSTR,
            RetryableErrorStateName::ReplicaReopen,
            State::OpeningSBReplicaWithSTR,
            L"000/411 [N/P SB U 1:2] - s");
    }

    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()
}
#pragma endregion
