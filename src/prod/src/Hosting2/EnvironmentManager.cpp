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
    ApplicationEnvironmentContextSPtr const appEnvironmentContext_;
    TimeoutHelper timeoutHelper_;
    ErrorCode lastError_;
};

ErrorCode EnvironmentManager::GetIsolatedNicIpAddress(std::wstring & ipAddress)
{
    ErrorCode error(ErrorCodeValue::Success);
    ipAddress = hosting_.FabricNodeConfigObj.IPAddressOrFQDN;

    std::wstring isolatedNetworkInterfaceName;
    error = FabricEnvironment::GetFabricIsolatedNetworkInterfaceName(isolatedNetworkInterfaceName);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceEnvironmentManager,
            Root.TraceId,
            "GetFabricIsolatedNetworkInterfaceName failed: ErrorCode={0}",
            error);

        return error;
    }

    auto ipInterface = wformatString("({0})", isolatedNetworkInterfaceName);

    wstring ipAddressOnAdapter;
    error = IpUtility::GetIpAddressOnAdapter(ipInterface, AF_INET, ipAddressOnAdapter);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceEnvironmentManager,
            Root.TraceId,
            "GetIpAddressOnAdapter failed: ErrorCode={0}",
            error);

        // Linux primary interface will not have an ip on it,
        // if open network is set up. The host ip is being used in this case
        // and we return success error code.
#if defined(PLATFORM_UNIX)
            WriteInfo(
                TraceEnvironmentManager,
                Root.TraceId,
                "GetIsolatedNicIpAddress: Using IP {0} for interface [{1}]",
                ipAddress, 
                isolatedNetworkInterfaceName);

            return ErrorCodeValue::Success;
#endif
        return error;
    }

    if (!ipAddressOnAdapter.empty())
    {
        ipAddress = ipAddressOnAdapter;
    }

    WriteInfo(
        TraceEnvironmentManager,
        Root.TraceId,
        "GetFabricIsolatedNetworkInterfaceName: found IP {0} for interface [{1}]",
        ipAddress, 
        isolatedNetworkInterfaceName);

    return error;
}

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
        wstring const & applicationName,
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        ServiceModel::ServicePackageDescription const & servicePackageDescription,
        int64 instanceId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        appEnvironmentContext_(appEnvironmentContext),
        applicationName_(applicationName),
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
        endpointsWithACL_(),
        runLayout_(owner_.Hosting.DeploymentFolder),
        codePackageDescriptionsWithNetwork_(),
        servicePackageEnvironmentId_(EnvironmentManager::GetServicePackageIdentifier(servicePackageInstanceId_.ToString(), instanceId_))
    {
        packageInstanceEnvironmentContext = make_shared<ServicePackageInstanceEnvironmentContext>(servicePackageInstanceId_);
        packageInstanceEnvironmentContext->AddNetworks(NetworkType::Enum::Isolated, GetIsolatedNetworks());
    }

    virtual ~SetupServicePackageInstanceAsyncOperation()
    {
    }

    static ErrorCode SetupServicePackageInstanceAsyncOperation::End(
        AsyncOperationSPtr const & operation,
        __out ServicePackageInstanceEnvironmentContextSPtr & packageEnvironmentContext)
    {
        auto thisPtr = AsyncOperation::End<SetupServicePackageInstanceAsyncOperation>(operation);

        packageEnvironmentContext = move(thisPtr->packageInstanceEnvironmentContext);

        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SetupEnvironment(thisSPtr);
    }

#if defined(PLATFORM_UNIX)
    bool ContainerGroupIsolated()
    {
        ContainerIsolationMode::Enum isolationMode = ContainerIsolationMode::process;
        ContainerIsolationMode::FromString(this->servicePackageDescription_.ContainerPolicyDescription.Isolation, isolationMode);
        return (isolationMode == ContainerIsolationMode::hyperv);
    }
#endif

private:
    void SetupEnvironment(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "SetupServicePackage: Id={0}, RunAsPolicyEnabled={1}, EndpointProviderEnabled={2}",
            servicePackageEnvironmentId_,
            HostingConfig::GetConfig().RunAsPolicyEnabled,
            HostingConfig::GetConfig().EndpointProviderEnabled);

        SetupDiagnostics(thisSPtr);
    }

    std::vector<std::wstring> GetIsolatedNetworks()
    {
        std::vector<std::wstring> isolatedNetworks = servicePackageDescription_.GetNetworks(NetworkType::IsolatedStr);

        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "Isolated networks retrieved for {0} exclusive mode {1} count {2}.",
            servicePackageInstanceId_,
            this->servicePackageInstanceId_.ActivationContext.IsExclusive,
            isolatedNetworks.size());

        return isolatedNetworks;
    }

    void PopulateCodePackageNetworkConfig()
    {
        // check service package level network policies
        NetworkType::Enum networkType = NetworkType::Enum::Other;
        for (auto const & cnp : servicePackageDescription_.NetworkPolicies.ContainerNetworkPolicies)
        {
            if (StringUtility::CompareCaseInsensitive(cnp.NetworkRef, NetworkType::OpenStr) != 0 &&
                StringUtility::CompareCaseInsensitive(cnp.NetworkRef, NetworkType::OtherStr) != 0)
            {
                networkType = networkType | NetworkType::Enum::Isolated;
            }
            else if (StringUtility::CompareCaseInsensitive(cnp.NetworkRef, NetworkType::OpenStr) == 0)
            {
                networkType = networkType | NetworkType::Enum::Open;
            }
        }

        // populate code package network configs
        for (auto it = servicePackageDescription_.DigestedCodePackages.begin(); it != servicePackageDescription_.DigestedCodePackages.end(); ++it)
        {
            it->ContainerPolicies.NetworkConfig.Type = it->ContainerPolicies.NetworkConfig.Type | networkType;
        }
    }

    void AssignResources(AsyncOperationSPtr const & thisSPtr)
    {
        int containerCount = 0;

#if defined(PLATFORM_UNIX)
        bool isolated = servicePackageDescription_.ContainerPolicyDescription.Isolation
                           == ContainerIsolationMode::EnumToString(ContainerIsolationMode::hyperv);
        WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "SetupServicePackage: Id={0}, Isolation={1}",
                servicePackageEnvironmentId_, servicePackageDescription_.ContainerPolicyDescription.Isolation);
