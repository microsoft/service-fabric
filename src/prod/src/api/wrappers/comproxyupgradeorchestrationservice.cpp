// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::UpgradeOrchestrationService;

StringLiteral const TraceComponent("ComFabricClient");

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
// ********************************************************************************************************************

//StartClusterConfigurationUpgradeOperation
class ComProxyUpgradeOrchestrationService::StartClusterConfigurationUpgradeOperation : public ComProxyAsyncOperation
{
    DENY_COPY(StartClusterConfigurationUpgradeOperation)

public:
    StartClusterConfigurationUpgradeOperation(
        __in IFabricUpgradeOrchestrationService & comImpl,
        __in FABRIC_START_UPGRADE_DESCRIPTION * startUpgradeDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , startUpgradeDescription_(startUpgradeDescription)
        , timeout_(timeout)
    {
    }

    virtual ~StartClusterConfigurationUpgradeOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<StartClusterConfigurationUpgradeOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        StartUpgradeDescription internalDescription;
        auto error = internalDescription.FromPublicApi(*startUpgradeDescription_);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "ProxyService/DataLoss/Could not convert back");
            return error.ToHResult();
        }

        return comImpl_.BeginUpgradeConfiguration(
            startUpgradeDescription_,
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndUpgradeConfiguration(context);
    }

    IFabricUpgradeOrchestrationService & comImpl_;
    FABRIC_START_UPGRADE_DESCRIPTION * startUpgradeDescription_;
    TimeSpan const timeout_;
};

//GetClusterConfigurationUpgradeStatusOperation
class ComProxyUpgradeOrchestrationService::GetClusterConfigurationUpgradeStatusOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetClusterConfigurationUpgradeStatusOperation)

public:
    GetClusterConfigurationUpgradeStatusOperation(
        __in IFabricUpgradeOrchestrationService & comImpl,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
    {
    }

    virtual ~GetClusterConfigurationUpgradeStatusOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricOrchestrationUpgradeStatusResult ** reply)
    {
        auto casted = AsyncOperation::End<GetClusterConfigurationUpgradeStatusOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetClusterConfigurationUpgradeStatus(
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetClusterConfigurationUpgradeStatus(context, (IFabricOrchestrationUpgradeStatusResult**)&result);
    }

    IFabricUpgradeOrchestrationService & comImpl_;
    IFabricOrchestrationUpgradeStatusResult * result;
    TimeSpan const timeout_;
};

//GetClusterConfigurationOperation
class ComProxyUpgradeOrchestrationService::GetClusterConfigurationOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetClusterConfigurationOperation)

public:
    GetClusterConfigurationOperation(
        __in IFabricUpgradeOrchestrationService & comImpl,
        wstring const & apiVersion,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
        , apiVersion_(apiVersion)
    {
    }

    virtual ~GetClusterConfigurationOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricStringResult ** reply)
    {
        auto casted = AsyncOperation::End<GetClusterConfigurationOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result);
        }

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetClusterConfiguration(
            apiVersion_.c_str(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetClusterConfiguration(context, (IFabricStringResult**)&result);
    }

    IFabricUpgradeOrchestrationService & comImpl_;
    IFabricStringResult * result;
    TimeSpan const timeout_;
    wstring const apiVersion_;
};

