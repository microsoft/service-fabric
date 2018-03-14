// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ImageModel;
using namespace ImageStore;
using namespace ServiceModel;
using namespace Query;

StringLiteral const Trace_DownloadManager("DownloadManager");
StringLiteral const UpdateFileStoreServicePrincipalTimerTag("UpdateFileStoreServicePrincipalTimer");

// ********************************************************************************************************************
// DownloadManager::OpenAsyncOperation Implementation
//
class DownloadManager::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent < TraceTaskCodes::Hosting >
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        __in DownloadManager & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        fileStoreServicePrincipalsCreated_(false)
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
        auto error = owner_.DeployFabricSystemPackages(timeoutHelper_.GetRemainingTime());
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "DeployFabricSystemPackages: ErrorCode={0}",
            error);

        if(!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        SecureString imageStoreConnectionString = ManagementConfig::GetConfig().ImageStoreConnectionString;
        if(StringUtility::AreEqualCaseInsensitive(
            imageStoreConnectionString.GetPlaintext(),
            Management::ImageStore::Constants::NativeImageStoreSchemaTag) ||
            Management::ImageStore::ImageStoreServiceConfig::GetConfig().Enabled)
        {
            this->CreateFileStoreServiceUsers(thisSPtr);
        }
        else
        {
            this->CreateImageStoreClient(thisSPtr);
        }
    }

private:
    void CreateFileStoreServiceUsers(AsyncOperationSPtr const & thisSPtr)
    {
        // Creating FileStoreService users for SMB authentication. The users are created here
        // because they are required by FileStoreService clients on all nodes and not just the
        // nodes on which the service replicas are present
        if (!Management::FileStoreService::Utility::HasAccountsConfigured())
        {
            // No security account is specified. Anonymous access has been enabled
            this->CreateImageStoreClient(thisSPtr);
            return;
        }

        if (Management::FileStoreService::Utility::HasThumbprintAccountsConfigured())
        {
            CreateFileStoreServiceUsersByThumbprintAndCommonNames(thisSPtr);
        }
        else
        {
            CreateFileStoreServiceUsersByCommonNames(thisSPtr);
        }
    }

    void CompleteConfigureSecurityPrincipalByThumbprint(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if(operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        PrincipalsProviderContextUPtr context;
        auto error = owner_.hosting_.FabricActivatorClientObj->EndConfigureSecurityPrincipalsForNode(
            operation,
            context);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "End(ConfigureSecurityPrincipalsForNode): FileStoreService Primary/Secondary accounts completed with {0}",
            error);

        if(!error.IsSuccess())
        {
            this->TryComplete(operation->Parent, error);
            return;
        }

        fileStoreServicePrincipalsCreated_ = true;
        this->CreateFileStoreServiceUsersByCommonNames(operation->Parent);
    }

    void CreateFileStoreServiceUsersByThumbprintAndCommonNames(AsyncOperationSPtr const & thisSPtr)
    {
        PrincipalsDescriptionUPtr principals = NULL;
        auto error = Management::FileStoreService::Utility::GetPrincipalsDescriptionFromConfigByThumbprint(principals);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        if (principals == NULL)
        {
            return;
        }

        WriteInfo(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "Begin(ConfigureSecurityPrincipalsForNode): FileStoreService Primary/Secondary accounts configuration starting.");

        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginConfigureSecurityPrincipalsForNode(
            ApplicationIdentifier::FabricSystemAppId->ToString(),
            ApplicationIdentifier::FabricSystemAppId->ApplicationNumber,
            *principals,
            1 /*allowedUserCreationFailureCount*/,
            fileStoreServicePrincipalsCreated_ /*updateExisting*/,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->CompleteConfigureSecurityPrincipalByThumbprint(operation, false); },
            thisSPtr);

        CompleteConfigureSecurityPrincipalByThumbprint(operation, true);
    }

    void CreateFileStoreServiceUsersByCommonNames(AsyncOperationSPtr const & thisSPtr)
    {
        if (!Management::FileStoreService::Utility::HasCommonNameAccountsConfigured())
        {
            // No security common name account is specified. Done creating users.
            this->CreateImageStoreClient(thisSPtr);
            return;
        }

        owner_.StartRefreshNTLMSecurityUsers(
            fileStoreServicePrincipalsCreated_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & error)
            {
                this->CompleteConfigureSecurityPrincipalByCommonNames(thisSPtr, error);
            },
            thisSPtr);
    }

    void CompleteConfigureSecurityPrincipalByCommonNames(
        AsyncOperationSPtr const & thisSPtr,
        Common::ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {
            TESTASSERT_IF(
                error.IsError(ErrorCodeValue::UpdatePending),
                "ConfigureNTLMSecurityUsersByX509CommonNames failed with UpdatePending, but no thread should be updating them");

            this->TryComplete(thisSPtr, error);
            return;
        }

        fileStoreServicePrincipalsCreated_ = true;

        // Start timer to refresh the principals periodically,
        // to find out other certificates with the same common name that may be installed.
        owner_.ScheduleRefreshNTLMSecurityUsersTimer();

        this->CreateImageStoreClient(thisSPtr);
    }

    void CreateImageStoreClient(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        SecureString imageStoreConnectionString = ManagementConfig::GetConfig().ImageStoreConnectionString;
        if(!imageStoreConnectionString.IsEmpty())
        {
            error = ImageStoreFactory::CreateImageStore(
                owner_.imageStore_,
                imageStoreConnectionString,
                Management::ManagementConfig::GetConfig().ImageStoreMinimumTransferBPS,
                owner_.passThroughClientFactoryPtr_,
                true, /*isInternal*/
                owner_.nodeConfig_->WorkingDir,
                owner_.hosting_.ImageCacheFolder);

            WriteTrace(
                error.ToLogLevel(),
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "CreateImageStore: ErrorCode={0}, ImageStoreLocation={1}, ImageCacheDirectory={2}",
                error,
                ManagementConfig::GetConfig().ImageStoreConnectionStringEntry.IsEncrypted() ? ManagementConfig::GetConfig().ImageStoreConnectionStringEntry.GetEncryptedValue() : imageStoreConnectionString.GetPlaintext(),
                owner_.hosting_.ImageCacheFolder);
        }
        else
        {
            WriteInfo(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "DownloadManager did not initialize imagestore as the ImageStoreLocation is empty.");
        }

        this->TryComplete(thisSPtr, error);
    }

private:
    DownloadManager & owner_;
    TimeoutHelper timeoutHelper_;
    bool fileStoreServicePrincipalsCreated_;
};

// ********************************************************************************************************************
// DownloadManager::CloseAsyncOperation Implementation
//
class DownloadManager::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent < TraceTaskCodes::Hosting >
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        __in DownloadManager & owner,
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
        owner_.CloseRefreshNTLMSecurityUsersTimer();
        owner_.ClosePendingDownloads();

        owner_.nonRetryableFailedDownloads_.Close();

        // We can remove the file store service users here.
        // However, this is not strictly required.
        // The cleanup happens when the node restarts.
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

private:
    DownloadManager & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// DownloadManager.DownloadAsyncOperation Implementation
//
// base class for downloading application instance or packages to the run layout from the store
class DownloadManager::DownloadAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent < TraceTaskCodes::Hosting >
{
    DENY_COPY(DownloadAsyncOperation)

public:
    enum Kind
    {
        DownloadApplicationPackageAsyncOperation,
        DownloadServicePackageAsyncOperation,
        DownloadFabricUpgradePackageAsyncOperation,
        DownloadServiceManifestAsyncOperation
    };

public:
    DownloadAsyncOperation(
        Kind kind,
        __in DownloadManager & owner,
        ApplicationIdentifier const & applicationId,
        wstring const & applicationName,
        wstring const & downloadId,
        int maxFailureCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        kind_(kind),
        owner_(owner),
        applicationId_(applicationId),
        applicationName_(applicationName),
        downloadId_(downloadId),
        retryTimer_(),
        lock_(),
        status_(),
        maxFailureCount_(maxFailureCount),
        symbolicLinks_(),
        random_((int)SequenceNumber::GetNext())
    {
    }

    virtual ~DownloadAsyncOperation()
    {
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!owner_.imageStore_)
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Invalid IamgeStore. Check the ImageStoreConnectionString configuration.");
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidState));
            return;
        }

        // check if there is a pending download for the same item
        auto error = owner_.pendingDownloads_->GetStatus(downloadId_, status_);
        if (error.IsSuccess())
        {
            TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::HostingDeploymentInProgress));
            return;
        }

        // error in getting the pending download, usually this means that we are closing down
        if (!error.IsError(ErrorCodeValue::NotFound))
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // try to add this operation in the pending downloads map
        error = owner_.pendingDownloads_->Start(downloadId_, 0, thisSPtr, status_);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::AlreadyExists))
            {
                // if other thread won, then complete this async operation with deployment in progress error code
                TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::HostingDeploymentInProgress));
            }
            else
            {
                // error in getting the pending download, usually this means that we are closing down
                TryComplete(thisSPtr, error);
            }

            return;
        }

        // If the download had failed with a non-retryable error in the
        // previous attempt, get the error and complete this AsyncOperation
        owner_.nonRetryableFailedDownloads_.TryGetAndRemove(downloadId_, error);
        if (error.IsSuccess())
        {
            AcquireWriteLock lock(lock_);

            if (status_.State == OperationState::Completed) { return; }
            error = this->RegisterComponent();
        }

        if (!error.IsSuccess())
        {
            if (TryStartComplete())
            {
                CompletePendingDownload(error);
                FinishComplete(thisSPtr, error);
            }

            return;
        }

        // this is the primary download operation for this id, start the download
        ScheduleDownload(thisSPtr);
    }

    virtual void OnCancel()
    {
        TimerSPtr timer;
        {
            AcquireWriteLock lock(lock_);
            timer = retryTimer_;
            retryTimer_.reset();
        }

        if (timer)
        {
            timer->Cancel();
        }

        CompletePendingDownload(ErrorCode(ErrorCodeValue::OperationCanceled));

        AsyncOperation::OnCancel();
    }

    ErrorCode CopySubPackageFromStore(
        wstring const & storePath,
        wstring const & runPath,
        wstring const & storeChecksumPath = L"",
        wstring const & expectedChecksumValue = L"",
        bool refreshCache = false,
        ImageStore::CopyFlag::Enum copyFlag = ImageStore::CopyFlag::Echo,
        bool copyToLocalCacheOnly = false)
    {
        return CopyFromStore(
            storePath,
            runPath,
            storeChecksumPath,
            expectedChecksumValue,
            refreshCache,
            copyFlag,
            copyToLocalCacheOnly,
            true); // checkForArchive
    }

    ErrorCode CopyFromStore(
        wstring const & storePath,
        wstring const & runPath,
        wstring const & storeChecksumPath = L"",
        wstring const & expectedChecksumValue = L"",
        bool refreshCache = false,
        ImageStore::CopyFlag::Enum copyFlag = ImageStore::CopyFlag::Echo,
        bool copyToLocalCacheOnly = false,
        bool checkForArchive = false)
    {
        auto error = owner_.imageStore_->DownloadContent(
            storePath,
            runPath,
            ServiceModelConfig::GetConfig().MaxOperationTimeout,
            storeChecksumPath,
            expectedChecksumValue,
            refreshCache,
            copyFlag,
            copyToLocalCacheOnly,
            checkForArchive);

        if (!error.IsSuccess())
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "CopyFromStore: ErrorCode={0}, StorePath={1}, RunPath={2}, StoreChecksumPath={3}, ExpectedChecksumValue={4}",
                error,
                storePath,
                runPath,
                storeChecksumPath,
                expectedChecksumValue);
        }

        return error;
    }

    virtual AsyncOperationSPtr BeginDownloadContent(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) = 0;

    virtual ErrorCode EndDownloadContent(AsyncOperationSPtr const & operation)
    {
        return CompletedAsyncOperation::End(operation);
    }

    virtual ErrorCode OnRegisterComponent() = 0;
    virtual void OnUnregisterComponent() = 0;
    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber) = 0;
    virtual bool IsNonRetryableError(ErrorCode const & error)
    {
        UNREFERENCED_PARAMETER(error);
        return false;
    }

