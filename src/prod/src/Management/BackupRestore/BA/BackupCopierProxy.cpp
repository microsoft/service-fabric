// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management;
using namespace BackupRestoreAgentComponent;
using namespace BackupCopier;

StringLiteral const TraceComponent("BackupCopierProxy");

DWORD const BackupCopierAbortedExitCode = ErrorCodeValue::BackupCopierAborted & 0x0000FFFF;

#if defined(PLATFORM_UNIX)
GlobalWString BackupCopierProxy::BackupCopierExeName = make_global<wstring>(L"BackupCopier.sh");
#else
GlobalWString BackupCopierProxy::BackupCopierExeName = make_global<wstring>(L"BackupCopier.exe");
#endif

GlobalWString BackupCopierProxy::TempWorkingDirectoryPrefix = make_global<wstring>(L"BC");
GlobalWString BackupCopierProxy::ErrorDetailsFilename = make_global<wstring>(L"ErrorDetails.txt");
GlobalWString BackupCopierProxy::ProgressDetailsFilename = make_global<wstring>(L"Progress.txt");

// Command Line Arguments Key
GlobalWString BackupCopierProxy::OperationKeyName = make_global<wstring>(L"/operation");

GlobalWString BackupCopierProxy::StorageCredentialsSourceLocationKeyName = make_global<wstring>(L"/storageCredentialsSourceLocation");

GlobalWString BackupCopierProxy::ConnectionStringKeyName = make_global<wstring>(L"/connectionString");
GlobalWString BackupCopierProxy::ContainerNameKeyName = make_global<wstring>(L"/containerName");
GlobalWString BackupCopierProxy::IsConnectionStringEncryptedKeyName = make_global<wstring>(L"/isconnectionStringEncrypted");
GlobalWString BackupCopierProxy::BackupStoreBaseFolderPathKeyName = make_global<wstring>(L"/backupStoreBaseFolderPath");

GlobalWString BackupCopierProxy::BackupMetadataFilePathKeyName = make_global<wstring>(L"/backupMetadataFilePath");

GlobalWString BackupCopierProxy::FileSharePathKeyName = make_global<wstring>(L"/fileSharePath");
GlobalWString BackupCopierProxy::FileShareAccessTypeKeyName = make_global<wstring>(L"/fileShareAccessType");
GlobalWString BackupCopierProxy::IsPasswordEncryptedKeyName = make_global<wstring>(L"/isPasswordEncrypted");
GlobalWString BackupCopierProxy::PrimaryUserNameKeyName = make_global<wstring>(L"/primaryUserName");
GlobalWString BackupCopierProxy::PrimaryPasswordKeyName = make_global<wstring>(L"/primaryPassword");
GlobalWString BackupCopierProxy::SecondaryUserNameKeyName = make_global<wstring>(L"/secondaryUserName");
GlobalWString BackupCopierProxy::SecondaryPasswordKeyName = make_global<wstring>(L"/secondaryPassword");

GlobalWString BackupCopierProxy::FileShareAccessTypeValue_None = make_global<wstring>(L"None");
GlobalWString BackupCopierProxy::FileShareAccessTypeValue_DomainUser = make_global<wstring>(L"DomainUser");

GlobalWString BackupCopierProxy::BooleanStringValue_True = make_global<wstring>(L"true");
GlobalWString BackupCopierProxy::BooleanStringValue_False = make_global<wstring>(L"false");

GlobalWString BackupCopierProxy::SourceFileOrFolderPathKeyName = make_global<wstring>(L"/sourceFileOrFolderPath");
GlobalWString BackupCopierProxy::TargetFolderPathKeyName = make_global<wstring>(L"/targetFolderPath");
GlobalWString BackupCopierProxy::TimeoutKeyName = make_global<wstring>(L"/timeout");
GlobalWString BackupCopierProxy::WorkingDirKeyName = make_global<wstring>(L"/workingDir");
GlobalWString BackupCopierProxy::ProgressFileKeyName = make_global<wstring>(L"/progressFile");
GlobalWString BackupCopierProxy::ErrorDetailsFileKeyName = make_global<wstring>(L"/errorDetailsFile");

