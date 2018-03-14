// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;
using namespace ImageModel;

StringLiteral const TraceCrashDumpProvider("CrashDumpProvider");

// ********************************************************************************************************************
// CrashDumpProvider::SetupCrashDumpAsyncOperation Implementation
//
class CrashDumpProvider::SetupCrashDumpAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SetupCrashDumpAsyncOperation)

public:
    SetupCrashDumpAsyncOperation(
        __in CrashDumpProvider & owner,
        ServicePackageDescription const & servicePackageDescription,
        ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        servicePackageDescription_(servicePackageDescription),
        packageEnvironmentContext_(packageEnvironmentContext),
        timeoutHelper_(timeout),
        exes_(),
        lastError_(ErrorCodeValue::Success)
    {
    }

    virtual ~SetupCrashDumpAsyncOperation()
    {
    }

    static ErrorCode SetupCrashDumpAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<SetupCrashDumpAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!owner_.ShouldConfigureCrashDumpCollection(
            packageEnvironmentContext_->ServicePackageInstanceId.ApplicationId.ToString()))
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        GetServiceExes();
        vector<wstring> exesForCrashDumpCollection(exes_.begin(), exes_.end());
        auto operation = owner_.environmentManager_.Hosting.FabricActivatorClientObj->BeginConfigureCrashDumps(
            true,
            packageEnvironmentContext_->ServicePackageInstanceId.ToString(),
            exesForCrashDumpCollection,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishConfigureCrashDumps(operation, false); }, 
            thisSPtr);
        this->FinishConfigureCrashDumps(operation, true);
    }

private:
    void FinishConfigureCrashDumps(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.environmentManager_.Hosting.FabricActivatorClientObj->EndConfigureCrashDumps(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceCrashDumpProvider,
            owner_.Root.TraceId,
            "{0}: EndConfigureCrashDumps (enable = true): error {1}",
            packageEnvironmentContext_->ServicePackageInstanceId.ToString(),
            error);
        TryComplete(operation->Parent, error);
    }

    void GetServiceExes()
    {
        for (auto codePackageIter = servicePackageDescription_.DigestedCodePackages.begin();
            codePackageIter != servicePackageDescription_.DigestedCodePackages.end();
            ++codePackageIter)
        {
            if (codePackageIter->CodePackage.HasSetupEntryPoint)
            {
                exes_.insert(Path::GetFileName(codePackageIter->CodePackage.SetupEntryPoint.Program));
            }

            if (EntryPointType::Exe == codePackageIter->CodePackage.EntryPoint.EntryPointType)
            {
                exes_.insert(Path::GetFileName(codePackageIter->CodePackage.EntryPoint.ExeEntryPoint.Program));
            }
        }
    }

private:
    CrashDumpProvider & owner_;
    ServicePackageDescription const servicePackageDescription_;
    ServicePackageInstanceEnvironmentContextSPtr const packageEnvironmentContext_;
    TimeoutHelper timeoutHelper_;
    set<std::wstring, IsLessCaseInsensitiveComparer<wstring>> exes_;
    ErrorCode lastError_;
};

// ********************************************************************************************************************
// CrashDumpProvider::CleanupCrashDumpAsyncOperation Implementation
//
class CrashDumpProvider::CleanupCrashDumpAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CleanupCrashDumpAsyncOperation)

public:
    CleanupCrashDumpAsyncOperation(
        CrashDumpProvider & owner,
        ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        packageEnvironmentContext_(packageEnvironmentContext),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CleanupCrashDumpAsyncOperation()
    {
    }

    static ErrorCode CleanupCrashDumpAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupCrashDumpAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!owner_.ShouldConfigureCrashDumpCollection(
            packageEnvironmentContext_->ServicePackageInstanceId.ApplicationId.ToString()))
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        vector<wstring> emptyExeList;
        auto operation = owner_.environmentManager_.Hosting.FabricActivatorClientObj->BeginConfigureCrashDumps(
            false,
            packageEnvironmentContext_->ServicePackageInstanceId.ToString(),
            emptyExeList,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishConfigureCrashDumps(operation, false); }, 
            thisSPtr);
        this->FinishConfigureCrashDumps(operation, true);
    }

private:
    void FinishConfigureCrashDumps(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.environmentManager_.Hosting.FabricActivatorClientObj->EndConfigureCrashDumps(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceCrashDumpProvider,
            owner_.Root.TraceId,
            "{0}: EndConfigureCrashDumps (enable = false): error {1}",
            packageEnvironmentContext_->ServicePackageInstanceId.ToString(),
            error);
        TryComplete(operation->Parent, error);
    }

private:
    CrashDumpProvider & owner_;
    ServicePackageInstanceEnvironmentContextSPtr const packageEnvironmentContext_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// CrashDumpProvider::OpenAsyncOperation Implementation
