// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace ApiMonitoring;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

FailoverUnitProxyOperationManagerBase::FailoverUnitProxyOperationManagerBase(
    FailoverUnitProxy & owner,
    Common::ApiMonitoring::InterfaceName::Enum interfaceName,
    CompatibleOperationTable const & compatibilityTable) :
    owner_(owner),
    opMgr_(compatibilityTable),
    interface_(interfaceName)
{
}

bool FailoverUnitProxyOperationManagerBase::CheckIfOperationCanBeStarted(ApiName::Enum operation) const
{
    return opMgr_.CheckIfOperationCanBeStarted(operation);
}

ApiCallDescriptionSPtr FailoverUnitProxyOperationManagerBase::TryStartOperation(
    ApiName::Enum operation)
{
    return TryStartOperation(operation, L"");
}

ApiCallDescriptionSPtr FailoverUnitProxyOperationManagerBase::TryStartOperation(
    ApiName::Enum operation,
    wstring const & metadata)
{
    auto apiCallDescription = CreateApiCallDescription(operation, metadata, owner_.ReplicaDescription);

    bool rv = opMgr_.TryStartOperation(apiCallDescription);
    if (rv)
    {
        owner_.StartOperationMonitoring(apiCallDescription);
        return apiCallDescription;
    }
    else
    {
        return ApiCallDescriptionSPtr();
    }
}

bool FailoverUnitProxyOperationManagerBase::TryContinueOperation(ApiName::Enum operation, AsyncOperationSPtr const & asyncOperation)
{
    return opMgr_.TryContinueOperation(operation, asyncOperation);
}

void FailoverUnitProxyOperationManagerBase::FinishOperation(ApiName::Enum operation, ErrorCode const & error)
{
    auto callDescription = opMgr_.FinishOperation(operation, error);
    owner_.StopOperationMonitoring(callDescription, error);
}

void FailoverUnitProxyOperationManagerBase::CancelOperations()
{
    opMgr_.CancelOperations();
}

bool FailoverUnitProxyOperationManagerBase::IsOperationRunning(ApiName::Enum operation) const
{
    return opMgr_.IsOperationRunning(operation);
}

bool FailoverUnitProxyOperationManagerBase::TryStartMultiInstanceOperation(
    Common::ApiMonitoring::ApiName::Enum apiName,
    std::wstring const & metadata,
    Reliability::ReplicaDescription const & replicaDescription,
    __out ExecutingOperation & storage)
{
    auto apiCallDescription = CreateApiCallDescription(apiName, metadata, replicaDescription);

    bool rv = opMgr_.TryStartMultiInstanceOperation(apiCallDescription, storage);
    if (rv)
    {
        owner_.StartOperationMonitoring(apiCallDescription);
    }

    return rv;
}

bool FailoverUnitProxyOperationManagerBase::TryContinueMultiInstanceOperation(
    Common::ApiMonitoring::ApiName::Enum operation,
    ExecutingOperation & storage,
    Common::AsyncOperationSPtr const & asyncOperation) const
{
    return opMgr_.TryContinueMultiInstanceOperation(operation, storage, asyncOperation);
}

void FailoverUnitProxyOperationManagerBase::FinishMultiInstanceOperation(
    Common::ApiMonitoring::ApiName::Enum operation,
    ExecutingOperation & storage,
    Common::ErrorCode const & error)
{
    auto description = opMgr_.FinishMultiInstanceOperation(operation, storage, error);

    owner_.StopOperationMonitoring(description, error);
}

void FailoverUnitProxyOperationManagerBase::OpenForBusiness()
{
    opMgr_.OpenForBusiness();
}

void FailoverUnitProxyOperationManagerBase::CloseForBusiness(bool isAbort)
{
    opMgr_.CloseForBusiness(isAbort);
}

ApiCallDescriptionSPtr FailoverUnitProxyOperationManagerBase::CreateApiCallDescription(
    ApiName::Enum operation,
    std::wstring const & metadata,
    Reliability::ReplicaDescription const & replicaDesc) const
{
    bool isSlowHealthReportRequired = true;
    bool traceServiceType = true;

    GetApiCallDescriptionProperties(operation, isSlowHealthReportRequired, traceServiceType);

    ApiNameDescription nameDescription(interface_, operation, move(metadata));
    return owner_.CreateApiCallDescription(
        move(nameDescription),
        replicaDesc,
        isSlowHealthReportRequired,
        traceServiceType);
}

void FailoverUnitProxyOperationManagerBase::WriteTo(TextWriter & writer, FormatOptions const & options) const
{
    opMgr_.WriteTo(writer, options);
}

string FailoverUnitProxyOperationManagerBase::ToString() const
{
    string result;
    StringWriterA(result).Write(*this);
    return result;
}
