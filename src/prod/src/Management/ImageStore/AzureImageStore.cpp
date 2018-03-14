// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;
using namespace ImageStore;
using namespace FileStoreService;

StringLiteral const AzureImageStoreLiteral("AzureImageStore");

GlobalWString AzureImageStore::SchemaTag = make_global<wstring>(L"xstore:");
GlobalWString ImageStoreClient = make_global<wstring>(L"ImageStoreClient.exe");
GlobalWString DownloadCommand = make_global<wstring>(L"Download");
GlobalWString UploadCommand = make_global<wstring>(L"Upload");
GlobalWString DeleteCommand = make_global<wstring>(L"Delete");
GlobalWString ExistsCommand = make_global<wstring>(L"Exists");

GlobalWString CopyIfDifferent = make_global<wstring>(L"CopyIfDifferent");
GlobalWString AtomicCopy = make_global<wstring>(L"AtomicCopy");

AzureImageStore::AzureImageStore(    
    wstring const & cacheUri, 
    wstring const & localRoot,
    wstring const & workingDirectory,
    int minTransferBPS)
    : IImageStore(localRoot, cacheUri)
    , workingDirectory_(workingDirectory)
    , minTransferBPS_(minTransferBPS)
{
    IImageStore::WriteInfo(
        AzureImageStoreLiteral,
        "AzureImageStore created with CacheUri={0}, LocalRoot={1}, WorkingDirectory={2}, MinTransferBPS={3}",
        cacheUri,
        localRoot,
        workingDirectory,
        minTransferBPS);
}

ErrorCode AzureImageStore::OnDownloadFromStore(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    vector<CopyFlag::Enum> const & flags,
	Common::TimeSpan const timeout)
{
    TimeoutHelper timeoutHelper(timeout);
    vector<wstring> localFileNames;    
    for(size_t i = 0; i < localObjects.size(); i++)
    {           
        localFileNames.push_back(GetLocalFileNameForDownload(localObjects[i]));        
    }

    wstring tempInputFileName = File::GetTempFileNameW(workingDirectory_);

    auto commandLine = PrepareCommandLine(remoteObjects, localFileNames, flags, *DownloadCommand, tempInputFileName);
    auto error = RunStoreExe(commandLine, timeoutHelper.GetRemainingTime());

    if(File::Exists(tempInputFileName))
    {
        File::Delete(tempInputFileName);
    }

    return error;
}

ErrorCode AzureImageStore::OnUploadToStore(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    vector<CopyFlag::Enum> const & flags,
	Common::TimeSpan const timeout)
{
    TimeoutHelper timeoutHelper(timeout);
    vector<wstring> localFileNames;    
    for(size_t i = 0; i < localObjects.size(); i++)
    {           
        localFileNames.push_back(GetLocalFileNameForUpload(localObjects[i]));        
    }

    wstring tempInputFileName = File::GetTempFileNameW(workingDirectory_);

    auto commandLine = PrepareCommandLine(remoteObjects, localFileNames, flags, *UploadCommand, tempInputFileName);
    auto error = RunStoreExe(commandLine, timeoutHelper.GetRemainingTime());

    if(File::Exists(tempInputFileName))
    {
        File::Delete(tempInputFileName);
    }

    return error;
}

ErrorCode AzureImageStore::OnRemoveObjectFromStore(
    vector<wstring> const & remoteObjects,
    Common::TimeSpan const timeout)
{
    TimeoutHelper timeoutHelper(timeout);
    vector<CopyFlag::Enum> flags;
    wstring tempInputFileName = File::GetTempFileNameW(workingDirectory_);

    auto commandLine = PrepareCommandLine(remoteObjects, remoteObjects, flags, *DeleteCommand, tempInputFileName);
    auto error = RunStoreExe(commandLine, timeoutHelper.GetRemainingTime());

    if(File::Exists(tempInputFileName))
    {
        File::Delete(tempInputFileName);
    }

    return error;
}

