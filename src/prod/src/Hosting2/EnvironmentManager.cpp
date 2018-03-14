// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Management;
using namespace ImageModel;

StringLiteral const TraceEnvironmentManager("EnvironmentManager");

// ********************************************************************************************************************
// EnvironmentManager::OpenAsyncOperation Implementation
//
class EnvironmentManager::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in EnvironmentManager & owner,
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
        // Create and open the providers

        // 1. Log Collection Provider
        auto error = LogCollectionProviderFactory::CreateLogCollectionProvider(
            owner_.Root,
            owner_.hosting_.NodeId,
            owner_.hosting_.DeploymentFolder,
            owner_.logCollectionProvider_);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "Create logCollectionProvider: error {0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // Open log collection provider
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(LogCollectionProvider->Open): timeout {1}",
            owner_.hosting_.NodeId,
            timeoutHelper_.GetRemainingTime());
        auto operation = owner_.logCollectionProvider_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnLogCollectionProviderOpenCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishLogCollectionProviderOpen(operation);
        }
    }

    void OnLogCollectionProviderOpenCompleted(
        AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishLogCollectionProviderOpen(operation);
        }
    }

    void FinishLogCollectionProviderOpen(
        AsyncOperationSPtr const & operation)
    {
        auto error = owner_.logCollectionProvider_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(logCollectionProvider->Open): error {1}",
            owner_.hosting_.NodeId,
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        if (!HostingConfig::GetConfig().EndpointProviderEnabled)
        {
            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint filtering disabled, do not create and open endpoint provider");
        }
        else
        {
            // 3. Endpoint Provider
            owner_.endpointProvider_ = make_unique<EndpointProvider>(
                owner_.Root,
                owner_.hosting_.NodeId,
                owner_.hosting_.StartApplicationPortRange,
                owner_.hosting_.EndApplicationPortRange);
            error = owner_.endpointProvider_->Open();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    owner_.Root.TraceId,
                    "Open endpointProvider failed: error {0}",
                    error);
                TryComplete(operation->Parent, error);
                return;
            }
        }

        // 4. ETW session provider
#if !defined(PLATFORM_UNIX)
        owner_.etwSessionProvider_ = make_unique<EtwSessionProvider>(
            owner_.Root,
            owner_.hosting_.NodeName,
            owner_.hosting_.DeploymentFolder);
        error = owner_.etwSessionProvider_->Open();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Open EtwSessionProvider failed: error {0}",
                error);
            TryComplete(operation->Parent, error);
            return;
        }
#endif
        // 5. Crash dump provider
        owner_.crashDumpProvider_ = make_unique<CrashDumpProvider>(
            owner_.Root,
            owner_);
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(CrashDumpProvider->Open): timeout {1}",
            owner_.hosting_.NodeId,
            timeoutHelper_.GetRemainingTime());
        auto nextOperation = owner_.crashDumpProvider_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & nextOperation) { this->OnCrashDumpProviderOpenCompleted(nextOperation); },
            operation->Parent);
        if (nextOperation->CompletedSynchronously)
        {
            FinishCrashDumpProviderOpen(nextOperation);
        }
    }

    void OnCrashDumpProviderOpenCompleted(
        AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCrashDumpProviderOpen(operation);
        }
    }

    void FinishCrashDumpProviderOpen(
        AsyncOperationSPtr const & operation)
    {
        auto error = owner_.crashDumpProvider_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(CrashDumpProvider->Open): error {1}",
            owner_.hosting_.NodeId,
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        GetCurrentUserSid(operation->Parent);
    }

    void GetCurrentUserSid(AsyncOperationSPtr thisSPtr)
    {

        SidUPtr currentUserSid;
        auto error = BufferedSid::GetCurrentUserSid(currentUserSid);
        if(!error.IsSuccess())
        {
            WriteWarning(TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Failed to get current user sid. Error={0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        owner_.isSystem_ = currentUserSid->IsLocalSystem();

        error = currentUserSid->ToString(owner_.currentUserSid_);
        WriteTrace(error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "Failed to get current user sid string. Error={0}",
            error);
        if (error.IsSuccess())
        {
            error = SecurityUtility::IsCurrentUserAdmin(owner_.isAdminUser_);
            WriteTrace(error.ToLogLevel(),
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Failed to check if current user is Admin. Error={0}",
                error);
        }
        TryComplete(thisSPtr, error);
    }

private:
    EnvironmentManager & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// EnvironmentManager::CloseAsyncOperation Implementation
//
class EnvironmentManager::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in EnvironmentManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        lastError_(ErrorCodeValue::Success)
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
        // Close the providers; on errors, save the error and
        // continue closing the other providers
        // 1. Endpoint Provider
        if (owner_.endpointProvider_)
        {
            ASSERT_IFNOT(
                HostingConfig::GetConfig().EndpointProviderEnabled,
                "Endpoint Filtering disabled, endpoint provider shouldn't exist");

            auto error = owner_.endpointProvider_->Close();
            WriteTrace(
                error.ToLogLevel(),
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Close endpointProvider: error {0}",
                error);

            if (!error.IsSuccess())
            {
                lastError_.Overwrite(error);
            }
        }

        // 3. ETW session provider
        if (owner_.etwSessionProvider_)
        {
            auto error = owner_.etwSessionProvider_->Close();
            WriteTrace(
                error.ToLogLevel(),
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Close EtwSessionProvider: error {0}",
                error);

            if (!error.IsSuccess())
            {
                lastError_.Overwrite(error);
            }
        }

        // 4. Crash dump provider
        if (owner_.crashDumpProvider_)
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Begin(CrashDumpProvider->Close): timeout {1}",
                owner_.hosting_.NodeId,
                timeoutHelper_.GetRemainingTime());
            auto operation = owner_.crashDumpProvider_->BeginClose(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCrashDumpProviderCloseCompleted(operation); },
                thisSPtr);
            if (operation->CompletedSynchronously)
            {
                FinishCrashDumpProviderClose(operation);
            }
        }
    }

    void OnCrashDumpProviderCloseCompleted(
        AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishCrashDumpProviderClose(operation);
        }
    }

    void FinishCrashDumpProviderClose(
        AsyncOperationSPtr const & operation)
    {
        auto error = owner_.crashDumpProvider_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(CrashDumpProvider->Close): error {1}",
            owner_.hosting_.NodeId,
            error);
        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        // 5. Log Collection Provider
        if (owner_.logCollectionProvider_)
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Begin(LogCollectionProvider->Close): timeout {1}",
                owner_.hosting_.NodeId,
                timeoutHelper_.GetRemainingTime());
            auto nextOperation = owner_.logCollectionProvider_->BeginClose(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & nextOperation) { this->OnLogCollectionProviderCloseCompleted(nextOperation); },
                operation->Parent);
            if (nextOperation->CompletedSynchronously)
            {
                FinishLogCollectionProviderClose(nextOperation);
            }
        }
    }

    void OnLogCollectionProviderCloseCompleted(
        AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishLogCollectionProviderClose(operation);
        }
    }

    void FinishLogCollectionProviderClose(
        AsyncOperationSPtr const & operation)
    {
        auto error = owner_.logCollectionProvider_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(logCollectionProvider->Close): error {1}",
            owner_.hosting_.NodeId,
            error);
        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        TryComplete(operation->Parent, lastError_);
    }

private:
    EnvironmentManager & owner_;
    TimeoutHelper timeoutHelper_;
    ErrorCode lastError_;
};

// ********************************************************************************************************************
// EnvironmentManager::SetupApplicationAsyncOperation Implementation
//
class EnvironmentManager::SetupApplicationAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SetupApplicationAsyncOperation)

public:
    SetupApplicationAsyncOperation(
        __in EnvironmentManager & owner,
        ApplicationIdentifier const & applicationId,
        ApplicationPackageDescription const & appPackageDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        applicationId_(applicationId),
        appPackageDescription_(appPackageDescription),
        timeoutHelper_(timeout),
        appEnvironmentContext_(),
        lastError_(ErrorCodeValue::Success)
    {
        appEnvironmentContext_ = make_shared<ApplicationEnvironmentContext>(applicationId_);
    }

    virtual ~SetupApplicationAsyncOperation()
    {
    }

    static ErrorCode SetupApplicationAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out ApplicationEnvironmentContextSPtr & appEnvironmentContext)
    {
        auto thisPtr = AsyncOperation::End<SetupApplicationAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            appEnvironmentContext = move(thisPtr->appEnvironmentContext_);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SetupEnvironment(thisSPtr);
    }