private:
    ErrorCode RegisterComponent()
    {
        // V1 upgrade message will not have applicationName. Health will not be
        // be reported then
        if (applicationName_.empty())
        {
            return ErrorCodeValue::Success;
        }

        return this->OnRegisterComponent();
    }

    void UnregisterComponent()
    {
        if (applicationName_.empty())
        {
            return;
        }

        this->OnUnregisterComponent();
    }

    void ReportHealth(SystemHealthReportCode::Enum reportCode, wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        if (applicationName_.empty())
        {
            return;
        }

        this->OnReportHealth(reportCode, description, sequenceNumber);
    }

    void ScheduleDownload(AsyncOperationSPtr const & thisSPtr)
    {
        TimerSPtr timer = Timer::Create("Hosting.ScheduleDownload", [this, thisSPtr](TimerSPtr const & timer) { this->OnScheduledCallback(timer, thisSPtr); }, false);

        TimeSpan dueTime;
        {
            AcquireWriteLock lock(lock_);

            if (status_.State == OperationState::Completed)
            {
                timer->Cancel();
                return;
            }

            if (retryTimer_) { retryTimer_->Cancel(); }
            retryTimer_ = timer;

            dueTime = GetRetryDueTime(
                status_.FailureCount,
                HostingConfig::GetConfig().DeploymentRetryBackoffInterval,
                HostingConfig::GetConfig().DeploymentMaxRetryInterval,
                random_);
        }

        WriteNoise(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "Retry due time: {0}", dueTime.TotalMilliseconds());

        timer->Change(dueTime);
    }

    void OnScheduledCallback(TimerSPtr const & timer, AsyncOperationSPtr const & thisSPtr)
    {
        timer->Cancel();
        {
            AcquireReadLock lock(lock_);
            if (status_.State == OperationState::Completed)
            {
                return;
            }
        }
        auto operation = this->BeginDownloadContent(
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadContentCompleted(operation, false);
        },
            thisSPtr);
        OnDownloadContentCompleted(operation, true);
    }

    void OnDownloadContentCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ULONG retryCount = 0;
        {
            AcquireReadLock lock(lock_);
            retryCount = status_.FailureCount;
        }
        auto error = this->EndDownloadContent(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "Download: {0}, ErrorCode={1}, RetryCount={2}",
            downloadId_,
            error,
            retryCount);

        if (!error.IsSuccess())
        {
            if (IsNonRetryableError(error))
            {
                // If AppManifest is not found or ServiceManifest is not found
                // complete the download
                if (TryStartComplete())
                {
                    CompletePendingDownload(error);
                    FinishComplete(operation->Parent, error);
                }
            }
            // If failure is due to file not found, query CM to get status of ApplicationInstance
            if (error.IsError(ErrorCodeValue::FileNotFound) &&
                kind_ != Kind::DownloadFabricUpgradePackageAsyncOperation)
            {
                QueryDeletedApplicationInstance(operation->Parent, retryCount);
                return;
            }

            // retry if possible
            if (retryCount + 1 < maxFailureCount_)
            {
                UpdatePendingDownload(error);
                ScheduleDownload(operation->Parent);
                return;
            }
        }

        if (error.IsSuccess())
        {
            if (!symbolicLinks_.empty())
            {
                SetupSymbolicLinks(operation->Parent);
            }
            else
            {
#if !defined(PLATFORM_UNIX)
                if (TryStartComplete())
                {
                    CompletePendingDownload(ErrorCodeValue::Success);
                    FinishComplete(operation->Parent, ErrorCodeValue::Success);
                    return;
                }
#else
                EnsureFolderAcl(operation->Parent);
#endif
            }
        }
        else
        {
            // complete the download
            if (TryStartComplete())
            {
                CompletePendingDownload(error);
                FinishComplete(operation->Parent, error);
            }
        }
    }

    void CompletePendingDownload(ErrorCode const error)
    {
        {
            AcquireWriteLock lock(lock_);

            status_.LastError = error;
            status_.State = OperationState::Completed;

            if (status_.FailureCount > 0)
            {
                owner_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->UnregisterFailure(downloadId_);
            }

            owner_.pendingDownloads_->Complete(downloadId_, 0, status_).ReadValue();
        }

        this->UnregisterComponent();

        owner_.pendingDownloads_->Remove(downloadId_, 0);
    }

    void UpdatePendingDownload(ErrorCode const error)
    {
        bool shouldReport = false;
        FABRIC_SEQUENCE_NUMBER healthSequence = 0;

        {
            AcquireWriteLock lock(lock_);
            if (status_.State == OperationState::Completed) { return; }

            if (this->IsInternalError(error) && 
                (status_.InternalFailureCount < HostingConfig().GetConfig().MaxRetryOnInternalError) &&
                (HostingConfig().GetConfig().RetryOnInternalError) )
            {
                status_.InternalFailureCount++;

                WriteInfo(
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "UpdatePendingDownload: Error '{0}' is an internal error and does not count as a real failure. FailureCount: {1}",
                    error,
                    status_.InternalFailureCount);
                return;
            }

            status_.LastError = error;
            status_.FailureCount++;

            if (status_.FailureCount == 1)
            {
                owner_.hosting_.FabricRuntimeManagerObj->ServiceTypeStateManagerObj->RegisterFailure(downloadId_);
                shouldReport = true;
                healthSequence = SequenceNumber::GetNext();
            }

            owner_.pendingDownloads_->UpdateStatus(downloadId_, 0, status_).ReadValue();
        }

        if (shouldReport)
        {
            wstring description = L"";
            if (!error.Message.empty())
            {
                description = error.Message;
            }
            this->ReportHealth(
                SystemHealthReportCode::Hosting_DownloadFailed,
                description /*extraDescription*/,
                healthSequence);
        }
    }

    bool IsInternalError(ErrorCode const & error)
    {
        //For Package sharing violation we won't increment the failure count unless it is not from FileLock Reader/Writer
        return error.IsError(ErrorCodeValue::SharingAccessLockViolation);
    }

    void SetupSymbolicLinks(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginCreateSymbolicLink(
            this->symbolicLinks_,
            SYMBOLIC_LINK_FLAG_DIRECTORY,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->FinishSymbolicLinksSetup(operation, false);
        },
            thisSPtr);
        FinishSymbolicLinksSetup(operation, true);
    }

    void FinishSymbolicLinksSetup(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.hosting_.FabricActivatorClientObj->EndCreateSymbolicLink(operation);
        if (error.IsSuccess())
        {
#if defined(PLATFORM_UNIX)
            EnsureFolderAcl(operation->Parent);
            return;
#endif
        }
        if (TryStartComplete())
        {
            CompletePendingDownload(error);
            FinishComplete(operation->Parent, error);
        }

    }

#if defined(PLATFORM_UNIX)
    void EnsureFolderAcl(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> applicationFolders;
        vector<CertificateAccessDescription> certificateAccessDescriptions;

        wstring applicationFolder = owner_.hosting_.RunLayout.GetApplicationFolder(applicationId_.ToString());
        if (applicationId_ == *ApplicationIdentifier::FabricSystemAppId)
        {
            if (TryStartComplete())
            {
                CompletePendingDownload(ErrorCodeValue::Success);
                FinishComplete(thisSPtr, ErrorCodeValue::Success);
            }
            return;
        }
        applicationFolders.push_back(applicationFolder);

        GetApplicationRootDirectories(applicationFolders);

        wstring groupName = ApplicationPrincipals::GetApplicationLocalGroupName(
            owner_.hosting_.NodeId,
            applicationId_.ApplicationNumber);
        groupName = groupName.substr(0, 32);
        WriteInfo(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "EnsureFolderAcl Group {0}: {1}",
            groupName,
            applicationFolders
        );

        SidSPtr groupSid;
        auto error = BufferedSid::CreateSPtr(groupName, groupSid);

        if (!error.IsSuccess())
        {
            error = BufferedSid::CreateGroupSPtr(Constants::AppRunAsAccountGroup, groupSid);
        }

        if (error.IsSuccess())
        {
            wstring sidStr;
            vector<wstring> sidStrings;
            error = groupSid->ToString(sidStr);
            sidStrings.push_back(sidStr);
            auto operation = owner_.hosting_.FabricActivatorClientObj->BeginConfigureResourceACLs(
                sidStrings,
                GENERIC_ALL,
                certificateAccessDescriptions,
                applicationFolders,
                applicationId_.ApplicationNumber,
                owner_.hosting_.NodeId,
                HostingConfig::GetConfig().RequestTimeout,
                [this](AsyncOperationSPtr const & operation)
                {
                    this->FinishEnsureFolderAcl(operation, false);
                },
                thisSPtr);
            FinishEnsureFolderAcl(operation, true);
        }
        else
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "EnsureFolderAcl Group {0}: {1}. Error: {2}.",
                groupName,
                applicationFolders,
                error
            );
            if (TryStartComplete())
            {
                CompletePendingDownload(error);
                FinishComplete(thisSPtr, error);
            }
        }
    }

    void FinishEnsureFolderAcl(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.hosting_.FabricActivatorClientObj->EndConfigureResourceACLs(operation);
        if (TryStartComplete())
        {
            CompletePendingDownload(error);
            FinishComplete(operation->Parent, error);
        }

    }

    void GetApplicationRootDirectories(__inout std::vector<wstring> & applicationFolders)
    {
        wstring appInstanceId = applicationId_.ToString();

        //Skip in case it is FabricApp as we donot setup the root folder for Fabric Application
        if (StringUtility::AreEqualCaseInsensitive(appInstanceId, ApplicationIdentifier::FabricSystemAppId->ToString()))
        {
            return;
        }

        Common::FabricNodeConfig::KeyStringValueCollectionMap const& jbodDrives = owner_.hosting_.FabricNodeConfigObj.LogicalApplicationDirectories;
        wstring nodeId = owner_.hosting_.NodeId;
        wstring rootDeployedfolder;
        for (auto iter = jbodDrives.begin(); iter != jbodDrives.end(); ++iter)
        {
            rootDeployedfolder = Path::Combine(iter->second, nodeId);
            rootDeployedfolder = Path::Combine(rootDeployedfolder, appInstanceId);
            applicationFolders.push_back(move(rootDeployedfolder));
        }
    }

#endif

    void QueryDeletedApplicationInstance(AsyncOperationSPtr const & thisSPtr, ULONG const retryCount)
    {
        vector<wstring> appIds;
        appIds.push_back(applicationId_.ToString());

        InternalDeletedApplicationsQueryObject queryObj(appIds);
        QueryArgumentMap argsMap;
        wstring queryStr;
        auto error = JsonHelper::Serialize<InternalDeletedApplicationsQueryObject>(queryObj, queryStr);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Failed to Serialize InternalDeletedApplicationsQueryObject: ErrorCode={0}",
                error);

            if (TryStartComplete())
            {
                CompletePendingDownload(error);
                FinishComplete(thisSPtr, error);
            }
            return;
        }

        argsMap.Insert(L"ApplicationIds", queryStr);

        WriteNoise(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "Begin(GetDeletedApplicationsListQuery)");
        auto operation = owner_.hosting_.QueryClient->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeletedApplicationsList),
            argsMap,
            HostingConfig::GetConfig().RequestTimeout,
            [this, retryCount](AsyncOperationSPtr const& operation) { this->OnGetDeletedApplicationsListCompleted(operation, retryCount); },
            thisSPtr);
    }

    void OnGetDeletedApplicationsListCompleted(AsyncOperationSPtr const & operation, ULONG const retryCount)
    {
        QueryResult result;
        wstring queryObStr;
        auto error = owner_.hosting_.QueryClient->EndInternalQuery(operation, result);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "End(GetDeletedApplicationsListQuery): ErrorCode={0}",
            error);

        if (error.IsSuccess())
        {
            InternalDeletedApplicationsQueryObject deletedApps;
            error = result.MoveItem(deletedApps);
            WriteTrace(
                error.ToLogLevel(),
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Deserialize DeletedApplicationsQueryObject: ErrorCode={0}",
                error);

            if (error.IsSuccess())
            {
                ASSERT_IF(deletedApps.ApplicationIds.size() > 1, "Unexpected DeletedApps count {0}", deletedApps.ApplicationIds.size());

                if (deletedApps.ApplicationIds.size() > 0 && deletedApps.ApplicationIds[0] == applicationId_.ToString())
                {
                    // ApplicationInstance has been deleted
                    // Add the ApplicationInstanceDeleted non-retryable error to the nonRetryableFailedDownloads_ map.
                    // Next time RA does a Find which initiates Download, it will get this non-retryable error.
                    if (TryStartComplete())
                    {
                        auto addError = owner_.nonRetryableFailedDownloads_.TryAdd(downloadId_, ErrorCodeValue::ApplicationInstanceDeleted);
                        ASSERT_IF(addError.IsError(ErrorCodeValue::AlreadyExists), "The downloadId:{0} is already present in the nonRetryableFailedDownloads_ map.", downloadId_);

                        CompletePendingDownload(ErrorCodeValue::ApplicationInstanceDeleted);
                        FinishComplete(operation->Parent, ErrorCodeValue::ApplicationInstanceDeleted);
                    }

                    return;
                }
                else
                {
                    error = ErrorCodeValue::FileNotFound;
                }
            }
        }

        if (retryCount + 1 < maxFailureCount_)
        {
            UpdatePendingDownload(error);
            ScheduleDownload(operation->Parent);
            return;
        }
        else if (TryStartComplete())
        {
            CompletePendingDownload(error);
            FinishComplete(operation->Parent, error);
        }
    }

    static wstring ToWString(vector<wstring> const & paths)
    {
        wstring retval(L"");
        for (auto iter = paths.begin(); iter != paths.end(); ++iter)
        {
            retval.append(*iter);
            retval.append(L",");
        }

        return retval;
    }

    static TimeSpan GetRetryDueTime(LONG retryCount, TimeSpan const retryBackoffInterval, TimeSpan const maxRetryInterval, Random & random)
    {
        auto deploymentFailedRetryIntervalRange = HostingConfig::GetConfig().DeploymentFailedRetryIntervalRange;

        if (retryCount == 0)
        {
            return GetRandomizedDueTime(deploymentFailedRetryIntervalRange, TimeSpan::Zero, random);
        }
        else
        {
            auto retryTime = TimeSpan::FromMilliseconds((double)retryCount * retryBackoffInterval.TotalMilliseconds());

            //Randomize the time to do download
            retryTime = GetRandomizedDueTime(deploymentFailedRetryIntervalRange, retryTime, random);

            if (retryTime.Ticks > maxRetryInterval.Ticks)
            {
                retryTime = GetRandomizedDueTime(deploymentFailedRetryIntervalRange, maxRetryInterval, random);
            }

            return retryTime;
        }
    }

    static TimeSpan GetRandomizedDueTime(TimeSpan const deploymentFailedRetryIntervalRange, TimeSpan const deploymentGraceInterval, Random & random)
    {
        return TimeSpan::FromMilliseconds((random.NextDouble() * deploymentFailedRetryIntervalRange.TotalMilliseconds()) + deploymentGraceInterval.TotalMilliseconds());
    }