// Command Line Arguments Values for operation command
GlobalWString BackupCopierProxy::AzureBlobStoreUpload = make_global<wstring>(L"AzureBlobStoreUpload");
GlobalWString BackupCopierProxy::AzureBlobStoreDownload = make_global<wstring>(L"AzureBlobStoreDownload");
GlobalWString BackupCopierProxy::DsmsAzureBlobStoreUpload = make_global<wstring>(L"DsmsAzureBlobStoreUpload");
GlobalWString BackupCopierProxy::DsmsAzureBlobStoreDownload = make_global<wstring>(L"DsmsAzureBlobStoreDownload");
GlobalWString BackupCopierProxy::FileShareUpload = make_global<wstring>(L"FileShareUpload");
GlobalWString BackupCopierProxy::FileShareDownload = make_global<wstring>(L"FileShareDownload");

BackupCopierProxy::BackupCopierProxy(wstring const & backupCopierExeDirectory, wstring const & workingDirectory, Federation::NodeInstance const & nodeInstance) :
    NodeTraceComponent(nodeInstance)
    , ComponentRoot()
    , backupCopierExePath_(Path::Combine(backupCopierExeDirectory, BackupCopierExeName))
    , workingDir_(workingDirectory)
    , isEnabled_(true)
    , processHandles_()
    , processHandlesLock_()
    , uploadBackupJobQueue_()
    , downloadBackupJobQueue_()
{
}

BackupCopierProxy::~BackupCopierProxy()
{
    if (uploadBackupJobQueue_)
    {
        uploadBackupJobQueue_->UnregisterAndClose();
    }

    if (downloadBackupJobQueue_)
    {
        downloadBackupJobQueue_->UnregisterAndClose();
    }
}

void BackupCopierProxy::Initialize()
{
    uploadBackupJobQueue_ = make_unique<UploadBackupJobQueue>(*this);
    downloadBackupJobQueue_ = make_unique<DownloadBackupJobQueue>(*this);
}

std::shared_ptr<BackupCopierProxy> BackupCopierProxy::Create(
    wstring const & backupCopierExeDirectory,
    wstring const & workingDirectory,
    Federation::NodeInstance const &nodeInstance)
{
    auto proxy = shared_ptr<BackupCopierProxy>(new BackupCopierProxy(backupCopierExeDirectory, workingDirectory, nodeInstance));

    proxy->Initialize();

    return proxy;
}