private:
    void SetupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.crashDumpProvider_->SetupApplicationCrashDumps(
            applicationId_.ToString(),
            appPackageDescription_);

        if ((!appPackageDescription_.DigestedEnvironment.Principals.Users.empty()) ||
            (!appPackageDescription_.DigestedEnvironment.Principals.Groups.empty()))
        {
            SetupPrincipals(thisSPtr);
        }
        else
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: RunAsPolicyEnabled disabled, do not set up principals",
                applicationId_);
            WritePrincipalSIDsToFile(thisSPtr);
            SetupLogCollectionPolicy(appPackageDescription_.DigestedEnvironment.Policies.LogCollectionEntries, thisSPtr);
        }
    }

    void SetupPrincipals(AsyncOperationSPtr const & thisSPtr)
    {
        // Set up the security users and the groups
        if (!HostingConfig::GetConfig().RunAsPolicyEnabled)
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: RunAsPolicyEnabled disabled, but users and groups specified",
                applicationId_);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed, StringResource::Get(IDS_HOSTING_RunAsPolicy_NotEnabled)));
            return;
        }
        else
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Begin setup principals",
                applicationId_);
            auto operation = owner_.hosting_.FabricActivatorClientObj->BeginConfigureSecurityPrincipals(
                applicationId_.ToString(),
                applicationId_.ApplicationNumber,
                appPackageDescription_.DigestedEnvironment.Principals,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnConfigureSecurityPrincipalsCompleted(operation, false); },
                thisSPtr);
            OnConfigureSecurityPrincipalsCompleted(operation, true);

        }
    }

    void OnConfigureSecurityPrincipalsCompleted(AsyncOperationSPtr operation,  bool expectedCompletedSynhronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }
        PrincipalsProviderContextUPtr principalsContext;
        auto error = owner_.hosting_.FabricActivatorClientObj->EndConfigureSecurityPrincipals(operation, principalsContext);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End ConfigureSecurityPrincipals: error {1}",
            applicationId_,
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent,
                ErrorCode(error.ReadValue(), wformatString("{0}{1}", StringResource::Get(IDS_HOSTING_ApplicationPrincipals_Setup_Failed), error.ErrorCodeValueToString())));
            return;
        }
        appEnvironmentContext_->SetPrincipalsProviderContext(move(principalsContext));
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End setup principals: error {1}",
            applicationId_,
            error);

        // Set up default runAs policy
        wstring const & runAsName = appPackageDescription_.DigestedEnvironment.Policies.DefaultRunAs.UserRef;
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Set default RunAs to \"{1}\"",
            applicationId_,
            runAsName);
        error = appEnvironmentContext_->SetDefaultRunAsUser(runAsName);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End set default RunAs: error {1}",
            applicationId_,
            error);

        if (!error.IsSuccess())
        {
            CleanupOnError(operation->Parent, error);
            return;
        }
        WritePrincipalSIDsToFile(operation->Parent);
        SetupLogCollectionPolicy(appPackageDescription_.DigestedEnvironment.Policies.LogCollectionEntries, operation->Parent);
    }

    void WritePrincipalSIDsToFile(AsyncOperationSPtr const & thisSPtr)
    {
        ImageModel::RunLayoutSpecification runLayout(owner_.Hosting.DeploymentFolder);

        map<wstring, wstring> sids;
        if(HostingConfig::GetConfig().RunAsPolicyEnabled && appEnvironmentContext_->PrincipalsContext)
        {
            for(auto it = appEnvironmentContext_->PrincipalsContext->PrincipalsInformation.begin(); it != appEnvironmentContext_->PrincipalsContext->PrincipalsInformation.end(); ++it)
            {
                sids.insert(make_pair((*it)->Name, (*it)->SidString));
            }
        }

        // Write the endpoint file
        wstring sidsFileName = runLayout.GetPrincipalSIDsFile(applicationId_.ToString());
        bool isSuccess = PrincipalsDescription::WriteSIDsToFile(sidsFileName, sids);

        if(!isSuccess)
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Could not write the SIDS to file {0}, ApplicationId={1}. Cleaning the application environment.",
                sidsFileName,
                applicationId_);
            CleanupOnError(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            return;
        }
    }

    void SetupLogCollectionPolicy(
        vector<LogCollectionPolicyDescription> const & policies,
        AsyncOperationSPtr const & thisSPtr)
    {
        if (policies.empty())
        {
            // Nothing to do
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Log Collection is not enabled",
                applicationId_);
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        RunLayoutSpecification runLayout(owner_.Hosting.DeploymentFolder);

        wstring logFolderPath = runLayout.GetApplicationLogFolder(applicationId_.ToString());
        appEnvironmentContext_->LogCollectionPath = logFolderPath;

        vector<wstring> paths;
        for(auto it = policies.begin(); it != policies.end(); ++it)
        {
            wstring path = it->Path.empty() ? logFolderPath : Path::Combine(logFolderPath, it->Path);
            paths.push_back(move(path));
        }

        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(LogCollectionProvider->AddLogPaths): root path {1}",
            applicationId_,
            logFolderPath);
        auto operation = owner_.logCollectionProvider_->BeginAddLogPaths(
            applicationId_.ToString(),
            paths,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnLogCollectionSetupCompleted(operation); },
            thisSPtr);
        if (operation->CompletedSynchronously)
        {
            FinishLogCollectionSetup(operation);
        }
    }

    void OnLogCollectionSetupCompleted(
        AsyncOperationSPtr const & operation)
    {
        if (!operation->CompletedSynchronously)
        {
            FinishLogCollectionSetup(operation);
        }
    }

    void FinishLogCollectionSetup(
        AsyncOperationSPtr const & operation)
    {
        auto error = owner_.logCollectionProvider_->EndAddLogPaths(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(logCollectionProvider->AddLogPaths): error {1}",
            applicationId_,
            error);
        if (!error.IsSuccess())
        {
            CleanupOnError(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, error);
    }

    void CleanupOnError(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const error)
    {
        // Call cleanup, best effort
        lastError_.Overwrite(error);
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(Setup->CleanupApplication due to error {1})",
            applicationId_,
            error);

        auto operation = owner_.BeginCleanupApplicationEnvironment(
            appEnvironmentContext_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCleanupApplication(operation, false); },
            thisSPtr);
        FinishCleanupApplication(operation, true);
    }

    void FinishCleanupApplication(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.EndCleanupApplicationEnvironment(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(Setup->CleanupApplication due to error {1}): error {2}",
            applicationId_,
            lastError_,
            error);

        // Complete with the saved error
        TryComplete(operation->Parent, lastError_);
    }

private:
    EnvironmentManager & owner_;
    ApplicationIdentifier const applicationId_;
    ApplicationPackageDescription const appPackageDescription_;
    TimeoutHelper timeoutHelper_;
    ApplicationEnvironmentContextSPtr appEnvironmentContext_;
    ErrorCode lastError_;
};

// ********************************************************************************************************************
// EnvironmentManager::CleanupApplicationAsyncOperation Implementation
//
class EnvironmentManager::CleanupApplicationAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CleanupApplicationAsyncOperation)

public:
    CleanupApplicationAsyncOperation(
        EnvironmentManager & appEnvironmentManager,
        ApplicationEnvironmentContextSPtr const & appEnvironmentContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(appEnvironmentManager),
        appEnvironmentContext_(appEnvironmentContext),
        timeoutHelper_(timeout),
        lastError_(ErrorCodeValue::Success)
    {
    }

    virtual ~CleanupApplicationAsyncOperation()
    {
    }

    static ErrorCode CleanupApplicationAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupApplicationAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!appEnvironmentContext_)
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }

        owner_.crashDumpProvider_->CleanupApplicationCrashDumps(
            appEnvironmentContext_->ApplicationId.ToString());

        if (!HostingConfig::GetConfig().RunAsPolicyEnabled)
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: App sandbox disabled, no principals to clean up",
                appEnvironmentContext_->ApplicationId);
            ASSERT_IF(appEnvironmentContext_->PrincipalsContext, "RunAsPolicyEnabled disabled, principals context shouldn't exist");
            // Disable log collection if enabled
            CleanupLogCollectionPolicy(thisSPtr);
        }
        else
        {
            if (!appEnvironmentContext_->PrincipalsContext)
            {
                WriteNoise(
                    TraceEnvironmentManager,
                    owner_.Root.TraceId,
                    "{0}: App PrincipalContext not present, no principals to clean up",
                    appEnvironmentContext_->ApplicationId);
                CleanupLogCollectionPolicy(thisSPtr);
                return;
            }
            auto operation = owner_.hosting_.FabricActivatorClientObj->BeginCleanupSecurityPrincipals(
                appEnvironmentContext_->ApplicationId.ToString(),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCleanupSecurityPrincipalsCompleted(operation, false); },
                thisSPtr);
            OnCleanupSecurityPrincipalsCompleted(operation, true);
        }
    }