public:
    Kind const kind_;

protected:
    DownloadManager & owner_;
    OperationStatus status_;
    wstring const downloadId_;
    wstring const applicationName_;
    ApplicationIdentifier applicationId_;
    vector<ArrayPair<wstring, wstring>> symbolicLinks_;

private:
    ULONG const maxFailureCount_;
    RwLock lock_;
    TimerSPtr retryTimer_;
    Random random_;
};

// ********************************************************************************************************************
// DownloadManager.DownloadApplicationPackageAsyncOperation Implementation
//
// Deploys the application instance to the run layout
class DownloadManager::DownloadApplicationPackageAsyncOperation
    : public DownloadManager::DownloadAsyncOperation
{
    DENY_COPY(DownloadApplicationPackageAsyncOperation)

public:
    DownloadApplicationPackageAsyncOperation(
        __in DownloadManager & owner,
        ApplicationIdentifier const & applicationId,
        ApplicationVersion const & applicationVersion,
        wstring const & applicationName,
        ULONG maxFailureCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DownloadAsyncOperation(
        Kind::DownloadApplicationPackageAsyncOperation,
        owner,
        applicationId,
        applicationName,
        DownloadManager::GetOperationId(applicationId, applicationVersion),
        maxFailureCount,
        callback,
        parent),
        applicationId_(applicationId),
        applicationVersion_(applicationVersion),
        applicationIdStr_(applicationId.ToString()),
        applicationPackageDescription_(),
        applicationVersionStr_(applicationVersion.ToString()),
        healthPropertyId_(wformatString("Download:{0}", applicationVersion))
    {
        ASSERT_IF(
            applicationId_.IsAdhoc(),
            "Cannot download an ad-hoc Application Package.");
    }

    virtual ~DownloadApplicationPackageAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out OperationStatus & downloadStatus,
        __out ApplicationPackageDescription & applicationPackageDescription)
    {
        auto thisPtr = AsyncOperation::End<DownloadApplicationPackageAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            hostingTrace.ApplicationPackageDownloaded(
                thisPtr->owner_.Root.TraceId,
                thisPtr->applicationIdStr_,
                thisPtr->applicationVersionStr_);
            applicationPackageDescription = move(thisPtr->applicationPackageDescription_);
        }
        downloadStatus = thisPtr->status_;
        return thisPtr->Error;
    }

    void GetApplicationInfo(__out wstring & applicationName, __out ApplicationIdentifier & applicationId)
    {
        applicationName = applicationName_;
        applicationId = applicationId_;
    }

protected:

    virtual AsyncOperationSPtr BeginDownloadContent(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (applicationId_ != *ApplicationIdentifier::FabricSystemAppId)
        {
            error = owner_.EnsureApplicationFolders(applicationIdStr_, false);
            if (error.IsSuccess())
            {
                error = SetupSymbolicLinks(applicationIdStr_);
                if (error.IsSuccess())
                {
                    error = EnsureApplicationPackageFileContents();
                }
            }
        }

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            error,
            callback,
            thisSPtr);
    }


    virtual ErrorCode OnRegisterComponent()
    {
        return owner_.hosting_.HealthManagerObj->RegisterSource(applicationId_, applicationName_, healthPropertyId_);
    }

    virtual void OnUnregisterComponent()
    {
        owner_.hosting_.HealthManagerObj->UnregisterSource(applicationId_, healthPropertyId_);
    }

    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, std::wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        owner_.hosting_.HealthManagerObj->ReportHealth(
            applicationId_,
            healthPropertyId_,
            reportCode,
            description,
            sequenceNumber);
    }

private:
    ErrorCode EnsureApplicationPackageFileContents()
    {
        wstring applicationPackageFilePath = owner_.RunLayout.GetApplicationPackageFile(applicationIdStr_, applicationVersionStr_);
        wstring remoteApplicationPackageFilePath = owner_.StoreLayout.GetApplicationPackageFile(applicationId_.ApplicationTypeName, applicationIdStr_, applicationVersionStr_);

        auto error = CopyFromStore(
            remoteApplicationPackageFilePath,
            applicationPackageFilePath);
        if (!error.IsSuccess()) { return error; }

        // parse application package file
        error = Parser::ParseApplicationPackage(applicationPackageFilePath, applicationPackageDescription_);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseApplicationPackageFile: ErrorCode={0}, ApplicationPackageFile={1}",
            error,
            applicationPackageFilePath);

        if (!error.IsSuccess())
        {
            Common::ErrorCode deleteError = this->owner_.DeleteCorruptedFile(applicationPackageFilePath, remoteApplicationPackageFilePath, L"ApplicationPackageFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteApplicationPackageFile: ErrorCode={0}, ApplicationPackageFile={1}",
                    deleteError,
                    applicationPackageFilePath);
            }
        }

        return error;
    }

    ErrorCode SetupSymbolicLinks(wstring const & applicationId)
    {
        Common::FabricNodeConfig::KeyStringValueCollectionMap const& jbodDrives = owner_.hosting_.FabricNodeConfigObj.LogicalApplicationDirectories;

        ErrorCode error = ErrorCode(ErrorCodeValue::Success);

        wstring workDir = owner_.RunLayout.GetApplicationWorkFolder(applicationId);
        wstring deployedDir;
        auto nodeId = owner_.hosting_.NodeId;
        wstring logDirectorySymbolicLink(L"");

        for (auto iter = jbodDrives.begin(); iter != jbodDrives.end(); ++iter)
        {
            if (StringUtility::AreEqualCaseInsensitive(iter->first, Constants::Log))
            {
                logDirectorySymbolicLink = iter->second;
                continue;
            }

            //Create the appInstance directory in the JBOD directory ex: JBODFoo:\directory\NodeId\%AppTYpe%_App%AppVersion%
            deployedDir = Path::Combine(iter->second, nodeId);
            deployedDir = Path::Combine(deployedDir, applicationId);

            error = Directory::Create2(deployedDir);
            WriteTrace(
                error.ToLogLevel(),
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "CreateApplicationInstanceDirectory: CreateFolder: {0}, ErrorCode={1}",
                deployedDir,
                error);
            if (!error.IsSuccess()) { return error; }

            //Symbolic Link
            ArrayPair<wstring, wstring> link;
            //key is %AppTYpe%_App%AppVersion%\work\directory
            link.key = Path::Combine(workDir, iter->first);
            link.value = deployedDir;
            symbolicLinks_.push_back(link);
        }

        error = SetupLogDirectory(applicationId, nodeId, logDirectorySymbolicLink);

        return error;
    }

    ErrorCode SetupLogDirectory(wstring const & applicationId, wstring const& nodeId, wstring const& logDirectorySymbolicLink)
    {
        if (logDirectorySymbolicLink.empty())
        {
            // If Log directory is not set in Disk Drive section create the Log directory
            return owner_.EnsureApplicationLogFolder(applicationId);
        }

        wstring deployedLogDir = Path::Combine(logDirectorySymbolicLink, nodeId);
        //Setup symbolic link from log to JBOD:\Dir\Log\NodeId\ApplicationId
        deployedLogDir = Path::Combine(deployedLogDir, applicationId);

        ErrorCode error = Directory::Create2(deployedLogDir);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "CreateApplicationInstanceLogDirectory: CreateFolder: {0}, ErrorCode={1}",
            deployedLogDir,
            error);

       if (!error.IsSuccess()){ return error; }

       //Symbolic Link
       ArrayPair<wstring, wstring> link;
       link.key = owner_.RunLayout.GetApplicationLogFolder(applicationId);
       link.value = deployedLogDir;
       symbolicLinks_.push_back(link);

       return error;
    }

private:
    ApplicationIdentifier const applicationId_;
    ApplicationVersion const applicationVersion_;
    ApplicationPackageDescription applicationPackageDescription_;
    wstring const applicationIdStr_;
    wstring const applicationVersionStr_;
    wstring const healthPropertyId_;
};

// ********************************************************************************************************************
// DownloadManager.DownloadContainerImagesAsyncOperation Implementation
//
// Downloads container images
class DownloadManager::DownloadContainerImagesAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(DownloadContainerImagesAsyncOperation)

public:
    DownloadContainerImagesAsyncOperation(
        vector<ContainerImageDescription> const & containerImages,
        ServicePackageActivationContext const & activationContext,
        __in DownloadManager & owner,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(
        callback,
        parent),
        owner_(owner),
        containerImages_(containerImages),
        activationContext_(activationContext)
    {
    }

    virtual ~DownloadContainerImagesAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        SendDownloadImagesRequest(thisSPtr);
    }

    void SendDownloadImagesRequest(AsyncOperationSPtr const & thisSPtr)
    {
        if (!containerImages_.empty())
        {
            WriteInfo(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Downloading container images count {0} for activationcontext {1}.",
                containerImages_.size(),
                activationContext_.ToString());
            auto operation = owner_.hosting_.FabricActivatorClientObj->BeginDownloadContainerImages(
                containerImages_,
                HostingConfig::GetConfig().ContainerImageDownloadTimeout,
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnDownloadContainerImagesCompleted(operation, false);
            },
                thisSPtr);
            OnDownloadContainerImagesCompleted(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void OnDownloadContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.hosting_.FabricActivatorClientObj->EndDownloadContainerImages(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Failed to import container images error {0}.",
                error);
        }
        WriteInfo(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "Download container images count {0} for activationcontext {1} error {2}.",
            containerImages_.size(),
            activationContext_.ToString(),
            error);
        TryComplete(operation->Parent, error);
    }

private:
    DownloadManager & owner_;
    vector<ContainerImageDescription> containerImages_;
    ServicePackageActivationContext activationContext_;
};

// ********************************************************************************************************************
// DownloadManager.DownloadServicePackageAsyncOperation Implementation
//
// Deploys the service package of an application to the run layout
class DownloadManager::DownloadServicePackageAsyncOperation
    : public DownloadManager::DownloadAsyncOperation
{
    DENY_COPY(DownloadServicePackageAsyncOperation)

public:
    DownloadServicePackageAsyncOperation(
        __in DownloadManager & owner,
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageVersion const & servicePackageVersion,
        ServicePackageActivationContext const & activationContext,
        wstring const & applicationName,
        ULONG maxFailureCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DownloadAsyncOperation(
        Kind::DownloadServicePackageAsyncOperation,
        owner,
        servicePackageId.ApplicationId,
        applicationName,
        DownloadManager::GetOperationId(servicePackageId, servicePackageVersion),
        maxFailureCount,
        callback,
        parent),
        servicePackageId_(servicePackageId),
        activationContext_(activationContext),
        svcPkgInstanceIdForHealth_(servicePackageId, ServicePackageActivationContext(), L""),
        servicePackageVersion_(servicePackageVersion),
        applicationIdStr_(servicePackageId.ApplicationId.ToString()),
        packageFilePath_(),
        manifestFilePath_(),
        healthPropertyId_(wformatString("Download:{0}", servicePackageVersion))
    {
        ASSERT_IF(
            servicePackageId_.ApplicationId.IsAdhoc(),
            "Cannot download ServicePackage of an ad-hoc application. ServicePackageId={0}",
            servicePackageId_);

        remoteObject_ = owner_.StoreLayout.GetServicePackageFile(
            servicePackageId_.ApplicationId.ApplicationTypeName,
            applicationIdStr_,
            servicePackageId_.ServicePackageName,
            servicePackageVersion_.RolloutVersionValue.ToString());
    }

    virtual ~DownloadServicePackageAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out OperationStatus & downloadStatus,
        __out ServicePackageDescription & servicePackageDescription)
    {
        auto thisPtr = AsyncOperation::End<DownloadServicePackageAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            hostingTrace.ServicePackageDownloaded(
                thisPtr->owner_.Root.TraceId,
                thisPtr->servicePackageId_.ToString(),
                thisPtr->servicePackageVersion_.ToString());
            servicePackageDescription = move(thisPtr->servicePackageDescription_);
        }
        downloadStatus = thisPtr->status_;
        return thisPtr->Error;
    }

    void GetServicePackageInfo(__out wstring & applicationName, __out wstring & serviceManifestName)
    {
        applicationName = applicationName_;
        serviceManifestName = servicePackageId_.ServicePackageName;
    }