#endif

        for (auto it = servicePackageDescription_.DigestedCodePackages.begin(); it != servicePackageDescription_.DigestedCodePackages.end(); ++it)
        {
            if (it->CodePackage.EntryPoint.EntryPointType == EntryPointType::ContainerHost)
            {
                containerCount++;
            }

            if ((it->ContainerPolicies.NetworkConfig.Type & NetworkType::Enum::Open) == NetworkType::Enum::Open ||
                (packageInstanceEnvironmentContext->NetworkExists(NetworkType::Enum::Isolated)))
            {
#if defined(PLATFORM_UNIX)
                if (codePackageDescriptionsWithNetwork_.empty())
                {
                    codePackageDescriptionsWithNetwork_.push_back(*it);
                }
#else
                codePackageDescriptionsWithNetwork_.push_back(*it);
#endif
            }
        }

#if defined(PLATFORM_UNIX)
        if (containerCount > 1 || ContainerGroupIsolated())
        {
            setupContainerGroup_ = true;
        }
#endif
        if (codePackageDescriptionsWithNetwork_.empty())
        {
            if (setupContainerGroup_)
            {
#if defined(PLATFORM_UNIX)
                if (ContainerGroupIsolated())
                {
                    SetupEndpoints(thisSPtr);
                    return;
                }
#endif
                SetupContainerGroup(thisSPtr, ServiceModel::NetworkType::Other, L"", std::map<std::wstring, std::wstring>());
            }
            else
            {
                SetupEndpoints(thisSPtr);
            }
        }
        else
        {
            // Assign open network resources
            BeginAssignIpAddress(thisSPtr);
        }
    }

    void BeginAssignIpAddress(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> codePackages;
        for (auto const & cp : codePackageDescriptionsWithNetwork_)
        {
            if ((cp.ContainerPolicies.NetworkConfig.Type & NetworkType::Enum::Open) == NetworkType::Enum::Open)
            {
                codePackages.push_back(cp.Name);
            }
        }

        bool codePackagesNeedIpsAllocated = (codePackages.size() > 0);
        if (codePackagesNeedIpsAllocated)
        {
            std::vector<std::wstring> networkNames;
            if (HostingConfig::GetConfig().LocalNatIpProviderEnabled)
            {
                networkNames.push_back(HostingConfig::GetConfig().LocalNatIpProviderNetworkName);
            }
            else
            {
                networkNames.push_back(Common::ContainerEnvironment::ContainerNetworkName);
            }
            packageInstanceEnvironmentContext->AddNetworks(NetworkType::Enum::Open, networkNames);
        }

        auto operation = owner_.Hosting.FabricActivatorClientObj->BeginAssignIpAddresses(
            this->servicePackageInstanceId_.ToString(),
            codePackages,
            false,
            timeoutHelper_.GetRemainingTime(),
            [this, codePackagesNeedIpsAllocated](AsyncOperationSPtr const & operation)
        {
            this->OnAssignIpAddressCompleted(operation, false, codePackagesNeedIpsAllocated);
        },
            thisSPtr);
        this->OnAssignIpAddressCompleted(operation, true, codePackagesNeedIpsAllocated);
    }

    void OnAssignIpAddressCompleted(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously, 
        bool codePackagesNeedIpsAllocated)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        vector<wstring> assignedIps;
        auto error = owner_.Hosting.FabricActivatorClientObj->EndAssignIpAddresses(operation, assignedIps);
        // Invoke clean up only if the request for open network resources resulted in error
        if (!error.IsSuccess() && codePackagesNeedIpsAllocated)
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End BeginAssignIpAddress. Error {1}",
                servicePackageEnvironmentId_,
                error);

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
        
        // Assign overlay network resources
        BeginAssignOverlayNetworkResources(operation->Parent);
    }

    void BeginAssignOverlayNetworkResources(AsyncOperationSPtr const & thisSPtr)
    {
        std::map<std::wstring, std::vector<std::wstring>> codePackageNetworkNames;
        if (packageInstanceEnvironmentContext->NetworkExists(NetworkType::Enum::Isolated))
        {
            std::vector<std::wstring> networkNames;
            packageInstanceEnvironmentContext->GetNetworks(NetworkType::Enum::Isolated, networkNames);
            for (auto const & cp : codePackageDescriptionsWithNetwork_)
            {
                codePackageNetworkNames.insert(make_pair(cp.Name, networkNames));
            }
        }

        bool codePackagesNeedOverlayNetworkResourcesAllocated = (codePackageNetworkNames.size() > 0);

        wstring nodeIpAddress = L"";
        if (codePackagesNeedOverlayNetworkResourcesAllocated)
        {
            auto error = owner_.GetIsolatedNicIpAddress(nodeIpAddress);
            if (!error.IsSuccess())
            {
                lastError_.Overwrite(error);
                CleanupOnError(thisSPtr, lastError_);
                return;
            }
        }

        auto operation = owner_.Hosting.FabricActivatorClientObj->BeginManageOverlayNetworkResources(
            owner_.hosting_.NodeName,
            nodeIpAddress,
            this->servicePackageInstanceId_.ToString(),
            codePackageNetworkNames,
            ManageOverlayNetworkAction::Assign,
            timeoutHelper_.GetRemainingTime(),
            [this, codePackagesNeedOverlayNetworkResourcesAllocated](AsyncOperationSPtr const & operation)
        {
            this->OnAssignOverlayNetworkResourcesCompleted(operation, false, codePackagesNeedOverlayNetworkResourcesAllocated);
        },
            thisSPtr);
        this->OnAssignOverlayNetworkResourcesCompleted(operation, true, codePackagesNeedOverlayNetworkResourcesAllocated);
    }

    void OnAssignOverlayNetworkResourcesCompleted(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously,
        bool codePackagesNeedOverlayNetworkResourcesAllocated)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        std::map<wstring, std::map<std::wstring, std::wstring>> assignedOverlayNetworkResources;
        auto error = owner_.Hosting.FabricActivatorClientObj->EndManageOverlayNetworkResources(operation, assignedOverlayNetworkResources);
        // Invoke clean up only if the request for overlay network resources resulted in error
        if (!error.IsSuccess() && codePackagesNeedOverlayNetworkResourcesAllocated)
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End BeginAssignOverlayNetworkResources. Error {1}",
                servicePackageEnvironmentId_,
                error);

            lastError_.Overwrite(error);
            CleanupOnError(operation->Parent, lastError_);
            return;
        }

        for (auto it = assignedOverlayNetworkResources.begin(); it != assignedOverlayNetworkResources.end(); ++it)
        {
            packageInstanceEnvironmentContext->AddAssignedOverlayNetworkResources(it->first, it->second);
        }

        if (setupContainerGroup_)
        {
            std::wstring openNetworkAssignedIp;
            std::map<std::wstring, std::wstring> overlayNetworkResources;
            NetworkType::Enum networkType = NetworkType::Enum::Other;
            GetGroupAssignedNetworkResource(openNetworkAssignedIp, overlayNetworkResources, networkType);

#if defined(PLATFORM_UNIX)
            if (ContainerGroupIsolated())
            {
                SetupEndpoints(operation->Parent);
                return;
            }
#endif
            SetupContainerGroup(operation->Parent, networkType, openNetworkAssignedIp, overlayNetworkResources);
        }
        else
        {
            SetupEndpoints(operation->Parent);
        }
    }

    void GetGroupAssignedNetworkResource(
        std::wstring & openNetworkAssignedIp,
        std::map<std::wstring, std::wstring> & overlayNetworkResources,
        ServiceModel::NetworkType::Enum & networkType)
    {
        wstring networkResourceList;
        packageInstanceEnvironmentContext->GetGroupAssignedNetworkResource(openNetworkAssignedIp, overlayNetworkResources, networkType, networkResourceList);

        WriteNoise(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "Id={0}, Container Group Network Type={1}, Network Resource List={2}",
            packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
            networkType,
            networkResourceList);
    }

    void SetupContainerGroup(
        AsyncOperationSPtr const & thisSPtr,
        ServiceModel::NetworkType::Enum networkType,
        wstring const & openNetworkAssignedIp,
        std::map<std::wstring, std::wstring> const & overlayNetworkResources)
    {
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Start SetupContainerGroup.",
            servicePackageEnvironmentId_);

        vector<wstring> dnsServers;