private:

    void OnCleanupSecurityPrincipalsCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.hosting_.FabricActivatorClientObj->EndCleanupSecurityPrincipals(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End ConfigureSecurityPrincipals: error {1}",
            appEnvironmentContext_->ApplicationId,
            error);
        if(!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        // Disable log collection if enabled
        CleanupLogCollectionPolicy(operation->Parent);
    }

    void CleanupLogCollectionPolicy(
        AsyncOperationSPtr const & thisSPtr)
    {
        wstring const & path = appEnvironmentContext_->LogCollectionPath;
        if (path.empty())
        {
            // Log collection policy not set, nothing to do
            TryComplete(thisSPtr, lastError_);
            return;
        }

        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(logCollectionProvider->RemoveLogPaths): path {1}",
            appEnvironmentContext_->ApplicationId,
            path);

        ASSERT_IFNOT(owner_.logCollectionProvider_, "Log collection provider should exist");
        auto operation = owner_.logCollectionProvider_->BeginRemoveLogPaths(
            path,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishLogCollectionCleanup(operation, false); },
            thisSPtr);
        FinishLogCollectionCleanup(operation, true);
    }

    void FinishLogCollectionCleanup(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.logCollectionProvider_->EndRemoveLogPaths(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(logCollectionProvider->RemoveLogPaths): error {1}",
            appEnvironmentContext_->ApplicationId,
            error);

        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        TryComplete(operation->Parent, lastError_);
    }

private:
    EnvironmentManager & owner_;
    ApplicationEnvironmentContextSPtr const & appEnvironmentContext_;
    TimeoutHelper timeoutHelper_;
    ErrorCode lastError_;
};

// ********************************************************************************************************************
// EnvironmentManager::SetupServicePackageInstanceAsyncOperation Implementation
//
class EnvironmentManager::SetupServicePackageInstanceAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(SetupServicePackageInstanceAsyncOperation)

public:
    SetupServicePackageInstanceAsyncOperation(
        __in EnvironmentManager & owner,
        ApplicationEnvironmentContextSPtr const & appEnvironmentContext,
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        ServiceModel::ServicePackageDescription const & servicePackageDescription,
        int64 instanceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        appEnvironmentContext_(appEnvironmentContext),
        servicePackageInstanceId_(servicePackageInstanceId),
        instanceId_(instanceId),
        servicePackageDescription_(servicePackageDescription),
        timeoutHelper_(timeout),
        packageInstanceEnvironmentContext(),
        endpoints_(),
        firewallPortsToOpen_(),
        endpointsAclCount_(0),
        setupContainerGroup_(false),
        lastError_(ErrorCodeValue::Success),
        endpointsWithACL_()
    {
        packageInstanceEnvironmentContext = make_shared<ServicePackageInstanceEnvironmentContext>(servicePackageInstanceId_);
    }

    virtual ~SetupServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode SetupServicePackageInstanceAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out ServicePackageInstanceEnvironmentContextSPtr & packageEnvironmentContext)
    {
        auto thisPtr = AsyncOperation::End<SetupServicePackageInstanceAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            packageEnvironmentContext = move(thisPtr->packageInstanceEnvironmentContext);
        }

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SetupEnvironment(thisSPtr);
    }

