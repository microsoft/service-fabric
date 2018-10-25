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

using namespace ApiMonitoring;

namespace
{
    CompatibleOperationTable CreateTable()
    {
        CompatibleOperationTableBuilder builder(false);

        builder.MarkTrue(ApiName::Open, ApiName::Close);

        return builder.CreateTable();
    }

    const CompatibleOperationTable DefaultOpTable = CreateTable();
    const ErrorCode DefaultError(ErrorCodeValue::AbandonedFileWriteLockFound);

    ApiCallDescriptionSPtr CreateDescription(ApiName::Enum name)
    {
        /*Common::Guid const & partitionId,
            Federation::NodeInstance const & nodeId,
            int64 replicaId,
            int64 replicaInstanceId,
            ApiNameDescription apiName,
            Common::StopwatchTime startTime) :*/

        ApiNameDescription nameDescription(InterfaceName::IReplicator, name, L"");

        MonitoringData data(Guid::NewGuid(), 1, 1, nameDescription, StopwatchTime::Zero);

        MonitoringParameters parameters;
        
        return make_shared<ApiCallDescription>(data, parameters);
    }
}

class MyOperationManager : public OperationManager
{
public:
    MyOperationManager() : OperationManager(DefaultOpTable) {}
};

class DummyAsyncOperation : public AsyncOperation
{
    DENY_COPY(DummyAsyncOperation);

public:
    DummyAsyncOperation(AsyncCallback const & cb, AsyncOperationSPtr const & parent) :
        AsyncOperation(cb, parent)
    {
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const &) override
    {
    }
};

class OperationManagerBaseTest : public TestBase
{

protected:
    ~OperationManagerBaseTest()
    {
        TryCompleteAsyncOp();
    }

    bool InvokeTryStart(ApiName::Enum name)
    {
        lastApiCallDescription_ = CreateDescription(name);
        return opMgr_.TryStartOperation(lastApiCallDescription_);
    }

    ApiCallDescriptionSPtr Finish(ApiName::Enum operation)
    {
        auto rv = opMgr_.FinishOperation(operation, DefaultError);
        Verify::IsTrue(!opMgr_.IsOperationRunning(operation), L"IsRunning");
        return rv;
    }

    void TryCompleteAsyncOp()
    {
        if (lastAsyncOp_ != nullptr && !lastAsyncOp_->IsCompleted)
        {
            lastAsyncOp_->TryComplete(lastAsyncOp_);
        }
    }

    bool InvokeTryContinue(ApiName::Enum operation)
    {
        TryCompleteAsyncOp();

        auto op = AsyncOperation::CreateAndStart<DummyAsyncOperation>([](AsyncOperationSPtr const &) {}, AsyncOperationSPtr());
        lastAsyncOp_ = dynamic_pointer_cast<DummyAsyncOperation>(op);

        return opMgr_.TryContinueOperation(operation, lastAsyncOp_);
    }

    bool InvokeTryStartMultiOperation(ApiName::Enum name)
    {
        lastApiCallDescription_ = CreateDescription(name);
        auto rv = opMgr_.TryStartMultiInstanceOperation(lastApiCallDescription_, storage_);
        return rv;
    }

    bool InvokeTryContinueMultiOperation(ApiName::Enum operation)
    {
        TryCompleteAsyncOp();

        auto op = AsyncOperation::CreateAndStart<DummyAsyncOperation>([](AsyncOperationSPtr const &) {}, AsyncOperationSPtr());
        lastAsyncOp_ = dynamic_pointer_cast<DummyAsyncOperation>(op);

        return opMgr_.TryContinueMultiInstanceOperation(operation, storage_, lastAsyncOp_);
    }

    ApiCallDescriptionSPtr FinishMulti(ApiName::Enum operation)
    {
        auto rv = opMgr_.FinishMultiInstanceOperation(operation, storage_, DefaultError);
        Verify::IsTrue(!opMgr_.IsOperationRunning(operation), L"IsRunning");
        return rv;
    }

    void StartAndVerify(ApiName::Enum name, bool expected)
    {
        bool check = opMgr_.CheckIfOperationCanBeStarted(name);
        auto rv = InvokeTryStart(name);
        Verify::AreEqual(expected, rv, wformatString("StartAndVerify {0}", name));
        Verify::AreEqual(check, rv, wformatString("Check did not match start {0}", name));
        Verify::AreEqual(opMgr_.IsOperationRunning(name), rv, L"IsRunning");
    }

    void StartMultiOperationAndVerify(ApiName::Enum name, bool expected)
    {
        bool rv = InvokeTryStartMultiOperation(name);
        Verify::AreEqual(expected, rv, L"TryStart");
    }

    void ContinueAndVerify(ApiName::Enum name, bool expected)
    {
        InvokeTryStart(name);

        auto rv = InvokeTryContinue(ApiName::Open);
        Verify::AreEqual(expected, rv, L"Try Continue");
    }

    void VerifyCancellationState(bool expected)
    {
        Verify::IsTrue(lastAsyncOp_ != nullptr, L"async op");

        if (lastAsyncOp_ != nullptr)
        {
            Verify::AreEqual(expected, lastAsyncOp_->IsCancelRequested, L"Was cancel called");
        }
    }

    void CancelOperations()
    {
        opMgr_.CancelOperations();
    }

    void Abort()
    {
        opMgr_.CloseForBusiness(true);
    }

    void Close()
    {
        opMgr_.CloseForBusiness(false);
    }

