// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace LeaseWrapper;
using namespace Transport;

StringLiteral const TraceEstablish("Establish");
StringLiteral const TraceTerminate("Terminate");
StringLiteral const TraceAbort("Abort");

void LeaseStates::WriteToTextWriter(Common::TextWriter & w, Enum const & e)
{
    switch (e)
    {
    case None: w << "None"; return;
    case Retrying: w << "Retrying"; return;
    case Establishing: w << "Establishing"; return;
    case Established: w << "Established"; return;
    case Failed: w << "Failed"; return;
    }

    Assert::CodingError("invalid lease state");
}

#pragma region AsyncOperation
class LeasePartner::EstablishLeaseAsyncOperation : public Common::TimedAsyncOperation
{
public:
    EstablishLeaseAsyncOperation(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & context,
        LeaseStates::Enum state
        )
        : TimedAsyncOperation(timeout, callback, context),
        state_(state)
    {
    }

protected:
    virtual void OnStart(Common::AsyncOperationSPtr const & asyncOperation)
    {
        if (state_ == LeaseStates::Established)
        {
            TryComplete(asyncOperation);
        }
        else
        {
            TimedAsyncOperation::OnStart(asyncOperation);
        }
    }

private:
    LeaseStates::Enum state_;
};
#pragma endregion

HANDLE LeasePartner::EstablishLease(LEASE_DURATION_TYPE leaseTimeoutType, PBOOL IsEstablished)
{
    durationType_ = leaseTimeoutType;

    if ( agent_.IsDummyLeaseDriverEnabled() )
    {
        return ::Dummy_EstablishLease(
            appHandle_,
            remoteId_.c_str(),
            remoteLeaseAgentAddress_.c_str(),
            remoteLeaseAgentInstanceId_);
    }

    TRANSPORT_LISTEN_ENDPOINT endPoint;
    if (!this->agent_.InitializeListenEndpoint(this->remoteLeaseAgentAddress_, endPoint))
    {
        return NULL;
    }

    return ::EstablishLease(
        appHandle_,
        remoteId_.c_str(),
        &endPoint,
        remoteLeaseAgentInstanceId_,
        leaseTimeoutType,
        IsEstablished);
}

BOOL LeasePartner::TerminateLease()
{
    if ( !agent_.IsDummyLeaseDriverEnabled() )
    {
        return ::TerminateLease(appHandle_, leaseHandle_, remoteId_.c_str());
    }
    else
    {
        return ::Dummy_TerminateLease(leaseHandle_);
    }
}

LeasePartner::LeasePartner(
    LeaseAgent & agent,
    HANDLE appHandle,
    std::wstring const & localId,
    std::wstring const & remoteId,
    std::wstring const & remoteFaultDomain,
    std::wstring const & remoteLeaseAgentAddress,
    int64 remoteLeaseAgentInstanceId
    ):
agent_(agent),
    appHandle_(appHandle),
    localId_(localId),
    remoteId_(remoteId),
    remoteFaultDomain_(remoteFaultDomain),
    remoteLeaseAgentAddress_(remoteLeaseAgentAddress),
    remoteLeaseAgentInstanceId_(remoteLeaseAgentInstanceId),
    durationType_(LEASE_DURATION),
    state_(LeaseStates::None),
    leaseHandle_(NULL),
    leaseOperations_(),
    timer_()
{
    ASSERT_IF(remoteLeaseAgentInstanceId == 0,
        "LeaseAgent instance is 0 for {0}", *this);
}

AsyncOperationSPtr LeasePartner::Establish(
    LEASE_DURATION_TYPE leaseTimeoutType,
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent
    )
{
    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<EstablishLeaseAsyncOperation>(timeout, callback, parent, state_);
    if (state_ != LeaseStates::Established)
    {
        leaseOperations_.push_back(operation);

        if (state_ == LeaseStates::None)
        {
            TryEstablish(leaseTimeoutType);
        }
    }
    else
    {
        operation->TryComplete(operation, ErrorCodeValue::Success);
    }

    return operation;
}

void LeasePartner::Terminate()
{
    CompleteAllOperations(false);

    if (timer_)
    {
        timer_->Cancel();
    }

    if (state_ == LeaseStates::Establishing || state_ == LeaseStates::Established)
    {
        LeaseAgent::WriteInfo(TraceTerminate,
            "{0} lease terminating with handle {1}",
            *this, leaseHandle_);

        if (!TerminateLease())
        {
            DWORD errorCode = GetLastError();
            ASSERT_IF(errorCode != ERROR_INVALID_HANDLE && errorCode != ERROR_INVALID_PARAMETER,
                "{0} invalid error code {1} from TerminateLease",
                *this, errorCode);

            LeaseAgent::WriteInfo(TraceTerminate,
                "{0} got error code {1} from TerminateLease",
                *this, errorCode);
        }
    }

    leaseHandle_ = NULL;
    state_ = LeaseStates::None;
}

