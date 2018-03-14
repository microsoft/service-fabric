// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace std;
using namespace ServiceModel;
using namespace Management;
using namespace ImageStore;
using namespace FileStoreService;

StringLiteral const TraceSource = "NativeImageStoreExecutor";

GlobalWString NativeImageStoreExecutor::CreateDirectoryCommand = make_global<wstring>(L"iss.createdir");
GlobalWString NativeImageStoreExecutor::CreateFileCommand = make_global<wstring>(L"iss.createfile");
GlobalWString NativeImageStoreExecutor::UploadCommand = make_global<wstring>(L"iss.upload");
GlobalWString NativeImageStoreExecutor::DeleteCommand = make_global<wstring>(L"iss.delete");
GlobalWString NativeImageStoreExecutor::DownloadCommand = make_global<wstring>(L"iss.download");
GlobalWString NativeImageStoreExecutor::ListCommand = make_global<wstring>(L"iss.list");
GlobalWString NativeImageStoreExecutor::WaitCommand = make_global<wstring>(L"iss.wait");
GlobalWString NativeImageStoreExecutor::VerifyCommand = make_global<wstring>(L"iss.verify");

struct NativeImageStoreExecutor::ResourceInfo
{
    DENY_COPY(ResourceInfo)

public:
    ResourceInfo(std::wstring const & name, std::wstring const & location, bool isFile)
        : Name(name), Location(location), IsFile(isFile)
    {
    }

    ~ResourceInfo()
    {
        if(IsFile)
        {
            Common::File::Delete(Location);
        }
        else
        {
            Common::Directory::Delete(Location, true, true);
        }
    }

    std::wstring Name;
    std::wstring Location;
    bool IsFile;
};

NativeImageStoreExecutor::NativeImageStoreExecutor(FabricTestDispatcher & dispatcher)
    : dispatcher_(dispatcher)
    , pendingAsyncOperationCount_(0)
{    
}

NativeImageStoreExecutor::~NativeImageStoreExecutor()
{
}

bool NativeImageStoreExecutor::IsNativeImageStoreClientCommand(std::wstring const & command)
{
    wstring iss(L"iss.");
    return StringUtility::StartsWith(command, iss) ? true : false;
}

bool NativeImageStoreExecutor::ExecuteCommand(std::wstring const & command, StringCollection const & params)
{
    if(command == NativeImageStoreExecutor::CreateDirectoryCommand)
    {
        return CreateTestDirectory(params);
    }
    else if(command == NativeImageStoreExecutor::CreateFileCommand)
    {
        return CreateTestFile(params);
    }
    else if(command == NativeImageStoreExecutor::UploadCommand)
    {
        return Upload(params);
    }
    else if(command == NativeImageStoreExecutor::DownloadCommand)
    {
        return Download(params);
    }
    else if(command == NativeImageStoreExecutor::DeleteCommand)
    {
        return Delete(params);
    }
    else if(command == NativeImageStoreExecutor::WaitCommand)
    {
        return Wait();
    }
    else if(command == NativeImageStoreExecutor::VerifyCommand)
    {
        int retryCount = 0;
        do
        {
            if(Verify())
            {
                return true;
            }
            
            //sleep for 5 seconds
            ::Sleep(5000);

        }while(++retryCount < 10);
    }

    return false;
}



bool NativeImageStoreExecutor::Upload(StringCollection const & params)
{
    wstring resourceName = params[0];
    wstring storeRelativePath = params[1];

    CommandLineParser parser(params, 2);
    wstring expectedErrorString;
    parser.TryGetString(L"error", expectedErrorString, L"Success");

    ErrorCode expectedError = FABRICSESSION.FabricDispatcher.ParseErrorCode(expectedErrorString);

    bool shouldOverwrite = parser.GetBool(L"overwrite");
    bool shouldPost = parser.GetBool(L"async");

    if(shouldPost)
    {        
        PostOperation(
            [this, resourceName, storeRelativePath, shouldOverwrite, expectedError]() { this->UploadToService(resourceName, storeRelativePath, shouldOverwrite, expectedError); });
    }
    else
    {
        this->UploadToService(resourceName, storeRelativePath, shouldOverwrite, expectedError);
    }

    return true;
}