private:
    void SetupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "SetupServicePackage: Id={0}, RunAsPolicyEnabled={1}, EndpointProviderEnabled={2}",
            servicePackageInstanceId_,
            HostingConfig::GetConfig().RunAsPolicyEnabled,
            HostingConfig::GetConfig().EndpointProviderEnabled);

        SetupDiagnostics(thisSPtr);
    }

    void AssignResources(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> codePackages;
        int containerCount = 0;
        for (auto it = servicePackageDescription_.DigestedCodePackages.begin(); it != servicePackageDescription_.DigestedCodePackages.end(); ++it)
        {
            if (it->CodePackage.EntryPoint.EntryPointType == EntryPointType::ContainerHost)
            {
                containerCount++;
            }
            if (it->ContainerPolicies.NetworkConfig.Type == NetworkType::Enum::Open)
            {
#if defined(PLATFORM_UNIX)
                if (codePackages.empty())
                {
                    codePackages.push_back(it->Name);
                }
#else
                codePackages.push_back(it->Name);
#endif
            }
        }

#if defined(PLATFORM_UNIX)
        if (containerCount > 1) 
        {
            setupContainerGroup_ = true;
        }
#endif
        if (codePackages.empty())
        {
            if (setupContainerGroup_)
            {
                SetupContainerGroup(thisSPtr, L"");
            }
            else
            {
                SetupEndpoints(thisSPtr);
            }
        }
        else
        {
            BeginAssignIpAddress(thisSPtr, codePackages);
        }
    }

    void BeginAssignIpAddress(AsyncOperationSPtr const & thisSPtr, vector<wstring> const & codePackages)
    {
        auto operation = owner_.Hosting.FabricActivatorClientObj->BeginAssignIpAddresses(
            this->servicePackageInstanceId_.ToString(),
            codePackages,
            false,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnAssignIpAddressCompleted(operation, false);
        },
            thisSPtr);
        this->OnAssignIpAddressCompleted(operation, true);
    }

    void OnAssignIpAddressCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        vector<wstring> assignedIps;
        auto error = owner_.Hosting.FabricActivatorClientObj->EndAssignIpAddresses(operation, assignedIps);
        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
            CleanupOnError(operation->Parent, lastError_);
            return;
        }
        for (auto it = assignedIps.begin(); it != assignedIps.end(); ++it)
        {
            wstring ipAddress;
            wstring codePackageName;
            StringUtility::SplitOnce(*it, ipAddress, codePackageName, L',');
            packageInstanceEnvironmentContext->AddAssignedIpAddresses(codePackageName, ipAddress);
        }
        if (setupContainerGroup_)
        {
            auto groupIp = packageInstanceEnvironmentContext->GetGroupAssignedIp();
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Group assigned Ip: Id={0}, Ip = {2}",
                servicePackageInstanceId_,
                groupIp);
            SetupContainerGroup(operation->Parent, groupIp);
        }
        else
        {
            SetupEndpoints(operation->Parent);
        }
    }

    void SetupContainerGroup(
        AsyncOperationSPtr const & thisSPtr,
        wstring const & assignedIp)
    {
        packageInstanceEnvironmentContext->SetContainerGroupSetupFlag(true);
        auto op = owner_.Hosting.FabricActivatorClientObj->BeginSetupContainerGroup(
            this->servicePackageInstanceId_.ToString(),
            assignedIp,
            owner_.Hosting.RunLayout.GetApplicationFolder(appEnvironmentContext_->ApplicationId.ToString()),
            StringUtility::ToWString(appEnvironmentContext_->ApplicationId.ApplicationNumber),
            this->servicePackageDescription_.ResourceGovernanceDescription,
            false,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & op)
        {
            this->OnSetupContainerGroupCompleted(op, false);
        },
            thisSPtr);
        OnSetupContainerGroupCompleted(op, true);
    }

    void OnSetupContainerGroupCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        wstring containerName;
        auto error = owner_.Hosting.FabricActivatorClientObj->EndSetupContainerGroup(operation, containerName);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        packageInstanceEnvironmentContext->AddGroupContainerName(containerName);
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "Container group with container name {0} set for service instance {1}",
            containerName,
            this->servicePackageInstanceId_);
        SetupEndpoints(operation->Parent);
    }

    void SetupEndpoints(AsyncOperationSPtr const & thisSPtr)
    {
        if (HostingConfig::GetConfig().EndpointProviderEnabled)
        {
            ASSERT_IFNOT(owner_.endpointProvider_, "Endpoint provider should exist");

            for(auto endpointIter = servicePackageDescription_.DigestedResources.DigestedEndpoints.begin();
                endpointIter != servicePackageDescription_.DigestedResources.DigestedEndpoints.end();
                ++endpointIter)
            {
                EndpointResourceSPtr endpointResource = make_shared<EndpointResource>(
                    endpointIter->second.Endpoint,
                    endpointIter->second.EndpointBindingPolicy);

                auto error = SetEndPointResourceIpAddress(endpointResource);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceEnvironmentManager,
                        owner_.Root.TraceId,
                        "SetEndPointResourceIpAddress() finished with Error={0} for endpoint resource={1}.",
                        error,
                        endpointResource->Name);

                    CleanupOnError(thisSPtr, error);
                    return;
                }

                if(!endpointIter->second.SecurityAccessPolicy.ResourceRef.empty())
                {
                    ASSERT_IFNOT(
                        StringUtility::AreEqualCaseInsensitive(endpointIter->second.SecurityAccessPolicy.ResourceRef, endpointIter->second.Endpoint.Name),
                        "Error in the DigestedEndpoint element. The ResourceRef name '{0}' in SecurityAccessPolicy does not match the resource name '{1}'.",
                        endpointIter->second.SecurityAccessPolicy.ResourceRef,
                        endpointIter->second.Endpoint.Name);

                    SecurityPrincipalInformationSPtr principalInfo;
                    error  = appEnvironmentContext_->PrincipalsContext->TryGetPrincipalInfo(endpointIter->second.SecurityAccessPolicy.PrincipalRef, principalInfo);
                    if(!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceEnvironmentManager,
                            owner_.Root.TraceId,
                            "Error getting the User object '{0}'.ApplicationId={1}",
                            endpointIter->second.SecurityAccessPolicy.PrincipalRef,
                            servicePackageInstanceId_.ApplicationId);
                        CleanupOnError(thisSPtr, error);
                        return;
                    }
                    endpointResource->AddSecurityAccessPolicy(principalInfo, endpointIter->second.SecurityAccessPolicy.Rights);
                }
                else if (EndpointProvider::IsHttpEndpoint(endpointResource) &&
                    !owner_.isAdminUser_)
                {
                    SecurityPrincipalInformationSPtr secPrincipal = make_shared<SecurityPrincipalInformation>(
                        owner_.currentUserSid_,
                        owner_.currentUserSid_,
                        owner_.currentUserSid_,
                        false);
                    endpointResource->AddSecurityAccessPolicy(secPrincipal, ServiceModel::GrantAccessType::Full);
                }

                if (endpointIter->second.Endpoint.ExplicitPortSpecified)
                {
                    WriteInfo(TraceEnvironmentManager,
                        owner_.Root.TraceId,
                        "ExplicitPort specified for Endpoint '{0}' Port {1}. ServicePackageInstanceId={2}",
                        endpointIter->second.Endpoint.Name,
                        endpointIter->second.Endpoint.Port,
                        servicePackageInstanceId_);

                    if (HostingConfig::GetConfig().FirewallPolicyEnabled)
                    {
                        firewallPortsToOpen_.push_back(endpointIter->second.Endpoint.Port);
                    }
                }
                else
                {
                    error = owner_.endpointProvider_->AddEndpoint(endpointResource);
                    if (!error.IsSuccess())
                    {
                        CleanupOnError(thisSPtr, error);
                        return;
                    }
                }
                if(EndpointProvider::IsHttpEndpoint(endpointResource))
                {
                    EnvironmentResource::ResourceAccess resourceAccess;
                    error  = endpointResource->GetDefaultSecurityAccess(resourceAccess);
                    if(error.IsSuccess())
                    {
                        endpointsWithACL_.push_back(endpointResource);
                    }
                    else if (error.IsError(ErrorCodeValue::NotFound))
                    {
                        AddEndpointResource(endpointResource);
                    }
                    else
                    {
                        WriteWarning(
                            TraceEnvironmentManager,
                            owner_.Root.TraceId,
                            "Error getting default security access for endpointResource '{0}'.Error={1}",
                            endpointResource,
                            error);
                        CleanupOnError(thisSPtr, error);
                        return;
                    }
                }
                else
                {
                    AddEndpointResource(endpointResource);
                }
            }
        }
        else
        {
            if (servicePackageDescription_.DigestedResources.DigestedEndpoints.size() > 0)
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    owner_.Root.TraceId,
                    "EndpointProvider is not enabled on this node, however the ServicePackage {0} contains {1} endpoints.",
                    servicePackageInstanceId_,
                    servicePackageDescription_.DigestedResources.DigestedEndpoints.size());
                auto error = ErrorCode(ErrorCodeValue::EndpointProviderNotEnabled);
                error.ReadValue();
                CleanupOnError(thisSPtr, error);
                return;
            }

        }
        if(endpointsWithACL_.size() > 0)
        {
            this->SetupEndpointSecurity(thisSPtr);
        }
        else
        {
            ConfigureEndpointBindingAndFirewallPolicy(thisSPtr);
        }
    }

    void SetupEndpointSecurity(AsyncOperationSPtr thisSPtr)
    {
        endpointsAclCount_.store(endpointsWithACL_.size());
        EnvironmentResource::ResourceAccess resourceAccess;
        for(auto iter = endpointsWithACL_.begin(); iter != endpointsWithACL_.end(); ++iter)
        {
            EndpointResourceSPtr endpointResource = *iter;

            auto error  = endpointResource->GetDefaultSecurityAccess(resourceAccess);
            ASSERT_IFNOT(error.IsSuccess(), "endpointresource getdefaultsecurityaccess returned error {0}", error);
            if(error.IsSuccess())
            {
                auto operation = owner_.Hosting.FabricActivatorClientObj->BeginConfigureEndpointSecurity(
                    resourceAccess.PrincipalInfo->SidString,
                    endpointResource->Port,
                    endpointResource->Protocol == ServiceModel::ProtocolType::Enum::Https,
                    false,
                    owner_.Hosting.FabricNodeConfigObj.IPAddressOrFQDN,
                    EnvironmentManager::GetServicePackageIdentifier(servicePackageInstanceId_.ToString(), instanceId_),
                    endpointResource->EndpointDescriptionObj.ExplicitPortSpecified,
                    [this, endpointResource](AsyncOperationSPtr const & operation)
                {
                    this->OnEndpointSecurityConfigurationCompleted(endpointResource, operation, false);
                },
                    thisSPtr);
                this->OnEndpointSecurityConfigurationCompleted(endpointResource, operation, true);
            }
        }
    }

    void OnEndpointSecurityConfigurationCompleted(
        EndpointResourceSPtr endpointResource,
        AsyncOperationSPtr operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndConfigureEndpointSecurity(operation);
        if(!error.IsSuccess())
        {
            lastError_.Overwrite(error);

            // Empty the endpoint which failed, so we don;t add it to the endpoits_.
            // O(n) search for endpoint is okay because the #endpoints to be very huge.
            auto endpointResourceToRemove = std::find(endpointsWithACL_.begin(), endpointsWithACL_.end(), endpointResource);

            ASSERT_IF(endpointResourceToRemove == endpointsWithACL_.end(), "Endpoint should exist in endpointWithACL");

            *endpointResourceToRemove = nullptr;
        }

        uint64 pendingOperationCount = --endpointsAclCount_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
            for (auto & endpoint : endpointsWithACL_)
            {
                //Donot add the endpoint if it is empty because it failed the SetupSecurity.
                if (endpoint)
                {
                    AddEndpointResource(endpoint);
                }
                
            }
            if(!lastError_.IsSuccess())
            {
                CleanupOnError(thisSPtr, lastError_);
                return;
            }

            ConfigureEndpointBindingAndFirewallPolicy(thisSPtr);
        }
    }

    void AddEndpointResource(EndpointResourceSPtr & endpointResource)
    {
        endpoints_.push_back(endpointResource->EndpointDescriptionObj);
        packageInstanceEnvironmentContext->AddEndpoint(move(endpointResource));
    }

    void ConfigureEndpointBindingAndFirewallPolicy(AsyncOperationSPtr const & thisSPtr)
    {
        bool enableFirewallPolicy = false;

        if (HostingConfig::GetConfig().FirewallPolicyEnabled &&
            !firewallPortsToOpen_.empty())
        {
            enableFirewallPolicy = true;
        }

        vector<EndpointCertificateBinding> endpointCertBindings;
        owner_.GetEndpointBindingPolicies(
            packageInstanceEnvironmentContext->Endpoints,
            servicePackageDescription_.DigestedResources.DigestedCertificates,
            endpointCertBindings);

        if (!endpointCertBindings.empty() || enableFirewallPolicy)
        {
            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginConfigureEndpointCertificateAndFirewallPolicy(
                EnvironmentManager::GetServicePackageIdentifier(servicePackageInstanceId_.ToString(), instanceId_),
                endpointCertBindings,
                false,
                false,
                firewallPortsToOpen_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnBeginConfigureEndpointCertificateAndFirewallPolicy(operation, false);
            },
                thisSPtr);
            this->OnBeginConfigureEndpointCertificateAndFirewallPolicy(operation, true);
        }
        else
        { 
            SetupContainerCertificates(thisSPtr);
        }
    }

    void OnBeginConfigureEndpointCertificateAndFirewallPolicy(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndConfigureEndpointCertificateAndFirewallPolicy(operation);
        if (!error.IsSuccess())
        {
            CleanupOnError(operation->Parent, error);
            return;
        }
  
        SetupContainerCertificates(operation->Parent);
    }

    void SetupContainerCertificates(AsyncOperationSPtr const & thisSPtr)
    {
        std::map<wstring, std::vector<ServiceModel::ContainerCertificateDescription>> allCertificateRef;
        for (auto digestedCodePackage = servicePackageDescription_.DigestedCodePackages.begin();
            digestedCodePackage != servicePackageDescription_.DigestedCodePackages.end();
            ++digestedCodePackage)
        {
            if (!digestedCodePackage->ContainerPolicies.CertificateRef.empty())
            {
                allCertificateRef[digestedCodePackage->CodePackage.Name] = digestedCodePackage->ContainerPolicies.CertificateRef;
            }
        }

        if (!allCertificateRef.empty())
        {
            wstring applicationWorkFolder = owner_.hosting_.RunLayout.GetApplicationWorkFolder(servicePackageInstanceId_.ApplicationId.ToString());
            wstring certificateFolder = Path::Combine(applicationWorkFolder, L"Certificates_" + servicePackageDescription_.ManifestName);
            ErrorCode error = Directory::Create2(certificateFolder); 
            if (!error.IsSuccess())
            {
                WriteError(
                    TraceEnvironmentManager,
                    owner_.Root.TraceId,
                    "Error creating directory {0}. Error={1}",
                    certificateFolder,
                    error);
                CleanupOnError(thisSPtr, error);
                return;
            }

            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginConfigureContainerCertificateExport(
                allCertificateRef,
                certificateFolder,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnConfigureContainerCertificateExportCompleted(operation, false);
            },
                thisSPtr);
            this->OnConfigureContainerCertificateExportCompleted(operation, true);
        }
        else
        {
            WriteResources(thisSPtr);
        }
    }

    void OnConfigureContainerCertificateExportCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        map<std::wstring, std::wstring> certificatePaths;
        map<std::wstring, std::wstring> certificatePasswordPaths;
        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndConfigureContainerCertificateExport(operation, certificatePaths, certificatePasswordPaths);
        if (!error.IsSuccess())
        {
            CleanupOnError(operation->Parent, error);
            return;
        }

        packageInstanceEnvironmentContext->AddCertificatePaths(certificatePaths, certificatePasswordPaths);
        WriteResources(operation->Parent);
    }

    void WriteResources(AsyncOperationSPtr const & thisSPtr)
    {
        ImageModel::RunLayoutSpecification runLayout(owner_.Hosting.DeploymentFolder);

        // Write the endpoint file
        wstring endpointFileName = runLayout.GetEndpointDescriptionsFile(
            servicePackageInstanceId_.ApplicationId.ToString(),
            servicePackageInstanceId_.ServicePackageName,
            servicePackageInstanceId_.PublicActivationId);

        bool isSuccess = EndpointDescription::WriteToFile(endpointFileName, endpoints_);

        if(isSuccess)
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "EndpointDescription successfully written to file {0}, ServicePackageInstanceId={1}, NodeVersion={2}.",
                endpointFileName,
                servicePackageInstanceId_,
                owner_.Hosting.FabricNodeConfigObj.NodeVersion);

            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
            return;
        }
        else
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Could not write the endpoints to file {0}, ServicePackageInstanceId={1}, NodeVersion={2}. Cleaning the ServicePackageInstanceEnvironmentContext and retrying.",
                endpointFileName,
                servicePackageInstanceId_,
                owner_.Hosting.FabricNodeConfigObj.NodeVersion);

            CleanupOnError(thisSPtr, ErrorCode(ErrorCodeValue::OperationFailed));
            return;
        }
    }

    void SetupDiagnostics(AsyncOperationSPtr const & thisSPtr)
    {
#if !defined(PLATFORM_UNIX)
        ASSERT_IFNOT(owner_.etwSessionProvider_, "ETW session provider should exist");
        auto error = owner_.etwSessionProvider_->SetupServiceEtw(servicePackageDescription_, packageInstanceEnvironmentContext);
        if(!error.IsSuccess())
        {
            CleanupOnError(thisSPtr, error);
            return;
        }
#endif
        ASSERT_IFNOT(owner_.crashDumpProvider_, "Crash dump provider should exist");
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: BeginSetupServiceCrashDumps.)",
            servicePackageInstanceId_);
        auto operation = owner_.crashDumpProvider_->BeginSetupServiceCrashDumps(
            servicePackageDescription_,
            packageInstanceEnvironmentContext,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishSetupServiceCrashDumps(operation, false); },
            thisSPtr);
        this->FinishSetupServiceCrashDumps(operation, true);
    }

    void FinishSetupServiceCrashDumps(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.crashDumpProvider_->EndSetupServiceCrashDumps(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: EndSetupServiceCrashDumps: error {1}",
            servicePackageInstanceId_,
            error);
        if(!error.IsSuccess())
        {
            CleanupOnError(operation->Parent, error);
            return;
        }

        AssignResources(operation->Parent);
    }

    void CleanupOnError(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        // Call cleanup, best effort
        lastError_.Overwrite(error);
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(Setup->BeginCleanupServicePackageEnvironment due to error {1})",
            servicePackageInstanceId_,
            error);

        auto operation = owner_.BeginCleanupServicePackageInstanceEnvironment(
            packageInstanceEnvironmentContext,
            servicePackageDescription_,
            instanceId_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCleanupServicePackageEnvironment(operation, false); },
            thisSPtr);
        this->FinishCleanupServicePackageEnvironment(operation, true);
    }

    void FinishCleanupServicePackageEnvironment(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.EndCleanupServicePackageInstanceEnvironment(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: End(Setup->EndCleanupServicePackageEnvironment due to error {1}): error {2}",
            servicePackageInstanceId_,
            lastError_,
            error);

        // Complete with the saved error
        TryComplete(operation->Parent, lastError_);
    }

    ErrorCode SetEndPointResourceIpAddress(EndpointResourceSPtr const & endpointResource)
    {
        auto codePackageRef = endpointResource->EndpointDescriptionObj.CodePackageRef;
        auto nodeIpAddress = owner_.Hosting.FabricNodeConfigObj.IPAddressOrFQDN;

        if (codePackageRef.empty())
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint resource '{0}' has no CodePackageRef. Assigning NodeIPAddress={1}.",
                endpointResource->Name,
                nodeIpAddress);

            endpointResource->IpAddressOrFqdn = nodeIpAddress;
            return ErrorCodeValue::Success;
        }

        wstring assignedIpAddress;
        auto error = packageInstanceEnvironmentContext->GetIpAddressAssignedToCodePackage(codePackageRef, assignedIpAddress);
        if (error.IsSuccess())
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint resource '{0}' has CodePackageRef='{1}'. Found assigned IPAddress={2}.",
                endpointResource->Name,
                codePackageRef,
                assignedIpAddress);

            endpointResource->IpAddressOrFqdn = assignedIpAddress;
            return error;
        }

        if (error.IsError(ErrorCodeValue::NotFound))
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint resource '{0}' has CodePackageRef='{1}'. Assigned IPAddress not found. Assigning NodeIPAddress={2}.",
                endpointResource->Name,
                codePackageRef,
                nodeIpAddress);

            endpointResource->IpAddressOrFqdn = nodeIpAddress;
            return ErrorCodeValue::Success;
        }

        return error;
    }

