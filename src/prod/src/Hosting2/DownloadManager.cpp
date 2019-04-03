// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace Management::CentralSecretService;
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
        ServicePackageActivationContext const & activationContext,
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
        downloadId_(wformatString("{0}:{1}", downloadId, activationContext.ToString())),
        retryTimer_(),
        lock_(),
        status_(),
        random_((int)SequenceNumber::GetNext()),
        maxFailureCount_(maxFailureCount)
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
        return owner_.ImageStore->DownloadContent(
            storePath,
            runPath,
            ServiceModelConfig::GetConfig().MaxOperationTimeout,
            storeChecksumPath,
            expectedChecksumValue,
            refreshCache,
            copyFlag,
            copyToLocalCacheOnly,
            checkForArchive);
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
                    return;
                }
            }
            // If failure is due to file not found, query CM to get status of ApplicationInstance
            if (error.IsError(ErrorCodeValue::FileNotFound) &&
                kind_ != Kind::DownloadFabricUpgradePackageAsyncOperation)
            {
                // passing in original error is necessary for a more descriptive error message
                QueryDeletedApplicationInstance(operation->Parent, retryCount, move(error));
                return;
            }

            // retry if possible
            if (retryCount + 1 < maxFailureCount_)
            {
                UpdatePendingDownload(error);
                ScheduleDownload(operation->Parent);
                return;
            }
            else
            {
                // hit retry limit
                if (TryStartComplete())
                {
                    CompletePendingDownload(error);
                    FinishComplete(operation->Parent, error);
                    return;
                }
            }
        }
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
            else
            {
                description = wformatString("{0}", error.ReadValue());
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

    void QueryDeletedApplicationInstance(AsyncOperationSPtr const & thisSPtr, ULONG const retryCount, ErrorCode && originalError)
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
            [this, retryCount, originalError](AsyncOperationSPtr const& operation) { this->OnGetDeletedApplicationsListCompleted(operation, retryCount, originalError); },
            thisSPtr);
    }

    void OnGetDeletedApplicationsListCompleted(AsyncOperationSPtr const & operation, ULONG const retryCount, ErrorCode const & originalError)
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
                    error = originalError;
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

private:
    ULONG const maxFailureCount_;
    RwLock lock_;
    TimerSPtr retryTimer_;
    Random random_;
};

class DownloadManager::DownloadPackagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadPackagesAsyncOperation)

public:
    DownloadPackagesAsyncOperation(
        __in DownloadManager & owner,
        vector<FileDownloadSpec> filesToDownload,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(
            callback,
            parent),
        owner_(owner),
        filesToDownload_(filesToDownload),
        pendingOperationCount_(filesToDownload.size()),
        lastError_(ErrorCodeValue::Success)
    {
    }

    virtual ~DownloadPackagesAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadPackagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto it = filesToDownload_.begin(); it != filesToDownload_.end(); ++it)
        {
            DownloadPackageContent(*it, it->Checksum, thisSPtr);
        }
    }

    void DownloadPackageContent(
        FileDownloadSpec const & downloadSpec,
        wstring const & expectedChecksum,
        AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(Trace_DownloadManager,
            owner_.Root.TraceId,
            "DownloadPackageContents for Code package remote location{0}:  local location ApplicationTypeName {1}, expectedChecksum {2}",
            downloadSpec.RemoteSourceLocation,
            downloadSpec.LocalDestinationLocation,
            expectedChecksum);

        auto source = downloadSpec.RemoteSourceLocation;
        auto target = downloadSpec.LocalDestinationLocation;
        auto operation = owner_.imageCacheManager_->BeginDownload(
            source,
            target,
            downloadSpec.ChecksumFileLocation,
            expectedChecksum,
            false,
            ImageStore::CopyFlag::Echo,
            downloadSpec.DownloadToCacheOnly,
            true,
            [this, source, target](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadPackageCompleted(operation, false, source, target);
        },
            thisSPtr);
        OnDownloadPackageCompleted(operation, true, source, target);
    }
    void OnDownloadPackageCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously,
        wstring const & source,
        wstring const & target)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.imageCacheManager_->EndDownload(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "DownloadPackagesAsyncOperation::OnDownloadPackageCompleted: ErrorCode={0}, source={1}, target={2}",
            error,
            source,
            target);
        DecrementAndCheckPendingOperations(operation->Parent, error);
    }

    void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {
        if (pendingOperationCount_.load() == 0)
        {
            Assert::CodingError("Pending operation count is already 0");
        }
        uint64 pendingOperationCount = --pendingOperationCount_;
        if (!error.IsSuccess())
        {
            lastError_.ReadValue();
            lastError_.Overwrite(error);
        }
        if (pendingOperationCount == 0)
        {
            TryComplete(thisSPtr, lastError_);
        }
    }
    

private:
    DownloadManager & owner_;
    vector<FileDownloadSpec> filesToDownload_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
};


class DownloadManager::DownloadApplicationPackageContentsAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadApplicationPackageContentsAsyncOperation)

public:
    DownloadApplicationPackageContentsAsyncOperation(
        __in DownloadManager & owner,
        ApplicationIdentifier const & applicationId,
        ApplicationVersion const & applicationVersion,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(
            callback,
            parent),
        owner_(owner),
        applicationId_(applicationId),
        applicationVersion_(applicationVersion),
        applicationIdStr_(applicationId.ToString()),
        applicationPackageDescription_(),
        applicationVersionStr_(applicationVersion.ToString())
    {
    }

    virtual ~DownloadApplicationPackageContentsAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadApplicationPackageContentsAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (applicationId_ != *ApplicationIdentifier::FabricSystemAppId)
        {
            auto error = owner_.EnsureApplicationFolders(applicationIdStr_, false);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Ensure application folders: ErrorCode={0}, ApplicationId={1}",
                    error,
                    applicationIdStr_);
            }
            applicationPackageFilePath_ = owner_.RunLayout.GetApplicationPackageFile(applicationIdStr_, applicationVersionStr_);
            remoteApplicationPackageFilePath_ = owner_.StoreLayout.GetApplicationPackageFile(applicationId_.ApplicationTypeName, applicationIdStr_, applicationVersionStr_);

            auto operation = owner_.imageCacheManager_->BeginDownload(
                remoteApplicationPackageFilePath_,
                applicationPackageFilePath_,
                L"",
                L"",
                false,
                ImageStore::CopyFlag::Echo,
                false,
                false,
                [this](AsyncOperationSPtr const & operation)
            {
                this->EnsureApplicationPackageFileContents(operation, false);
            },
                thisSPtr);
            this->EnsureApplicationPackageFileContents(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }


    void EnsureApplicationPackageFileContents(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.imageCacheManager_->EndDownload(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        // parse application package file
        error = Parser::ParseApplicationPackage(applicationPackageFilePath_, applicationPackageDescription_);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseApplicationPackageFile: ErrorCode={0}, ApplicationPackageFile={1}",
            error,
            applicationPackageFilePath_);

        if (!error.IsSuccess())
        {
            Common::ErrorCode deleteError = this->owner_.DeleteCorruptedFile(applicationPackageFilePath_, remoteApplicationPackageFilePath_, L"ApplicationPackageFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteApplicationPackageFile: ErrorCode={0}, ApplicationPackageFile={1}",
                    deleteError,
                    applicationPackageFilePath_);
            }
            TryComplete(operation->Parent, error);
            return;
        }
        ConfigureSymbolicLinks(operation->Parent, applicationIdStr_); 
    }

    void ConfigureSymbolicLinks(
        AsyncOperationSPtr const & thisSPtr,
        wstring const & applicationId)
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
            if (!error.IsSuccess())
            { 
                TryComplete(thisSPtr, error);
                return;
            }

            //Symbolic Link
            ArrayPair<wstring, wstring> link;
            //key is %AppTYpe%_App%AppVersion%\work\directory
            link.key = Path::Combine(workDir, iter->first);
            link.value = deployedDir;
            symbolicLinks_.push_back(link);
        }

        error = SetupLogDirectory(applicationId, nodeId, logDirectorySymbolicLink);
        if (!error.IsSuccess() ||
            symbolicLinks_.empty())
        {
            TryComplete(thisSPtr, error);
            return;
        }
        SetupSymbolicLinks(thisSPtr);
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

        if (!error.IsSuccess()) { return error; }

        //Symbolic Link
        ArrayPair<wstring, wstring> link;
        link.key = owner_.RunLayout.GetApplicationLogFolder(applicationId);
        link.value = deployedLogDir;
        symbolicLinks_.push_back(link);

        return error;
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
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "CreateSymbolicLink for application package returned ErrorCode={0}",
             error);
        TryComplete(operation->Parent, error);
    }

private:
    DownloadManager & owner_;
    ApplicationIdentifier const applicationId_;
    ApplicationVersion const applicationVersion_;
    ApplicationPackageDescription applicationPackageDescription_;
    wstring const applicationIdStr_;
    wstring const applicationVersionStr_;
    wstring const healthPropertyId_;
    wstring applicationPackageFilePath_;
    wstring remoteApplicationPackageFilePath_;
    std::vector<ArrayPair<wstring, wstring>> symbolicLinks_;
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
        ServicePackageActivationContext const & activationContext,
        wstring const & applicationName,
        ULONG maxFailureCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : DownloadAsyncOperation(
        Kind::DownloadApplicationPackageAsyncOperation,
        owner,
        applicationId,
        activationContext,
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
        healthPropertyId_(wformatString("Download:{0}, Application {1}", applicationVersion, activationContext.ToString()))
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
        return AsyncOperation::CreateAndStart<DownloadApplicationPackageContentsAsyncOperation>(
            owner_,
            this->applicationId_,
            this->applicationVersion_,
            callback,
            thisSPtr);
    }

    virtual ErrorCode EndDownloadContent(AsyncOperationSPtr const & operation)
    {
        return DownloadApplicationPackageContentsAsyncOperation::End(operation);
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


class DownloadManager::DownloadServicePackageContentsAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadServicePackageContentsAsyncOperation)

public:
    DownloadServicePackageContentsAsyncOperation(
        __in DownloadManager & owner,
        wstring const & applicationIdStr,
        ServicePackageIdentifier const & servicePackageId,
        ServicePackageVersion const & servicePackageVersion,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(
            callback,
            parent),
        owner_(owner),
        applicationIdStr_(applicationIdStr),
        servicePackageId_(servicePackageId),
        servicePackageVersion_(servicePackageVersion),
        servicePackageIdStr_(servicePackageId.ToString())
    {
        remoteObject_ = owner_.StoreLayout.GetServicePackageFile(
            servicePackageId_.ApplicationId.ApplicationTypeName,
            applicationIdStr_,
            servicePackageId_.ServicePackageName,
            servicePackageVersion_.RolloutVersionValue.ToString());
    }

    virtual ~DownloadServicePackageContentsAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out ServicePackageDescription & packageDescription)
    {
        auto thisPtr = AsyncOperation::End<DownloadServicePackageContentsAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            packageDescription = move(thisPtr->servicePackageDescription_);
        }
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (servicePackageId_.ApplicationId != *ApplicationIdentifier::FabricSystemAppId)
        {
            EnsurePackageFile(thisSPtr);
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
            TryComplete(thisSPtr, error);
        }
    }

    void EnsurePackageFile(AsyncOperationSPtr const & thisSPtr)
    {
        packageFilePath_ = owner_.RunLayout.GetServicePackageFile(
            applicationIdStr_,
            servicePackageId_.ServicePackageName,
            servicePackageVersion_.RolloutVersionValue.ToString());

        auto operation = owner_.imageCacheManager_->BeginDownload(
            remoteObject_,
            packageFilePath_,
            L"",
            L"",
            false,
            ImageStore::CopyFlag::Echo,
            false,
            false,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCopyServicePackageFileCompleted(operation, false);
        },
            thisSPtr);
        this->OnCopyServicePackageFileCompleted(operation, true);
    }

    void OnCopyServicePackageFileCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.imageCacheManager_->EndDownload(operation);
        if (!error.IsSuccess())
        {

            TryComplete(operation->Parent, error);
            return;
        }
        EnsurePackageFileContents(operation->Parent);
    }

    void EnsurePackageFileContents(AsyncOperationSPtr const & thisSPtr)
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
            TryComplete(thisSPtr, error);
            return;
        }
        servicePackageDescription_ = move(servicepackageDescription);

        // ensure manifest file is present
        manifestFilePath_ = owner_.RunLayout.GetServiceManifestFile(
            applicationIdStr_,
            servicePackageDescription_.ManifestName,
            servicePackageDescription_.ManifestVersion);

        storeManifestFilePath_ = owner_.StoreLayout.GetServiceManifestFile(
            servicePackageId_.ApplicationId.ApplicationTypeName,
            servicePackageDescription_.ManifestName,
            servicePackageDescription_.ManifestVersion);

        wstring storeManifestChechsumFilePath = owner_.StoreLayout.GetServiceManifestChecksumFile(
            servicePackageId_.ApplicationId.ApplicationTypeName,
            servicePackageDescription_.ManifestName,
            servicePackageDescription_.ManifestVersion);

        auto operation = owner_.imageCacheManager_->BeginDownload(
            storeManifestFilePath_,
            manifestFilePath_,
            storeManifestChechsumFilePath,
            servicePackageDescription_.ManifestChecksum,
            false,
            ImageStore::CopyFlag::Echo,
            false,
            false,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadServiceManifesFileCompleted(operation, false);
        },
            thisSPtr);
        this->OnDownloadServiceManifesFileCompleted(operation, true);
     
    }

    void OnDownloadServiceManifesFileCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.imageCacheManager_->EndDownload(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
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
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(manifestFilePath_, storeManifestFilePath_, L"ServiceManifestFile");
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

            TryComplete(operation->Parent, error);
            return;
        }

        GetCodePackages(servicePackageDescription_, operation->Parent);
    }

    void GetCodePackages(ServicePackageDescription const & servicePackage, AsyncOperationSPtr const & thisSPtr)
    {
        filesToDownload_.clear();
        containerImages_.clear();
        vector<wstring> secretStoreRefs;
        bool getRespositoryPwdForSecretStore = true;
        for (auto iter = servicePackage.DigestedCodePackages.begin();
            iter != servicePackage.DigestedCodePackages.end();
            ++iter)
        {
            std::wstring imageSource;
            FileDownloadSpec downloadSpec;

            if (iter->CodePackage.EntryPoint.ContainerEntryPoint.ImageName.empty() ||
                !iter->CodePackage.SetupEntryPoint.Program.empty())
            {

                downloadSpec.RemoteSourceLocation = owner_.StoreLayout.GetCodePackageFolder(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->CodePackage.Name,
                    iter->CodePackage.Version);

                downloadSpec.ChecksumFileLocation = owner_.StoreLayout.GetCodePackageChecksumFile(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->CodePackage.Name,
                    iter->CodePackage.Version);

                downloadSpec.Checksum = iter->ContentChecksum;

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
                    downloadSpec.LocalDestinationLocation = owner_.SharedLayout.GetCodePackageFolder(
                        servicePackageId_.ApplicationId.ApplicationTypeName,
                        servicePackage.ManifestName,
                        iter->CodePackage.Name,
                        iter->CodePackage.Version);

                    filesToDownload_.push_back(downloadSpec);

                    if (!Directory::IsSymbolicLink(runCodePackagePath))
                    {
                        ArrayPair<wstring, wstring> link;
                        link.key = runCodePackagePath;
                        link.value = downloadSpec.LocalDestinationLocation;
                        this->symbolicLinks_.push_back(link);
                    }
                }
                else
                {
                    downloadSpec.LocalDestinationLocation = runCodePackagePath;
                    filesToDownload_.push_back(downloadSpec);
                }

                if (!iter->CodePackage.EntryPoint.ContainerEntryPoint.FromSource.empty())
                {
                    imageSource = Path::Combine(runCodePackagePath, iter->CodePackage.EntryPoint.ContainerEntryPoint.FromSource);
                }
            }
            if (!iter->CodePackage.EntryPoint.ContainerEntryPoint.ImageName.empty())
            {
                wstring imageName = iter->CodePackage.EntryPoint.ContainerEntryPoint.ImageName;
                auto error = ContainerHelper::GetContainerHelper().GetContainerImageName(iter->ContainerPolicies.ImageOverrides, imageName);

                if (!error.IsSuccess())
                {
                    WriteError(
                        Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "Failed to get the image name to be downloaded with error {0}",
                        error);
                    TryComplete(thisSPtr, error);
                    return;
                }

                this->containerImages_.push_back(
                    ContainerImageDescription(
                        imageName,
                        iter->ContainerPolicies.UseDefaultRepositoryCredentials,
                        iter->ContainerPolicies.UseTokenAuthenticationCredentials,
                        iter->ContainerPolicies.RepositoryCredentials));

                GetSecretStoreRefForRepositoryCredentials(secretStoreRefs, iter->ContainerPolicies, getRespositoryPwdForSecretStore);
            }
        }

        if (!secretStoreRefs.empty())
        {
            auto secretStoreOp = HostingHelper::BeginGetSecretStoreRef(
                owner_.hosting_,
                secretStoreRefs,
                [this](AsyncOperationSPtr const & secretStoreOp)
            {
                this->OnSecretStoreOpCompleted(secretStoreOp, false);
            },
                thisSPtr);
            OnSecretStoreOpCompleted(secretStoreOp, true);
        }
        else
        {
            GetConfigAndDataPackages(thisSPtr);
        }
    }

    void OnSecretStoreOpCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        map<wstring, wstring> decryptedSecretStoreRef;
        auto error = HostingHelper::EndGetSecretStoreRef(operation, decryptedSecretStoreRef);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "GetSecretStoreRef Values for RepositoryCredentials returned error:{0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, move(error));
            return;
        }

        error = ReplaceRepositoryCredentialValues(decryptedSecretStoreRef);
        if (!error.IsSuccess())
        {
            WriteError(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Failed to substitute SecretStoreRef {0}",
                error);
            TryComplete(operation->Parent, move(error));
            return;
        }

        GetConfigAndDataPackages(operation->Parent);
    }

    void GetSecretStoreRefForRepositoryCredentials(
        _Out_ vector<wstring> & secretStoreRefs,
        ServiceModel::ContainerPoliciesDescription const& containerPolicies,
        _Inout_ bool & getRespositoryPwdForSecretStore)
    {
        // Retrive the default repository credentials.
        // Only reterieve if :
        // 1. you haven't retrieved before
        // 2. Type is SecretsStoreRef
        // 3. Default credentials in cluster manifest are not empty.
        if (containerPolicies.UseDefaultRepositoryCredentials
            && getRespositoryPwdForSecretStore
            && !HostingConfig::GetConfig().DefaultContainerRepositoryAccountName.empty()
            && !HostingConfig::GetConfig().DefaultContainerRepositoryPassword.empty()
            && !HostingConfig::GetConfig().DefaultContainerRepositoryPasswordType.empty()
            && StringUtility::AreEqualCaseInsensitive(HostingConfig::GetConfig().DefaultContainerRepositoryPasswordType, Constants::SecretsStoreRef))
        {
            secretStoreRefs.push_back(HostingConfig::GetConfig().DefaultContainerRepositoryPassword);
            getRespositoryPwdForSecretStore = false;
        }
        if (StringUtility::AreEqualCaseInsensitive(containerPolicies.RepositoryCredentials.Type, Constants::SecretsStoreRef))
        {
            secretStoreRefs.push_back(containerPolicies.RepositoryCredentials.Password);
        }
    }

    ErrorCode ReplaceRepositoryCredentialValues(map<wstring, wstring> const& decryptedSecretStoreRef)
    {
        for (auto & containerImageDescription : this->containerImages_)
        {
            if (StringUtility::AreEqualCaseInsensitive(containerImageDescription.RepositoryCredentials.Type, Constants::SecretsStoreRef))
            {
                auto it = decryptedSecretStoreRef.find(containerImageDescription.RepositoryCredentials.Password);
                if (it == decryptedSecretStoreRef.end())
                {
                    return ErrorCode(ErrorCodeValue::NotFound,
                        wformatString(StringResource::Get(IDS_HOSTING_SecretStoreReferenceDecryptionFailed), L"RepositoryCredentials", containerImageDescription.RepositoryCredentials.Password));
                }

                containerImageDescription.SetContainerRepositoryPassword(it->second);
            }
        }

        return ErrorCodeValue::Success;
    }

    void GetConfigAndDataPackages(AsyncOperationSPtr const & thisSPtr)
    {
        GetConfigPackages(servicePackageDescription_);
        GetDataPackages(servicePackageDescription_);
        if (!filesToDownload_.empty())
        {
            auto downloadOp = AsyncOperation::CreateAndStart<DownloadPackagesAsyncOperation>(
                owner_,
                filesToDownload_,
                [this](AsyncOperationSPtr const & downloadOp)
            {
                this->OnDownloadPackagesCompleted(downloadOp, false);
            },
                thisSPtr);
            OnDownloadPackagesCompleted(downloadOp, true);
        }
        else
        {
            DownloadContainerImages(thisSPtr);
        }
    }

    void OnDownloadPackagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = DownloadPackagesAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        DownloadContainerImages(operation->Parent);
    }

    void DownloadContainerImages(AsyncOperationSPtr const & thisSPtr)
    {
        bool skipdownloadImages = false;
#if defined(PLATFORM_UNIX)
        ContainerIsolationMode::Enum isolationMode = ContainerIsolationMode::process;
        ContainerIsolationMode::FromString(servicePackageDescription_.ContainerPolicyDescription.Isolation, isolationMode);
        if (isolationMode == ContainerIsolationMode::hyperv)
        {
            WriteInfo(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Skip downloading container images for {0}",
                servicePackageId_.ApplicationId);
            skipdownloadImages = true;
        }
#endif
        if (!containerImages_.empty() &&
            !skipdownloadImages)
        {
            auto containerOp = AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>
                (
                    this->containerImages_,
                    ServicePackageActivationContext(),
                    this->owner_,
                    [this](AsyncOperationSPtr const & containerOp)
            {
                this->OnContainerImagesDownloaded(containerOp, false);
            },
                    thisSPtr);
            OnContainerImagesDownloaded(containerOp, true);
        }
        else
        {
            SetupSymbolicLinks(thisSPtr);
        }
    }
    void OnContainerImagesDownloaded(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = DownloadContainerImagesAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        SetupSymbolicLinks(operation->Parent);
    }

    void GetConfigPackages(ServicePackageDescription const & servicePackage)
    {
        for (auto iter = servicePackage.DigestedConfigPackages.begin();
            iter != servicePackage.DigestedConfigPackages.end();
            ++iter)
        {
            FileDownloadSpec downloadSpec;
            downloadSpec.RemoteSourceLocation = owner_.StoreLayout.GetConfigPackageFolder(
                servicePackageId_.ApplicationId.ApplicationTypeName,
                servicePackage.ManifestName,
                iter->ConfigPackage.Name,
                iter->ConfigPackage.Version);

            downloadSpec.ChecksumFileLocation = owner_.StoreLayout.GetConfigPackageChecksumFile(
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
            downloadSpec.Checksum = iter->ContentChecksum;
            if (HostingConfig::GetConfig().EnableProcessDebugging && !iter->DebugParameters.ConfigPackageLinkFolder.empty())
            {
                ArrayPair<wstring, wstring> link;
                link.key = runConfigPackagePath;
                link.value = iter->DebugParameters.ConfigPackageLinkFolder;
                this->symbolicLinks_.push_back(link);
            }
            else if (iter->IsShared)
            {
                downloadSpec.LocalDestinationLocation = owner_.SharedLayout.GetConfigPackageFolder(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->ConfigPackage.Name,
                    iter->ConfigPackage.Version);

                filesToDownload_.push_back(downloadSpec);

                if (!Directory::IsSymbolicLink(runConfigPackagePath))
                {
                    ArrayPair<wstring, wstring> link;
                    link.key = runConfigPackagePath;
                    link.value = downloadSpec.LocalDestinationLocation;
                    this->symbolicLinks_.push_back(link);
                }
            }
            else
            {
                downloadSpec.LocalDestinationLocation = runConfigPackagePath;
                filesToDownload_.push_back(downloadSpec);
            }
         
        }
    }

    void GetDataPackages(ServicePackageDescription const & servicePackage)
    {
        for (auto iter = servicePackage.DigestedDataPackages.begin();
            iter != servicePackage.DigestedDataPackages.end();
            ++iter)
        {
            FileDownloadSpec downloadSpec;
            downloadSpec.RemoteSourceLocation = owner_.StoreLayout.GetDataPackageFolder(
                servicePackageId_.ApplicationId.ApplicationTypeName,
                servicePackage.ManifestName,
                iter->DataPackage.Name,
                iter->DataPackage.Version);

            downloadSpec.ChecksumFileLocation = owner_.StoreLayout.GetDataPackageChecksumFile(
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
            downloadSpec.Checksum = iter->ContentChecksum;
            if (HostingConfig::GetConfig().EnableProcessDebugging && !iter->DebugParameters.DataPackageLinkFolder.empty())
            {
                ArrayPair<wstring, wstring> link;
                link.key = runDataPackagePath;
                link.value = iter->DebugParameters.DataPackageLinkFolder;
                this->symbolicLinks_.push_back(link);
            }
            else if (iter->IsShared)
            {
                downloadSpec.LocalDestinationLocation = owner_.SharedLayout.GetDataPackageFolder(
                    servicePackageId_.ApplicationId.ApplicationTypeName,
                    servicePackage.ManifestName,
                    iter->DataPackage.Name,
                    iter->DataPackage.Version);

                filesToDownload_.push_back(downloadSpec);

                if (!Directory::IsSymbolicLink(runDataPackagePath))
                {
                    ArrayPair<wstring, wstring> link;
                    link.key = runDataPackagePath;
                    link.value = downloadSpec.LocalDestinationLocation;
                    this->symbolicLinks_.push_back(link);
                }
            }
            else
            {
                downloadSpec.LocalDestinationLocation = runDataPackagePath;
                filesToDownload_.push_back(downloadSpec);
            }
        }
    }
   
    void SetupSymbolicLinks(AsyncOperationSPtr const & thisSPtr)
    {
        if (!this->symbolicLinks_.empty())
        {
            WriteNoise(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Setting up symbolic links for {0}",
                symbolicLinks_.size());
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
        else
        {
            SetupConfigPackagePolicies(thisSPtr);
        }
    }

    void FinishSymbolicLinksSetup(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.hosting_.FabricActivatorClientObj->EndCreateSymbolicLink(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "CreateSymbolicLink for service package returned ErrorCode={0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        SetupConfigPackagePolicies(operation->Parent);
    }

    void SetupConfigPackagePolicies(
        AsyncOperationSPtr const & thisSPtr)
    {
        bool writeConfigSettings = false;

        for (auto const& digestedCodePackage : servicePackageDescription_.DigestedCodePackages)
        {
            if (!digestedCodePackage.ConfigPackagePolicy.ConfigPackages.empty())
            {
                writeConfigSettings = true;
                break;
            }
        }

        if (!writeConfigSettings)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        map<wstring, ConfigSettings> configSettingsMapResult;

        auto error = HostingHelper::GenerateAllConfigSettings(
            servicePackageId_.ApplicationId.ToString(),
            servicePackageId_.ServicePackageName,
            owner_.RunLayout,
            servicePackageDescription_.DigestedConfigPackages,
            configSettingsMapResult);

        if (!error.IsSuccess() || configSettingsMapResult.empty())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        vector<wstring> secretStoreRef;
        error = HostingHelper::DecryptAndGetSecretStoreRefForConfigSetting(configSettingsMapResult, secretStoreRef);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        if (!secretStoreRef.empty())
        {
            SetupSecretStoreServiceRefs(configSettingsMapResult, secretStoreRef, thisSPtr);
        }
        else
        {
            error = WriteConfigSettingToFile(configSettingsMapResult);
            TryComplete(thisSPtr, error);
        }
    }

    void SetupSecretStoreServiceRefs(map<wstring, ConfigSettings> const& configSettingsMapResult, vector<std::wstring> const & secretStoreRefs, AsyncOperationSPtr const & thisSPtr)
    {
        auto secretStoreOp = HostingHelper::BeginGetSecretStoreRef(
            owner_.hosting_,
            secretStoreRefs,
            [this, configSettingsMapResult](AsyncOperationSPtr const & secretStoreOp)
        {
            this->OnBeginGetSecretsCompleted(configSettingsMapResult, secretStoreOp, false);
        },
            thisSPtr);

        this->OnBeginGetSecretsCompleted(configSettingsMapResult, secretStoreOp, true);

    }

    void OnBeginGetSecretsCompleted(
        map<wstring, ConfigSettings> const& configSettingsMapResult,
        AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        map<wstring, wstring> decryptedSecretStoreRef;
        auto error = HostingHelper::EndGetSecretStoreRef(operation, decryptedSecretStoreRef);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "GetSecretStoreRef Values for ConfigPackagePolicies Settings returned error:{0}",
            error);

        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        error = WriteConfigSettingToFile(configSettingsMapResult, decryptedSecretStoreRef);
        TryComplete(operation->Parent, error);
    }

    ErrorCode WriteConfigSettingToFile(map<wstring, ConfigSettings> const& configSettingsMapResult, map<wstring, wstring> const& secrteStoreRef = map<wstring, wstring>())
    {
        wstring settingsDirectory = owner_.RunLayout.GetApplicationSettingsFolder(servicePackageId_.ApplicationId.ToString());

        for (auto configPackage : configSettingsMapResult)
        {
            wstring configPackageVerison;
            for (auto digestedConfigPackage : servicePackageDescription_.DigestedConfigPackages)
            {
                if (StringUtility::AreEqualCaseInsensitive(digestedConfigPackage.Name, configPackage.first))
                {
                    configPackageVerison = digestedConfigPackage.ConfigPackage.Version;
                    break;
                }
            }

            //In case of upgrades we need the config package version. If rollback happens then we do not have to query secret store again.
            wstring ConfigPackagNameVersion = wformatString("{0}{1}", configPackage.first, configPackageVerison);

            wstring fileName = Path::Combine(settingsDirectory, servicePackageId_.ServicePackageName);
            fileName = Path::Combine(fileName, ConfigPackagNameVersion);

            for (auto section : configPackage.second.Sections)
            {
                wstring sectionFile = Path::Combine(fileName, section.first);

                if (section.second.Parameters.size() > 0)
                {
                    for (auto parameter : section.second.Parameters)
                    {
                        if (!Directory::Exists(sectionFile))
                        {
                            auto error = Directory::Create2(sectionFile);
                            if (!error.IsSuccess())
                            {
                                return error;
                            }
                        }

                        wstring parameterFileName = Path::Combine(sectionFile, parameter.second.Name);
                        if (File::Exists(parameterFileName))
                        {
                            WriteInfo(
                                Trace_DownloadManager,
                                owner_.Root.TraceId,
                                "Parameter File {0} already exists.",
                                parameterFileName);
                            continue;
                        }

                        if (StringUtility::AreEqualCaseInsensitive(parameter.second.Type, Constants::SecretsStoreRef))
                        {
                            auto it = secrteStoreRef.find(parameter.second.Value);

                            if (it != secrteStoreRef.end())
                            {
                                parameter.second.Value = it->second;
                            }
                            else
                            {
                                WriteError(
                                    Trace_DownloadManager,
                                    owner_.Root.TraceId,
                                    "Can't find secret value for given secret {0} in secret store",
                                    parameter.second.Value);
                                return ErrorCodeValue::NotFound;
                            }
                        }

                        auto error = HostingHelper::WriteSettingsToFile(parameter.second.Name, parameter.second.Value, parameterFileName);
                        if (!error.IsSuccess())
                        {
                            return error;
                        }
                    }
                }
            }
        }

        return ErrorCodeValue::Success;
    }

private:
    DownloadManager & owner_;
    ServicePackageIdentifier const servicePackageId_;
    ServicePackageVersion const servicePackageVersion_;
    ServicePackageDescription servicePackageDescription_;
    wstring const servicePackageIdStr_;
    wstring const applicationIdStr_;
    wstring packageFilePath_;
    wstring remoteObject_;
    wstring manifestFilePath_;
    wstring storeManifestFilePath_;
    vector<ContainerImageDescription> containerImages_;
    ServicePackageActivationContext activationContext_;
    std::vector<ArrayPair<wstring, wstring>> symbolicLinks_;
    vector<FileDownloadSpec> filesToDownload_;
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
        activationContext,
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
        healthPropertyId_(wformatString("Download:{0}:{1}", servicePackageVersion, activationContext.ToString()))
    {
        ASSERT_IF(
            servicePackageId_.ApplicationId.IsAdhoc(),
            "Cannot download ServicePackage of an ad-hoc application. ServicePackageId={0}",
            servicePackageId_);
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
        return AsyncOperation::CreateAndStart<DownloadServicePackageContentsAsyncOperation>(
                owner_,
                applicationIdStr_,
                servicePackageId_,
                servicePackageVersion_,
                callback,
                thisSPtr);
    }

    virtual ErrorCode EndDownloadContent(AsyncOperationSPtr const & operation)
    {
        return DownloadServicePackageContentsAsyncOperation::End(operation, servicePackageDescription_);
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

class DownloadManager::DownloadServiceManifestContentsAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadServiceManifestContentsAsyncOperation)

public:
    DownloadServiceManifestContentsAsyncOperation(
        DownloadManager & owner,
        wstring const & applicationTypeName,
        wstring const & applicationTypeVersion,
        wstring const & serviceManifestName,
        PackageSharingPolicyList const & sharingPolicies,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(
            callback,
            parent),
        owner_(owner),
        applicationTypeName_(applicationTypeName),
        applicationTypeVersion_(applicationTypeVersion),
        serviceManifestName_(serviceManifestName),
        sharingPolicies_(sharingPolicies),
        sharingScope_(0),
        cacheLayout_(owner.hosting_.ImageCacheFolder),
        containerImages_()

    {
    }

    virtual ~DownloadServiceManifestContentsAsyncOperation()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadServiceManifestContentsAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "Predeployment invoked for ApplicationType: {0}, ApplicationVersion: {1}, ServiceManifestName: {2}. PackageSharingPolicies: {3}",
            applicationTypeName_,
            applicationTypeVersion_,
            serviceManifestName_,
            sharingPolicies_);

        if (!ManagementConfig::GetConfig().ImageCachingEnabled)
        {
            ErrorCode err(ErrorCodeValue::PreDeploymentNotAllowed, StringResource::Get(IDS_HOSTING_Predeployment_NotAllowed));
            WriteWarning(
                Trace_DownloadManager,
                owner_.Root.TraceId,
                "Predeployment cannot be performed since ImageCache is not enabled");   
            TryComplete(thisSPtr, err);
            return;
        }

        storeAppManifestPath_ = owner_.StoreLayout.GetApplicationManifestFile(
            this->applicationTypeName_,
            this->applicationTypeVersion_);

        cacheAppManifestPath_ = cacheLayout_.GetApplicationManifestFile(applicationTypeName_, applicationTypeVersion_);

        auto operation = owner_.imageCacheManager_->BeginDownload(
            storeAppManifestPath_,
            cacheAppManifestPath_,
            L"",
            L"",
            false,
            ImageStore::CopyFlag::Echo,
            false,
            false,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCopyAppManifestCompleted(operation, false);
        },
            thisSPtr);
        OnCopyAppManifestCompleted(operation, true);
    }

    void OnCopyAppManifestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.imageCacheManager_->EndDownload(operation);
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
            TryComplete(operation->Parent, err);
            return;
        }
        ApplicationManifestDescription appDescription;
        error = Parser::ParseApplicationManifest(cacheAppManifestPath_, appDescription);

        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseApplicationManifestFile: ErrorCode={0}, PackageFile={1}",
            error,
            cacheAppManifestPath_);

        if (!error.IsSuccess())
        {
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(cacheAppManifestPath_, storeAppManifestPath_, L"ApplicationManifestFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteApplicationManifestFile: ErrorCode={0}, PackageFile={1}",
                    deleteError,
                    cacheAppManifestPath_);
            }
            TryComplete(operation->Parent, error);
            return;
        }

        ServiceManifestDescription serviceDescription;
        wstring manifestVersion;

        for (auto iter = appDescription.ServiceManifestImports.begin(); iter != appDescription.ServiceManifestImports.end(); iter++)
        {
            if (StringUtility::AreEqualCaseInsensitive((*iter).ServiceManifestRef.Name, serviceManifestName_))
            {
                manifestVersion = (*iter).ServiceManifestRef.Version;
                containerPolicies_ = iter->ContainerHostPolicies;
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

            TryComplete(operation->Parent, err);
            return;
        }

        storeManifestChechsumFilePath_ = owner_.StoreLayout.GetServiceManifestChecksumFile(
            applicationTypeName_,
            serviceManifestName_,
            manifestVersion);

        wstring expectedChecksum = L"";

        auto checksumOperation = owner_.imageCacheManager_->BeginGetChecksum(
            storeManifestChechsumFilePath_,
            [this, manifestVersion](AsyncOperationSPtr const & checksumOperation)
        {
            this->OnGetChecksumCompleted(manifestVersion, checksumOperation, false);
        },
            operation->Parent);
        OnGetChecksumCompleted(manifestVersion, checksumOperation, true);
     
    }

    void OnGetChecksumCompleted(
        wstring const & manifestVersion,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        wstring checksum;
        auto error = owner_.imageCacheManager_->EndGetChecksum(operation, checksum);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        
        cacheManifestPath_ = cacheLayout_.GetServiceManifestFile(applicationTypeName_, serviceManifestName_, manifestVersion);

        storeManifestFilePath_ = owner_.StoreLayout.GetServiceManifestFile(
            applicationTypeName_,
            serviceManifestName_,
            manifestVersion);

        auto downloadOperation = owner_.imageCacheManager_->BeginDownload(
            storeManifestFilePath_,
            cacheManifestPath_,
            storeManifestChechsumFilePath_,
            checksum,
            false,
            ImageStore::CopyFlag::Echo,
            false,
            false,
            [this](AsyncOperationSPtr const & downloadOperation)
        {
            this->OnCopyServiceManifestCompleted(downloadOperation, false);
        },
            operation->Parent);
        OnCopyServiceManifestCompleted(downloadOperation, true);
    }

    void OnCopyServiceManifestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.imageCacheManager_->EndDownload(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        // parse manifest file
        ServiceManifestDescription serviceManifestDescription;
        error = Parser::ParseServiceManifest(cacheManifestPath_, serviceManifestDescription);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "ParseServiceManifestFile: ErrorCode={0}, ManifestFile={1}",
            error,
            cacheManifestPath_);

        if (!error.IsSuccess())
        {
            ErrorCode deleteError = this->owner_.DeleteCorruptedFile(cacheManifestPath_, storeManifestFilePath_, L"ServiceManifestFile");
            if (!deleteError.IsSuccess())
            {
                WriteTrace(
                    deleteError.ToLogLevel(),
                    Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DeleteServiceManifestFile: ErrorCode={0}, ManifestFile={1}",
                    deleteError,
                    cacheManifestPath_);
            }
            TryComplete(operation->Parent, error);
            return;
        }
        
        error = GetPackagesToCopy(serviceManifestDescription);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "DownloadServiceManifestContents downloading {0}",
            error);
        auto op = AsyncOperation::CreateAndStart<DownloadPackagesAsyncOperation>(
            owner_,
            filesToDownload_,
            [this](AsyncOperationSPtr const & op)
        {
            this->OnDownloadPackagesCompleted(op, false);
        },
            operation->Parent);
        OnDownloadPackagesCompleted(op, true);
    }

    void OnDownloadPackagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = DownloadPackagesAsyncOperation::End(operation);
        if (!error.IsSuccess() || containerImages_.empty())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        auto op = AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
            this->containerImages_,
            ServicePackageActivationContext(),
            this->owner_,
            [this](AsyncOperationSPtr const & op)
        {
            this->OnContainerImagesDownloaded(op, false);
        },
            operation->Parent);
        OnContainerImagesDownloaded(op, true);
    }

    void OnContainerImagesDownloaded(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = DownloadContainerImagesAsyncOperation::End(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "DownloadContainerImages::End error{0}",
            error);

        TryComplete(operation->Parent, error);
    }

    ErrorCode GetPackagesToCopy(ServiceManifestDescription const & serviceManifestDescription)
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
            FileDownloadSpec downloadSpec;
            std::wstring imageSource;
            if (iter->EntryPoint.ContainerEntryPoint.ImageName.empty() ||
                !iter->SetupEntryPoint.Program.empty())
            {
                downloadSpec.RemoteSourceLocation = owner_.StoreLayout.GetCodePackageFolder(
                    applicationTypeName_,
                    serviceManifestName_,
                    iter->Name,
                    iter->Version);

                downloadSpec.ChecksumFileLocation = owner_.StoreLayout.GetCodePackageChecksumFile(
                    applicationTypeName_,
                    serviceManifestName_,
                    iter->Name,
                    iter->Version);

                downloadSpec.LocalDestinationLocation = cacheLayout_.GetCodePackageFolder(
                    applicationTypeName_,
                    serviceManifestName_,
                    iter->Name,
                    iter->Version);

                // If we are downloading the Code/Config/Data we should be downloading their corresponding checksum files. The marker file _zip for archived files is also created
                // with this checksum value to prevent the re-extraction from happening during runtime deployment.
                // Passing the expected checksum triggers the download of the checksum file on the node. So, while doing an actual deployment at runtime we do not end up refreshing
                // the cache due to a mismatch since there is no checksum file on the node and is expected(from the digested package) to be there and when we compare the marker file
                // _zip checksum with the expected it won't match because the marker file will be empty if we did not pass in the exepected checksum value during predeployment.

                error = owner_.imageCacheManager_->GetStoreChecksumValue(downloadSpec.ChecksumFileLocation, downloadSpec.Checksum);
                if (!error.IsSuccess())
                {
                    return error;
                }

                downloadSpec.DownloadToCacheOnly = true;
                if (sharingScopeCode ||
                    sharingPolicies.find(iter->Name) != sharingPolicies.end())
                {
                    WriteNoise(Trace_DownloadManager,
                        owner_.Root.TraceId,
                        "Sharing is enabled for Code package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                        iter->Name,
                        applicationTypeName_,
                        applicationTypeVersion_);

                    downloadSpec.LocalDestinationLocation = owner_.SharedLayout.GetCodePackageFolder(applicationTypeName_, serviceManifestName_, iter->Name, iter->Version);
                    downloadSpec.DownloadToCacheOnly = false;
                }
                WriteInfo(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "DownloadServiceManifestContents for Code package remote location{0}:  local location ApplicationTypeName {1}, expectedChecksum {2}",
                    downloadSpec.RemoteSourceLocation,
                    downloadSpec.LocalDestinationLocation,
                    downloadSpec.Checksum);
                filesToDownload_.push_back(downloadSpec);
            }
            if (!iter->EntryPoint.ContainerEntryPoint.ImageName.empty())
            {
                RepositoryCredentialsDescription repositoryCredentials;
                ImageOverridesDescription imageOverrides;
                bool useDefaultRepositoryCredentials = false;
                bool useTokenAuthenticationCredentials = false;

                for (auto it = containerPolicies_.begin(); it != containerPolicies_.end(); ++it)
                {
                    if (StringUtility::AreEqualCaseInsensitive(it->CodePackageRef, iter->Name))
                    {
                        repositoryCredentials = it->RepositoryCredentials;
                        imageOverrides = it->ImageOverrides;
                        useDefaultRepositoryCredentials = it->UseDefaultRepositoryCredentials;
                        useTokenAuthenticationCredentials = it->UseTokenAuthenticationCredentials;
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
                        useDefaultRepositoryCredentials,
                        useTokenAuthenticationCredentials,
                        repositoryCredentials));
            }
        }

        bool sharingScopeConfig = (sharingScope_ & ServicePackageSharingType::Config || sharingScope_ & ServicePackageSharingType::All);
        for (auto iter = serviceManifestDescription.ConfigPackages.begin(); iter != serviceManifestDescription.ConfigPackages.end(); iter++)
        {
            FileDownloadSpec downloadSpec;

            downloadSpec.RemoteSourceLocation = owner_.StoreLayout.GetConfigPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            downloadSpec.ChecksumFileLocation = owner_.StoreLayout.GetConfigPackageChecksumFile(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            downloadSpec.LocalDestinationLocation = cacheLayout_.GetConfigPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

            error = owner_.imageCacheManager_->GetStoreChecksumValue(downloadSpec.ChecksumFileLocation, downloadSpec.Checksum);
            if (!error.IsSuccess())
            {
                return error;
            }

            downloadSpec.DownloadToCacheOnly = true;
            if (sharingScopeConfig || sharingPolicies.find(iter->Name) != sharingPolicies.end())
            {
                WriteNoise(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Sharing is enabled for Config package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                    iter->Name,
                    applicationTypeName_,
                    applicationTypeVersion_);

                downloadSpec.LocalDestinationLocation = owner_.SharedLayout.GetConfigPackageFolder(applicationTypeName_, serviceManifestName_, iter->Name, iter->Version);
                downloadSpec.DownloadToCacheOnly = false;
            }

            WriteInfo(Trace_DownloadManager,
                owner_.Root.TraceId,
                "DownloadServiceManifestContents for config package remote location{0}:  local location ApplicationTypeName {1}, expectedChecksum {2}",
                downloadSpec.RemoteSourceLocation,
                downloadSpec.LocalDestinationLocation,
                downloadSpec.Checksum);
            filesToDownload_.push_back(downloadSpec);
        }

        bool sharingScopeData = (sharingScope_ & ServicePackageSharingType::Data || sharingScope_ & ServicePackageSharingType::All);
        for (auto iter = serviceManifestDescription.DataPackages.begin(); iter != serviceManifestDescription.DataPackages.end(); iter++)
        {
            FileDownloadSpec downloadSpec;

            downloadSpec.ChecksumFileLocation = owner_.StoreLayout.GetDataPackageChecksumFile(
                applicationTypeName_,
                serviceManifestName_,
                (*iter).Name,
                (*iter).Version);

            downloadSpec.RemoteSourceLocation = owner_.StoreLayout.GetDataPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);


             downloadSpec.LocalDestinationLocation = cacheLayout_.GetDataPackageFolder(
                applicationTypeName_,
                serviceManifestName_,
                iter->Name,
                iter->Version);

             error = owner_.imageCacheManager_->GetStoreChecksumValue(downloadSpec.ChecksumFileLocation, downloadSpec.Checksum);
             if (!error.IsSuccess())
             {
                 return error;
             }

             downloadSpec.DownloadToCacheOnly = true;
            if (sharingScopeData ||
                sharingPolicies.find(iter->Name) != sharingPolicies.end())
            {
                WriteNoise(Trace_DownloadManager,
                    owner_.Root.TraceId,
                    "Sharing is enabled for Data package {0}:  ApplicationTypeName {1}, ApplicationTypeVersion {2}",
                    iter->Name,
                    applicationTypeName_,
                    applicationTypeVersion_);
                downloadSpec.DownloadToCacheOnly = false;
                downloadSpec.LocalDestinationLocation = owner_.SharedLayout.GetDataPackageFolder(applicationTypeName_, serviceManifestName_, iter->Name, iter->Version);

            }
            WriteInfo(Trace_DownloadManager,
                owner_.Root.TraceId,
                "DownloadServiceManifestContents for data package remote location{0}:  local location ApplicationTypeName {1}, expectedChecksum {2}",
                downloadSpec.RemoteSourceLocation,
                downloadSpec.LocalDestinationLocation,
                downloadSpec.Checksum);
            filesToDownload_.push_back(downloadSpec);
        }
        return ErrorCodeValue::Success;
    }