bool NativeImageStoreExecutor::Download(StringCollection const & params)
{
    wstring storeRelativePath = params[0];    

    CommandLineParser parser(params, 1);
    wstring expectedErrorString;
    parser.TryGetString(L"error", expectedErrorString, L"Success");
    ErrorCode expectedError = FABRICSESSION.FabricDispatcher.ParseErrorCode(expectedErrorString);

    bool shouldPost = parser.GetBool(L"async");

    if(shouldPost)
    {
        PostOperation(
            [this, storeRelativePath, expectedError]() { this->DownloadFromService(storeRelativePath, expectedError); });
    }
    else
    {
        this->DownloadFromService(storeRelativePath, expectedError);
    }

    return true;
}

bool NativeImageStoreExecutor::Delete(StringCollection const & params)
{
    wstring storeRelativePath = params[0];    
    CommandLineParser parser(params, 1);
    wstring expectedErrorString;
    parser.TryGetString(L"error", expectedErrorString, L"Success");
    ErrorCode expectedError = FABRICSESSION.FabricDispatcher.ParseErrorCode(expectedErrorString);

    bool shouldPost = parser.GetBool(L"async");

    if(shouldPost)
    {
        PostOperation(
            [this, storeRelativePath, expectedError]() { this->DeleteFromService(storeRelativePath, expectedError); });
    }
    else
    {
        this->DeleteFromService(storeRelativePath, expectedError);
    }

    return true;
}

bool NativeImageStoreExecutor::Wait()
{
    pendingOperationsEvent_.WaitOne();
    return true;
}

void NativeImageStoreExecutor::PostOperation(std::function<void()> const & operation)
{
    {
        AcquireWriteLock lock(lock_);
        if(pendingAsyncOperationCount_++ == 0)
        {
            pendingOperationsEvent_.Reset();
        }
    }

    Threadpool::Post(
        [this, operation]()
    {
        operation();

        {
            AcquireWriteLock lock(lock_);
            if(--pendingAsyncOperationCount_ == 0)
            {
                pendingOperationsEvent_.Set();
            }
        }
    });
}

void NativeImageStoreExecutor::UploadToService(std::wstring const & resourceName, std::wstring const & storeRelativePath, bool const shouldOverwrite, ErrorCode const & expectedError)
{    
    auto fileStoreClient = make_shared<InternalFileStoreClient>(
        *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
        dispatcher_.CreateClientFactory());

    NativeImageStore nativeImageStore(
        Management::ImageStore::Constants::NativeImageStoreSchemaTag,
        L"",
        L"",
        fileStoreClient);

    auto iter = createdResourcesMap_.find(resourceName);
    TestSession::FailTestIf(iter == createdResourcesMap_.end(), "Resource {0} is not created.", resourceName);

    if(expectedError.IsSuccess())
    {
        // If the upload is expected to succeed, then add it to the uploaded map
        // iss.verify will validate that the files are indeed upload
        AcquireWriteLock lock(lock_);
        auto existingIter = uploadedResourceMap_.find(storeRelativePath);
        if(existingIter != uploadedResourceMap_.end())
        {
            uploadedResourceMap_.erase(existingIter);
        }

        uploadedResourceMap_.insert(
            make_pair(storeRelativePath, iter->second));
    }

    ResourceInfo & resourceInfo = *(iter->second);

    auto error = nativeImageStore.UploadContent(
        storeRelativePath,
        resourceInfo.Location,
        ServiceModelConfig::GetConfig().MaxFileOperationTimeout, 
        shouldOverwrite ? CopyFlag::Overwrite : CopyFlag::Echo);

    bool success = error.IsError(expectedError.ReadValue());
    TestSession::FailTestIf(
        !success,
        "Upload failed. ResourceName:{0}, SourceLocation:{1}, StoreRelativePath:{2}, ShouldOverwrite:{3}, ExpectedError:{4}, ActualError:{5}",
        resourceName, resourceInfo.Location, storeRelativePath, shouldOverwrite, expectedError, error);
}