protected:
    virtual AsyncOperationSPtr BeginDownloadContent(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & thisSPtr)
    {
        if (servicePackageId_.ApplicationId != *ApplicationIdentifier::FabricSystemAppId)
        {
            auto error = EnsurePackageFile();
            if (error.IsSuccess())
            {
                error = EnsurePackageFileContents();
            }
            if (!error.IsSuccess() || containerImages_.empty())
            {
                containerImages_.clear();
                return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                    error,
                    callback,
                    thisSPtr);
            }

            return AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
                this->containerImages_,
                this->activationContext_,
                this->owner_,
                callback,
                thisSPtr);
        }
        else
        {
            // The system package is already copied to the apps folder
            packageFilePath_ = owner_.RunLayout.GetServicePackageFile(
                applicationIdStr_,
                servicePackageId_.ServicePackageName,
                servicePackageVersion_.RolloutVersionValue.ToString());

            auto error = Parser::ParseServicePackage(packageFilePath_, servicePackageDescription_);
            WriteTrace(
                error.ToLogLevel(),
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "ParseServicePackageFile: ErrorCode={0}, PackageFile={1}",
                error,
                packageFilePath_);

            if (!error.IsSuccess())
            {
                ErrorCode deleteError = this->owner_.DeleteCorruptedFile(packageFilePath_, remoteObject_, L"ServicePackageFile");
                if (!deleteError.IsSuccess())
                {
                    WriteTrace(
                        deleteError.ToLogLevel(),
                        Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "DeleteServicePackageFile: ErrorCode={0}, PackageFile={1}",
                        deleteError,
                        packageFilePath_);
                }
            }

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                thisSPtr);
        }
    }

    virtual ErrorCode EndDownloadContent(AsyncOperationSPtr const & operation)
    {
        if (!containerImages_.empty())
        {
            return DownloadContainerImagesAsyncOperation::End(operation);
        }
        else
        {
            return CompletedAsyncOperation::End(operation);
        }
    }

    ErrorCode EnsurePackageFile()
    {
        packageFilePath_ = owner_.RunLayout.GetServicePackageFile(
            applicationIdStr_,
            servicePackageId_.ServicePackageName,
            servicePackageVersion_.RolloutVersionValue.ToString());

        return CopyFromStore(
            remoteObject_,
            packageFilePath_);
    }

    ErrorCode EnsurePackageFileContents()
    {
        // parse service package file
        ServicePackageDescription servicepackageDescription;
        auto error = Parser::ParseServicePackage(packageFilePath_, servicepackageDescription);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseServicePackageFile: ErrorCode={0}, PackageFile={1}",
            error,
            packageFilePath_);

        if (!error.IsSuccess())
        {
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(packageFilePath_, remoteObject_, L"ServicePackageFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteServicePackageFile: ErrorCode={0}, PackageFile={1}",
                    deleteError,
                    packageFilePath_);
            }

            return error;
        }
        servicePackageDescription_ = move(servicepackageDescription);

        // ensure manifest file is present
        manifestFilePath_ = owner_.RunLayout.GetServiceManifestFile(
            applicationIdStr_,
            servicePackageDescription_.ManifestName,
            servicePackageDescription_.ManifestVersion);

        wstring storeManifestFilePath = owner_.StoreLayout.GetServiceManifestFile(
            servicePackageId_.ApplicationId.ApplicationTypeName,
            servicePackageDescription_.ManifestName,
            servicePackageDescription_.ManifestVersion);

        wstring storeManifestChechsumFilePath = owner_.StoreLayout.GetServiceManifestChecksumFile(
            servicePackageId_.ApplicationId.ApplicationTypeName,
            servicePackageDescription_.ManifestName,
            servicePackageDescription_.ManifestVersion);

        error = CopyFromStore(
            storeManifestFilePath,
            manifestFilePath_,
            storeManifestChechsumFilePath,
            servicePackageDescription_.ManifestChecksum);
        if (!error.IsSuccess()) { return error; }

        // parse manifest file
        ServiceManifestDescription serviceManifest;
        error = Parser::ParseServiceManifest(manifestFilePath_, serviceManifest);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseServiceManifestFile: ErrorCode={0}, ManifestFile={1}",
            error,
            manifestFilePath_);

        if (!error.IsSuccess())
        {
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(manifestFilePath_, storeManifestFilePath, L"ServiceManifestFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteServiceManifestFile: ErrorCode={0}, ManifestFile={1}",
                    deleteError,
                    manifestFilePath_);
            }

            return error;
        }

        error = GetCodePackages(servicePackageDescription_);
        if (!error.IsSuccess()) { return error; }

        error = GetConfigPackages(servicePackageDescription_);
        if (!error.IsSuccess()) { return error; }

        error = GetDataPackages(servicePackageDescription_);
        if (!error.IsSuccess()) { return error; }

        return error;
    }

    ErrorCode GetCodePackages(ServicePackageDescription const & servicePackage)
    {
        ErrorCode error = ErrorCodeValue::Success;
        containerImages_.clear();
        for (auto iter = servicePackage.DigestedCodePackages.begin();
            iter != servicePackage.DigestedCodePackages.end();
            ++iter)
        {
            std::wstring imageSource;
            if (iter->CodePackage.EntryPoint.ContainerEntryPoint.ImageName.empty() ||
                !iter->CodePackage.SetupEntryPoint.Program.empty())
            {

                wstring storeCodePackagePath = owner_.StoreLayout.GetCodePackageFolder(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->CodePackage.Name,
                    iter->CodePackage.Version);

                wstring storeCodePackageChecksumPath = owner_.StoreLayout.GetCodePackageChecksumFile(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->CodePackage.Name,
                    iter->CodePackage.Version);

                wstring runCodePackagePath = owner_.RunLayout.GetCodePackageFolder(
                    applicationIdStr_,
                    servicePackageId_.ServicePackageName,
                    iter->CodePackage.Name,
                    iter->CodePackage.Version,
                    iter->IsShared);

                if (HostingConfig::GetConfig().EnableProcessDebugging && !iter->DebugParameters.CodePackageLinkFolder.empty())
                {
                    ArrayPair<wstring, wstring> link;
                    link.key = runCodePackagePath;
                    link.value = iter->DebugParameters.CodePackageLinkFolder;
                    this->symbolicLinks_.push_back(link);
                }
                else if (iter->IsShared)
                {
                    wstring sharedCodePackagePath = owner_.SharedLayout.GetCodePackageFolder(
                        servicePackageId_.ApplicationId.ApplicationTypeName,
                        servicePackage.ManifestName,
                        iter->CodePackage.Name,
                        iter->CodePackage.Version);

                    error = CopySubPackageFromStore(
                        storeCodePackagePath,
                        sharedCodePackagePath,
                        storeCodePackageChecksumPath,
                        iter->ContentChecksum);

                    if (error.IsSuccess() && !Directory::IsSymbolicLink(runCodePackagePath))
                    {
                        ArrayPair<wstring, wstring> link;
                        link.key = runCodePackagePath;
                        link.value = sharedCodePackagePath;
                        this->symbolicLinks_.push_back(link);
                    }
                }
                else
                {
                    error = CopySubPackageFromStore(
                        storeCodePackagePath,
                        runCodePackagePath,
                        storeCodePackageChecksumPath,
                        iter->ContentChecksum);
                }
                if (!error.IsSuccess())
                {
                    return error;
                }

                if (!iter->CodePackage.EntryPoint.ContainerEntryPoint.FromSource.empty())
                {
                    imageSource = Path::Combine(runCodePackagePath, iter->CodePackage.EntryPoint.ContainerEntryPoint.FromSource);
                }
            }
            if (!iter->CodePackage.EntryPoint.ContainerEntryPoint.ImageName.empty())
            {
                wstring imageName = iter->CodePackage.EntryPoint.ContainerEntryPoint.ImageName;
                error = ContainerHelper::GetContainerHelper().GetContainerImageName(iter->ContainerPolicies.ImageOverrides, imageName);

                if (!error.IsSuccess())
                {
                    WriteError(
                        Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "Failed to get the image name to be downloaded with error {0}",
                        error);

                    return error;
                }

                this->containerImages_.push_back(
                    ContainerImageDescription(
                        imageName,
                        iter->ContainerPolicies.RepositoryCredentials));
            }
        }
        return error;
    }

    ErrorCode GetConfigPackages(ServicePackageDescription const & servicePackage)
    {
        ErrorCode error = ErrorCodeValue::Success;

        for (auto iter = servicePackage.DigestedConfigPackages.begin();
            iter != servicePackage.DigestedConfigPackages.end();
            ++iter)
        {
            wstring storeConfigPackagePath = owner_.StoreLayout.GetConfigPackageFolder(
                servicePackageId_.ApplicationId.ApplicationTypeName,
                servicePackage.ManifestName,
                iter->ConfigPackage.Name,
                iter->ConfigPackage.Version);

            wstring storeConfigPackageChecksumPath = owner_.StoreLayout.GetConfigPackageChecksumFile(
                servicePackageId_.ApplicationId.ApplicationTypeName,
                servicePackage.ManifestName,
                iter->ConfigPackage.Name,
                iter->ConfigPackage.Version);

            wstring runConfigPackagePath = owner_.RunLayout.GetConfigPackageFolder(
                applicationIdStr_,
                servicePackageId_.ServicePackageName,
                iter->ConfigPackage.Name,
                iter->ConfigPackage.Version,
                iter->IsShared);

            if (HostingConfig::GetConfig().EnableProcessDebugging && !iter->DebugParameters.ConfigPackageLinkFolder.empty())
            {
                ArrayPair<wstring, wstring> link;
                link.key = runConfigPackagePath;
                link.value = iter->DebugParameters.ConfigPackageLinkFolder;
                this->symbolicLinks_.push_back(link);
            }
            else if (iter->IsShared)
            {
                wstring sharedConfigPackagePath = owner_.SharedLayout.GetConfigPackageFolder(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->ConfigPackage.Name,
                    iter->ConfigPackage.Version);

                error = CopySubPackageFromStore(
                    storeConfigPackagePath,
                    sharedConfigPackagePath,
                    storeConfigPackageChecksumPath,
                    iter->ContentChecksum);

                if (error.IsSuccess() && !Directory::IsSymbolicLink(runConfigPackagePath))
                {
                    ArrayPair<wstring, wstring> link;
                    link.key = runConfigPackagePath;
                    link.value = sharedConfigPackagePath;
                    this->symbolicLinks_.push_back(link);
                }
            }
            else
            {
                error = CopySubPackageFromStore(
                    storeConfigPackagePath,
                    runConfigPackagePath,
                    storeConfigPackageChecksumPath,
                    iter->ContentChecksum);
            }
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        return error;
    }

    ErrorCode GetDataPackages(ServicePackageDescription const & servicePackage)
    {
        ErrorCode error = ErrorCodeValue::Success;

        for (auto iter = servicePackage.DigestedDataPackages.begin();
            iter != servicePackage.DigestedDataPackages.end();
            ++iter)
        {
            wstring storeDataPackagePath = owner_.StoreLayout.GetDataPackageFolder(
                servicePackageId_.ApplicationId.ApplicationTypeName,
                servicePackage.ManifestName,
                iter->DataPackage.Name,
                iter->DataPackage.Version);

            wstring storeDataPackageChecksumPath = owner_.StoreLayout.GetDataPackageChecksumFile(
                servicePackageId_.ApplicationId.ApplicationTypeName,
                servicePackage.ManifestName,
                iter->DataPackage.Name,
                iter->DataPackage.Version);

            wstring runDataPackagePath = owner_.RunLayout.GetDataPackageFolder(
                applicationIdStr_,
                servicePackageId_.ServicePackageName,
                iter->DataPackage.Name,
                iter->DataPackage.Version,
                iter->IsShared);

            if (HostingConfig::GetConfig().EnableProcessDebugging && !iter->DebugParameters.DataPackageLinkFolder.empty())
            {
                ArrayPair<wstring, wstring> link;
                link.key = runDataPackagePath;
                link.value = iter->DebugParameters.DataPackageLinkFolder;
                this->symbolicLinks_.push_back(link);
            }
            else if (iter->IsShared)
            {
                wstring sharedDataPackagePath = owner_.SharedLayout.GetDataPackageFolder(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->DataPackage.Name,
                    iter->DataPackage.Version);

                error = CopySubPackageFromStore(
                    storeDataPackagePath,
                    sharedDataPackagePath,
                    storeDataPackageChecksumPath,
                    iter->ContentChecksum);

                if (error.IsSuccess() && !Directory::IsSymbolicLink(runDataPackagePath))
                {
                    ArrayPair<wstring, wstring> link;
                    link.key = runDataPackagePath;
                    link.value = sharedDataPackagePath;
                    this->symbolicLinks_.push_back(link);
                }
            }
            else
            {
                error = CopySubPackageFromStore(
                    storeDataPackagePath,
                    runDataPackagePath,
                    storeDataPackageChecksumPath,
                    iter->ContentChecksum);
            }

            if (!error.IsSuccess())
            {
                return error;
            }
        }

        return error;
    }

    virtual ErrorCode OnRegisterComponent()
    {
        return owner_.hosting_.HealthManagerObj->RegisterSource(svcPkgInstanceIdForHealth_, applicationName_, healthPropertyId_);
    }

    virtual void OnUnregisterComponent()
    {
        owner_.hosting_.HealthManagerObj->UnregisterSource(svcPkgInstanceIdForHealth_, healthPropertyId_);
    }

    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, std::wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        owner_.hosting_.HealthManagerObj->ReportHealth(
            svcPkgInstanceIdForHealth_,
            healthPropertyId_,
            reportCode,
            description,
            sequenceNumber);
    }