#if defined(PLATFORM_UNIX)
        std::vector<PodPortMapping> portMappings;
        if (ContainerGroupIsolated())
        {
            for (auto pb : this->servicePackageDescription_.ContainerPolicyDescription.PortBindings)
            {
                if (this->servicePackageDescription_.DigestedResources.DigestedEndpoints.find(pb.EndpointResourceRef)
                    != this->servicePackageDescription_.DigestedResources.DigestedEndpoints.end())
                {
                    EndpointDescription const &ep = this->servicePackageDescription_.DigestedResources.DigestedEndpoints.at(pb.EndpointResourceRef).Endpoint;
                    int port = ep.Port;
                    if (port == 0)
                    {
                        for (auto iep : packageInstanceEnvironmentContext->Endpoints)
                        {
                            if (iep->Name == ep.Name)
                            {
                                port = iep->Port; break;
                            }
                        }
                    }
                    portMappings.emplace_back(
                            owner_.Hosting.FabricNodeConfigObj.IPAddressOrFQDN,
                            ep.Protocol == ProtocolType::Udp ? PodPortMapping::UDP : PodPortMapping::TCP,
                            pb.ContainerPort,
                            port);
                }
            }
        }

        packageInstanceEnvironmentContext->SetContainerGroupIsolatedFlag(ContainerGroupIsolated());

        // If both open and isolated network resources are empty, then NAT port mappings are passed to container group set up.
        ContainerPodDescription podDescription(
            this->servicePackageDescription_.ContainerPolicyDescription.Hostname,
            ContainerGroupIsolated() ? ContainerIsolationMode::hyperv : ContainerIsolationMode::process,
            (openNetworkAssignedIp.empty() && overlayNetworkResources.empty()) ? portMappings : std::vector<PodPortMapping>());

        // For Isolated and NAT, we end up using the constant defined here - ClearContainerHelper::ClearContainerLogHelperConstants::DnsServiceIP4Address
        // For MIP, we find the vm host ip.
        if ((networkType & NetworkType::Enum::Open) == NetworkType::Enum::Open && !openNetworkAssignedIp.empty())
        {
            std::wstring dnsServerIP;
            auto err = Common::IpUtility::GetAdapterAddressOnTheSameNetwork(openNetworkAssignedIp, /*out*/dnsServerIP);
            if (err.IsSuccess())
            {
                dnsServers.push_back(dnsServerIP);
            }
            else
            {
                WriteWarning(
                    TraceEnvironmentManager, 
                    owner_.Root.TraceId,
                    "Failed to get DNS IP address for container with assigned IP={0} : ErrorCode={1}",
                    openNetworkAssignedIp, err);
            }
        }