//
class CrashDumpProvider::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        CrashDumpProvider & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode OpenAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.environmentManager_.Hosting.FabricActivatorClientObj->IsIpcClientInitialized())
        {
            wstring emptyServicePackageId;
            vector<wstring> emptyExeList;
            auto operation = owner_.environmentManager_.Hosting.FabricActivatorClientObj->BeginConfigureCrashDumps(
                false,
                emptyServicePackageId,
                emptyExeList,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->FinishConfigureCrashDumps(operation, false); }, 
                thisSPtr);
            this->FinishConfigureCrashDumps(operation, true);
        }
        else
        {
            // IPC client is not initialized in some CITs
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

private:
    void FinishConfigureCrashDumps(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.environmentManager_.Hosting.FabricActivatorClientObj->EndConfigureCrashDumps(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceCrashDumpProvider,
            owner_.Root.TraceId,
            "CrashDumpProvider::OpenAsyncOperation: EndConfigureCrashDumps: error {0}",
            error);
        TryComplete(operation->Parent, error);
    }

private:
    CrashDumpProvider & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// CrashDumpProvider::CloseAsyncOperation Implementation
//
class CrashDumpProvider::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        CrashDumpProvider & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode CloseAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        {
            AcquireWriteLock writeLock(owner_.appSetCollectingCrashDumpsLock_);
            owner_.appSetCollectingCrashDumpsIsClosed_ = true;
        }

        if (owner_.environmentManager_.Hosting.FabricActivatorClientObj->IsIpcClientInitialized())
        {
            wstring emptyServicePackageId;
            vector<wstring> emptyExeList;
            auto operation = owner_.environmentManager_.Hosting.FabricActivatorClientObj->BeginConfigureCrashDumps(
                false,
                emptyServicePackageId,
                emptyExeList,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->FinishConfigureCrashDumps(operation, false); }, 
                thisSPtr);
            this->FinishConfigureCrashDumps(operation, true);
        }
        else
        {
            // IPC client is not initialized in some CITs
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
        }
    }

private:
    void FinishConfigureCrashDumps(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.environmentManager_.Hosting.FabricActivatorClientObj->EndConfigureCrashDumps(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceCrashDumpProvider,
            owner_.Root.TraceId,
            "CrashDumpProvider::CloseAsyncOperation: EndConfigureCrashDumps: error {0}",
            error);
        TryComplete(operation->Parent, error);
    }

private:
    CrashDumpProvider & owner_;
    TimeoutHelper timeoutHelper_;
};


CrashDumpProvider::CrashDumpProvider(
    ComponentRoot const & root,
    EnvironmentManager & environmentManager)
    : RootedObject(root),
    AsyncFabricComponent(),
    environmentManager_(environmentManager),
    appSetCollectingCrashDumps_(),
    appSetCollectingCrashDumpsLock_(),
    appSetCollectingCrashDumpsIsClosed_(false)
{
}

CrashDumpProvider::~CrashDumpProvider()
{
}

void CrashDumpProvider::SetupApplicationCrashDumps(
    wstring const & applicationId,
    ServiceModel::ApplicationPackageDescription const & applicationPackageDescription)
{
    wstring trueAsString = L"true";
    if (StringUtility::AreEqualCaseInsensitive(
            applicationPackageDescription.DigestedEnvironment.Diagnostics.CrashDumpSource.IsEnabled,
            trueAsString))
    {
        {
            AcquireWriteLock writeLock(appSetCollectingCrashDumpsLock_);
            if (!appSetCollectingCrashDumpsIsClosed_)
            {
                appSetCollectingCrashDumps_.insert(applicationId);
            }
        }
    }
}

void CrashDumpProvider::CleanupApplicationCrashDumps(
    wstring const & applicationId)
{
    {
        AcquireWriteLock writeLock(appSetCollectingCrashDumpsLock_);
        if (!appSetCollectingCrashDumpsIsClosed_)
        {
            appSetCollectingCrashDumps_.erase(applicationId);
        }
    }
}

bool CrashDumpProvider::ShouldConfigureCrashDumpCollection(wstring const & applicationId)
{
    {
        AcquireWriteLock writeLock(appSetCollectingCrashDumpsLock_);
        if (!appSetCollectingCrashDumpsIsClosed_)
        {
            return (appSetCollectingCrashDumps_.find(applicationId) != appSetCollectingCrashDumps_.end());
        }
        else
        {
            return false;
        }
    }
}

AsyncOperationSPtr CrashDumpProvider::BeginSetupServiceCrashDumps(
    ServiceModel::ServicePackageDescription const & servicePackageDescription,
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SetupCrashDumpAsyncOperation>(
        *this,
        servicePackageDescription,
        packageEnvironmentContext,
        timeout,
        callback,
        parent);
}

ErrorCode CrashDumpProvider::EndSetupServiceCrashDumps(
    AsyncOperationSPtr const & asyncOperation)
{
    return SetupCrashDumpAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr CrashDumpProvider::BeginCleanupServiceCrashDumps(
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CleanupCrashDumpAsyncOperation>(
        *this,
        packageEnvironmentContext,
        timeout,
        callback,
        parent);
}

ErrorCode CrashDumpProvider::EndCleanupServiceCrashDumps(
    AsyncOperationSPtr const & asyncOperation)
{
    return CleanupCrashDumpAsyncOperation::End(asyncOperation);
}

void CrashDumpProvider::CleanupServiceCrashDumps(
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext)
{
    auto waiter = make_shared<AsyncOperationWaiter>();
        
    this->BeginCleanupServiceCrashDumps(
        packageEnvironmentContext,
        HostingConfig::GetConfig().CrashDumpConfigurationTimeout,
        [this, waiter] (AsyncOperationSPtr const & operation) 
        {
            auto error = this->EndCleanupServiceCrashDumps(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();
    error.ReadValue(); // ignore error in abort path
}

// ********************************************************************************************************************
// AsyncFabricComponent methods

AsyncOperationSPtr CrashDumpProvider::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode CrashDumpProvider::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr CrashDumpProvider::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode CrashDumpProvider::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void CrashDumpProvider::OnAbort()
{
    // Exit without cleaning up. Clean up will occur on next open.
}