Common::AsyncOperationSPtr BackupCopierProxy::BeginUploadBackupToAzureStorage(
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent,
    wstring const & connectionString,
    bool isConnectionStringEncrypted,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath,
    wstring const & backupMetadataFileName)
{
    auto operation = shared_ptr<AzureStorageUploadBackupAsyncOperation>(new AzureStorageUploadBackupAsyncOperation(
        *this,
        activityId,
        timeout,
        callback,
        parent,
        connectionString,
        isConnectionStringEncrypted,
        containerName,
        backupStoreBaseFolderPath,
        sourceFileOrFolderPath,
        targetFolderPath,
        backupMetadataFileName));

    uploadBackupJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

Common::ErrorCode BackupCopierProxy::EndUploadBackupToAzureStorage(
    Common::AsyncOperationSPtr const & operation)
{
    return AzureStorageUploadBackupAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr BackupCopierProxy::BeginDownloadBackupFromAzureStorage(
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent,
    wstring const & connectionString,
    bool isConnectionStringEncrypted,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath)
{
    auto operation = shared_ptr<AzureStorageDownloadBackupAsyncOperation>(new AzureStorageDownloadBackupAsyncOperation(
        *this,
        activityId,
        timeout,
        callback,
        parent,
        connectionString,
        isConnectionStringEncrypted,
        containerName,
        backupStoreBaseFolderPath,
        sourceFileOrFolderPath,
        targetFolderPath));

    uploadBackupJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

Common::ErrorCode BackupCopierProxy::EndDownloadBackupFromAzureStorage(
    Common::AsyncOperationSPtr const & operation)
{
    return AzureStorageDownloadBackupAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr BackupCopierProxy::BeginUploadBackupToFileShare(
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent,
    BackupStoreFileShareAccessType::Enum const & accessType,
    bool isPasswordEncrypted,
    wstring const & fileSharePath,
    wstring const & primaryUserName,
    wstring const & primaryPassword,
    wstring const & secondaryUserName,
    wstring const & secondaryPassword,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath,
    wstring const & backupMetadataFileName)
{
    auto operation = shared_ptr<FileShareUploadBackupAsyncOperation>(new FileShareUploadBackupAsyncOperation(
        *this,
        activityId,
        timeout,
        callback,
        parent,
        accessType,
        isPasswordEncrypted,
        fileSharePath,
        primaryUserName,
        primaryPassword,
        secondaryUserName,
        secondaryPassword,
        sourceFileOrFolderPath,
        targetFolderPath,
        backupMetadataFileName));

    uploadBackupJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

Common::ErrorCode BackupCopierProxy::EndUploadBackupToFileShare(
    Common::AsyncOperationSPtr const & operation)
{
    return FileShareUploadBackupAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr BackupCopierProxy::BeginDownloadBackupFromFileShare(
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent,
    BackupStoreFileShareAccessType::Enum const & accessType,
    bool isPasswordEncrypted,
    wstring const & fileSharePath,
    wstring const & primaryUserName,
    wstring const & primaryPassword,
    wstring const & secondaryUserName,
    wstring const & secondaryPassword,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath)
{
    auto operation = shared_ptr<FileShareDownloadBackupAsyncOperation>(new FileShareDownloadBackupAsyncOperation(
        *this,
        activityId,
        timeout,
        callback,
        parent,
        accessType,
        isPasswordEncrypted,
        fileSharePath,
        primaryUserName,
        primaryPassword,
        secondaryUserName,
        secondaryPassword,
        sourceFileOrFolderPath,
        targetFolderPath));

    uploadBackupJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

Common::ErrorCode BackupCopierProxy::EndDownloadBackupFromFileShare(
    Common::AsyncOperationSPtr const & operation)
{
    return FileShareDownloadBackupAsyncOperation::End(operation);
}

void BackupCopierProxy::Abort(HANDLE hProcess)
{
    if (TryRemoveProcessHandle(hProcess))
    {
        auto result = ::TerminateProcess(hProcess, BackupCopierAbortedExitCode);

        WriteInfo(
            TraceComponent,
            "{0} termination of BackupCopier process {1}: handle = {2}",
            NodeTraceComponent::TraceId,
            result == TRUE ? "succeeded" : "failed",
            HandleToInt(hProcess));
    }
}

Common::AsyncOperationSPtr BackupCopierProxy::BeginUploadBackupToDsmsAzureStorage(
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent,
    wstring const & storageCredentialsSourceLocation,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath,
    wstring const & backupMetadataFileName)
{
    auto operation = shared_ptr<DsmsAzureStorageUploadBackupAsyncOperation>(new DsmsAzureStorageUploadBackupAsyncOperation(
        *this,
        activityId,
        timeout,
        callback,
        parent,
        storageCredentialsSourceLocation,
        containerName,
        backupStoreBaseFolderPath,
        sourceFileOrFolderPath,
        targetFolderPath,
        backupMetadataFileName));

    uploadBackupJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

Common::ErrorCode BackupCopierProxy::EndUploadBackupToDsmsAzureStorage(
    Common::AsyncOperationSPtr const & operation)
{
    return DsmsAzureStorageUploadBackupAsyncOperation::End(operation);
}

Common::AsyncOperationSPtr BackupCopierProxy::BeginDownloadBackupFromDsmsAzureStorage(
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent,
    wstring const & storageCredentialsSourceLocation,
    wstring const & containerName,
    wstring const & backupStoreBaseFolderPath,
    wstring const & sourceFileOrFolderPath,
    wstring const & targetFolderPath)
{
    auto operation = shared_ptr<DsmsAzureStorageDownloadBackupAsyncOperation>(new DsmsAzureStorageDownloadBackupAsyncOperation(
        *this,
        activityId,
        timeout,
        callback,
        parent,
        storageCredentialsSourceLocation,
        containerName,
        backupStoreBaseFolderPath,
        sourceFileOrFolderPath,
        targetFolderPath));

    uploadBackupJobQueue_->TryEnqueueOrFail(*this, operation);

    return operation;
}

Common::ErrorCode BackupCopierProxy::EndDownloadBackupFromDsmsAzureStorage(
    Common::AsyncOperationSPtr const & operation)
{
    return DsmsAzureStorageDownloadBackupAsyncOperation::End(operation);
}

// Private function definitions

bool BackupCopierProxy::TryRemoveProcessHandle(HANDLE hProcess)
{
    AcquireExclusiveLock lock(processHandlesLock_);

    auto findIt = find_if(processHandles_.begin(), processHandles_.end(),
        [hProcess](HANDLE const & handle) { return (handle == hProcess); });

    if (findIt != processHandles_.end())
    {
        processHandles_.erase(findIt);

        return true;
    }
    else
    {
        return false;
    }
}

int64 BackupCopierProxy::HandleToInt(HANDLE handle)
{
    return reinterpret_cast<int64>(handle);
}

void BackupCopierProxy::InitializeCommandLineArguments(
    __inout wstring & args,
    __inout wstring & argsLogString)
{
    args.append(L"\"");
    args.append(backupCopierExePath_);
    args.append(L"\" ");

    argsLogString.append(L"\"");
    argsLogString.append(backupCopierExePath_);
    argsLogString.append(L"\" ");
}

void BackupCopierProxy::AddCommandLineArgument(
    __inout wstring & args,
    __inout wstring & argsLogString,
    wstring const & argSwitch,
    wstring const & argValue)
{
    args.append(argSwitch);
    args.append(L":\"");
    args.append(argValue);
    args.append(L"\" ");

    argsLogString.append(argSwitch);
    argsLogString.append(L":\"");
    if ((argSwitch == PrimaryPasswordKeyName) || (argSwitch == SecondaryPasswordKeyName) || (argSwitch == ConnectionStringKeyName))
    {
        // Append dummy value indicating that original value is not logged.
        argsLogString.append(L"***");
    }
    else
    {
        argsLogString.append(argValue);
    }

    argsLogString.append(L"\" ");
}

AsyncOperationSPtr BackupCopierProxy::BeginRunBackupCopierExe(
    ActivityId const & activityId,
    wstring const & operation,
    __in wstring & args,
    __in wstring & argsLogString,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<RunBackupCopierExeAsyncOperation>(
        *this,
        activityId,
        operation,
        args,
        argsLogString,
        timeout,
        callback,
        parent);
}

ErrorCode BackupCopierProxy::EndRunBackupCopierExe(
    AsyncOperationSPtr const & operation)
{
    return RunBackupCopierExeAsyncOperation::End(operation);
}

ErrorCode BackupCopierProxy::TryStartBackupCopierProcess(
    wstring const & operation,
    __in wstring & args,
    __in wstring & argsLogString,
    __inout TimeSpan & timeout,
    __out wstring & tempWorkingDirectory,
    __out wstring & errorDetailsFile,
    __out HANDLE & hProcess,
    __out HANDLE & hThread,
    __out pid_t & pid)
{
    pid = 0;

    vector<wchar_t> envBlock(0);
    auto error = ProcessUtility::CreateDefaultEnvironmentBlock(envBlock);
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to create EnvironmentBlock for BackupCopier due to {1}",
            NodeTraceComponent::TraceId,
            error);
        return error;
    }

    wchar_t* env = envBlock.data();

    PROCESS_INFORMATION processInfo = { 0 };
    STARTUPINFO startupInfo = { 0 };
    startupInfo.cb = sizeof(STARTUPINFO);

    // Set the bInheritHandle flag so pipe handles are inherited (from MSDN example)
    SECURITY_ATTRIBUTES secAttr;
    secAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = NULL;

    tempWorkingDirectory = this->GetBackupCopierTempWorkingDirectory();
    errorDetailsFile = Path::Combine(tempWorkingDirectory, ErrorDetailsFilename);

    error = CreateOutputDirectory(tempWorkingDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Add common commandline arguments
    //
    AddCommandLineArgument(args, argsLogString, OperationKeyName, operation);
    AddCommandLineArgument(args, argsLogString, WorkingDirKeyName, tempWorkingDirectory);
    AddCommandLineArgument(args, argsLogString, ErrorDetailsFileKeyName, errorDetailsFile);

    // Adjust the timeout so that we can return an specific timeout error
    // to the client (e.g. avoid client operation timing out before server). Don't adjust
    // if the timeout is already "too small". The client-side will just get a generic timeout
    // error is such cases.
    //
    auto timeoutBuffer = BackupRestoreService::BackupRestoreServiceConfig::GetConfig().BackupCopierTimeoutBuffer;
    if (timeout.TotalMilliseconds() > timeoutBuffer.TotalMilliseconds() * 2)
    {
        timeout = timeout.SubtractWithMaxAndMinValueCheck(timeoutBuffer);
    }

    AddCommandLineArgument(args, argsLogString, TimeoutKeyName, wformatString("{0}", timeout.Ticks));

    BAEventSource::Events->BackupCopierCreating(
        this->NodeInstance,
        argsLogString,
        timeout,
        timeoutBuffer);

#if defined(PLATFORM_UNIX)
    vector<char> ansiEnvironment;
    while (*env)
    {
        string enva;
        wstring envw(env);
        StringUtility::Utf16ToUtf8(envw, enva);
        ansiEnvironment.insert(ansiEnvironment.end(), enva.begin(), enva.end());
        ansiEnvironment.push_back(0);
        env += envw.length() + 1;
    }

#endif

    SecureString secureArgs(move(args));

    BOOL success = FALSE;
    {
        AcquireExclusiveLock lock(processHandlesLock_);

        if (isEnabled_)
        {
            success = ::CreateProcessW(
                NULL,
                const_cast<LPWSTR>(secureArgs.GetPlaintext().c_str()),
                NULL,   // process attributes
                NULL,   // thread attributes
                FALSE,  // do not inherit handles
#if defined(PLATFORM_UNIX)
                0,
                ansiEnvironment.data(),
#else
                CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, // creation flags
                env,
#endif
                NULL,   // current directory
                &startupInfo,
                &processInfo);

            if (success == TRUE)
            {
                processHandles_.push_back(processInfo.hProcess);
            }
        }
        else
        {
            ::SetLastError(ErrorCode(ErrorCodeValue::ObjectClosed).ToHResult());
        }
    }

    if (success == TRUE)
    {
        BAEventSource::Events->BackupCopierCreated(
            this->NodeInstance,
            HandleToInt(processInfo.hProcess),
            processInfo.dwProcessId);

        hProcess = processInfo.hProcess;
        hThread = processInfo.hThread;
        pid = processInfo.dwProcessId;

        return ErrorCodeValue::Success;
    }
    else
    {
        DWORD winError = ::GetLastError();
        error = ErrorCode::FromWin32Error(winError);

        WriteInfo(
            TraceComponent,
            "{0} could not create BackupCopier process due to {1} ({2})",
            NodeTraceComponent::TraceId,
            error,
            winError);

        return error;
    }
}

ErrorCode BackupCopierProxy::FinishBackupCopierProcess(
    ErrorCode const & waitForProcessError,
    DWORD exitCode,
    wstring const & tempWorkingDirectory,
    wstring const & errorDetailsFile,
    HANDLE hProcess,
    HANDLE hThread)
{
    WriteInfo(
        TraceComponent,
        "{0} BackupCopier process finished with exitCode '{1}'",
        NodeTraceComponent::TraceId,
        exitCode);

    auto error = waitForProcessError;

    if (error.IsSuccess())
    {
        error = GetBackupCopierError(exitCode, errorDetailsFile);
    }
    else
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to wait for BackupCopier process due to error '{1}': handle = {2}",
            NodeTraceComponent::TraceId,
            error,
            HandleToInt(hProcess));

        // BackupCopier.exe should consume the /timeout parameter and self-terminate,
        // but also perform a best-effort termination of the process here as well.
        //
        if (::TerminateProcess(hProcess, BackupCopierAbortedExitCode) == FALSE)
        {
            WriteInfo(
                TraceComponent,
                "{0} failed to terminate BackupCopier process on error '{1}': handle = {2}",
                NodeTraceComponent::TraceId,
                error,
                HandleToInt(hProcess));
        }
    }

    if (error.IsError(ErrorCodeValue::Timeout))
    {
        // Return a more specific error code for better debugging
        //
        error = ErrorCodeValue::BackupCopierTimeout;
    }

    else if (error.IsError(ErrorCodeValue::BackupCopierAborted))
    {
        // BackupCopierAborted currently has no corresponding public error code.
        // Expose public HRESULT if/when we find a need for it.
        //
        error = ErrorCodeValue::OperationCanceled;
    }

    DeleteDirectory(tempWorkingDirectory);

    // best-effort
    //
    ::CloseHandle(hProcess);
    ::CloseHandle(hThread);

    TryRemoveProcessHandle(hProcess);

    return error;
}

wstring BackupCopierProxy::GetBackupCopierTempWorkingDirectory()
{
    return Path::Combine(
        workingDir_,
        Path::Combine(
            TempWorkingDirectoryPrefix,
            wformatString("{0}", SequenceNumber::GetNext())));
}

ErrorCode BackupCopierProxy::CreateOutputDirectory(wstring const & directory)
{
    if (Directory::Exists(directory))
    {
        WriteNoise(
            TraceComponent,
            "{0} directory '{1}' already exists",
            NodeTraceComponent::TraceId,
            directory);
        return ErrorCodeValue::Success;
    }

    ErrorCode error = Directory::Create2(directory);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "{0} could not create BackupCopierProxy temporary directory '{1}' due to {2}",
            NodeTraceComponent::TraceId,
            directory,
            error);
    }

    return error;
}

void BackupCopierProxy::DeleteDirectory(wstring const & directory)
{
    if (!Directory::Exists(directory))
    {
        WriteNoise(
            TraceComponent,
            "{0} directory '{1}' not found - skipping delete",
            NodeTraceComponent::TraceId,
            directory);
        return;
    }

    ErrorCode error = Directory::Delete(directory, true);

    // best effort clean-up (do not fail the operation even if clean-up fails)
    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} could not cleanup BackupCopier temporary directory '{1}' due to {2}",
            NodeTraceComponent::TraceId,
            directory,
            error);
    }
}