//GetUpgradesPendingApprovalOperation
class ComProxyUpgradeOrchestrationService::GetUpgradesPendingApprovalOperation : public ComProxyAsyncOperation
{
    DENY_COPY(GetUpgradesPendingApprovalOperation)

public:
    GetUpgradesPendingApprovalOperation(
        __in IFabricUpgradeOrchestrationService & comImpl,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
    {
    }

    virtual ~GetUpgradesPendingApprovalOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<GetUpgradesPendingApprovalOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetUpgradesPendingApproval(
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetUpgradesPendingApproval(context);
    }

    IFabricUpgradeOrchestrationService & comImpl_;
    TimeSpan const timeout_;
};

//StartApprovedUpgradesOperation
class ComProxyUpgradeOrchestrationService::StartApprovedUpgradesOperation : public ComProxyAsyncOperation
{
    DENY_COPY(StartApprovedUpgradesOperation)

public:
    StartApprovedUpgradesOperation(
        __in IFabricUpgradeOrchestrationService & comImpl,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
    {
    }

    virtual ~StartApprovedUpgradesOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<StartApprovedUpgradesOperation>(operation);

        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginStartApprovedUpgrades(
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndStartApprovedUpgrades(context);
    }

    IFabricUpgradeOrchestrationService & comImpl_;
    TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxyUpgradeOrchestrationService Implementation
//
ComProxyUpgradeOrchestrationService::ComProxyUpgradeOrchestrationService(
    ComPointer<IFabricUpgradeOrchestrationService> const & comImpl)
    : IUpgradeOrchestrationService()
    , ComProxySystemServiceBase<IFabricUpgradeOrchestrationService>(comImpl)
{
}

ComProxyUpgradeOrchestrationService::~ComProxyUpgradeOrchestrationService()
{
}

//StartClusterConfigurationUpgradeOperation
AsyncOperationSPtr ComProxyUpgradeOrchestrationService::BeginUpgradeConfiguration(
    FABRIC_START_UPGRADE_DESCRIPTION * startUpgradeDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<StartClusterConfigurationUpgradeOperation>(
        *comImpl_.GetRawPointer(),
        startUpgradeDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyUpgradeOrchestrationService::EndUpgradeConfiguration(
    AsyncOperationSPtr const & asyncOperation)
{
    return StartClusterConfigurationUpgradeOperation::End(asyncOperation);
}

//GetClusterConfigurationUpgradeStatusOperation
AsyncOperationSPtr ComProxyUpgradeOrchestrationService::BeginGetClusterConfigurationUpgradeStatus(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetClusterConfigurationUpgradeStatusOperation>(
        *comImpl_.GetRawPointer(),
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyUpgradeOrchestrationService::EndGetClusterConfigurationUpgradeStatus(
    AsyncOperationSPtr const & asyncOperation,
    IFabricOrchestrationUpgradeStatusResult ** reply)
{
    return GetClusterConfigurationUpgradeStatusOperation::End(asyncOperation, reply);
}

//GetClusterConfiguration
AsyncOperationSPtr ComProxyUpgradeOrchestrationService::BeginGetClusterConfiguration(
    wstring const & apiVersion,
    TimeSpan const timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetClusterConfigurationOperation>(
        *comImpl_.GetRawPointer(),
        apiVersion,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyUpgradeOrchestrationService::EndGetClusterConfiguration(
    AsyncOperationSPtr const &operation,
    IFabricStringResult** result)
{
    return GetClusterConfigurationOperation::End(operation, result);
}

//GetUpgradesPendingApprovalOperation
AsyncOperationSPtr ComProxyUpgradeOrchestrationService::BeginGetUpgradesPendingApproval(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<GetUpgradesPendingApprovalOperation>(
        *comImpl_.GetRawPointer(),
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyUpgradeOrchestrationService::EndGetUpgradesPendingApproval(
    AsyncOperationSPtr const & asyncOperation)
{
    return GetUpgradesPendingApprovalOperation::End(asyncOperation);
}

//StartApprovedUpgradesOperation
AsyncOperationSPtr ComProxyUpgradeOrchestrationService::BeginStartApprovedUpgrades(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<StartApprovedUpgradesOperation>(
        *comImpl_.GetRawPointer(),
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyUpgradeOrchestrationService::EndStartApprovedUpgrades(
    AsyncOperationSPtr const & asyncOperation)
{
    return StartApprovedUpgradesOperation::End(asyncOperation);
}

// SystemServiceCall
AsyncOperationSPtr ComProxyUpgradeOrchestrationService::BeginCallSystemService(
    std::wstring const & action,
    std::wstring const & inputBlob,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
	return BeginCallSystemServiceInternal(
		action,
		inputBlob,
		timeout,
		callback,
		parent);
}

ErrorCode ComProxyUpgradeOrchestrationService::EndCallSystemService(
    Common::AsyncOperationSPtr const & asyncOperation,
    __inout std::wstring & outputBlob)
{
	return EndCallSystemServiceInternal(
		asyncOperation,
		outputBlob);
}