private:
    ServicePackageIdentifier const servicePackageId_;
    ServicePackageInstanceIdentifier const svcPkgInstanceIdForHealth_;
    ServicePackageVersion const servicePackageVersion_;
    ServicePackageDescription servicePackageDescription_;
    wstring const applicationIdStr_;
    wstring packageFilePath_;
    wstring remoteObject_;
    wstring manifestFilePath_;
    wstring const healthPropertyId_;
    vector<ContainerImageDescription> containerImages_;
    ServicePackageActivationContext activationContext_;
};

// ********************************************************************************************************************
// DownloadManager.DownloadServiceManifestAsyncOperation Implementation
//
// Deploys service manifest and associated packages to node's imagecache and sharedpackages folder.
//
// ********************************************************************************************************************
// DownloadManager.DownloadServiceManifestAsyncOperation Implementation
//
class DownloadManager::DownloadServiceManifestAsyncOperation
    : public DownloadManager::DownloadAsyncOperation
{
    DENY_COPY(DownloadServiceManifestAsyncOperation)

public:
    DownloadServiceManifestAsyncOperation(
        __in DownloadManager & owner,
        wstring const & applicationTypeName,
        wstring const & applicationTypeVersion,
        wstring const & serviceManifestName,
        PackageSharingPolicyList const & sharingPolicies,
        ULONG maxFailureCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DownloadAsyncOperation(
        Kind::DownloadServiceManifestAsyncOperation,
        owner,
        ApplicationIdentifier() /*Empty ApplicationId*/,
        L"" /* empty applicationName*/,
        DownloadManager::GetOperationId(applicationTypeName, applicationTypeVersion, serviceManifestName),
        maxFailureCount,
        callback,
        parent),
        applicationTypeName_(applicationTypeName),
        applicationTypeVersion_(applicationTypeVersion),
        serviceManifestName_(serviceManifestName),
        sharingPolicies_(sharingPolicies),
        sharingScope_(0),
        cacheLayout_(owner.hosting_.ImageCacheFolder),
        containerImages_()
    {
    }

    virtual ~DownloadServiceManifestAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadServiceManifestAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginDownloadContent(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & thisSPtr)
    {
        if (!ManagementConfig::GetConfig().ImageCachingEnabled)
        {
            ErrorCode err(ErrorCodeValue::PreDeploymentNotAllowed, StringResource::Get(IDS_HOSTING_Predeployment_NotAllowed));
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Predeployment cannot be performed since ImageCache is not enabled");

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                err,
                callback,
                thisSPtr);
        }

        wstring storeAppManifestPath = owner_.StoreLayout.GetApplicationManifestFile(
            this->applicationTypeName_,
            this->applicationTypeVersion_);

        wstring cacheAppManifestPath = cacheLayout_.GetApplicationManifestFile(applicationTypeName_, applicationTypeVersion_);

        auto error = CopyFromStore(storeAppManifestPath, cacheAppManifestPath);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "DownloadServiceManifestOperation: CopyApplicationManifest for AppType {0} Version {1} returned: ErrorCode={2}",
            applicationTypeName_,
            applicationTypeVersion_,
            error);

        if (!error.IsSuccess())
        {
            auto err = ErrorCode(ErrorCodeValue::FileNotFound,
                wformatString("{0} {1}, {2}",
                StringResource::Get(IDS_HOSTING_AppManifestDownload_Failed),
                applicationTypeName_,
                applicationTypeVersion_));

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                err,
                callback,
                thisSPtr);
        }

        ApplicationManifestDescription appDescription;
        error = Parser::ParseApplicationManifest(cacheAppManifestPath, appDescription);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseApplicationManifestFile: ErrorCode={0}, PackageFile={1}",
            error,
            cacheAppManifestPath);

        if (!error.IsSuccess())
        {
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(cacheAppManifestPath, storeAppManifestPath, L"ApplicationManifestFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteApplicationManifestFile: ErrorCode={0}, PackageFile={1}",
                    deleteError,
                    cacheAppManifestPath);
            }

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                thisSPtr);
        }

        ServiceManifestDescription serviceDescription;
        wstring manifestVersion;
        vector<ContainerPoliciesDescription> containerPolicies;
        for (auto iter = appDescription.ServiceManifestImports.begin(); iter != appDescription.ServiceManifestImports.end(); iter++)
        {
            if (StringUtility::AreEqualCaseInsensitive((*iter).ServiceManifestRef.Name, serviceManifestName_))
            {
                manifestVersion = (*iter).ServiceManifestRef.Version;
                containerPolicies = iter->ContainerHostPolicies;
                break;
            }
        }
        if (manifestVersion.empty())
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Failed to find ServiceManifest with Name {0} in ApplicationManifest Name {1}, Version {2}",
                serviceManifestName_,
                applicationTypeName_,
                applicationTypeVersion_);

            auto err = ErrorCode(ErrorCodeValue::ServiceManifestNotFound,
                wformatString("{0} {1}", StringResource::Get(IDS_HOSTING_ServiceManifestNotFound), serviceManifestName_));

            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                err,
                callback,
                thisSPtr);
        }

        wstring storeManifestFilePath = owner_.StoreLayout.GetServiceManifestFile(
            applicationTypeName_,
            serviceManifestName_,
            manifestVersion);

        wstring storeManifestChechsumFilePath = owner_.StoreLayout.GetServiceManifestChecksumFile(
            applicationTypeName_,
            serviceManifestName_,
            manifestVersion);

        wstring expectedChecksum = L"";

        error = owner_.imageStore_->GetStoreChecksumValue(storeManifestChechsumFilePath, expectedChecksum);

        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                thisSPtr);
        }

        wstring cacheManifestPath = cacheLayout_.GetServiceManifestFile(applicationTypeName_, serviceManifestName_, manifestVersion);

        error = CopyFromStore(
            storeManifestFilePath,
            cacheManifestPath,
            storeManifestChechsumFilePath,
            expectedChecksum);
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                thisSPtr);
        }

        // parse manifest file
        ServiceManifestDescription serviceManifestDescription;
        error = Parser::ParseServiceManifest(cacheManifestPath, serviceManifestDescription);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseServiceManifestFile: ErrorCode={0}, ManifestFile={1}",
            error,
            cacheManifestPath);

        if (!error.IsSuccess())
        {
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(cacheManifestPath, storeManifestFilePath, L"ServiceManifestFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteServiceManifestFile: ErrorCode={0}, ManifestFile={1}",
                    deleteError,
                    cacheManifestPath);
            }
        }
        if (error.IsSuccess())
        {
            error = EnsurePackages(
                        serviceManifestDescription,
                        containerPolicies);
        }
        if (!error.IsSuccess() || containerImages_.empty())
        {
            containerImages_.clear();
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                thisSPtr);
        }

        return AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
            this->containerImages_,
            ServicePackageActivationContext(),
            this->owner_,
            callback,
            thisSPtr);
    }

    virtual ErrorCode EndDownloadContent(AsyncOperationSPtr const & operation)
    {
        if (!containerImages_.empty())
        {
            return DownloadContainerImagesAsyncOperation::End(operation);
        }
        else
        {
            return CompletedAsyncOperation::End(operation);
        }
    }

    virtual ErrorCode OnRegisterComponent()
    {
        // No health report for Fabric download
        return ErrorCodeValue::Success;
    }

    virtual void OnUnregisterComponent()
    {
        // No health report for Fabric download
    }

    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, std::wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        // No health report for Fabric download
        UNREFERENCED_PARAMETER(reportCode);
        UNREFERENCED_PARAMETER(description);
        UNREFERENCED_PARAMETER(sequenceNumber);
    }

    virtual bool IsNonRetryableError(ErrorCode const & error)
    {
        if (error.IsError(ErrorCodeValue::FileNotFound) ||
            error.IsError(ErrorCodeValue::ServiceManifestNotFound) ||
            error.IsError(ErrorCodeValue::PreDeploymentNotAllowed) ||
            error.IsError(ErrorCodeValue::InvalidPackageSharingPolicy))
        {
            return true;
        }
        return false;
    }