void LeasePartner::OnEstablished(HANDLE leaseHandle)
{
    if (leaseHandle != leaseHandle_)
    {
        return;
    }

    if (LeaseStates::Established == state_)
    {
        LeaseAgent::WriteInfo(TraceEstablish,
            "{0} lease have already been established with handle {1}",
            *this, leaseHandle_);

        return;
    }

    state_ = LeaseStates::Established;

    CompleteAllOperations(true);

    LeaseAgent::WriteInfo(TraceEstablish,
        "{0} lease established with handle {1}",
        *this, leaseHandle_);
}

void LeasePartner::OnArbitration()
{
    state_ = LeaseStates::Failed;
}

void LeasePartner::Abort()
{
    CompleteAllOperations(false);    
    state_ = LeaseStates::None;
    leaseHandle_ = NULL;
    appHandle_ = NULL;
    if (timer_)
    {
        timer_->Cancel();
    }

    LeaseAgent::WriteInfo(TraceAbort, "Lease {0} aborted", *this);
}

void LeasePartner::CompleteAllOperations(bool isSuccessful)
{
    while (leaseOperations_.size() != 0)
    {
        auto it = leaseOperations_.begin();
        AsyncOperationSPtr operation = *it;
        leaseOperations_.erase(it);

        Threadpool::Post([operation, isSuccessful] () { operation->TryComplete(operation, 
            isSuccessful ? ErrorCodeValue::Success : ErrorCodeValue::OperationCanceled); });
    }
}

void LeasePartner::Retry(LEASE_DURATION_TYPE leaseTimeoutType)
{
    if (state_ == LeaseStates::Retrying)
    {
        TryEstablish(leaseTimeoutType);
    }
}

void LeasePartner::TryEstablish(LEASE_DURATION_TYPE leaseTimeoutType)
{
    ASSERT_IF(appHandle_ == NULL, "{0} Lease agent is null", *this);
    ASSERT_IF(state_ == LeaseStates::Establishing || state_ == LeaseStates::Established,
        "{0} invalid state before TryEstablish", *this);

    int IsEstablished = 0;

    leaseHandle_ = EstablishLease(leaseTimeoutType, &IsEstablished);

    if (leaseHandle_ == NULL)
    {
        DWORD errorCode = GetLastError();
        LeaseAgent::WriteWarning(TraceEstablish,
            "{0} got error code {1} from EstablishLease",
            *this, errorCode); 

        state_ = LeaseStates::Retrying;

        if (!timer_)
        {
            ComponentRootSPtr pRoot = agent_.GetOwner();
            LeasePartnerSPtr thisPtr = shared_from_this();
            timer_ = Timer::Create("Lease.Establish.Retry", [thisPtr, pRoot, leaseTimeoutType] ( TimerSPtr const & )
            {
                thisPtr->agent_.RetryEstablish(thisPtr, leaseTimeoutType);
            } );
        }

        //TODO change the timeout from magic number to FederationConfig constant.
        timer_->Change(TimeSpan::FromSeconds(1));
    }
    else
    {
        if (IsEstablished)
        {
            LeaseAgent::WriteInfo(TraceEstablish,
                "{0} is already established lease with handle {1}",
                *this, leaseHandle_);

            state_ = LeaseStates::Established;

            CompleteAllOperations(true);
        }
        else
        {
            LeaseAgent::WriteInfo(TraceEstablish,
                "{0} establishing lease with handle {1}",
                *this, leaseHandle_);

            state_ = LeaseStates::Establishing;
        }
    }
}

void LeasePartner::EstablishAfterArbitration(int64 remoteLeaseAgentInstance)
{
    if (remoteLeaseAgentInstanceId_ != remoteLeaseAgentInstance ||
        state_ != LeaseStates::Failed)
    {
        LeaseAgent::WriteWarning(TraceEstablish, "{0} skipped re-establishing lease with {1} after arbitration", remoteLeaseAgentInstance, *this);
    }
    else
    {
        LeaseAgent::WriteInfo(TraceEstablish, "{0} re-establish lease with {1} after arbitration", remoteLeaseAgentInstance, *this);
        TryEstablish(durationType_);
    }
}

void LeasePartner::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("{0}/{1}-{2}@{3}:{4} {5}",
        appHandle_, agent_,
        remoteId_, remoteLeaseAgentAddress_, remoteLeaseAgentInstanceId_,
        state_);
}