    void Open()
    {
        opMgr_.OpenForBusiness();
    }

    void MarkForCancel(ApiName::Enum operation)
    {
        opMgr_.CancelOrMarkOperationForCancel(operation);
    }

    void RemoveForCancel(ApiName::Enum operation)
    {
        opMgr_.RemoveOperationForCancel(operation);
    }

    ExecutingOperation storage_;
    shared_ptr<DummyAsyncOperation> lastAsyncOp_;
    ApiCallDescriptionSPtr lastApiCallDescription_;
    MyOperationManager opMgr_;
};

BOOST_AUTO_TEST_SUITE(Unit)

BOOST_FIXTURE_TEST_SUITE(OperationManagerBaseTestSuite, OperationManagerBaseTest)

BOOST_AUTO_TEST_CASE(Open_AllowsForOperationToBeStarted)
{
    StartAndVerify(ApiName::Open, true);
}

BOOST_AUTO_TEST_CASE(Open_DoesNotStartIncompatibleOperations)
{
    InvokeTryStart(ApiName::Open);

    StartAndVerify(ApiName::ChangeRole, false);
}

BOOST_AUTO_TEST_CASE(Open_StartsIncompatibleOperations)
{
    InvokeTryStart(ApiName::Open);

    StartAndVerify(ApiName::Close, true);
}

BOOST_AUTO_TEST_CASE(Abort_NoOperationsAreStarted)
{
    Abort();

    StartAndVerify(ApiName::Close, false);
}

BOOST_AUTO_TEST_CASE(Close_CloseCanStillBeRun)
{
    Close();

    StartAndVerify(ApiName::Close, true);
}

BOOST_AUTO_TEST_CASE(Close_OtherCannotBeStarted)
{
    Close();

    StartAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Reopen_OperationCanBeStarted)
{
    Close();

    Open();

    StartAndVerify(ApiName::Open, true);
}

BOOST_AUTO_TEST_CASE(FinishOperation_ReturnsMonitoredOperation)
{
    InvokeTryStart(ApiName::Open);

    auto expected = lastApiCallDescription_;

    auto actual = Finish(ApiName::Open);

    Verify::IsTrue(expected == actual, L"Api call description");
}

BOOST_AUTO_TEST_CASE(FinishOperation_AssertsIfOpNotFound)
{
    Verify::Asserts([&] ()
                    {
                        Finish(ApiName::Open);
                    }, L"Finish assert");
}

BOOST_AUTO_TEST_CASE(Continue_ReturnsTrueIfNotClosed)
{
    InvokeTryStart(ApiName::Open);

    ContinueAndVerify(ApiName::Open, true);
}

BOOST_AUTO_TEST_CASE(Continue_ReturnsFalseIfNotClosed)
{
    InvokeTryStart(ApiName::Open);

    Close();

    ContinueAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Continue_ReturnsFalseIfAborted)
{
    InvokeTryStart(ApiName::Open);

    Abort();

    ContinueAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Cancel_StopsAllExecutingOperations)
{
    InvokeTryStart(ApiName::Open);

    InvokeTryContinue(ApiName::Open);

    Close();

    CancelOperations();

    VerifyCancellationState(true);
}

BOOST_AUTO_TEST_CASE(Cancel_DoesNotCancelAllowedOperations)
{
    // Close is allowed when the op mgr has been closed so cancel should not cancel it
    InvokeTryStart(ApiName::Close);

    InvokeTryContinue(ApiName::Close);

    Close();

    CancelOperations();

    VerifyCancellationState(false);
}

BOOST_AUTO_TEST_CASE(Mark_DisallowedOpIsNotRun)
{
    MarkForCancel(ApiName::Open);

    StartAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Mark_CancelsRunningDisallowedOp)
{
    InvokeTryStart(ApiName::Open);

    InvokeTryContinue(ApiName::Open);

    MarkForCancel(ApiName::Open);

    VerifyCancellationState(true);
}

BOOST_AUTO_TEST_CASE(Remove_AllowsOpToRunAgain)
{
    MarkForCancel(ApiName::Open);

    RemoveForCancel(ApiName::Open);

    StartAndVerify(ApiName::Open, true);
}

BOOST_AUTO_TEST_CASE(Multi_TryStartReturnsTrue)
{
    StartMultiOperationAndVerify(ApiName::Open, true);
}

BOOST_AUTO_TEST_CASE(Multi_ClosedTryStartReturnsFalse)
{
    Close();

    StartMultiOperationAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Multi_AbortedTryStartReturnsFalse)
{
    Abort();

    StartMultiOperationAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Multi_CompatibleIsChecked)
{
    InvokeTryStart(ApiName::Open);

    StartMultiOperationAndVerify(ApiName::Open, false);
}

BOOST_AUTO_TEST_CASE(Multi_FinishReturnsCorrectValue)
{
    InvokeTryStartMultiOperation(ApiName::Open);

    auto expected = lastApiCallDescription_;

    auto actual = FinishMulti(ApiName::Open);

    Verify::IsTrue(expected == actual, L"RV of finish");
}

BOOST_AUTO_TEST_CASE(Multi_ContinueStoresTheAsyncOp)
{
    InvokeTryStartMultiOperation(ApiName::Open);

    InvokeTryContinueMultiOperation(ApiName::Open);

    Verify::IsTrue(lastAsyncOp_ == storage_.AsyncOperation, L"Operation should match");
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