private:
    EnvironmentManager & owner_;
    ApplicationEnvironmentContextSPtr const appEnvironmentContext_;
    ServicePackageInstanceIdentifier const servicePackageInstanceId_;
    ServiceModel::ServicePackageDescription const servicePackageDescription_;
    int64 instanceId_;
    TimeoutHelper timeoutHelper_;
    ServicePackageInstanceEnvironmentContextSPtr packageInstanceEnvironmentContext;
    bool setupContainerGroup_;
    vector<EndpointDescription> endpoints_;
    vector<LONG> firewallPortsToOpen_;
    atomic_uint64 endpointsAclCount_;
    ErrorCode lastError_;
    vector<EndpointResourceSPtr> endpointsWithACL_;
};

// ********************************************************************************************************************
// EnvironmentManager::CleanupServicePackageInstanceAsyncOperation Implementation
//
class EnvironmentManager::CleanupServicePackageInstanceAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CleanupServicePackageInstanceAsyncOperation)

public:
    CleanupServicePackageInstanceAsyncOperation(
        EnvironmentManager & owner,
        ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
        ServiceModel::ServicePackageDescription const & servicePackageDescription,
        int64 instanceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        packageInstanceEnvironmentContext(packageEnvironmentContext),
        servicePackageDescription_(servicePackageDescription),
        instanceId_(instanceId),
        timeoutHelper_(timeout),
        pendingOperations_(0),
        clearFirewallPolicy_(false),
        lastError_(ErrorCodeValue::Success)
    {
    }

    virtual ~CleanupServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode CleanupServicePackageInstanceAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupServicePackageInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IFNOT(owner_.crashDumpProvider_, "Crash dump provider should exist");
        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: BeginCleanupServiceCrashDumps.)",
            packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString());
        auto operation = owner_.crashDumpProvider_->BeginCleanupServiceCrashDumps(
            packageInstanceEnvironmentContext,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->FinishCleanupServiceCrashDumps(operation, false); },
            thisSPtr);
        this->FinishCleanupServiceCrashDumps(operation, true);
    }