void NativeImageStoreExecutor::DownloadFromService(std::wstring const & storeRelativePath, ErrorCode const & expectedError)
{
    auto fileStoreClient = make_shared<InternalFileStoreClient>(
        *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
        dispatcher_.CreateClientFactory());

    NativeImageStore nativeImageStore(
        Management::ImageStore::Constants::NativeImageStoreSchemaTag,
        L"",
        L"",
        fileStoreClient);

    wstring destinationPath = File::GetTempFileNameW(FabricTestDispatcher::TestDataDirectory);

    auto error = nativeImageStore.DownloadContent(
        storeRelativePath,
        destinationPath,
        ServiceModelConfig::GetConfig().MaxFileOperationTimeout,
        CopyFlag::Overwrite);

    bool success = error.IsError(expectedError.ReadValue());
    TestSession::FailTestIf(
        !success,
        "Download failed. SourceLocation:{0}, DestinationPath:{1}, ExpectedError:{2}, ActualError:{3}",
        storeRelativePath, destinationPath, expectedError, error);

    if(error.IsSuccess())
    {
        wstring originalLocalPath;
        {
            AcquireReadLock lock(lock_);
            auto iter = uploadedResourceMap_.find(storeRelativePath);
            if(iter == uploadedResourceMap_.end())
            {
                TestSession::FailTest("The StoreRelativePath:{0} is not found in the UploadedResourceMap", storeRelativePath);
                return;
            }

            originalLocalPath = iter->second->Location;
        }

        success = IsContentEqual(destinationPath, originalLocalPath);
        if(!success)
        {
            TestSession::FailTest("The download resouce does not match the original resource. OriginalResource:{0}, DownloadResource:{1}", originalLocalPath, destinationPath);
        }
    }
}

void NativeImageStoreExecutor::DeleteFromService(std::wstring const & storeRelativePath, ErrorCode const & expectedError)
{
    auto fileStoreClient = make_shared<InternalFileStoreClient>(
        *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
        dispatcher_.CreateClientFactory());

    NativeImageStore nativeImageStore(
        Management::ImageStore::Constants::NativeImageStoreSchemaTag,
        L"",
        L"",
        fileStoreClient);

    auto error = nativeImageStore.DeleteContent(storeRelativePath, Common::TimeSpan::Zero);

    bool success = error.IsError(expectedError.ReadValue());
    TestSession::FailTestIf(
        !success,
        "Delete failed. SourceLocation:{0}, ExpectedError:{1}, ActualError:{2}",
        storeRelativePath, expectedError, error);

    if(error.IsSuccess())
    {
        auto iter = uploadedResourceMap_.find(storeRelativePath);
        if(iter != uploadedResourceMap_.end())
        {
            uploadedResourceMap_.erase(iter);
        }        
    }
}

bool NativeImageStoreExecutor::Verify()
{
    UploadedResourceMap uploadedResourceMap;
    {
        AcquireReadLock lock(lock_);
        uploadedResourceMap = uploadedResourceMap_;
    }

    auto clientFactory = this->dispatcher_.CreateClientFactory();

    Api::IInternalFileStoreServiceClientPtr fileStoreServiceClient;
    auto error = clientFactory->CreateInternalFileStoreServiceClient(fileStoreServiceClient);
    if(!error.IsSuccess())
    {
        return false;
    }

    auto const & range = *Reliability::ConsistencyUnitId::FileStoreIdRange;
    Reliability::ConsistencyUnitId imageStoreServiceCUID = Reliability::ConsistencyUnitId::CreateFirstReservedId(range);
    ManualResetEvent operationCompleted(false);
    vector<StoreFileInfo> metadataList;
    vector<StoreFolderInfo> folderList;
    vector<wstring> availableShares;
   
    fileStoreServiceClient->BeginListFiles(
        imageStoreServiceCUID.Guid,
        L"",
        L"",
        false,
        true,
        false,
        TimeSpan::FromSeconds(30),
        [&fileStoreServiceClient, &metadataList, &folderList, &availableShares, &operationCompleted, &error](AsyncOperationSPtr const & operation)
    {
        wstring continuationToken;
        error = fileStoreServiceClient->EndListFiles(operation, metadataList, folderList, availableShares, continuationToken);
        operationCompleted.Set();
    },
        AsyncOperationSPtr());

    operationCompleted.WaitOne();
    if (!error.IsSuccess())
    {
        TestSession::WriteWarning(
            TraceSource, 
            "Could not get the MetadataList from the ImageStoreServices");
        return false;
    }

    for (auto const & shareLocation : availableShares)
    {
        if(!ValidateStore(shareLocation, metadataList, uploadedResourceMap))
        {
            TestSession::WriteWarning(
                TraceSource, 
                "Validation of the store failed. StoreLocation:{0}",
                shareLocation);
            return false;
        }
    }

    return true;
}