ErrorCode BackupCopierProxy::GetBackupCopierError(
    DWORD BackupCopierError,
    wstring const & errorDetailsFile)
{
    auto error = ErrorCode::FromWin32Error(BackupCopierError);

    ErrorCodeValue::Enum errorValue = ErrorCodeValue::Success;
    wstring errorStack;
    if (!error.IsSuccess())
    {
        // this->PerfCounters.RateOfBackupCopierFailures.Increment();
        if (File::Exists(errorDetailsFile))
        {
            wstring details;
            auto innerError = ReadBackupCopierOutput(errorDetailsFile, details);
            if (innerError.IsSuccess() && !details.empty())
            {
                wstring delimiter = L",";
                size_t found = details.find(delimiter);
                if (found != string::npos)
                {
                    wstring errorCodeString = details.substr(0, found);
                    errorStack = details.substr(found + 1);
                    int errorCodeInt;
                    StringUtility::TryFromWString<int>(errorCodeString, errorCodeInt);
                    errorValue = ErrorCode::FromWin32Error((unsigned int)errorCodeInt).ReadValue();
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0} failed to get BackupCopier return code due to wrong format of error file",
                        NodeTraceComponent::TraceId);
                    errorValue = error.ReadValue();
                    errorStack = details;
                }
            }
        }
    }

    else if (error.IsError(ErrorCode::FromWin32Error(BackupCopierAbortedExitCode).ReadValue()))
    {
        // Error details file will not exist if BackupCopier.exe is explicitly terminated
        errorValue = ErrorCodeValue::BackupCopierAborted;
    }

    if (!error.IsSuccess())
    {
        if (errorValue == ErrorCodeValue::Success)
        {
            auto msg = wformatString(
                "{0} unexpected success error code on BackupCopier failure {1} ({2}) - overriding with BackupCopierUnexpectedError",
                NodeTraceComponent::TraceId,
                BackupCopierError,
                error);

            WriteError(TraceComponent, "{0}", msg);

            Assert::TestAssert("{0}", msg);

            errorValue = ErrorCodeValue::BackupCopierUnexpectedError;
        }

        error = ErrorCode(errorValue, move(errorStack));

        BAEventSource::Events->BackupCopierFailed(
            this->NodeInstance,
            error,
            error.Message);
    }

    return error;
}