private:
    ErrorCode EnsurePackages(
        ServiceManifestDescription const & serviceManifestDescription,
        vector<ContainerPoliciesDescription> const & containerPolcies)
    {
        set<wstring> sharingPolicies;
        for (auto iter = sharingPolicies_.PackageSharingPolicy.begin(); iter != sharingPolicies_.PackageSharingPolicy.end(); ++iter)
        {
            if (!iter->SharedPackageName.empty())
            {
                sharingPolicies.insert(iter->SharedPackageName);
            }
            else if (iter->PackageSharingPolicyScope == ServicePackageSharingType::Enum::None)
            {
                return ErrorCode(ErrorCodeValue::InvalidPackageSharingPolicy, StringResource::Get(IDS_HOSTING_Invalid_PackageSharingPolicy));
            }
            sharingScope_ |= iter->PackageSharingPolicyScope;
        }

        bool sharingScopeCode = (sharingScope_ & ServicePackageSharingType::Code || sharingScope_ & ServicePackageSharingType::All);
        ErrorCode error(ErrorCodeValue::Success);
        for (auto iter = serviceManifestDescription.CodePackages.begin(); iter != serviceManifestDescription.CodePackages.end(); iter++)
        {
            std::wstring imageSource;
            if (iter->EntryPoint.ContainerEntryPoint.ImageName.empty() ||
                !iter->SetupEntryPoint.Program.empty())
            {
                wstring storeCodePackagePath = owner_.StoreLayout.GetCodePackageFolder(
                    applicationTypeName_,
                    serviceManifestName_,
                    iter->Name,
                    iter->Version);

                wstring storeCodePackageChecksumPath = owner_.StoreLayout.GetCodePackageChecksumFile(
                    applicationTypeName_,
                    serviceManifestName_,
                    iter->Name,
                    iter->Version);

                wstring cacheCodePackagePath = cacheLayout_.GetCodePackageFolder(
                    applicationTypeName_,
                    serviceManifestName_,
                    iter->Name,
                    iter->Version);

                wstring checksum;
                error = owner_.imageStore_->GetStoreChecksumValue(storeCodePackageChecksumPath, checksum);
                if (!error.IsSuccess())
                {
                    return error;
                }

                if (sharingScopeCode ||
                    sharingPolicies.find(iter->Name) != sharingPolicies.end())
                {
                    WriteNoise(Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "Sharing is enabled for Code package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                        iter->Name,
                        applicationTypeName_,
                        applicationTypeVersion_);

                    wstring sharedPath = owner_.SharedLayout.GetCodePackageFolder(applicationTypeName_, serviceManifestName_, iter->Name, iter->Version);
                    error = CopySubPackageFromStore(
                        storeCodePackagePath,
                        sharedPath,
                        storeCodePackageChecksumPath,
                        checksum);
                }
                else
                {
                    WriteNoise(Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "Sharing is not enabled for Code package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                        iter->Name,
                        applicationTypeName_,
                        applicationTypeVersion_);

                    error = CopySubPackageFromStore(
                        storeCodePackagePath,
                        cacheCodePackagePath,
                        storeCodePackageChecksumPath,
                        checksum,
                        false,
                        ImageStore::CopyFlag::Echo,
                        true);
                }
            }
            if (!iter->EntryPoint.ContainerEntryPoint.ImageName.empty())
            {
                RepositoryCredentialsDescription repositoryCredentials;
                ImageOverridesDescription imageOverrides;

                for (auto it = containerPolcies.begin(); it != containerPolcies.end(); ++it)
                {
                    if (StringUtility::AreEqualCaseInsensitive(it->CodePackageRef, iter->Name))
                    {
                        repositoryCredentials = it->RepositoryCredentials;
                        imageOverrides = it->ImageOverrides;
                        break;
                    }
                }

                wstring imageName = iter->EntryPoint.ContainerEntryPoint.ImageName;
                error = ContainerHelper::GetContainerHelper().GetContainerImageName(imageOverrides, imageName);

                if (!error.IsSuccess())
                {
                    WriteError(
                        Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "Failed to get the image name to be downloaded with error {0}",
                        error);

                    return error;
                }

                this->containerImages_.push_back(
                    ContainerImageDescription(
                        imageName,
                        repositoryCredentials));
            }
            if (!error.IsSuccess()) { return error; }
        }

        bool sharingScopeConfig = (sharingScope_ & ServicePackageSharingType::Config || sharingScope_ & ServicePackageSharingType::All);
        for (auto iter = serviceManifestDescription.ConfigPackages.begin(); iter != serviceManifestDescription.ConfigPackages.end(); iter++)
        {

            wstring storeConfigPackagePath = owner_.StoreLayout.GetConfigPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            wstring storeConfigPackageChecksumPath = owner_.StoreLayout.GetConfigPackageChecksumFile(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            wstring cacheConfigPackagePath = cacheLayout_.GetConfigPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            wstring checksum;
            error = owner_.imageStore_->GetStoreChecksumValue(storeConfigPackageChecksumPath, checksum);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (sharingScopeConfig || sharingPolicies.find(iter->Name) != sharingPolicies.end())
            {
                WriteNoise(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Sharing is enabled for Config package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                    iter->Name,
                    applicationTypeName_,
                    applicationTypeVersion_);

                wstring sharedPath = owner_.SharedLayout.GetConfigPackageFolder(applicationTypeName_, serviceManifestName_, iter->Name, iter->Version);
                error = CopySubPackageFromStore(
                    storeConfigPackagePath,
                    sharedPath,
                    storeConfigPackageChecksumPath,
                    checksum);
            }
            else
            {
                WriteNoise(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Sharing is not enabled for Config package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                    iter->Name,
                    applicationTypeName_,
                    applicationTypeVersion_);

                error = CopySubPackageFromStore(
                    storeConfigPackagePath,
                    cacheConfigPackagePath,
                    storeConfigPackageChecksumPath,
                    checksum,
                    false,
                    ImageStore::CopyFlag::Echo,
                    true);
            }
            if (!error.IsSuccess()) { return error; }
        }

        bool sharingScopeData = (sharingScope_ & ServicePackageSharingType::Data || sharingScope_ & ServicePackageSharingType::All);
        for (auto iter = serviceManifestDescription.DataPackages.begin(); iter != serviceManifestDescription.DataPackages.end(); iter++)
        {
            wstring dataChecksumPath = cacheLayout_.GetDataPackageChecksumFile(
                applicationTypeName_,
                serviceManifestName_,
                (*iter).Name,
                (*iter).Version);

            wstring storeDataPackagePath = owner_.StoreLayout.GetDataPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            wstring storeDataPackageChecksumPath = owner_.StoreLayout.GetDataPackageChecksumFile(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            wstring cacheDataPackagePath = cacheLayout_.GetDataPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            wstring checksum;
            error = owner_.imageStore_->GetStoreChecksumValue(storeDataPackageChecksumPath, checksum);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (sharingScopeData ||
                sharingPolicies.find(iter->Name) != sharingPolicies.end())
            {
                WriteNoise(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Sharing is enabled for Data package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                    iter->Name,
                    applicationTypeName_,
                    applicationTypeVersion_);

                wstring sharedPath = owner_.SharedLayout.GetConfigPackageFolder(applicationTypeName_, serviceManifestName_, iter->Name, iter->Version);
                error = CopySubPackageFromStore(
                    storeDataPackagePath,
                    sharedPath,
                    storeDataPackageChecksumPath,
                    checksum);
            }
            else
            {
                WriteNoise(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Sharing is not enabled for data package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                    iter->Name,
                    applicationTypeName_,
                    applicationTypeVersion_);
                error = CopySubPackageFromStore(
                    storeDataPackagePath,
                    cacheDataPackagePath,
                    storeDataPackageChecksumPath,
                    checksum,
                    false,
                    ImageStore::CopyFlag::Echo,
                    true);
            }
            if (!error.IsSuccess()) { return error; }
        }
        return ErrorCodeValue::Success;
    }

private:
    wstring const applicationTypeName_;
    wstring const applicationTypeVersion_;
    wstring const serviceManifestName_;
    StoreLayoutSpecification cacheLayout_;
    PackageSharingPolicyList sharingPolicies_;
    DWORD sharingScope_;
    vector<ContainerImageDescription> containerImages_;
};


// ********************************************************************************************************************
// DownloadManager.DownloadFabricUpgradePackageAsyncOperation Implementation
//
// Deploys the fabric pacakge to node's work folder
class DownloadManager::DownloadFabricUpgradePackageAsyncOperation
    : public DownloadManager::DownloadAsyncOperation
{
    DENY_COPY(DownloadFabricUpgradePackageAsyncOperation)

public:
    DownloadFabricUpgradePackageAsyncOperation(
        __in DownloadManager & owner,
        FabricVersion const & fabricVersion,
        ULONG maxFailureCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DownloadAsyncOperation(
        Kind::DownloadFabricUpgradePackageAsyncOperation,
        owner,
        ApplicationIdentifier() /*Empty ApplicationId*/,
        *SystemServiceApplicationNameHelper::SystemServiceApplicationName,
        DownloadManager::GetOperationId(fabricVersion),
        maxFailureCount,
        callback,
        parent),
        fabricVersion_(fabricVersion),
        healthPropertyId_(wformatString("FabricDownload:{0}", fabricVersion))
    {
    }

    virtual ~DownloadFabricUpgradePackageAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadFabricUpgradePackageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual AsyncOperationSPtr BeginDownloadContent(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & thisSPtr)
    {
        auto error = EnsurePatchFile();
        if (error.IsSuccess())
        {
            error = EnsureClusterManifestFile();
        }
        if (error.IsSuccess())
        {
            error = EnsureCurrentVersionFiles();
        }
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            error,
            callback,
            thisSPtr);
    }

    virtual ErrorCode OnRegisterComponent()
    {
        return owner_.hosting_.HealthManagerObj->RegisterSource(healthPropertyId_);
    }

    virtual void OnUnregisterComponent()
    {
        return owner_.hosting_.HealthManagerObj->UnregisterSource(healthPropertyId_);
    }

    virtual void OnReportHealth(SystemHealthReportCode::Enum reportCode, std::wstring const & description, FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        owner_.hosting_.HealthManagerObj->ReportHealth(
            healthPropertyId_,
            reportCode,
            description,
            sequenceNumber);
    }

private:
    ErrorCode EnsurePatchFile()
    {
        return CopyFabricCodePackage(fabricVersion_.CodeVersion.ToString());
    }

    ErrorCode EnsureClusterManifestFile()
    {
        wstring runLayoutClusterManifestFilePath = owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(fabricVersion_.ConfigVersion.ToString());
        wstring clusterManifestStorePath = owner_.fabricUpgradeStoreLayout_.GetClusterManifestFile(fabricVersion_.ConfigVersion.ToString());
        auto error = CopyFromStore(
            clusterManifestStorePath,
            runLayoutClusterManifestFilePath,
            L"" /* storeChecksumPath */,
            L"" /* expectedChecksumValue */,
            false /*refreshCache*/);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "EnsureClusterManifestFile: Downloading the ClusterManifest file. ErrorCode={0}, ClusterManifestFilePath={1}",
            error,
            clusterManifestStorePath);

        return error;
    }

    ErrorCode EnsureCurrentVersionFiles()
    {
        auto nodeVersionInstance = owner_.hosting_.FabricNodeConfigObj.NodeVersion;

        if (nodeVersionInstance.Version.ConfigVersion != fabricVersion_.ConfigVersion)
        {
            wstring storeLayoutClusterManifestPath = owner_.fabricUpgradeStoreLayout_.GetClusterManifestFile(nodeVersionInstance.Version.ConfigVersion.ToString());
            wstring runLayoutClusterManifestPath = owner_.fabricUpgradeRunLayout_.GetClusterManifestFile(nodeVersionInstance.Version.ConfigVersion.ToString());

            auto error = CopyFromStore(
                storeLayoutClusterManifestPath,
                runLayoutClusterManifestPath,
                L"" /* storeChecksumPath */,
                L"" /* expectedChecksumValue */,
                false /*refreshCache*/);

            WriteTrace(
                error.ToLogLevel(),
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "EnsureCurrentVersionFiles ClusterManfiest: Downloading the ClusterManifest file. ErrorCode={0}, ClusterManifestFilePath={1}",
                error,
                storeLayoutClusterManifestPath);

            if (!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::FileNotFound))
                {
                    // ignore this failure. rollback will NOT be supported during Code upgrade.
                }
                else
                {
                    return error;
                }
            }
        }

        if (nodeVersionInstance.Version.CodeVersion != fabricVersion_.CodeVersion)
        {
            auto error = CopyFabricCodePackage(nodeVersionInstance.Version.CodeVersion.ToString());

            if (!error.IsSuccess())
            {
                if (error.IsError(ErrorCodeValue::FileNotFound))
                {
                    // ignore this failure. rollback will NOT be supported during Code upgrade.
                }
                else
                {
                    return error;
                }
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode CopyFabricCodePackage(std::wstring const & codePackageVersion)
    {
        vector<pair<wstring, wstring>> packagePaths;
        packagePaths.push_back(make_pair(
            owner_.fabricUpgradeStoreLayout_.GetPatchFile(codePackageVersion),
            owner_.fabricUpgradeRunLayout_.GetPatchFile(codePackageVersion)));
        packagePaths.push_back(make_pair(
            owner_.fabricUpgradeStoreLayout_.GetCabPatchFile(codePackageVersion),
            owner_.fabricUpgradeRunLayout_.GetCabPatchFile(codePackageVersion)));
        packagePaths.push_back(make_pair(
            owner_.fabricUpgradeStoreLayout_.GetCodePackageFolder(codePackageVersion),
            owner_.fabricUpgradeRunLayout_.GetCodePackageFolder(codePackageVersion)));

        // Check if any package type exists in cache. If so, use this type to copy.
        bool exists = false;
        ErrorCode error;
        wstring storePath = L"";
        wstring runPath = L"";
        for (auto& packagePath : packagePaths)
        {
            error = owner_.imageStore_->DoesContentExistInCache(packagePath.first, exists);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "CopyFabricCodePackage: Error {0} checking if content {1} exists in cache.",
                    error,
                    packagePath.first);
                return error;
            }

            if (exists)
            {
                storePath = packagePath.first;
                runPath = packagePath.second;
                break;
            }
        }

        // If no package existed in cache, check the same list in the image store.
        if (!exists)
        {
            for (auto& packagePath : packagePaths)
            {
                error = ContentExistsInStore(packagePath.first, exists);
                if (!error.IsSuccess())
                {
                    WriteTrace(
                        error.ToLogLevel(),
                        Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "CopyFabricCodePackage: Error {0} while checking if the content {1} exists on the store.",
                        error,
                        packagePath);
                    return error;
                }

                if (exists)
                {
                    storePath = packagePath.first;
                    runPath = packagePath.second;
                    break;
                }
            }
        }

        if (!exists)
        {
            WriteError(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "CopyFabricCodePackage: No package with version {0} exists on the image store.",
                codePackageVersion);
            return ErrorCodeValue::FileNotFound;
        }

        error = CopyFromStore(
            storePath,
            runPath,
            L"" /* storeChecksumPath */,
            L"" /* expectedChecksumValue */,
            false /*refreshCache*/);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "CopyFabricCodePackage: Downloading Fabric Code Package. ErrorCode={0}, CodePackagePath={1}",
            error,
            storePath);

        return error;
    }

    ErrorCode  ContentExistsInStore(wstring const & content, bool & exists)
    {
        exists = false;
        vector<wstring> contents;
        contents.push_back(content);
        map<wstring, bool> result;
        auto error = owner_.imageStore_->DoesContentExist(contents, ServiceModelConfig::GetConfig().MaxOperationTimeout, result);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "ContentExistsInStore: Checked if contents exists in store. ErrorCode={0}, content={1}",
                error,
                content);
            return error;
        }

        auto it = result.find(content);
        exists = (it != result.end()) && it->second;
        return error;
    }

private:
    FabricVersion const fabricVersion_;
    wstring const healthPropertyId_;
};