private:

    void FinishCleanupServiceCrashDumps(AsyncOperationSPtr const & operation, bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        auto error = owner_.crashDumpProvider_->EndCleanupServiceCrashDumps(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: EndCleanupServiceCrashDumps: error {1}",
            packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
            error);
        if(!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        if (!packageInstanceEnvironmentContext)
        {
            TryComplete(operation->Parent, lastError_);
        }
        else
        {
            auto thisSPtr = operation->Parent;
            if (packageInstanceEnvironmentContext->SetupContainerGroup)
            {
                auto op = owner_.Hosting.FabricActivatorClientObj->BeginSetupContainerGroup(
                    packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
                    wstring(),
                    wstring(),
                    wstring(),
                    ServicePackageResourceGovernanceDescription(),
                    true,
                    timeoutHelper_.GetRemainingTime(),
                    [this](AsyncOperationSPtr const & op)
                {
                    this->OnCleanupContainerGroupSetupCompleted(op, false);
                }, thisSPtr);

                OnCleanupContainerGroupSetupCompleted(op, true);
            }
            else
            {
                ReleaseAssignedIPs(operation->Parent);
            }
        }
    }

    void OnCleanupContainerGroupSetupCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        wstring result;
        auto error = owner_.Hosting.FabricActivatorClientObj->EndSetupContainerGroup(operation, result);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "EndSetupContainerGroupCleanup:  ServicePackageId={0}, ErrorCode={1}.",
                packageInstanceEnvironmentContext->ServicePackageInstanceId,
                error);
            lastError_.Overwrite(error);
        }
        ReleaseAssignedIPs(operation->Parent);
    }

    void ReleaseAssignedIPs(AsyncOperationSPtr const & thisSPtr)
    {
        if (packageInstanceEnvironmentContext->HasIpsAssigned)
        {
            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginAssignIpAddresses(
                packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
                vector<wstring>(),
                true,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnIPAddressesReleased(operation, false);
            },
                thisSPtr);
            this->OnIPAddressesReleased(operation, true);
        }
        else
        {
            CleanupEndpointSecurity(thisSPtr);
        }
    }

    void OnIPAddressesReleased(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        vector<wstring> result;
        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndAssignIpAddresses(operation, result);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "EndReleaseIPAddresses:  ServicePackageId={0}, ErrorCode={1}.",
                packageInstanceEnvironmentContext->ServicePackageInstanceId,
                error);
            lastError_.Overwrite(error);
        }
        CleanupEndpointSecurity(operation->Parent);
    }

    void CleanupEndpointSecurity(AsyncOperationSPtr const & thisSPtr)
    {
        vector<EndpointResourceSPtr> acldEndpoints;
        if (HostingConfig::GetConfig().EndpointProviderEnabled)
        {
            ASSERT_IFNOT(owner_.endpointProvider_, "Endpoint provider must not be null as EndpointFiltering is enabled");

            for(auto iter = packageInstanceEnvironmentContext->Endpoints.begin();
                iter != packageInstanceEnvironmentContext->Endpoints.end();
                ++iter)
            {
                EndpointResourceSPtr endpointResource = *iter;
                SecurityPrincipalInformationSPtr principalInfo;
                auto error = EndpointProvider::GetPrincipal(endpointResource, principalInfo);
                if(error.IsSuccess() &&
                    EndpointProvider::IsHttpEndpoint(endpointResource))
                {
                    acldEndpoints.push_back(endpointResource);
                }
                else
                {
                    if(!error.IsSuccess() &&
                        !error.IsError(ErrorCodeValue::NotFound))
                    {
                        WriteWarning(TraceEnvironmentManager,
                            owner_.Root.TraceId,
                            "Retrieve PrincipalInfo for endpoint {0} returned ErrorCode={1}. Continuing to remove other endpoints.",
                            endpointResource->Name,
                            error);
                        lastError_.Overwrite(error);
                    }

                    error = owner_.endpointProvider_->RemoveEndpoint(*iter);
                    if(!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceEnvironmentManager,
                            owner_.Root.TraceId,
                            "RemoveEndpoint failed. ServicePackageId={0}, Endpoint={1}, ErrorCode={2}. Continuing to remove other endpoints.",
                            packageInstanceEnvironmentContext->ServicePackageInstanceId,
                            (*iter)->Name,
                            error);
                        lastError_.Overwrite(error);
                    }
                }
                if (endpointResource->EndpointDescriptionObj.ExplicitPortSpecified)
                {
                    clearFirewallPolicy_ = true;
                }
            }
        }
        if(acldEndpoints.size() == 0)
        {
            CheckPendingOperations(thisSPtr, 0);
        }
        else
        {
            CleanupEndpointSecurity(acldEndpoints, thisSPtr);
        }
    }

    void CleanupEndpointSecurity(vector<EndpointResourceSPtr> const & endpointResources, AsyncOperationSPtr thisSPtr)
    {
        pendingOperations_.store(endpointResources.size());

        for(auto iter = endpointResources.begin(); iter != endpointResources.end(); iter++)
        {
            EndpointResourceSPtr endpointResource = *iter;
            SecurityPrincipalInformationSPtr principalInfo;
            auto error = EndpointProvider::GetPrincipal(endpointResource, principalInfo);
            ASSERT_IFNOT(error.IsSuccess(), "EndpointProvider::GetPrincipal returned error {0}", error);

            auto operation = owner_.hosting_.FabricActivatorClientObj->BeginConfigureEndpointSecurity(
                principalInfo->SidString,
                endpointResource->Port,
                endpointResource->Protocol == ServiceModel::ProtocolType::Enum::Https,
                true,
                owner_.Hosting.FabricNodeConfigObj.IPAddressOrFQDN,
                EnvironmentManager::GetServicePackageIdentifier(packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(), instanceId_),
                endpointResource->EndpointDescriptionObj.ExplicitPortSpecified,
                [this, endpointResource](AsyncOperationSPtr const & operation)
            {
                this->OnEndpointSecurityConfigurationCompleted(endpointResource, operation, false);
            },
                thisSPtr);
            this->OnEndpointSecurityConfigurationCompleted(endpointResource, operation, true);
        }

    }

    void OnEndpointSecurityConfigurationCompleted(
        EndpointResourceSPtr const & endpointResource,
        AsyncOperationSPtr operation,
        bool expectedCompletedSynhronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynhronously)
        {
            return;
        }

        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndConfigureEndpointSecurity(operation);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "EndConfigureEndpointSecurity:  ServicePackageId={0}, Endpoint={1}, ErrorCode={2}.",
                packageInstanceEnvironmentContext->ServicePackageInstanceId,
                endpointResource->Name,
                error);
            lastError_.Overwrite(error);
        }

        error = owner_.endpointProvider_->RemoveEndpoint(endpointResource);
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "RemoveEndpoint failed. ServicePackageId={0}, Endpoint={1}, ErrorCode={2}. Continuing to remove other endpoints.",
                packageInstanceEnvironmentContext->ServicePackageInstanceId,
                endpointResource->Name,
                error);
            lastError_.Overwrite(error);
        }

        uint64 pendingOperationCount = --pendingOperations_;
        CheckPendingOperations(operation->Parent, pendingOperationCount);
    }

    void CheckPendingOperations(AsyncOperationSPtr thisSPtr, uint64 pendingOperationCount)
    {
        if (pendingOperationCount == 0)
        {
#if !defined(PLATFORM_UNIX)
            ASSERT_IFNOT(owner_.etwSessionProvider_, "ETW session provider should exist");
            owner_.etwSessionProvider_->CleanupServiceEtw(packageInstanceEnvironmentContext);
#endif
            ConfigureEndpointBindingAndFirewallPolicyPolicy(thisSPtr);
        }
    }

    void ConfigureEndpointBindingAndFirewallPolicyPolicy(AsyncOperationSPtr const & thisSPtr)
    {
        bool cleanupFirewallPolicy = false;

        if (HostingConfig::GetConfig().FirewallPolicyEnabled &&
            clearFirewallPolicy_)
        {
            cleanupFirewallPolicy = true;
        }

        vector<EndpointCertificateBinding> endpointCertBindings;
        auto error = owner_.GetEndpointBindingPolicies(packageInstanceEnvironmentContext->Endpoints, servicePackageDescription_.DigestedResources.DigestedCertificates, endpointCertBindings);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Error getting EndpointBindingPolicies {0}",
                error);
        }

        auto operation = owner_.Hosting.FabricActivatorClientObj->BeginConfigureEndpointCertificateAndFirewallPolicy(
            EnvironmentManager::GetServicePackageIdentifier(packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(), instanceId_),
            endpointCertBindings,
            true,
            cleanupFirewallPolicy,
            vector<LONG>(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
         {
            this->OnConfigureEndpointBindingsAndFirewallPolicyCompleted(operation, false);
         },
            thisSPtr);
         this->OnConfigureEndpointBindingsAndFirewallPolicyCompleted(operation, true);
    }

    void OnConfigureEndpointBindingsAndFirewallPolicyCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndConfigureEndpointCertificateAndFirewallPolicy(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "EndConfigureEndpointBinding:  ServicePackageInstanceId={0} ErrorCode={1}.",
                packageInstanceEnvironmentContext->ServicePackageInstanceId,
                error);
            lastError_.Overwrite(error);
        }

        CleanupContainerCertificates(operation->Parent);
    }

    void RemoveResourceFile(AsyncOperationSPtr thisSPtr)
    {
        ImageModel::RunLayoutSpecification runLayout(owner_.Hosting.DeploymentFolder);

        auto svcPkgInstanceId = packageInstanceEnvironmentContext->ServicePackageInstanceId;
        auto endpointFileName = runLayout.GetEndpointDescriptionsFile(
            svcPkgInstanceId.ApplicationId.ToString(),
            svcPkgInstanceId.ServicePackageName,
            svcPkgInstanceId.PublicActivationId);

        auto error = File::Delete2(endpointFileName, true);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::FileNotFound))
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Failed to remove enpoint resource file={0}. Error={1}. NodeVersion={2}.",
                endpointFileName,
                error,
                owner_.Hosting.FabricNodeConfigObj.NodeVersion);

            lastError_.Overwrite(error);
        }
        else
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Successfully removed enpoint resource file={0}. Error={1}. NodeVersion={2}.",
                endpointFileName,
                error,
                owner_.Hosting.FabricNodeConfigObj.NodeVersion);
        }

        if (lastError_.IsSuccess())
        {
            packageInstanceEnvironmentContext->Reset();
        }

        TryComplete(thisSPtr, lastError_);
    }

    void CleanupContainerCertificates(AsyncOperationSPtr thisSPtr)
    {
        if (!packageInstanceEnvironmentContext->CertificatePaths.empty())
        {
            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginCleanupContainerCertificateExport(
                packageInstanceEnvironmentContext->CertificatePaths,
                packageInstanceEnvironmentContext->CertificatePasswordPaths,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnCleanupContainerCertificateExportCompleted(operation, false);
            },
                thisSPtr);
                this->OnCleanupContainerCertificateExportCompleted(operation, true);
        }
        else
        {
            this->RemoveResourceFile(thisSPtr);
        }
    }

    void OnCleanupContainerCertificateExportCompleted(AsyncOperationSPtr operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndCleanupContainerCertificateExport(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "CleanupContainerCertificateExport:  ServicePackageInstanceId={0} ErrorCode={1}.",
                packageInstanceEnvironmentContext->ServicePackageInstanceId,
                error);
            lastError_.Overwrite(error);
        }

        this->RemoveResourceFile(operation->Parent);
    }

