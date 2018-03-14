// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Common::ApiMonitoring;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

OperationManager::OperationManager(
    CompatibleOperationTable const & compatibilityTable) :
    compatibilityTable_(compatibilityTable),
    state_(State::Opened)
{
}

void OperationManager::OpenForBusiness()
{
    AcquireWriteLock grab(lock_);

    state_ = State::Opened;
}

void OperationManager::CloseForBusiness(bool isAbort)
{
    AcquireWriteLock grab(lock_);

    state_ = isAbort ? State::Aborted : State::Closed;
}

bool OperationManager::IsCompatibleOperation(ApiName::Enum operationToRun) const
{
    // Caller holds the lock
    auto outstandingOperations = outstandingOperations_.GetRunningOperations();

    bool retVal = true;

    for (auto iter = outstandingOperations.begin(); iter != outstandingOperations.end(); ++iter)
    {
        retVal = retVal && compatibilityTable_.Lookup(*iter, operationToRun);
    }

    return retVal;
}

bool OperationManager::IsAllowedOperation(ApiName::Enum operationToRun) const
{
    // Callers hold the lock

    switch (state_)
    {
    case State::Opened:
    {
        return disallowedOperations_.find(operationToRun) == disallowedOperations_.cend();
    }
    case State::Aborted:
        return false;
    case State::Closed:
        return operationToRun == ApiName::Close;
    default:
        Assert::CodingError("ReplicatorOperationManager:IsAllowedOperation Unknown state");
    }
}

bool OperationManager::CheckIfOperationCanBeStarted(ApiName::Enum operation) const
{
    AcquireReadLock grab(lock_);
    return CheckIfOperationCanBeStartedCallerHoldsLock(operation);
}

bool OperationManager::CheckIfOperationCanBeStartedCallerHoldsLock(ApiMonitoring::ApiName::Enum operation) const
{
    return IsAllowedOperation(operation) && IsCompatibleOperation(operation);
}

bool OperationManager::TryStartOperation(ApiMonitoring::ApiCallDescriptionSPtr const & operation)
{
    AcquireWriteLock grab(lock_);

    auto name = operation->Data.Api.Api;

    if (!CheckIfOperationCanBeStartedCallerHoldsLock(name))
    {
        return false;
    }

    outstandingOperations_.StartOperation(name, operation);
    return true;
}

bool OperationManager::TryContinueOperation(ApiName::Enum operation, AsyncOperationSPtr const & asyncOperation)
{
    AcquireWriteLock grab(lock_);

    if (!IsAllowedOperation(operation))
    {
        return false;
    }

    outstandingOperations_.ContinueOperation(operation, asyncOperation);

    return true;
}

ApiCallDescriptionSPtr OperationManager::FinishOperation(ApiName::Enum operation, Common::ErrorCode const & error)
{
    UNREFERENCED_PARAMETER(error);

    AcquireWriteLock grab(lock_);

    return outstandingOperations_.FinishOperation(operation);    
}

void OperationManager::CancelOperations()
{
    vector<AsyncOperationSPtr> outstandingAsyncOperations;

    {
        AcquireReadLock grab(lock_);

        if (state_ == State::Opened)
        {
            // Cancel might have been completed, just return
            return;
        }

        RAPEventSource::Events->OperationManagerCancelOperations(formatString(outstandingOperations_));

        outstandingAsyncOperations = outstandingOperations_.GetRunningAsyncOperations([this](ApiName::Enum operation)
        {
            return !IsAllowedOperation(operation);
        });
    }

    for (auto & operation : outstandingAsyncOperations)
    {
        operation->Cancel();
    }
}

bool OperationManager::IsOperationRunning(ApiMonitoring::ApiName::Enum operation) const
{
    AcquireReadLock grab(lock_);

    return outstandingOperations_.GetOperation(operation) != nullptr;
}

bool OperationManager::TryStartMultiInstanceOperation(
    ApiMonitoring::ApiCallDescriptionSPtr const & operation,
    ExecutingOperation & storage)
{
    AcquireWriteLock grab(lock_);

    auto name = operation->Data.Api.Api;
    if (!IsAllowedOperation(name) || !IsCompatibleOperation(name))
    {
        return false;
    }

    ExecutingOperation executingOp;
    executingOp.StartOperation(operation);
    storage = move(executingOp);

    outstandingOperations_.AddInstance(name);

    return true;
}

bool OperationManager::TryContinueMultiInstanceOperation(
    ApiMonitoring::ApiName::Enum operationName,
    ExecutingOperation & storage,
    Common::AsyncOperationSPtr const & asyncOperation) const
{
    AcquireWriteLock grab(lock_);

    if (!IsAllowedOperation(operationName))
    {
        return false;
    }

    ASSERT_IF(storage.IsRunning, "Storage not empty");
    ASSERT_IFNOT(asyncOperation, "Invalid async operations.");

    storage.ContinueOperation(asyncOperation);

    return true;
}

ApiCallDescriptionSPtr OperationManager::FinishMultiInstanceOperation(
    ApiMonitoring::ApiName::Enum operationName,
    ExecutingOperation & storage,
    Common::ErrorCode const & error)
{
    UNREFERENCED_PARAMETER(error);

    AcquireWriteLock grab(lock_);

    auto description = storage.FinishOperation();

    outstandingOperations_.RemoveInstance(operationName);

    return description;
}

void OperationManager::CancelOrMarkOperationForCancel(
    ApiMonitoring::ApiName::Enum operation)
{
    AsyncOperationSPtr operationToCancel;

    {
        AcquireReadLock grab(lock_);

        if (state_ != State::Opened)
        {
            return;
        }

        auto value = outstandingOperations_.GetOperation(operation);
        if (value != nullptr)
        {
            operationToCancel = value->AsyncOperation;
        }

        disallowedOperations_.insert(operation);
    }

    if (operationToCancel)
    {
        operationToCancel->Cancel();
    }
}

void OperationManager::RemoveOperationForCancel(
    ApiMonitoring::ApiName::Enum operation)
{
    AcquireWriteLock grab(lock_);
    disallowedOperations_.erase(operation);
}

void OperationManager::WriteTo(TextWriter & writer, FormatOptions const & options) const
{
    AcquireReadLock grab(lock_);
    outstandingOperations_.WriteTo(writer, options);
}

void OperationManager::GetDetailsForQuery(
    __out std::vector<ApiMonitoring::ApiName::Enum> & operations,
    __out Common::DateTime & startTime) const
{
    startTime = DateTime::Zero;

    {
        AcquireReadLock grab(lock_);

        if (state_ != State::Opened)
        {
            return;
        }

        operations = outstandingOperations_.GetRunningOperations();
        startTime = outstandingOperations_.LastOperationStartTime;
    }
}