bool NativeImageStoreExecutor::ValidateStore(
    wstring const & storeShareLocation, 
    vector<StoreFileInfo> const & metadataList,
    UploadedResourceMap const & uploadedResources)
{    
    for (auto const & uploadedResource : uploadedResources)
    {
        ResourceInfo & info = *(uploadedResource.second);
        if(info.IsFile)
        {
            if(!ValidateFile(uploadedResource.first, uploadedResource.second->Location, storeShareLocation, metadataList))
            {
                TestSession::WriteWarning(
                    TraceSource, 
                    "ValidateFile failed. LocalFilePath:{0}, StoreShareLocation:{1}, StoreRelativePath:{2}",
                    uploadedResource.second->Location,
                    storeShareLocation,
                    uploadedResource.first);
                return false;
            }            
        }
        else
        {
            vector<wstring> filesInDir = Directory::Find(info.Location, L"*", UINT_MAX, true);
            for (auto const & fileInDir : filesInDir)
            {
                wstring fileName = fileInDir;
                StringUtility::Replace<wstring>(fileName, info.Location, L"");

                StringUtility::Trim<wstring>(fileName, L"\\");
                
                wstring storeRelativePath = Path::Combine(uploadedResource.first, fileName);
                if(!ValidateFile(storeRelativePath, fileInDir, storeShareLocation, metadataList))
                {
                    TestSession::WriteWarning(
                        TraceSource, 
                        "ValidateFile for directory failed. LocalFilePath:{0}, StoreShareLocation:{1}, StoreRelativePath:{2}",
                        fileInDir,
                        storeShareLocation,
                        storeRelativePath);
                    return false;
                }     
            }
        }        
    }

    return true;
}

bool NativeImageStoreExecutor::ValidateFile(wstring const & storeRelativePath, wstring const & localFilePath, wstring const & storeShareLocation, vector<StoreFileInfo> const & metadataList)
{
    wstring normalizedStoreRelativePath = storeRelativePath;
    StringUtility::Replace<wstring>(normalizedStoreRelativePath, L"/", L"\\");
    StringUtility::TrimSpaces(normalizedStoreRelativePath);    
    StringUtility::TrimLeading<wstring>(normalizedStoreRelativePath, L"\\");
    StringUtility::TrimTrailing<wstring>(normalizedStoreRelativePath, L"\\");
    StringUtility::ToLower(normalizedStoreRelativePath);

    for (auto const & metadata : metadataList)
    {
        if(StringUtility::AreEqualCaseInsensitive(metadata.StoreRelativePath, normalizedStoreRelativePath))
        {
            wstring storeFilePath = Utility::GetVersionedFileFullPath(storeShareLocation, metadata.StoreRelativePath, metadata.Version);
            return IsFileContentEqual(localFilePath, storeFilePath);
        }
    }

    TestSession::WriteWarning(
        TraceSource, 
        "StoreRelativePath:{0} is not found in the MetadataList",
        storeRelativePath);
    return false;
}

bool NativeImageStoreExecutor::CreateTestFile(StringCollection const & params)
{
    wstring resourceName = params[0];

    CommandLineParser parser(params, 1);
    wstring sizeInString;
    parser.TryGetString(L"size", sizeInString, L"Small");
    Size fileSize = FromString(sizeInString);
    bool shouldGenerateLargeFile = ShouldGenerateLargeFile(fileSize);

    wstring fileName = File::GetTempFileNameW(FabricTestDispatcher::TestDataDirectory);

    if(!GenerateFile(fileName, shouldGenerateLargeFile))
    {
        return false;
    }

    createdResourcesMap_.insert(
        make_pair(
        resourceName,
        make_shared<ResourceInfo>(resourceName, fileName, true)));

    return true;
}

bool NativeImageStoreExecutor::CreateTestDirectory(StringCollection const & params)
{
    wstring resourceName = params[0];
    auto iter = createdResourcesMap_.find(resourceName);
    if(iter != createdResourcesMap_.end())
    {
        TestSession::WriteError(TraceSource, "Resource {0} is already created.", resourceName);
        return false;
    }

    CommandLineParser parser(params, 1);
    int fileCount;
    parser.TryGetInt(L"filecount", fileCount, 10);

    wstring sizeInString;
    parser.TryGetString(L"size", sizeInString, L"Small");
    Size fileSize = FromString(sizeInString);    

    wstring dirPath = File::GetTempFileNameW(FabricTestDispatcher::TestDataDirectory);
    for(int i=0; i<fileCount; i++)
    {
        int level = rand_.Next(3);
        wstring fileDirPath = dirPath;
        while(level-- != 0)
        {
            fileDirPath = File::GetTempFileNameW(fileDirPath);
        }

        wstring fileName = File::GetTempFileNameW(fileDirPath);

        bool shouldGenerateLargeFile = ShouldGenerateLargeFile(fileSize);
        if(!GenerateFile(fileName, shouldGenerateLargeFile))
        {
            return false;
        }
    }

    createdResourcesMap_.insert(
        make_pair(
        resourceName,
        make_shared<ResourceInfo>(resourceName, dirPath, false)));

    return true;
}