private:
    EnvironmentManager & owner_;
    ServicePackageInstanceEnvironmentContextSPtr const packageInstanceEnvironmentContext;
    TimeoutHelper timeoutHelper_;
    ErrorCode lastError_;
    atomic_uint64 pendingOperations_;
    bool clearFirewallPolicy_;
    ServiceModel::ServicePackageDescription servicePackageDescription_;
    int64 instanceId_;
};

EnvironmentManager::EnvironmentManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    AsyncFabricComponent(),
    hosting_(hosting),
    logCollectionProvider_(),
    endpointProvider_(),
    crashDumpProvider_(),
    currentUserSid_(),
    isSystem_(false),
    isAdminUser_(false)
{
}

EnvironmentManager::~EnvironmentManager()
{
}

// ********************************************************************************************************************
// AsyncFabricComponent methods

AsyncOperationSPtr EnvironmentManager::OnBeginOpen(
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

ErrorCode EnvironmentManager::OnEndOpen(
    AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr EnvironmentManager::OnBeginClose(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode EnvironmentManager::OnEndClose(
    AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void EnvironmentManager::OnAbort()
{
    if (logCollectionProvider_)
    {
        logCollectionProvider_->Abort();
    }

    if (etwSessionProvider_)
    {
        etwSessionProvider_->Abort();
    }

    if (crashDumpProvider_)
    {
        crashDumpProvider_->Abort();
    }

    if (endpointProvider_)
    {
        ASSERT_IFNOT(
            HostingConfig::GetConfig().EndpointProviderEnabled,
            "Endpoint filtering disabled, endpoint provider shouldn't exist");
        endpointProvider_->Abort();
    }
}

// ****************************************************
// Application environment specific methods
// ****************************************************
AsyncOperationSPtr EnvironmentManager::BeginSetupApplicationEnvironment(
    ApplicationIdentifier const & applicationId,
    ApplicationPackageDescription const & appPackageDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SetupApplicationAsyncOperation>(
        *this,
        applicationId,
        appPackageDescription,
        timeout,
        callback,
        parent);
}

ErrorCode EnvironmentManager::EndSetupApplicationEnvironment(
    AsyncOperationSPtr const & asyncOperation,
    __out ApplicationEnvironmentContextSPtr & context)
{
    return SetupApplicationAsyncOperation::End(asyncOperation, context);
}

AsyncOperationSPtr EnvironmentManager::BeginCleanupApplicationEnvironment(
    ApplicationEnvironmentContextSPtr const & appEnvironmentContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CleanupApplicationAsyncOperation>(
        *this,
        appEnvironmentContext,
        timeout,
        callback,
        parent);
}

ErrorCode EnvironmentManager::EndCleanupApplicationEnvironment(
    AsyncOperationSPtr const & asyncOperation)
{
    return CleanupApplicationAsyncOperation::End(asyncOperation);
}

void EnvironmentManager::AbortApplicationEnvironment(ApplicationEnvironmentContextSPtr const & appEnvironmentContext)
{
    if (appEnvironmentContext == nullptr) { return;  }

    if (HostingConfig::GetConfig().RunAsPolicyEnabled)
    {
        if(appEnvironmentContext->PrincipalsContext)
        {
            hosting_.FabricActivatorClientObj->AbortApplicationEnvironment(appEnvironmentContext->ApplicationId.ToString());
        }
        else
        {
            WriteNoise(
                TraceEnvironmentManager,
                Root.TraceId,
                "No security principals setup for Application Id {0}, proceeding without cleanup",
                appEnvironmentContext->ApplicationId);
        }
    }

    // log collection provider does not have a way to cleanup without wait
    wstring const & path = appEnvironmentContext->LogCollectionPath;
    if (path.empty())
    {
        ASSERT_IFNOT(logCollectionProvider_, "Log collection provider should exist");

        WriteNoise(
            TraceEnvironmentManager,
            Root.TraceId,
            "{0}: logCollectionProvider_->RemoveLogPaths: path {1}",
            appEnvironmentContext->ApplicationId,
            path);

        logCollectionProvider_->CleanupLogPaths(path);
    }
}

// ****************************************************
// ServicePackage environment specific methods
// ****************************************************

AsyncOperationSPtr EnvironmentManager::BeginSetupServicePackageInstanceEnvironment(
    ApplicationEnvironmentContextSPtr const & appEnvironmentContext,
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    int64 instanceId,
    ServiceModel::ServicePackageDescription const & servicePackageDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<SetupServicePackageInstanceAsyncOperation>(
        *this,
        appEnvironmentContext,
        servicePackageInstanceId,
        move(servicePackageDescription),
        instanceId,
        timeout,
        callback,
        parent);
}

ErrorCode EnvironmentManager::EndSetupServicePackageInstanceEnvironment(
    AsyncOperationSPtr const & asyncOperation,
    __out ServicePackageInstanceEnvironmentContextSPtr & packageEnvironmentContext)
{
    return SetupServicePackageInstanceAsyncOperation::End(asyncOperation, packageEnvironmentContext);
}

AsyncOperationSPtr EnvironmentManager::BeginCleanupServicePackageInstanceEnvironment(
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
    ServiceModel::ServicePackageDescription const & packageDescription,
    int64 instanceId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CleanupServicePackageInstanceAsyncOperation>(
        *this,
        packageEnvironmentContext,
        packageDescription,
        instanceId,
        timeout,
        callback,
        parent);
}

ErrorCode EnvironmentManager::EndCleanupServicePackageInstanceEnvironment(
    AsyncOperationSPtr const & asyncOperation)
{
    return CleanupServicePackageInstanceAsyncOperation::End(asyncOperation);
}

void EnvironmentManager::AbortServicePackageInstanceEnvironment(
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
    ServiceModel::ServicePackageDescription const & packageDescription,
    int64 instanceId)
{
    auto error = CleanupServicePackageInstanceEnvironment(
        packageEnvironmentContext,
        packageDescription,
        instanceId);

    error.ReadValue(); // ignore error in abort path

    // crash dump provider does not have a way to cleanup without wait
    ASSERT_IFNOT(crashDumpProvider_, "Crash dump provider should exist");
    crashDumpProvider_->CleanupServiceCrashDumps(packageEnvironmentContext);
}

ErrorCode EnvironmentManager::CleanupServicePackageInstanceEnvironment(
    ServicePackageInstanceEnvironmentContextSPtr const & packageEnvironmentContext,
    ServicePackageDescription const & packageDescription,
    int64)
{
    if (!packageEnvironmentContext)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode lastError(ErrorCodeValue::Success);
    if (HostingConfig::GetConfig().EndpointProviderEnabled)
    {
        ASSERT_IFNOT(endpointProvider_, "Endpoint provider must not be null as EndpointFiltering is enabled");

        bool cleanupFirewallPolicy = false;

        for(auto iter = packageEnvironmentContext->Endpoints.begin();
            iter != packageEnvironmentContext->Endpoints.end();
            ++iter)
        {
            EndpointResourceSPtr endpointResource = *iter;
            SecurityPrincipalInformationSPtr principalInfo;
            auto error = EndpointProvider::GetPrincipal(endpointResource, principalInfo);
            if(error.IsSuccess())
            {
                if (EndpointProvider::IsHttpEndpoint(endpointResource))
                {
                    error = this->hosting_.FabricActivatorClientObj->ConfigureEndpointSecurity(
                        principalInfo->SidString,
                        endpointResource->Port,
                        endpointResource->Protocol == ServiceModel::ProtocolType::Enum::Https,
                        true,
                        packageEnvironmentContext->ServicePackageInstanceId.ToString(),
                        endpointResource->EndpointDescriptionObj.ExplicitPortSpecified);

                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceEnvironmentManager,
                            Root.TraceId,
                            "Remove EndpointSecurity failed. Endpoint={0}, ErrorCode={1}. Continuing to remove other endpoints.",
                            endpointResource->Name,
                            error);
                    }             
                }

                if (endpointResource->EndpointDescriptionObj.ExplicitPortSpecified && HostingConfig::GetConfig().FirewallPolicyEnabled)
                {
                    cleanupFirewallPolicy = true;
                }
            }
            else
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    Root.TraceId,
                    "Unable to retrieve PrincipalInfo for endpoint. ErrorCode={1}. Continuing to remove other endpoints.",
                    endpointResource->Name,
                    error);
            }
            error = endpointProvider_->RemoveEndpoint(*iter);
            if(!error.IsSuccess())
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    Root.TraceId,
                    "RemoveEndpoint failed. ServicePackageInstanceId={0}, Endpoint={1}, ErrorCode={2}. Continuing to remove other endpoints.",
                    packageEnvironmentContext->ServicePackageInstanceId,
                    (*iter)->Name,
                    error);
                lastError.Overwrite(error);
            }
        }
        vector<EndpointCertificateBinding> endpointCertBindings;
        auto error = GetEndpointBindingPolicies(packageEnvironmentContext->Endpoints, packageDescription.DigestedResources.DigestedCertificates, endpointCertBindings);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                Root.TraceId,
                "Error getting EndpointBindingPolicies {0}",
                error);
        }

        if (!endpointCertBindings.empty() || cleanupFirewallPolicy)
        {
            error = Hosting.FabricActivatorClientObj->ConfigureEndpointBindingAndFirewallPolicy(
                packageEnvironmentContext->ServicePackageInstanceId.ToString(),
                endpointCertBindings,
                true,
                cleanupFirewallPolicy,
                vector<LONG>());

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    Root.TraceId,
                    "Error in ConfigureEndpointBindingAndFirewallPolicy {0}",
                    error);
            }
        }
        if (packageEnvironmentContext->SetupContainerGroup)
        {
           Hosting.FabricActivatorClientObj->AbortProcess(packageEnvironmentContext->ServicePackageInstanceId.ToString());
        }
        if (packageEnvironmentContext->HasIpsAssigned)
        {
            error = Hosting.FabricActivatorClientObj->CleanupAssignedIPs(packageEnvironmentContext->ServicePackageInstanceId.ToString());
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    Root.TraceId,
                    "Error in CleanupAssignedIps {0}",
                    error);
            }
        }
        else
        {
            WriteNoise(TraceEnvironmentManager,
                Root.TraceId,
                "No assigned IPs for {0}",
                packageEnvironmentContext->ServicePackageInstanceId.ToString());
        }
    }