// ********************************************************************************************************************
// DownloadManager.DownloadApplicationAsyncOperation Implementation
//
// Downloads application and packages based on the DownloadSpecification
//
class DownloadManager::DownloadApplicationAsyncOperation : public AsyncOperation,
    private TextTraceComponent < TraceTaskCodes::Hosting >
{
    DENY_COPY(DownloadApplicationAsyncOperation)

public:
    DownloadApplicationAsyncOperation(
        __in DownloadManager & owner,
        ApplicationDownloadSpecification const & downloadSpecification,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        downloadSpec_(downloadSpecification),
        statusMap_(),
        packageCount_(0),
        lastError_()
    {
        statusMap_ = make_shared<OperationStatusMap>();
    }

    virtual ~DownloadApplicationAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out OperationStatusMapSPtr & appPendingDownload)
    {
        auto thisPtr = AsyncOperation::End<DownloadApplicationAsyncOperation>(operation);
        appPendingDownload = thisPtr->statusMap_;
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "DownloadApplication: DownloadSpec={0}",
            downloadSpec_);
        ASSERT_IF(downloadSpec_.ApplicationId.IsAdhoc(), "Cannot download an ad-hoc Application Package.");

        InitializeStatus();
        DownloadApplicationPackage(thisSPtr);
    }

private:
    void InitializeStatus()
    {
        statusMap_->Initialize(DownloadManager::GetOperationId(downloadSpec_.ApplicationId, downloadSpec_.AppVersion));
        for (auto iter = downloadSpec_.PackageDownloads.begin(); iter != downloadSpec_.PackageDownloads.end(); ++iter)
        {
            statusMap_->Initialize(DownloadManager::GetOperationId(
                ServicePackageIdentifier(downloadSpec_.ApplicationId, iter->ServicePackageName),
                ServicePackageVersion(downloadSpec_.AppVersion, iter->RolloutVersionValue)));
        }
    }

    void DownloadApplicationPackage(AsyncOperationSPtr const & thisSPtr)
    {
        statusMap_->SetState(DownloadManager::GetOperationId(downloadSpec_.ApplicationId, downloadSpec_.AppVersion), OperationState::InProgress);

        auto operation = owner_.BeginDownloadApplicationPackage(
            downloadSpec_.ApplicationId,
            downloadSpec_.AppVersion,
            downloadSpec_.AppName,
            [this](AsyncOperationSPtr const & operation){ this->FinishApplicationPackageDownload(operation, false); },
            thisSPtr);
        this->FinishApplicationPackageDownload(operation, true);
    }

    void FinishApplicationPackageDownload(AsyncOperationSPtr const & operation, bool expectCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectCompletedSynchronously)
        {
            return;
        }

        OperationStatus downloadStatus;
        ApplicationPackageDescription appPackageDescription;

        statusMap_->Set(DownloadManager::GetOperationId(downloadSpec_.ApplicationId, downloadSpec_.AppVersion), downloadStatus);
        auto error = owner_.EndDownloadApplicationPackage(
            operation,
            downloadStatus,
            appPackageDescription);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        DownloadServicePackages(operation->Parent);
    }

    void DownloadServicePackages(AsyncOperationSPtr const & thisSPtr)
    {
        packageCount_.store(downloadSpec_.PackageDownloads.size());

        for (auto iter = downloadSpec_.PackageDownloads.begin(); iter != downloadSpec_.PackageDownloads.end(); ++iter)
        {
            auto packageDownload = (*iter);
            auto servicePackageId = ServicePackageIdentifier(downloadSpec_.ApplicationId, packageDownload.ServicePackageName);
            auto servicePackageVersion = ServicePackageVersion(downloadSpec_.AppVersion, packageDownload.RolloutVersionValue);

            WriteNoise(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Begin(DownloadServicePackage): ServicePackageId={0}, ServicePackageVersion={1}",
                servicePackageId,
                servicePackageVersion);

            statusMap_->SetState(
                DownloadManager::GetOperationId(servicePackageId, servicePackageVersion),
                OperationState::InProgress);

            auto operation = owner_.BeginDownloadServicePackage(
                servicePackageId,
                servicePackageVersion,
                ServicePackageActivationContext(),
                downloadSpec_.AppName,
                [this, servicePackageId, servicePackageVersion](AsyncOperationSPtr const & operation)
            {
                this->FinishDownloadServicePackage(operation, servicePackageId, servicePackageVersion, false);
            },
                thisSPtr);
            FinishDownloadServicePackage(operation, servicePackageId, servicePackageVersion, true);
        }

        CheckPendingDownloads(thisSPtr, downloadSpec_.PackageDownloads.size());
    }

    void FinishDownloadServicePackage(
        AsyncOperationSPtr const & operation,
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageVersion const & servicePackageVersion,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        OperationStatus downloadStatus;
        ServicePackageDescription packageDescription;
        auto error = owner_.EndDownloadServicePackage(operation, downloadStatus, packageDescription);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "End(DownloadServicePackage): ServicePackageId={0}, ServicePackageVersion={1}, ErrorCode={2}",
            servicePackageId,
            servicePackageVersion,
            error);
        if (!error.IsSuccess())
        {
            lastError_.Overwrite(error);
        }

        statusMap_->Set(DownloadManager::GetOperationId(servicePackageId, servicePackageVersion), downloadStatus);

        uint64 currentPackageCount = --packageCount_;
        CheckPendingDownloads(operation->Parent, currentPackageCount);
    }

    void CheckPendingDownloads(AsyncOperationSPtr const & thisSPtr, uint64 currentPackageCount)
    {
        if (currentPackageCount == 0)
        {
            TryComplete(thisSPtr, lastError_);
            return;
        }
    }

private:
    DownloadManager & owner_;
    ApplicationDownloadSpecification const downloadSpec_;
    OperationStatusMapSPtr statusMap_;
    atomic_uint64 packageCount_;
    ErrorCode lastError_;
};

// ********************************************************************************************************************
// DownloadManager Implementation
//

DownloadManager::DownloadManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting,
    FabricNodeConfigSPtr const & nodeConfig)
    : RootedObject(root),
    hosting_(hosting),
    runLayout_(hosting.DeploymentFolder),
    storeLayout_(),
    fabricUpgradeRunLayout_(hosting.FabricUpgradeDeploymentFolder),
    fabricUpgradeStoreLayout_(),
    sharedLayout_(Path::Combine(hosting.DeploymentFolder, Constants::SharedFolderName)),
    imageStore_(),
    nodeConfig_(nodeConfig),
    pendingDownloads_(),
    nonRetryableFailedDownloads_(),
    passThroughClientFactoryPtr_(),
    ntlmUserThumbprints_(),
    pendingNTLMUserThumbprints_(),
    refreshNTLMUsersTimer_(),
    refreshNTLMUsersTimerLock_()
{
    auto pendingDownloads = make_unique<PendingOperationMap>();
    pendingDownloads_ = move(pendingDownloads);
}

DownloadManager::~DownloadManager()
{
}