#endif

        packageInstanceEnvironmentContext->SetContainerGroupSetupFlag(true);
        auto op = owner_.Hosting.FabricActivatorClientObj->BeginSetupContainerGroup(
            this->servicePackageInstanceId_.ToString(),
            networkType,
            openNetworkAssignedIp,
            overlayNetworkResources,
            dnsServers,
            owner_.Hosting.RunLayout.GetApplicationFolder(appEnvironmentContext_->ApplicationId.ToString()),
            StringUtility::ToWString(appEnvironmentContext_->ApplicationId.ApplicationNumber),
            this->applicationName_,
            this->servicePackageInstanceId_.ActivationContext.ToString(),
            this->servicePackageInstanceId_.PublicActivationId,
            this->servicePackageDescription_.ResourceGovernanceDescription,
#if defined(PLATFORM_UNIX)
            podDescription,
#endif
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
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End SetupContainerGroup. Error{1}",
                servicePackageEnvironmentId_,
                error);

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

#if defined(PLATFORM_UNIX)
        if (ContainerGroupIsolated())
        {
            TryComplete(operation->Parent, error);
            return;
        }
#endif
        SetupEndpoints(operation->Parent);
    }

    void SetupEndpoints(AsyncOperationSPtr const & thisSPtr)
    {
        if (HostingConfig::GetConfig().EndpointProviderEnabled)
        {
            ASSERT_IFNOT(owner_.endpointProvider_, "Endpoint provider should exist");

            auto endpointIsolatedNetworkMap = servicePackageDescription_.GetEndpointNetworkMap(NetworkType::IsolatedStr);

            for (auto endpointIter = servicePackageDescription_.DigestedResources.DigestedEndpoints.begin();
                endpointIter != servicePackageDescription_.DigestedResources.DigestedEndpoints.end();
                ++endpointIter)
            {
                EndpointResourceSPtr endpointResource = make_shared<EndpointResource>(
                    endpointIter->second.Endpoint,
                    endpointIter->second.EndpointBindingPolicy);

                auto error = SetEndPointResourceIpAddress(endpointResource, endpointIsolatedNetworkMap);
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

                if (!endpointIter->second.SecurityAccessPolicy.ResourceRef.empty())
                {
                    ASSERT_IFNOT(
                        StringUtility::AreEqualCaseInsensitive(endpointIter->second.SecurityAccessPolicy.ResourceRef, endpointIter->second.Endpoint.Name),
                        "Error in the DigestedEndpoint element. The ResourceRef name '{0}' in SecurityAccessPolicy does not match the resource name '{1}'.",
                        endpointIter->second.SecurityAccessPolicy.ResourceRef,
                        endpointIter->second.Endpoint.Name);

                    SecurityPrincipalInformationSPtr principalInfo;
                    error = appEnvironmentContext_->PrincipalsContext->TryGetPrincipalInfo(endpointIter->second.SecurityAccessPolicy.PrincipalRef, principalInfo);
                    if (!error.IsSuccess())
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
                        "ExplicitPort specified for Endpoint '{0}' Port {1}. ServicePackageInstanceId={2} Network Enabled={3} CodePackageRef={4}",
                        endpointIter->second.Endpoint.Name,
                        endpointIter->second.Endpoint.Port,
                        servicePackageInstanceId_,
                        !codePackageDescriptionsWithNetwork_.empty(),
                        endpointResource->EndpointDescriptionObj.CodePackageRef);

                    if (HostingConfig::GetConfig().FirewallPolicyEnabled && codePackageDescriptionsWithNetwork_.empty())
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
                if (EndpointProvider::IsHttpEndpoint(endpointResource))
                {
                    EnvironmentResource::ResourceAccess resourceAccess;
                    error = endpointResource->GetDefaultSecurityAccess(resourceAccess);
                    if (error.IsSuccess())
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
                    servicePackageEnvironmentId_,
                    servicePackageDescription_.DigestedResources.DigestedEndpoints.size());
                auto error = ErrorCode(ErrorCodeValue::EndpointProviderNotEnabled);
                error.ReadValue();
                CleanupOnError(thisSPtr, error);
                return;
            }

        }
        if (endpointsWithACL_.size() > 0)
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
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "Start ConfigureEndpointSecurity for servicepackage {0}",
            servicePackageEnvironmentId_);

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

    ErrorCode SetEndPointResourceIpAddress(EndpointResourceSPtr const & endpointResource, std::map<std::wstring, std::wstring> endpointIsolatedNetworkMap)
    {
        auto codePackageRef = endpointResource->EndpointDescriptionObj.CodePackageRef;
        auto nodeIpAddress = owner_.Hosting.FabricNodeConfigObj.IPAddressOrFQDN;

        if (codePackageRef.empty())
        {
            endpointResource->IpAddressOrFqdn = nodeIpAddress;

            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint resource '{0}' has no CodePackageRef. Assigning NodeIPAddress={1}.",
                endpointResource->Name,
                nodeIpAddress);

            return ErrorCodeValue::Success;
        }

        wstring openNetworkAssignedIpAddress;
        std::map<wstring, wstring> overlayNetworkResources;
        auto error = packageInstanceEnvironmentContext->GetNetworkResourceAssignedToCodePackage(
            codePackageRef, 
            openNetworkAssignedIpAddress, 
            overlayNetworkResources);

        if (error.IsSuccess() && !overlayNetworkResources.empty())
        {
            auto endpointIter = endpointIsolatedNetworkMap.find(endpointResource->Name);
            if (endpointIter != endpointIsolatedNetworkMap.end())
            {
                wstring isolatedNetworkName = endpointIter->second;
                auto onrIter = overlayNetworkResources.find(isolatedNetworkName);
                if (onrIter != overlayNetworkResources.end())
                {
                    wstring overlayNetworkAssignedIpAddress;
                    wstring overlayNetworkAssignedMacAddress;
                    StringUtility::SplitOnce(onrIter->second, overlayNetworkAssignedIpAddress, overlayNetworkAssignedMacAddress, L',');

                    if (!overlayNetworkAssignedIpAddress.empty())
                    {
                        endpointResource->IpAddressOrFqdn = overlayNetworkAssignedIpAddress;

                        WriteNoise(
                            TraceEnvironmentManager,
                            owner_.Root.TraceId,
                            "Endpoint resource '{0}' has CodePackageRef='{1}'. Found assigned IPAddress={2}.",
                            endpointResource->Name,
                            codePackageRef,
                            overlayNetworkAssignedIpAddress);

                        return error;
                    }
                }
            }
        }

        if (!openNetworkAssignedIpAddress.empty())
        {
            endpointResource->IpAddressOrFqdn = openNetworkAssignedIpAddress;

            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint resource '{0}' has CodePackageRef='{1}'. Found assigned IPAddress={2}.",
                endpointResource->Name,
                codePackageRef,
                openNetworkAssignedIpAddress);

            return error;
        }

        if (error.IsError(ErrorCodeValue::NotFound) || endpointResource->IpAddressOrFqdn.empty())
        {
            endpointResource->IpAddressOrFqdn = nodeIpAddress;

            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Endpoint resource '{0}' has CodePackageRef='{1}'. Assigned IPAddress not found. Assigning NodeIPAddress={2}.",
                endpointResource->Name,
                codePackageRef,
                nodeIpAddress);

            return ErrorCodeValue::Success;
        }

        return error;
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
            WriteTrace(
                lastError_.ToLogLevel(),
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End ConfigureEndpointSecurity. lastError {1}",
                servicePackageEnvironmentId_,
                lastError_);

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
        if (!firewallPortsToOpen_.empty())
        {
            packageInstanceEnvironmentContext->FirewallPorts = firewallPortsToOpen_;
        }

        vector<EndpointCertificateBinding> endpointCertBindings;
        owner_.GetEndpointBindingPolicies(
            packageInstanceEnvironmentContext->Endpoints,
            servicePackageDescription_.DigestedResources.DigestedCertificates,
            endpointCertBindings);

        if (!endpointCertBindings.empty() || !packageInstanceEnvironmentContext->FirewallPorts.empty())
        {
            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Start ConfigureEndpointBindingAndFirewallPolicy",
                servicePackageEnvironmentId_);

            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginConfigureEndpointCertificateAndFirewallPolicy(
                EnvironmentManager::GetServicePackageIdentifier(servicePackageInstanceId_.ToString(), instanceId_),
                endpointCertBindings,
                false,
                false,
                packageInstanceEnvironmentContext->FirewallPorts,
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
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End ConfigureEndpointBindingAndFirewallPolicy. Error {1}",
                servicePackageEnvironmentId_,
                error);
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
            wstring applicationWorkFolder = runLayout_.GetApplicationWorkFolder(servicePackageInstanceId_.ApplicationId.ToString());
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

            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Start ConfigureContainerCertificateExport.",
                servicePackageEnvironmentId_);

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
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End ConfigureContainerCertificateExport. Error {1}",
                servicePackageEnvironmentId_,
                error);
            CleanupOnError(operation->Parent, error);
            return;
        }

        packageInstanceEnvironmentContext->AddCertificatePaths(certificatePaths, certificatePasswordPaths);
        WriteResources(operation->Parent);
    }

    void WriteResources(AsyncOperationSPtr const & thisSPtr)
    {
        // Write the endpoint file
        wstring endpointFileName = runLayout_.GetEndpointDescriptionsFile(
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
                servicePackageEnvironmentId_,
                owner_.Hosting.FabricNodeConfigObj.NodeVersion);

#if defined(PLATFORM_UNIX)
            if (ContainerGroupIsolated())
            {
                std::wstring openNetworkAssignedIp;
                std::map<std::wstring, std::wstring> overlayNetworkResources;
                NetworkType::Enum networkType = NetworkType::Enum::Other;
                GetGroupAssignedNetworkResource(openNetworkAssignedIp, overlayNetworkResources, networkType);

                SetupContainerGroup(thisSPtr, networkType, openNetworkAssignedIp, overlayNetworkResources);
                return;
            }
#endif
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
                servicePackageEnvironmentId_,
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
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End SetupServiceEtw. Error {1}",
                servicePackageEnvironmentId_,
                error);
            CleanupOnError(thisSPtr, error);
            return;
        }