ErrorCode AzureImageStore::OnDoesContentExistInStore(
    vector<wstring> const & remoteObjects,
    Common::TimeSpan const timeout,
    __out map<wstring, bool> & objectExistsMap)
{    
    TimeoutHelper timeoutHelper(timeout);
    vector<wstring> dummyLocalObjects;
    vector<CopyFlag::Enum> dummyFlags;

    for(auto iter = remoteObjects.begin(); iter!= remoteObjects.end(); ++iter)
    {
        dummyLocalObjects.push_back(L"");
        dummyFlags.push_back(CopyFlag::Enum::Echo);
    }

    wstring tempInputFileName = File::GetTempFileNameW(workingDirectory_);
    wstring tempOutputFileName = File::GetTempFileNameW(workingDirectory_);

    auto commandLine = PrepareCommandLine(remoteObjects, dummyLocalObjects, dummyFlags, *ExistsCommand, tempInputFileName, tempOutputFileName);

    auto error = RunStoreExe(commandLine, timeoutHelper.GetRemainingTime());
    if(error.IsSuccess())
    {
        File outputFile;
        error = outputFile.TryOpen(tempOutputFileName, FileMode::Open, FileAccess::Read);
        if (!error.IsSuccess())
        {
            IImageStore::WriteError(
                AzureImageStoreLiteral,
                "OnDoesContentExistInStore: Unable to open the output file {0}: {1}",
                tempOutputFileName,
                error);
            return error;
        }
        
        wstring text;
        int fileSize = static_cast<int>(outputFile.size());
        vector<byte> buffer(fileSize);

        if (outputFile.TryRead(reinterpret_cast<void*>(buffer.data()), fileSize) > 0)
        {
            // ensure null termination
            buffer.push_back(0);
            buffer.push_back(0);

            // skip byte-order mark
            text = wstring(reinterpret_cast<wchar_t *>(&buffer[2]));            
        }

        if (timeoutHelper.GetRemainingTime() <= Common::TimeSpan::Zero)
        {
            return ErrorCodeValue::Timeout;
        }

        vector<wstring> lines;
        StringUtility::Split<wstring>(text, lines, L"\r\n");
        for(auto iter = lines.begin(); iter != lines.end(); ++iter)
        {
            vector<wstring> keyValue;
            StringUtility::Split<wstring>(*iter, keyValue, L":");
            if(keyValue.size() == 2)
            {                
                objectExistsMap[keyValue[0]] = StringUtility::AreEqualCaseInsensitive(keyValue[1], L"true") ? true : false;
            }
        }        
    }

    if(File::Exists(tempInputFileName))
    {
        File::Delete(tempInputFileName);
    }

    if(File::Exists(tempOutputFileName))
    {
        File::Delete(tempOutputFileName);
    }        

    return error;
}

ErrorCode AzureImageStore::RunStoreExe(
    wstring const & cmdLine,
    Common::TimeSpan timeout)
{
//LINUXTODO
#if defined(PLATFORM_UNIX)
    Assert::CodingError("Not implemented");
#else

    vector<wchar_t> envBlock;
    HandleUPtr jobHandle;
    HandleUPtr processHandle;	
    HandleUPtr threadHandle;

    TimeoutHelper timeoutHelper(timeout);

    ErrorCode err = ProcessUtility::CreateDefaultEnvironmentBlock(envBlock);
    if (!err.IsSuccess())
    {
        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "RunStoreExe: CreateDefaultEnvironmentBlock failed with {0}",
            err);
        return err;
    }

    err = ProcessUtility::CreateAnnonymousJob(false /*allowBreakaway*/, jobHandle);
    if (!err.IsSuccess())
    {
        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "RunStoreExe: CreateAnnonymousJob failed with {0}",
            err);
        return err;
    }

    err = ProcessUtility::CreateProcess(cmdLine, L"", envBlock, ProcessUtility::CreationFlags_SuspendedProcessWithJobBreakawayNoWindow, processHandle, threadHandle);
    if (!err.IsSuccess())
    {
        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "RunStoreExe: CreateProcess failed with {0}",
            err);
        return err;
    }

    err = ProcessUtility::AssociateProcessToJob(jobHandle, processHandle);
    if (!err.IsSuccess())
    {
        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "RunStoreExe: AssociateProcessToJob failed with {0}",
            err);
        return err;
    }

    err = ProcessUtility::ResumeProcess(processHandle, threadHandle);
    if (!err.IsSuccess())
    {
        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "RunStoreExe: ResumeProcess failed with {0}",
            err);
        return err;
    }

    AutoResetEvent arEvent;
    auto result = ThreadpoolWait::Create(
        Handle(*processHandle,Handle::DUPLICATE()),
        [&arEvent] (Handle const &, ErrorCode const &)
    {
        arEvent.Set();
    });

    Common::TimeSpan remainingTime = timeoutHelper.GetRemainingTime();

    if(!arEvent.WaitOne(remainingTime))
    {
        // If the ImageStoreClient.exe does not complete within the configured timeout,
        // kill the process

        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "ImageStoreClient.exe did not complete within {0}. Terminating the process.",
            remainingTime);

        ProcessUtility::TerminateProcess(processHandle);
    }

    DWORD exitCode;
    if (::GetExitCodeProcess(processHandle->Value, &exitCode))
    {
        ErrorCode error;

        if (exitCode == 0)
        {
            error = ErrorCodeValue::Success;
        }
        else if (exitCode == 2)
        {
            error = ErrorCodeValue::FileNotFound;
        }
        else 
        {
            error = ErrorCodeValue::ImageStoreUnableToPerformAzureOperation;
        }

        if (exitCode == (ErrorCodeValue::ProcessDeactivated & 0x0000FFFF)) // This exit code indicates timeout happened
        {
            IImageStore::WriteInfo(
                AzureImageStoreLiteral,
                "ImageStoreClient.exe exited with operation timed out.");
        }
        else
        {
            IImageStore::WriteInfo(
                AzureImageStoreLiteral,
                "ImageStoreClient.exe exited with exit code {0}.",
                exitCode);
        }

        return error;
    }
    else
    {
        err = ErrorCode::FromWin32Error();
        IImageStore::WriteWarning(
            AzureImageStoreLiteral,
            "Could not get exit code of ImageStoreClient.exe. Last error : {0}.",
            err);
        return err;
    }