AsyncOperationSPtr DownloadManager::OnBeginOpen(
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

ErrorCode DownloadManager::OnEndOpen(
    AsyncOperationSPtr const & asyncOperation)
{
    return OpenAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr DownloadManager::OnBeginClose(
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

ErrorCode DownloadManager::OnEndClose(
    AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void DownloadManager::OnAbort()
{
    CloseRefreshNTLMSecurityUsersTimer();
    ClosePendingDownloads();
    nonRetryableFailedDownloads_.Close();
}

void DownloadManager::ClosePendingDownloads()
{
    auto removed = pendingDownloads_->Close();
    for (auto iter = removed.begin(); iter != removed.end(); ++iter)
    {
        (*iter)->Cancel();
    }
}

void DownloadManager::ScheduleRefreshNTLMSecurityUsersTimer()
{
    AcquireWriteLock lock(refreshNTLMUsersTimerLock_);
    auto root = this->Root.CreateComponentRoot();
    ASSERT_IF(refreshNTLMUsersTimer_, "ScheduleRefreshNTLMSecurityUsersTimer: the timer already exists");
    refreshNTLMUsersTimer_ = Timer::Create(
        UpdateFileStoreServicePrincipalTimerTag,
        [this, root](TimerSPtr const &) { this->RefreshNTLMSecurityUsers(); },
        false /*allowConcurrency*/);

    refreshNTLMUsersTimer_->Change(
        HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval,
        HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshInterval);
}

void DownloadManager::CloseRefreshNTLMSecurityUsersTimer()
{
    AcquireWriteLock lock(refreshNTLMUsersTimerLock_);
    if (refreshNTLMUsersTimer_)
    {
        refreshNTLMUsersTimer_->Cancel();
        refreshNTLMUsersTimer_.reset();
    }
}

void DownloadManager::RefreshNTLMSecurityUsers()
{
    StartRefreshNTLMSecurityUsers(
        true /*updateExisting*/,
        HostingConfig::GetConfig().NTLMSecurityUsersByX509CommonNamesRefreshTimeout,
        [this](AsyncOperationSPtr const & /*root*/, Common::ErrorCode const & error)
        {
            // Tolerate error. The errors may be because of timeout during configuration of the users etc
            error.ReadValue();
        },
        this->Root.CreateAsyncOperationRoot());
}

std::vector<std::wstring> DownloadManager::Test_GetNtlmUserThumbprints() const
{
    AcquireReadLock lock(refreshNTLMUsersTimerLock_);
    return ntlmUserThumbprints_;
}

void DownloadManager::StartRefreshNTLMSecurityUsers(
    bool updateExisting,
    Common::TimeSpan const timeout,
    RefreshNTLMSecurityUsersCallback const & onCompleteCallback,
    Common::AsyncOperationSPtr const & parent)
{
    PrincipalsDescriptionUPtr principals;
    TimeoutHelper timeoutHelper(timeout);
    ErrorCode error(ErrorCodeValue::Success);
    size_t previousThumbprintCount = 0;
    size_t newThumbprintCount = 0;

    { // lock
        AcquireWriteLock lock(refreshNTLMUsersTimerLock_);
        if (pendingNTLMUserThumbprints_)
        {
            // Update already in progress
            error = ErrorCode(ErrorCodeValue::UpdatePending);
        }
        else
        {
            previousThumbprintCount = ntlmUserThumbprints_.size();

            // Get a principal with the new found users.
            // Use previously configured thumbprints to only get new users.
            vector<wstring> newThumbprints;
            error = Management::FileStoreService::Utility::GetPrincipalsDescriptionFromConfigByCommonName(
                ntlmUserThumbprints_,
                principals,
                newThumbprints);
            if (error.IsSuccess() && !newThumbprints.empty())
            {
                newThumbprintCount = newThumbprints.size();
                pendingNTLMUserThumbprints_ = make_unique<vector<wstring>>(move(newThumbprints));
            }
        }
    } // endlock

    if (!error.IsSuccess())
    {
        onCompleteCallback(parent, error);
        return;
    }

    if (newThumbprintCount + previousThumbprintCount == 0)
    {
        WriteWarning(
            Trace_DownloadManager,
            this->Root.TraceId,
            "ConfigureNTLMSecurityUsersByX509CommonNames: no certificates with specified common names found, can't create users.");
        onCompleteCallback(
            parent,
            ErrorCode::Success()); // return success because for SFRP the cluster manifest is upgraded before cert is installed
        return;
    }

    if (newThumbprintCount == 0 || principals == NULL)
    {
        WriteInfo(
            Trace_DownloadManager,
            this->Root.TraceId,
            "RefreshNTLMSecurityUsers: no new thumbprints found, done.");
        onCompleteCallback(parent, error);
        return;
    }

    WriteInfo(
        Trace_DownloadManager,
        this->Root.TraceId,
        "RefreshNTLMSecurityUsers: Begin(ConfigureSecurityPrincipalsForNode): {0} new users.", principals->Users.size());

    // Create a pointer to avoid copying the vector or thumbprints
    auto operation = hosting_.FabricActivatorClientObj->BeginConfigureSecurityPrincipalsForNode(
        ApplicationIdentifier::FabricSystemAppId->ToString(),
        ApplicationIdentifier::FabricSystemAppId->ApplicationNumber,
        *principals,
        0 /*allowedUserCreationFailureCount*/,
        updateExisting,
        timeoutHelper.GetRemainingTime(),
        [this, onCompleteCallback](AsyncOperationSPtr const & operation)
    {
        this->OnRefreshNTLMSecurityUsersCompleted(operation, onCompleteCallback, false);
    },
        parent);
    this->OnRefreshNTLMSecurityUsersCompleted(operation, onCompleteCallback, true);
}

void DownloadManager::OnRefreshNTLMSecurityUsersCompleted(
    Common::AsyncOperationSPtr const & operation,
    RefreshNTLMSecurityUsersCallback const & callback,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    PrincipalsProviderContextUPtr context;
    auto error = hosting_.FabricActivatorClientObj->EndConfigureSecurityPrincipalsForNode(operation, context);

    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        this->Root.TraceId,
        "End(ConfigureSecurityPrincipalsForNode): FileStoreService common names accounts completed: {0}",
        error);

    if (error.IsSuccess())
    {
        // Since the update was successful, remember the new thumbprints so that future refreshes ignore them.
        AcquireWriteLock lock(refreshNTLMUsersTimerLock_);
        ASSERT_IFNOT(pendingNTLMUserThumbprints_, "OnRefreshNTLMSecurityUsersCompleted: pendingNTLMUserThumbprints_ should be set");
        for (auto && thumbprint : *pendingNTLMUserThumbprints_)
        {
            ntlmUserThumbprints_.push_back(move(thumbprint));
        }

        pendingNTLMUserThumbprints_.reset();
    }

    callback(operation->Parent, error);
}

AsyncOperationSPtr DownloadManager::BeginDownloadApplication(
    ApplicationDownloadSpecification const & appDownloadSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadApplicationAsyncOperation>(
        *this,
        appDownloadSpec,
        callback,
        parent);
}

ErrorCode DownloadManager::EndDownloadApplication(
    AsyncOperationSPtr const & operation,
    __out OperationStatusMapSPtr & appPendingDownload)
{
    return DownloadApplicationAsyncOperation::End(operation, appPendingDownload);
}

AsyncOperationSPtr DownloadManager::BeginDownloadApplicationPackage(
    ApplicationIdentifier const & applicationId,
    ApplicationVersion const & applicationVersion,
    wstring const & applicationName,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadApplicationPackageAsyncOperation>(
        *this,
        applicationId,
        applicationVersion,
        applicationName,
        HostingConfig::GetConfig().DeploymentMaxFailureCount,
        callback,
        parent);
}

AsyncOperationSPtr DownloadManager::BeginDownloadApplicationPackage(
    ApplicationIdentifier const & applicationId,
    ApplicationVersion const & applicationVersion,
    wstring const & applicationName,
    ULONG maxFailureCount,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadApplicationPackageAsyncOperation>(
        *this,
        applicationId,
        applicationVersion,
        applicationName,
        maxFailureCount,
        callback,
        parent);
}

ErrorCode DownloadManager::EndDownloadApplicationPackage(
    AsyncOperationSPtr const & operation,
    __out OperationStatus & downloadStatus,
    __out ApplicationPackageDescription & applicationPackageDescription)
{
    return DownloadApplicationPackageAsyncOperation::End(operation, downloadStatus, applicationPackageDescription);
}

AsyncOperationSPtr DownloadManager::BeginDownloadServicePackage(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageVersion const & servicePackageVersion,
    ServicePackageActivationContext const & activationContext,
    wstring const & applicationName,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return BeginDownloadServicePackage(
        servicePackageId,
        servicePackageVersion,
        activationContext,
        applicationName,
        HostingConfig::GetConfig().DeploymentMaxFailureCount,
        callback,
        parent);
}

AsyncOperationSPtr DownloadManager::BeginDownloadServicePackage(
    ServicePackageIdentifier const & servicePackageId,
    ServicePackageVersion const & servicePackageVersion,
    ServicePackageActivationContext const & activationContext,
    wstring const & applicationName,
    ULONG maxFailureCount,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadServicePackageAsyncOperation>(
        *this,
        servicePackageId,
        servicePackageVersion,
        activationContext,
        applicationName,
        maxFailureCount,
        callback,
        parent);
}

ErrorCode DownloadManager::EndDownloadServicePackage(
    AsyncOperationSPtr const & operation,
    __out OperationStatus & downloadStatus,
    __out ServicePackageDescription & servicePackageDescription)
{
    return DownloadServicePackageAsyncOperation::End(operation, downloadStatus, servicePackageDescription);
}

AsyncOperationSPtr DownloadManager::BeginDownloadFabricUpgradePackage(
    FabricVersion const & fabricVersion,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadFabricUpgradePackageAsyncOperation>(
        *this,
        fabricVersion,
        HostingConfig::GetConfig().DeploymentMaxFailureCount,
        callback,
        parent);
}

ErrorCode DownloadManager::EndDownloadFabricUpgradePackage(AsyncOperationSPtr const & operation)
{
    return DownloadFabricUpgradePackageAsyncOperation::End(operation);
}

AsyncOperationSPtr DownloadManager::BeginDownloadServiceManifestPackages(
    wstring const & applicationTypeName,
    wstring const & applicationTypeVersion,
    wstring const & serviceManifestName,
    PackageSharingPolicyList const & packageSharingPolicies,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadServiceManifestAsyncOperation>(
        *this,
        applicationTypeName,
        applicationTypeVersion,
        serviceManifestName,
        packageSharingPolicies,
        HostingConfig::GetConfig().DeploymentMaxFailureCount,
        callback,
        parent);
}

ErrorCode DownloadManager::EndDownloadServiceManifestPackages(AsyncOperationSPtr const & operation)
{
    return DownloadServiceManifestAsyncOperation::End(operation);
}

wstring DownloadManager::GetOperationId(ApplicationIdentifier const & applicationId, ApplicationVersion const & applicationVersion)
{
    return wformatString("Download:{0}:{1}", applicationId, applicationVersion);
}

wstring DownloadManager::GetOperationId(ServicePackageIdentifier const & servicePackageId, ServicePackageVersion const & servicePackageVersion)
{
    return wformatString("Download:{0}:{1}", servicePackageId, servicePackageVersion);
}

wstring DownloadManager::GetOperationId(FabricVersion const & fabricVersion)
{
    return wformatString("Download:{0}", fabricVersion);
}

wstring DownloadManager::GetOperationId(wstring const & applicationTypeName, wstring const & applicationTypeVersion, wstring const & serviceManifestName)
{
    return wformatString("Download:{0}:{1}:{2}", applicationTypeName, applicationTypeVersion, serviceManifestName);
}

ErrorCode DownloadManager::GetApplicationsQueryResult(std::wstring const & filterApplicationName, __out vector<DeployedApplicationQueryResult> & deployedApplications, wstring const & continuationToken)
{
    vector<AsyncOperationSPtr> pendingAsyncOperations;
    auto error = pendingDownloads_->GetPendingAsyncOperations(pendingAsyncOperations);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto iter = pendingAsyncOperations.begin(); iter != pendingAsyncOperations.end(); ++iter)
    {
        auto downloadAsyncOperation = AsyncOperation::Get<DownloadAsyncOperation>(*iter);
        if (downloadAsyncOperation->kind_ == DownloadAsyncOperation::Kind::DownloadApplicationPackageAsyncOperation)
        {
            wstring applicationName;
            ApplicationIdentifier applicationId;
            auto downloadApplicationPackageAsyncOperation = AsyncOperation::Get<DownloadApplicationPackageAsyncOperation>(*iter);
            downloadApplicationPackageAsyncOperation->GetApplicationInfo(applicationName, applicationId);

            auto queryResult = DeployedApplicationQueryResult(applicationName, applicationId.ApplicationTypeName, DeploymentStatus::Downloading);

            // If the application name doesn't respect the continuation token, then skip this item
            if (!continuationToken.empty() && (continuationToken >= applicationName))
            {
                continue;
            }

            if (filterApplicationName.empty())
            {
                deployedApplications.push_back(move(queryResult));
            }
            else if (filterApplicationName == applicationName)
            {
                deployedApplications.push_back(move(queryResult));
                return ErrorCodeValue::Success;
            }
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode DownloadManager::GetServiceManifestQueryResult(std::wstring const & filterApplicationName, std::wstring const & filterServiceManifestName, vector<DeployedServiceManifestQueryResult> & deployedServiceManifests)
{
    ASSERT_IF(filterApplicationName.empty(), "filterApplicationName should not be empty");

    vector<AsyncOperationSPtr> pendingAsyncOperations;
    auto error = pendingDownloads_->GetPendingAsyncOperations(pendingAsyncOperations);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto iter = pendingAsyncOperations.begin(); iter != pendingAsyncOperations.end(); ++iter)
    {
        auto downloadAsyncOperation = AsyncOperation::Get<DownloadAsyncOperation>(*iter);
        if (downloadAsyncOperation->kind_ == DownloadAsyncOperation::Kind::DownloadServicePackageAsyncOperation)
        {
            wstring applicationName, serviceManifestName;
            auto downloadServicePackageAsyncOperation = AsyncOperation::Get<DownloadServicePackageAsyncOperation>(*iter);
            downloadServicePackageAsyncOperation->GetServicePackageInfo(applicationName, serviceManifestName);

            if (filterApplicationName != applicationName) { continue; }

            auto queryResult = DeployedServiceManifestQueryResult(
                serviceManifestName,
                L"", /* ServicePackage ActivationId not known when downloading*/
                L"",  /*ServiceManifest version not known when downloading*/
                DeploymentStatus::Downloading);

            if (filterServiceManifestName.empty())
            {
                deployedServiceManifests.push_back(move(queryResult));
            }
            else if (StringUtility::AreEqualCaseInsensitive(filterServiceManifestName, serviceManifestName))
            {
                deployedServiceManifests.push_back(move(queryResult));
                return ErrorCodeValue::Success;
            }
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode DownloadManager::DeleteCorruptedFile(std::wstring const & localFilePath, std::wstring const & remoteObject, std::wstring const & type)
{
    if (!File::Exists(localFilePath))
    {
        return ErrorCodeValue::NotFound;
    }

    FileWriterLock writerLock(localFilePath);
    ErrorCode error = writerLock.Acquire();
    if (!error.IsSuccess())
    {
        return error;
    }

    error = File::Delete2(localFilePath);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = this->imageStore_->RemoveFromCache(remoteObject);
    if (error.IsSuccess())
    {
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            Root.TraceId,
            "DeleteCorrupted{0}: ErrorCode={1}, PackageFile={2}",
            type,
            error,
            localFilePath);
    }

    return error;
}

ErrorCode DownloadManager::DeployFabricSystemPackages(TimeSpan timeout)
{
    wstring fabricSystemAppIdStr = ApplicationIdentifier::FabricSystemAppId->ToString();
    wstring sourceFolder = Path::Combine(hosting_.FabricBinFolder, fabricSystemAppIdStr);

    if (!Directory::Exists(sourceFolder))
    {
        // no fabric system applications are needed to be deployed
        return ErrorCode(ErrorCodeValue::Success);
    }

    // ensure work, log and temp folders
    auto error = EnsureApplicationFolders(fabricSystemAppIdStr);
    if (!error.IsSuccess())
    {
        return error;
    }

    wstring destinationFolder = RunLayout.GetApplicationFolder(fabricSystemAppIdStr);
    TimeoutHelper timeoutHelper(timeout);
    TimeSpan backoff = HostingConfig::GetConfig().DeployFabricSystemPackageRetryBackoff;

    do
    {
        error = Directory::Copy(sourceFolder, destinationFolder, true);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            Root.TraceId,
            "DeployFabricSystemPackages: CopyFolder: Source={0}, Destination={1}, ErrorCode={2}",
            sourceFolder,
            destinationFolder,
            error);
        if (error.IsSuccess())
        {
            return ErrorCodeValue::Success;
        }

        // Sometimes copying the system application folder can hit a file in use error.  Backoff and retry.
        // The reason for the backoff is because in some cases Copy() above will immediately return with an error.
        Sleep(backoff.Milliseconds);
    } while (timeoutHelper.GetRemainingTime() > TimeSpan::Zero);

    return error;
}

ErrorCode DownloadManager::EnsureApplicationFolders(std::wstring const & applicationId, bool createLogFolder)
{
    // ensure log, work and temp folders for the application
    auto error = Directory::Create2(RunLayout.GetApplicationWorkFolder(applicationId));
    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        Root.TraceId,
        "DeployFabricSystemPackages: CreateWorkFolder: {0}, ErrorCode={1}",
        RunLayout.GetApplicationWorkFolder(applicationId),
        error);
    if (!error.IsSuccess()) { return error; }

    //For Fabric Package create the Log Folder.
    //For Application Package we will check if symbolic link needs to be setup
    if (createLogFolder)
    {
        error = EnsureApplicationLogFolder(applicationId);
        if (!error.IsSuccess()) { return error; }
    }

    error = Directory::Create2(RunLayout.GetApplicationTempFolder(applicationId));
    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        Root.TraceId,
        "DeployFabricSystemPackages: CreateTempFolder: {0}, ErrorCode={1}",
        RunLayout.GetApplicationTempFolder(applicationId),
        error);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode DownloadManager::EnsureApplicationLogFolder(std::wstring const & applicationId)
{
    auto error = Directory::Create2(RunLayout.GetApplicationLogFolder(applicationId));
    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        Root.TraceId,
        "DeployFabricSystemPackages: CreateLogFolder: {0}, ErrorCode={1}",
        RunLayout.GetApplicationLogFolder(applicationId),
        error);
    return error;
}

void DownloadManager::InitializePassThroughClientFactory(Api::IClientFactoryPtr const &passThroughClientFactoryPtr)
{
    passThroughClientFactoryPtr_ = passThroughClientFactoryPtr;
}