#endif
        ASSERT_IFNOT(owner_.crashDumpProvider_, "Crash dump provider should exist");
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Start BeginSetupServiceCrashDumps.",
            servicePackageEnvironmentId_);
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
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End SetupServiceCrashDumps. Error {1}",
                servicePackageEnvironmentId_,
                error);
            CleanupOnError(operation->Parent, error);
            return;
        }

        // translate service package network policies to 
        // code package network config
        PopulateCodePackageNetworkConfig();

        AssignResources(operation->Parent);
    }

    void CleanupOnError(AsyncOperationSPtr const & thisSPtr, ErrorCode const error)
    {
        // Call cleanup, best effort
        lastError_.Overwrite(error);
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Begin(Setup->BeginCleanupServicePackageEnvironment due to error {1})",
            servicePackageEnvironmentId_,
            error);

        auto operation = owner_.BeginCleanupServicePackageInstanceEnvironment(
            packageInstanceEnvironmentContext,
            servicePackageDescription_,
            instanceId_,
            timeoutHelper_.OriginalTimeout,
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
            servicePackageEnvironmentId_,
            lastError_,
            error);

        // Complete with the saved error
        TryComplete(operation->Parent, lastError_);
    }

private:
    EnvironmentManager & owner_;
    ApplicationEnvironmentContextSPtr const appEnvironmentContext_;
    wstring const applicationName_;
    ServicePackageInstanceIdentifier const servicePackageInstanceId_;
    ServiceModel::ServicePackageDescription servicePackageDescription_;
    int64 instanceId_;
    TimeoutHelper timeoutHelper_;
    ServicePackageInstanceEnvironmentContextSPtr packageInstanceEnvironmentContext;
    bool setupContainerGroup_;
    vector<EndpointDescription> endpoints_;
    vector<LONG> firewallPortsToOpen_;
    atomic_uint64 endpointsAclCount_;
    ErrorCode lastError_;
    vector<EndpointResourceSPtr> endpointsWithACL_;
    ImageModel::RunLayoutSpecification runLayout_;
    vector<DigestedCodePackageDescription> codePackageDescriptionsWithNetwork_;
    wstring servicePackageEnvironmentId_;
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
        lastError_(ErrorCodeValue::Success),
        servicePackageInstaceId_(EnvironmentManager::GetServicePackageIdentifier(packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(), instanceId_))
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
        // Note: For cleanup we use OrignalTimeout for each of the operation individually.
        // This will give it a chance to cleanup atleast every resource if even one of them timesout.
        // If we still fail to do a cleanup then Abort should do it.

        ASSERT_IFNOT(owner_.crashDumpProvider_, "Crash dump provider should exist");
        WriteInfo(
            TraceEnvironmentManager,
            owner_.Root.TraceId,
            "{0}: Start CleanupServiceCrashDumps.",
            servicePackageInstaceId_);
        auto operation = owner_.crashDumpProvider_->BeginCleanupServiceCrashDumps(
            packageInstanceEnvironmentContext,
            timeoutHelper_.OriginalTimeout,
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
        if(!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: End CleanupServiceCrashDumps. Error {1}",
                servicePackageInstaceId_,
                error);

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
                WriteInfo(
                    TraceEnvironmentManager,
                    owner_.Root.TraceId,
                    "{0}: Start SetupContainerGroup Cleanup",
                    servicePackageInstaceId_);

                wstring openNetworkAssignedIp;
                std::map<std::wstring, std::wstring> overlayNetworkResources;
                ServiceModel::NetworkType::Enum networkType;
                wstring networkResourceList;
                packageInstanceEnvironmentContext->GetGroupAssignedNetworkResource(openNetworkAssignedIp, overlayNetworkResources, networkType, networkResourceList);

                WriteNoise(
                    TraceEnvironmentManager,
                    owner_.Root.TraceId,
                    "Id={0}, Container Group Network Type={1}, Network Resource List={2}",
                    packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
                    networkType,
                    networkResourceList);

                auto op = owner_.Hosting.FabricActivatorClientObj->BeginSetupContainerGroup(
                    packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
                    networkType,
                    openNetworkAssignedIp,
                    overlayNetworkResources,
                    vector<wstring>(),
                    wstring(),
                    wstring(),
                    wstring(),
                    wstring(),
                    wstring(),
                    ServicePackageResourceGovernanceDescription(),
#if defined(PLATFORM_UNIX)
                    ContainerPodDescription(),
#endif
                    true,
                    timeoutHelper_.OriginalTimeout,
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
                "{0}: End SetupContainerGroup Cleanup. Error {1}",
                servicePackageInstaceId_,
                error);

            lastError_.Overwrite(error);
        }
        ReleaseAssignedIPs(operation->Parent);
    }

    void ReleaseAssignedIPs(AsyncOperationSPtr const & thisSPtr)
    {
        if (packageInstanceEnvironmentContext->HasIpsAssigned)
        {
            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Start AssignIpAddress Cleanup",
                servicePackageInstaceId_);

            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginAssignIpAddresses(
                packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
                vector<wstring>(),
                true,
                timeoutHelper_.OriginalTimeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnIPAddressesReleased(operation, false);
            },
                thisSPtr);
            this->OnIPAddressesReleased(operation, true);
        }
        else if (packageInstanceEnvironmentContext->HasOverlayNetworkResourcesAssigned)
        {
            ReleaseOverlayNetworkResources(thisSPtr);
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
                "{0}: End AssignIpAddress Cleanup. Error {1}",
                servicePackageInstaceId_,
                error);

            lastError_.Overwrite(error);
        }

        ReleaseOverlayNetworkResources(operation->Parent);
    }

    void ReleaseOverlayNetworkResources(AsyncOperationSPtr const & thisSPtr)
    {
        if (packageInstanceEnvironmentContext->HasOverlayNetworkResourcesAssigned)
        {
            std::map<wstring, vector<wstring>> codePackageNetworkNames;
            packageInstanceEnvironmentContext->GetCodePackageOverlayNetworkNames(codePackageNetworkNames);

            wstring nodeIpAddress = L"";
            if (!codePackageNetworkNames.empty())
            {
                auto error = owner_.GetIsolatedNicIpAddress(nodeIpAddress);
                if (!error.IsSuccess())
                {
                    lastError_.Overwrite(error);
                }
            }
            
            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginManageOverlayNetworkResources(
                owner_.hosting_.NodeName,
                nodeIpAddress,
                packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(),
                codePackageNetworkNames,
                ManageOverlayNetworkAction::Unassign,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnOverlayNetworkResourcesReleased(operation, false);
            },
                thisSPtr);
            this->OnOverlayNetworkResourcesReleased(operation, true);
        }
        else
        {
            CleanupEndpointSecurity(thisSPtr);
        }
    }

    void OnOverlayNetworkResourcesReleased(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        std::map<wstring, std::map<std::wstring, std::wstring>> assignedOverlayNetworkResources;
        ErrorCode error = owner_.Hosting.FabricActivatorClientObj->EndManageOverlayNetworkResources(operation, assignedOverlayNetworkResources);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "EndReleaseOverlayNetworkResources:  ServicePackageId={0}, ErrorCode={1}.",
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
                            servicePackageInstaceId_,
                            (*iter)->Name,
                            error);
                        lastError_.Overwrite(error);
                    }
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

            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Start ConfigureEndpointSecurity Cleanup. Endpoint Name:{1}, Port: {2}",
                servicePackageInstaceId_,
                endpointResource->Name,
                endpointResource->Port);

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
                "{0}: End ConfigureEndpointSecurity Cleanup: EndpointName={1} Port={2}, ErrorCode={3}.",
                servicePackageInstaceId_,
                endpointResource->Name,
                endpointResource->Port,
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
        vector<EndpointCertificateBinding> endpointCertBindings;
        auto error = owner_.GetEndpointBindingPolicies(packageInstanceEnvironmentContext->Endpoints, servicePackageDescription_.DigestedResources.DigestedCertificates, endpointCertBindings);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Error getting EndpointBindingPolicies {0}",
                error);
            lastError_.Overwrite(error);
        }

        if (!endpointCertBindings.empty() || !packageInstanceEnvironmentContext->FirewallPorts.empty())
        {
            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Start ConfigureEndpointCertificateAndFirewallPolicy Cleanup. CleanupFirewallPolicy {1}, CleanupEndpointCertificate {2}",
                servicePackageInstaceId_,
                !packageInstanceEnvironmentContext->FirewallPorts.empty(),
                !endpointCertBindings.empty());

            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginConfigureEndpointCertificateAndFirewallPolicy(
                EnvironmentManager::GetServicePackageIdentifier(packageInstanceEnvironmentContext->ServicePackageInstanceId.ToString(), instanceId_),
                endpointCertBindings,
                true,
                true,
                packageInstanceEnvironmentContext->FirewallPorts,
                timeoutHelper_.OriginalTimeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnConfigureEndpointBindingsAndFirewallPolicyCompleted(operation, false);
            },
                thisSPtr);
            this->OnConfigureEndpointBindingsAndFirewallPolicyCompleted(operation, true);
        }
        else
        {
            CleanupContainerCertificates(thisSPtr);
        }
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
                "{0}: End ConfigureEndpointCertificateAndFirewallPolicy cleanup. ErrorCode={1}.",
                servicePackageInstaceId_,
                error);
            lastError_.Overwrite(error);
        }
        else
        {
            // This is an optimization and needed because the firewall rule API's are slow.
            packageInstanceEnvironmentContext->FirewallPorts = vector<LONG>();
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

        if (File::Exists(endpointFileName))
        {
            auto error = File::Delete2(endpointFileName, true);
            if (!error.IsSuccess())
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
        }
        else
        {
            WriteNoise(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "Successfully removed enpoint resource file={0}. NodeVersion={1}.",
                endpointFileName,
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
            WriteInfo(
                TraceEnvironmentManager,
                owner_.Root.TraceId,
                "{0}: Start CleanupContainerCertificateExport Cleanup.",
                servicePackageInstaceId_);

            auto operation = owner_.Hosting.FabricActivatorClientObj->BeginCleanupContainerCertificateExport(
                packageInstanceEnvironmentContext->CertificatePaths,
                packageInstanceEnvironmentContext->CertificatePasswordPaths,
                timeoutHelper_.OriginalTimeout,
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
                "{0}: CleanupContainerCertificateExport cleanup. ErrorCode={1}.",
                servicePackageInstaceId_,
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
    ServiceModel::ServicePackageDescription servicePackageDescription_;
    int64 instanceId_;
    wstring servicePackageInstaceId_;
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
    wstring const & applicationName,
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
        applicationName,
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
    int64 instanceId)
{
    if (!packageEnvironmentContext)
    {
        return ErrorCode(ErrorCodeValue::Success);
    }

    auto servicePackageInstanceId = EnvironmentManager::GetServicePackageIdentifier(packageEnvironmentContext->ServicePackageInstanceId.ToString(), instanceId);

    ErrorCode lastError(ErrorCodeValue::Success);
    if (HostingConfig::GetConfig().EndpointProviderEnabled)
    {
        ASSERT_IFNOT(endpointProvider_, "Endpoint provider must not be null as EndpointFiltering is enabled");

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
                        servicePackageInstanceId,
                        endpointResource->EndpointDescriptionObj.ExplicitPortSpecified);

                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceEnvironmentManager,
                            Root.TraceId,
                            "Remove EndpointSecurity failed. Endpoint={0}, ErrorCode={1}. Continuing to remove other endpoints.",
                            endpointResource->Name,
                            error);
                        lastError.Overwrite(error);
                    }
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
                    servicePackageInstanceId,
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
            lastError.Overwrite(error);
        }

        if (!endpointCertBindings.empty() || !packageEnvironmentContext->FirewallPorts.empty())
        {
            error = Hosting.FabricActivatorClientObj->ConfigureEndpointBindingAndFirewallPolicy(
                servicePackageInstanceId,
                endpointCertBindings,
                true,
                true,
                packageEnvironmentContext->FirewallPorts);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceEnvironmentManager,
                    Root.TraceId,
                    "{0}: Error in ConfigureEndpointBindingAndFirewallPolicy during Abort. Error {1}",
                    servicePackageInstanceId,
                    error);
            }
        }
    }

    if (packageEnvironmentContext->SetupContainerGroup)
    {
        Hosting.FabricActivatorClientObj->AbortProcess(packageEnvironmentContext->ServicePackageInstanceId.ToString());
    }

    if (packageEnvironmentContext->HasIpsAssigned)
    {
        auto error = Hosting.FabricActivatorClientObj->CleanupAssignedIPs(packageEnvironmentContext->ServicePackageInstanceId.ToString());
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                Root.TraceId,
                "{0}: Error in CleanupAssignedIps {1}",
                servicePackageInstanceId,
                error);
            lastError.Overwrite(error);
        }
    }
    else
    {
        WriteNoise(TraceEnvironmentManager,
            Root.TraceId,
            "No IPs assigned for {0}",
            servicePackageInstanceId);
    }

    if (packageEnvironmentContext->HasOverlayNetworkResourcesAssigned)
    {
        std::map<std::wstring, std::vector<std::wstring>> codePackageNetworkNames;
        packageEnvironmentContext->GetCodePackageOverlayNetworkNames(codePackageNetworkNames);

        auto error = Hosting.FabricActivatorClientObj->CleanupAssignedOverlayNetworkResources(
            codePackageNetworkNames,
            hosting_.NodeName,
            hosting_.FabricNodeConfigObj.IPAddressOrFQDN,
            packageEnvironmentContext->ServicePackageInstanceId.ToString());
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceEnvironmentManager,
                Root.TraceId,
                "{0}: Error in CleanupAssignedOverlayNetworkResources {1}",
                servicePackageInstanceId,
                error);
            lastError.Overwrite(error);
        }
    }
    else
    {
        WriteNoise(TraceEnvironmentManager,
            Root.TraceId,
            "No overlay network resources assigned for {0}",
            servicePackageInstanceId);
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