ErrorCode BackupCopierProxy::ReadBackupCopierOutput(
    wstring const & inputFile,
    __out wstring & result,
    __in bool isBOMPresent)
{
    File file;
    auto error = file.TryOpen(
        inputFile,
        FileMode::Open,
        FileAccess::Read,
        FileShare::Read,
#if defined(PLATFORM_UNIX)
        FileAttributes::Normal
#else
        FileAttributes::ReadOnly
#endif
    );

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to open '{1}' for read: error={2}",
            NodeTraceComponent::TraceId,
            inputFile,
            error);

        return ErrorCodeValue::BackupCopierRetryableError;
    }

    error = ReadFromFile(file, result, isBOMPresent);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            "{0} failed to read from file '{1}': error={2}",
            NodeTraceComponent::TraceId,
            inputFile,
            error);
    }

    return error;
}

ErrorCode BackupCopierProxy::ReadFromFile(
    __in File & file,
    __out wstring & result,
    __in bool isBOMPresent)
{
    int bytesRead = 0;
    int fileSize = static_cast<int>(file.size());
    vector<byte> buffer(fileSize);

    if ((bytesRead = file.TryRead(reinterpret_cast<void*>(buffer.data()), fileSize)) > 0)
    {
        // ensure null termination
        buffer.push_back(0);

        // wide null for unicode
        buffer.push_back(0);


        // if the caller indicated that BOM is present, skip byte-order mark
        if (isBOMPresent)
        {
            result = wstring(reinterpret_cast<wchar_t *>(&buffer[2]));
        }

        return ErrorCodeValue::Success;
    }

    WriteInfo(
        TraceComponent,
        "{0} failed to read from file: expected {1} bytes, read {2} bytes",
        NodeTraceComponent::TraceId,
        fileSize,
        bytesRead);

    return ErrorCodeValue::BackupCopierRetryableError;
}