bool NativeImageStoreExecutor::GenerateFile(std::wstring const & fileName, bool shouldGenerateLargeFile)
{
    if(!Directory::Exists(Path::GetDirectoryName(fileName)))
    {
        auto error = Directory::Create2(Path::GetDirectoryName(fileName));
        if(!error.IsSuccess())
        {
            TestSession::WriteError(TraceSource, "CreateDirectory of '{0}' failed. Error={1}.", Path::GetDirectoryName(fileName), error);
            return false;
        }
    }

    Random rand;
    int fileSize;
    if(shouldGenerateLargeFile)
    {
        fileSize = rand.Next(MinLargeFileSizeInBytes, MaxLargeFileSizeInBytes);
    }
    else
    {
        fileSize = rand.Next(MinSmallFileSizeInBytes, MaxSmallFileSizeInBytes);
    }

    vector<BYTE> fileContent(fileSize);

	rand.NextBytes(fileContent);

    File f;
    auto error = f.TryOpen(fileName, FileMode::Create, FileAccess::Write, FileShare::None);
    if (!error.IsSuccess())
    {
        TestSession::WriteError(TraceSource, "TryOpen of '{0}' failed. Error={1}.", fileName, error);
        return false;
    }

    f.Write(fileContent.data(), (int) fileContent.size());
    f.Close();

    return true;
}

bool NativeImageStoreExecutor::IsContentEqual(std::wstring const & path1, std::wstring const & path2)
{
    if(File::Exists(path1) && File::Exists(path2))
    {
        return IsFileContentEqual(path1, path2);
    }

    if(!Directory::Exists(path1) || !Directory::Exists(path2))
    {
        TestSession::WriteWarning(TraceSource, "Path1:{0}, Path2:{2} are not equal. One of the directories is not found.", path1, path2);
        return false;
    }

    vector<wstring> path1Files = Directory::Find(path1, L"*", UINT_MAX, true);
    for (auto const & path1File : path1Files)
    {
        wstring fileName = path1File;
        StringUtility::Replace<wstring>(fileName, path1, L"");

        wstring path2File = Path::Combine(path2, fileName);
        if(!IsFileContentEqual(path1File, path2File))
        {
            return false;
        }
    }

    return true;
}

bool NativeImageStoreExecutor::IsFileContentEqual(std::wstring const & filePath1, std::wstring const & filePath2)
{
    if(!File::Exists(filePath1))
    {
        TestSession::WriteWarning(TraceSource, "File1:{0} does not exist", filePath1);
        return false;
    }

    if(!File::Exists(filePath2))
    {
        TestSession::WriteWarning(TraceSource, "File2:{0} does not exist", filePath2);
        return false;
    }

    vector<byte> fileContent1;
    auto error = TryReadFile(filePath1, fileContent1);
    if(!error.IsSuccess()) { return false; }

    vector<byte> fileContent2;
    error = TryReadFile(filePath1, fileContent2);
    if(!error.IsSuccess()) { return false; }

    if(fileContent1.size() != fileContent2.size())
    {
        TestSession::WriteError(TraceSource, "File1 size:{0} does not match File2 size:{1}", fileContent1.size(), fileContent2.size());
        return false;
    }

    return true;
}

ErrorCode NativeImageStoreExecutor::TryReadFile(std::wstring const & fileName, __out vector<byte> & fileContent)
{
    File file;
    auto error = file.TryOpen(fileName);
    if(!error.IsSuccess())
    {
        TestSession::WriteWarning(TraceSource, "File:{0} could not be opened for read. Error:{1}", fileName, error);
        return error;
    }
    
    int fileSize = static_cast<int>(file.size());
    vector<byte> buffer(fileSize);
    file.TryRead(reinterpret_cast<void*>(buffer.data()), fileSize);

    fileContent = buffer;

    return ErrorCodeValue::Success;
}