#if !defined(PLATFORM_UNIX)
    ASSERT_IFNOT(etwSessionProvider_, "ETW session provider should exist");
    etwSessionProvider_->CleanupServiceEtw(packageEnvironmentContext);
#endif
    packageEnvironmentContext->Reset();

    return lastError;
}

ErrorCode EnvironmentManager::GetEndpointBindingPolicies(
    vector<EndpointResourceSPtr> const & endpointResources,
    map<wstring, EndpointCertificateDescription> digestedCertificates,
    vector<EndpointCertificateBinding> & bindings)
{
    for (auto endpointIter = endpointResources.begin();
        endpointIter != endpointResources.end();
        ++endpointIter)
    {
        EndpointResourceSPtr endPointResource = *endpointIter;
        wstring principalSid;

        EnvironmentResource::ResourceAccess resourceAccess;
        auto error = endPointResource->GetDefaultSecurityAccess(resourceAccess);
        if (error.IsSuccess())
        {
            principalSid = resourceAccess.PrincipalInfo->SidString;
        }
        else if (!error.IsError(ErrorCodeValue::NotFound))
        {
            WriteWarning(
                TraceEnvironmentManager,
                Root.TraceId,
                "Error getting default security access for endpointResource '{0}'.Error={1}",
                endPointResource->Name,
                error);
            return error;
        }
        if (!endPointResource->EndPointBindingPolicy.CertificateRef.empty())
        {
            auto it = digestedCertificates.find(endPointResource->EndPointBindingPolicy.CertificateRef);
            if (it == digestedCertificates.end())
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    Root.TraceId,
                    "Error getting the certificate '{0}' for endpoint {1}",
                    endPointResource->EndPointBindingPolicy.CertificateRef,
                    endPointResource->Name);

                return ErrorCodeValue::NotFound;
            }

            EndpointCertificateBinding binding(
                endPointResource->Port,
                endPointResource->EndpointDescriptionObj.ExplicitPortSpecified,
                principalSid,
                it->second.X509FindValue,
                it->second.X509StoreName,
                it->second.X509FindType);
            bindings.push_back(binding);
        }
    }
    return ErrorCodeValue::Success;
}

std::wstring EnvironmentManager::GetServicePackageIdentifier(std::wstring const & servicePackageId, int64 instanceId)
{
    return wformatString("{0}:{1}", servicePackageId, instanceId);
}