private:
    DownloadManager & owner_;
    wstring const applicationTypeName_;
    wstring const applicationTypeVersion_;
    StoreLayoutSpecification cacheLayout_;
    wstring const serviceManifestName_;
    wstring cacheAppManifestPath_;
    wstring storeAppManifestPath_;
    wstring cacheManifestPath_;
    wstring storeManifestFilePath_;
    wstring storeManifestChechsumFilePath_;
    PackageSharingPolicyList sharingPolicies_;
    DWORD sharingScope_;
    vector<ContainerImageDescription> containerImages_;
    vector<ContainerPoliciesDescription> containerPolicies_;
    vector<FileDownloadSpec> filesToDownload_;
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
        ServicePackageActivationContext(), /*Empty ServicePackageActivation Guid*/
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
        return AsyncOperation::CreateAndStart<DownloadServiceManifestContentsAsyncOperation>(
            owner_,
            applicationTypeName_,
            applicationTypeVersion_,
            serviceManifestName_,
            sharingPolicies_,
            callback,
            thisSPtr);
    }

    virtual ErrorCode EndDownloadContent(AsyncOperationSPtr const & operation)
    {
        return DownloadServiceManifestContentsAsyncOperation::End(operation);
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
        ServicePackageActivationContext(), /*Empty ServicePackageActivationId*/
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

        WriteInfo(
            Trace_DownloadManager,
            owner_.Root.TraceId,
            "FabricUpgrade Version -> Node-{0}, Requested-{1}. CodeVersion: Node-{2}, Requested-{3}. ConfigVersion: Node-{4}, Requested-{5}.",
            nodeVersionInstance,
            fabricVersion_,
            nodeVersionInstance.Version.CodeVersion,
            fabricVersion_.CodeVersion,
            nodeVersionInstance.Version.ConfigVersion,
            fabricVersion_.ConfigVersion);

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
            ServicePackageActivationContext(),
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
    imageCacheManager_(),
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
    auto imageCacheManager = make_unique<ImageCacheManager>(root, imageStore_);
    imageCacheManager_ = move(imageCacheManager);
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
    ServicePackageActivationContext const & activationContext,
    wstring const & applicationName,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadApplicationPackageAsyncOperation>(
        *this,
        applicationId,
        applicationVersion,
        activationContext,
        applicationName,
        HostingConfig::GetConfig().DeploymentMaxFailureCount,
        callback,
        parent);
}