#endif
}

wstring AzureImageStore::PrepareCommandLine(
    vector<wstring> const & storeTags, 
    vector<wstring> const & cacheTags, 
    vector<CopyFlag::Enum> const & flags,
    wstring const & command,
    wstring const & inputFile,
    wstring const & outputFile)
{
    if (command == *DeleteCommand)
    {
        ASSERT_IFNOT(storeTags.size() == 1 && cacheTags.size() == 1, "Deletion should specify just 1 store tag and cache tags (cache tags would not be used).");
        return wformatString("{0} {1} -x \"{2}\"", *ImageStoreClient, *DeleteCommand, storeTags[0]);
    }
    if (storeTags.size() == 1)
    {
        ASSERT_IFNOT(storeTags.size() == cacheTags.size(), "Cache tags and store tags should have same count.");
        if(outputFile == L"")
        {
            return wformatString(
                "{0} {1} -x \"{2}\" -l \"{3}\" -g {4} -b {5}", 
                *ImageStoreClient, 
                command,                 
                storeTags[0], 
                cacheTags[0], 
                flags[0] == CopyFlag::Echo ? CopyIfDifferent : AtomicCopy,
                minTransferBPS_);
        }
        else
        {
            return wformatString(
                "{0} {1} -x \"{2}\" -l \"{3}\" -g {4} -b {5} -o \"{6}\"", 
                *ImageStoreClient, 
                command,
                storeTags[0], 
                cacheTags[0], 
                flags[0] == CopyFlag::Echo ? CopyIfDifferent : AtomicCopy,
                minTransferBPS_,
                outputFile);
        }

    }
    File tempFile;        
    auto error = tempFile.TryOpen(inputFile, FileMode::Create, FileAccess::Write);
    if (!error.IsSuccess())
    {
        IImageStore::WriteError(
            AzureImageStoreLiteral,
            "Unable to open temporary file to pass the tags values: {0}",
            error);
        return L"";
    }

    string fileContent = "";
    for (size_t i = 0; i < storeTags.size(); i++)
    {

        fileContent = formatString(
            "{0}{1}|{2}|{3}\n", 
            fileContent,
            storeTags[i], 
            cacheTags[i], 
            flags[i] == CopyFlag::Echo ? CopyIfDifferent : AtomicCopy);
    }
    tempFile.TryWrite(&fileContent[0], (int) fileContent.size());
    tempFile.Close();

    IImageStore::WriteInfo(
        AzureImageStoreLiteral,
        "PrepareCommandLine: Wrote file tags to file:'{0}' for operation:{1}",
        inputFile,
        command);

    if(outputFile == L"")
    {
        return wformatString("{0} {1} -f \"{2}\" -b {3}", *ImageStoreClient, command, inputFile, minTransferBPS_);
    }
    else
    {
        return wformatString("{0} {1} -f \"{2}\" -b {3} -o \"{4}\"", *ImageStoreClient, command, inputFile, minTransferBPS_, outputFile);
    }
}