AsyncOperationSPtr DownloadManager::BeginDownloadApplicationPackage(
    ApplicationIdentifier const & applicationId,
    ApplicationVersion const & applicationVersion,
    ServicePackageActivationContext const & activationContext,
    wstring const & applicationName,
    ULONG maxFailureCount,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadApplicationPackageAsyncOperation>(
        *this,
        applicationId,
        applicationVersion,
        activationContext,
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
    // ensure log, work, temp and settings folders for the application
    auto error = Directory::Create2(RunLayout.GetApplicationWorkFolder(applicationId));
    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        Root.TraceId,
        "EnsureApplicationFolders: CreateWorkFolder: {0}, ErrorCode={1}",
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
        "EnsureApplicationFolders: CreateTempFolder: {0}, ErrorCode={1}",
        RunLayout.GetApplicationTempFolder(applicationId),
        error);
    if (!error.IsSuccess()) { return error; }

    error = Directory::Create2(RunLayout.GetApplicationSettingsFolder(applicationId));
    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        Root.TraceId,
        "DeployFabricSystemPackages: CreateSettingsFolder: {0}, ErrorCode={1}",
        RunLayout.GetApplicationSettingsFolder(applicationId),
        error);

    return error;
}

ErrorCode DownloadManager::EnsureApplicationLogFolder(std::wstring const & applicationId)
{
    auto error = Directory::Create2(RunLayout.GetApplicationLogFolder(applicationId));
    WriteTrace(
        error.ToLogLevel(),
        Trace_DownloadManager,
        Root.TraceId,
        "EnsureApplicationLogFolder: CreateLogFolder: {0}, ErrorCode={1}",
        RunLayout.GetApplicationLogFolder(applicationId),
        error);
    return error;
}

void DownloadManager::InitializePassThroughClientFactory(Api::IClientFactoryPtr const &passThroughClientFactoryPtr)
{
    passThroughClientFactoryPtr_ = passThroughClientFactoryPtr;
}
