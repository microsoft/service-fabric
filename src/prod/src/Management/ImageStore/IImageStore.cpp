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

StringLiteral const ImageStoreLiteral("ImageStore");

double const IImageStore::CacheLockAcquireTimeoutInSeconds = 20;
double const IImageStore::CopyOperationLockAcquireTimeoutInSeconds = 2;
wstring const IImageStore::CacheTempDirectory(L"tmp");
wstring const IImageStore::CacheStagingExtension(L".staging");

GlobalWString IImageStore::ArchiveMarkerFileName = make_global<wstring>(L"_.zip");

IImageStore::IImageStore(
    wstring const& localRoot, 
    wstring const& cacheRoot)
    :localRoot_(localRoot),
    cacheRoot_(cacheRoot)
{
    WriteNoise(
        ImageStoreLiteral,
        "IImageStore constructor called. CacheRoot:{0}, LocalRoot:{1}",        
        cacheRoot_,
        localRoot_);
}

std::wstring IImageStore::GetCacheFileName(std::wstring const & tag)
{
    ASSERT_IF(cacheRoot_.empty() , "GetCacheFileName should not be called when cache is not used");
    return Path::Combine(cacheRoot_, tag);
}

wstring IImageStore::GetLocalFileNameForUpload(wstring const & tag)
{    
    return localRoot_.empty() ? tag : Path::Combine(localRoot_, tag);
}

wstring IImageStore::GetLocalFileNameForDownload(wstring const & tag)
{    
    if(!cacheRoot_.empty())
    {
        return Path::Combine(cacheRoot_, tag);
    }
    else if(!localRoot_.empty())
    {
        return Path::Combine(localRoot_, tag);
    }
    else
    {
        return tag;
    }    
}

ErrorCode IImageStore::RemoveRemoteContent(wstring const & object)
{
    vector<wstring> objects;
    objects.push_back(object);
    return RemoveRemoteContent(objects);
}

ErrorCode IImageStore::RemoveRemoteContent(vector<wstring> const & objects)
{
    if (cacheRoot_ != L"")
    {
        vector<wstring> cacheTags;
        for (size_t i = 0; i < objects.size(); i++)
        {
            cacheTags.push_back(GetCacheFileName(objects[i]));
        }
        
        DeleteObjects(cacheTags, ImageStoreServiceConfig::GetConfig().ClientDefaultTimeout).ReadValue();
    }

    return OnRemoveObjectFromStore(objects, ImageStoreServiceConfig::GetConfig().ClientDefaultTimeout);
}

ErrorCode IImageStore::DeleteObjects(vector<wstring> const & tags, Common::TimeSpan const timeout)
{
    TimeoutHelper timeoutHelper(timeout);
    for (size_t i = 0; i < tags.size(); i++)
    {
        if (timeoutHelper.GetRemainingTime() <= Common::TimeSpan::Zero)
        {
            return ErrorCodeValue::Timeout;
        }

        wstring const & fileName = tags[i];
        if (Directory::Exists(fileName))
        {
            ErrorCode err = Directory::Delete(fileName, true);
            if (!err.IsSuccess())
            {
                return err;
            }
        }
        else if(File::Exists(fileName))
        {
            if (!File::Delete(fileName, NOTHROW()))
            {
                return ErrorCode::FromHResult(HRESULT_FROM_WIN32(::GetLastError()));
            }
        }
    }
    return ErrorCodeValue::Success;
}

ErrorCode IImageStore::DownloadToCache(
    std::wstring const & remoteObject,
    bool shouldOverwrite,
    bool checkForArchive,
    bool isStaging,
    Common::TimeSpan const timeout,
    __out std::wstring & cacheObject,
    __out bool & alreadyInCache)
{
    TimeoutHelper timeoutHelper(timeout);
    cacheObject = GetCacheFileName(remoteObject);
    auto path = Path::GetDirectoryName(cacheObject);
    auto remoteArchive = ImageModelUtility::GetSubPackageArchiveFileName(remoteObject);
    auto cacheArchive = ImageModelUtility::GetSubPackageArchiveFileName(cacheObject);

    ASSERT_IF(path.empty(), "ImageStore.DownloadToCache: cacheObject path cannot be empty");

    ErrorCode error;
    if(!Directory::Exists(path))
    {
        error = Directory::Create2(path);
        if (!error.IsSuccess())
        {
            WriteWarning(
                ImageStoreLiteral,
                "Directory::Create2 failed for path {0} with {1}",
                path,
                error);
            return error;
        }
    }

    if (timeoutHelper.GetRemainingTime() > Common::TimeSpan::Zero)
    {
        ExclusiveFile file(cacheObject + L".CacheLock");
        error = file.Acquire(TimeSpan::FromSeconds(CacheLockAcquireTimeoutInSeconds));

        if (!error.IsSuccess() )
        {
            //IImageStore returns Timeout irrespective of the error. To preserve original functionality we return Timeout.
            if (!error.IsError(ErrorCodeValue::SharingAccessLockViolation))
            {
                return ErrorCodeValue::Timeout;
            }
            return error;
        }

        bool cachedObjectExists = false;
        {
            FileReaderLock readerLock(cacheObject);
            error = readerLock.Acquire();
            if(error.IsSuccess())
            {
                alreadyInCache = cachedObjectExists = (Directory::Exists(cacheObject) || File::Exists(cacheObject));

                if (!alreadyInCache && checkForArchive && File::Exists(cacheArchive))
                {
                    alreadyInCache = true;
                    cachedObjectExists = true;
                    cacheObject = cacheArchive;
                }
            }
            else if(error.IsError(ErrorCodeValue::CorruptedImageStoreObjectFound))
            {
                // If there is an abandoned writer lock, we delete both lock and 
                // the content associated with the lock
                error = RemoveCorruptedContent(cacheObject, timeoutHelper);
                if (!error.IsSuccess()) { return error; }
            }
            else
            {
                return error;
            }
        }

        if (shouldOverwrite || !cachedObjectExists)
        {
            WriteNoise(
                ImageStoreLiteral,
                "Cache object '{0}' does not exist so downloading",
                cacheObject);

            vector<wstring> storeTagsToDownload;
            vector<wstring> cacheTagsToUpdate;
            vector<CopyFlag::Enum> copyFlags;
            copyFlags.push_back(CopyFlag::Overwrite);

            if (checkForArchive)
            {
                bool archiveExists;
                error = DoesArchiveExistInStore(remoteArchive, timeoutHelper.GetRemainingTime(), archiveExists);
                if (!error.IsSuccess())
                {
                    return error;
                }

                if (archiveExists)
                {
                    cacheObject = cacheArchive;
                    storeTagsToDownload.push_back(remoteArchive);
                    cacheTagsToUpdate.push_back(remoteArchive);
                }
            }

            if (storeTagsToDownload.empty())
            {
                storeTagsToDownload.push_back(remoteObject);
            }

            if (cacheTagsToUpdate.empty())
            {
                // GetLocalFileNameForDownload will prefix with the root directory if applicable 
                cacheTagsToUpdate.push_back(isStaging ? remoteObject + CacheStagingExtension : remoteObject);
            }

            error = OnDownloadFromStore(storeTagsToDownload, cacheTagsToUpdate, copyFlags, timeoutHelper.GetRemainingTime());

            if (!error.IsSuccess())
            {
                WriteWarning(
                    ImageStoreLiteral,
                    "OnDownloadFromStore for cached objects failed with error {0}",
                    error);
                return error;
            }
        }
    }
    else
    {
        // File download is in progress. Complete with timeout 
        return ErrorCodeValue::Timeout;
    }

    return ErrorCodeValue::Success;
}

ErrorCode IImageStore::RemoveCorruptedContent(
    std::wstring const & cacheObject,
    Common::TimeoutHelper const & timeoutHelper)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);

    FileWriterLock writerLock(cacheObject);
    error = writerLock.Acquire();
    if (!error.IsSuccess()) { return error; }

    vector<wstring> cacheObjectsToDelete;
    cacheObjectsToDelete.push_back(cacheObject);

    error = DeleteObjects(cacheObjectsToDelete, timeoutHelper.GetRemainingTime());

    return error;
}

Common::ErrorCode IImageStore::DownloadContent(
    std::wstring const & remoteObject,
    std::wstring const & localObject,
    Common::TimeSpan const timeout,
    std::wstring const & remoteChecksumObject,
    std::wstring const & expectedChecksumValue,
    bool refreshCache,
    CopyFlag::Enum copyFlag,
    bool copyToLocalStoreLayoutOnly,
    bool checkForArchive)
{
    Trace.WriteNoise(
        ImageStoreLiteral,
        "DownloadContent called. RemoteObject={0}, LocalObject={1}, RemoteChecksumObject={2}, ExpectedChecksumValue={3}",
        remoteObject,
        localObject,
        remoteChecksumObject,
        expectedChecksumValue);

    ASSERT_IF(
        !expectedChecksumValue.empty() && remoteChecksumObject.empty(),
        "The ExpectedChecksumValue is {0}, but the RemoteChecksumObject is empty.",
        expectedChecksumValue);

    TimeoutHelper timeoutHelper(timeout);
    vector<wstring> remoteObjects;
    vector<wstring> localObjects;
    vector<shared_ptr<vector<wstring>>> skipFiles;
    vector<CopyFlag::Enum> flags;
    vector<BOOLEAN> checkMarkFiles;

    remoteObjects.push_back(remoteObject);
    localObjects.push_back(localObject);
    skipFiles.push_back(nullptr);
    flags.push_back(copyFlag);
    checkMarkFiles.push_back(false);

    bool shouldOverwriteCache = false;
    if (cacheRoot_.empty())
    {
        if (checkForArchive)
        {
            auto remoteArchive = ImageModelUtility::GetSubPackageArchiveFileName(remoteObject);
            auto localArchive = ImageModelUtility::GetSubPackageArchiveFileName(localObject);

            bool archiveExists;
            auto error = DoesArchiveExistInStore(remoteArchive, timeoutHelper.GetRemainingTime(), archiveExists);
            if (!error.IsSuccess())
            {
                return error;
            }

            if (archiveExists)
            {
                remoteObjects.clear();
                remoteObjects.push_back(remoteArchive);
                localObjects.clear();
                localObjects.push_back(localArchive);
            }
        }

        auto error = OnDownloadFromStore(remoteObjects, localObjects, flags, timeoutHelper.GetRemainingTime());

        if (checkForArchive && error.IsSuccess())
        { 
            auto downloadedObject = *localObjects.begin();
            if (ImageModelUtility::IsArchiveFile(downloadedObject))
            {
                error = ExtractArchive(downloadedObject, localObject, false); // overwrite

                if (error.IsSuccess())
                {
                    error = File::Delete2(downloadedObject);
                }
            }
        }

        return error;
    }

    wstring cacheChecksumObject;
    if (!expectedChecksumValue.empty())
    {  
        bool alreadyInCache = false;

        auto error = DownloadToCache(
            remoteChecksumObject,
            false, // overwrite
            false, // checkForArchive
            true, // isStaging
            timeoutHelper.GetRemainingTime(),
            cacheChecksumObject,
            alreadyInCache);
        if (!error.IsSuccess()) { return error; }

        wstring actualCheckSumValue;
        error = ReadChecksumFile(alreadyInCache ? cacheChecksumObject : cacheChecksumObject + CacheStagingExtension, actualCheckSumValue);
        if (!error.IsSuccess()) { return error; }

        if (StringUtility::AreEqualCaseInsensitive(expectedChecksumValue, actualCheckSumValue))
        {
            if (!alreadyInCache)
            {
                // The checksum file is not there in the cache, but is present in the store.
                // In this case, overwrite the cache object with the store object
                shouldOverwriteCache = true;
            }
            else
            {
                cacheChecksumObject.clear();
            }
        }
        else
        {
            if (alreadyInCache)
            {
                // The checksum present in the cache does not match the expected checksum value.
                // This indicates that the object in the cache might be stale. Overwriting the checksum
                // file in the cache with the checksum file in the store
                cacheChecksumObject.clear();
                error = DownloadToCache(
                    remoteChecksumObject,
                    true, // overwrite
                    false, // checkForArchive
                    true, //isStaging
                    timeoutHelper.GetRemainingTime(),
                    cacheChecksumObject,
                    alreadyInCache);
                if (!error.IsSuccess()) { return error; }

                actualCheckSumValue.clear();
                error = ReadChecksumFile(cacheChecksumObject + CacheStagingExtension, actualCheckSumValue);
                if (!error.IsSuccess()) { return error; }

                if (StringUtility::AreEqualCaseInsensitive(expectedChecksumValue, actualCheckSumValue))
                {
                    // The checksum value got from store matches the expected checksum. The cache object is  
                    // stale. Overwrite the cache object with the store object.
                    shouldOverwriteCache = true;
                }
                else
                {
                    // The checksum value got from the store does not match the expected checksum. This can only be 
                    // possible if the app has been unprovisioned and re-provisioned while download/activation 
                    // is going on.
                    Trace.WriteWarning(
                        ImageStoreLiteral,
                        "The store checksum does not match the expected checksum. StoreCheckSum:{0}, ExpectedChecksum: {1}",
                        actualCheckSumValue,
                        expectedChecksumValue);

                    return ErrorCode(ErrorCodeValue::InvalidOperation, StringResource::Get(IDS_HOSTING_AppDownloadChecksum));
                }
            }
            else
            {
                // The checksum value got from the store does not match the expected checksum. This can only be 
                // possible if the app has been unprovisioned and re-provisioned while download/activation 
                // is going on.
                Trace.WriteWarning(
                    ImageStoreLiteral,
                    "The store checksum does not match the expected checksum. StoreCheckSum:{0}, ExpectedChecksum: {1}",
                    actualCheckSumValue,
                    expectedChecksumValue);

                return ErrorCode(ErrorCodeValue::InvalidOperation, StringResource::Get(IDS_HOSTING_AppDownloadChecksum));
            }
        }
    }

    if (!shouldOverwriteCache) { shouldOverwriteCache = refreshCache; }

    if (timeoutHelper.GetRemainingTime() <= Common::TimeSpan::Zero) { return ErrorCodeValue::Timeout; }

    wstring cacheObject;
    bool alreadyInCache = false;

    auto error = DownloadToCache(
        remoteObject, 
        shouldOverwriteCache,
        checkForArchive,
        false,
        timeoutHelper.GetRemainingTime(), 
        cacheObject,
        alreadyInCache);

    if (!error.IsSuccess())
    {
        // If DownloadToCache fails and shouldOverwriteCache is true,
        // then delete the CacheObject. The CacheObject is deleted because
        // shouldOverwriteCache indicates that it is stale. The checksum file
        // is up to date, but the content is not. If we do not delete, then
        // the next retry would NOT overwrite the CacheObject because the
        // checksum value would match
        if (shouldOverwriteCache)
        {
            ErrorCode deleteError = ErrorCodeValue::Success;
            deleteError = this->RemoveCorruptedContent(cacheObject, timeoutHelper);

            Trace.WriteTrace(
                deleteError.ToLogLevel(),
                ImageStoreLiteral,
                "Deleting CacheObject:{0}. Error:{1}",
                cacheObject,
                deleteError);
        }

        return error;
    }
    else if (shouldOverwriteCache && !cacheChecksumObject.empty())
    {
        //If DownloadToCache succeeds and ShouldOverWriteCache is true,
        //then the current outdated checksum file will be overwritten by the new staging checksum file.
        //If the copy operation fails, then download the checksum file again directly to non-staging location.
        error = File::Copy(cacheChecksumObject + CacheStagingExtension, cacheChecksumObject, true);
        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                ImageStoreLiteral,
                "Copy staging CacheChecksumObject:{0}. Error:{1}",
                cacheChecksumObject,
                error);

            cacheChecksumObject.clear();
            error = DownloadToCache(
                remoteChecksumObject,
                true, // overwrite
                false, // checkForArchive
                false, //isStaging
                timeoutHelper.GetRemainingTime(),
                cacheChecksumObject,
                alreadyInCache);
            if (!error.IsSuccess()) { return error; }
        }

        error = File::Delete2(cacheChecksumObject + CacheStagingExtension, true);
        if (!error.IsSuccess())
        {
            WriteTrace(
                error.ToLogLevel(),
                ImageStoreLiteral,
                "Delete staging CacheChecksumObject:{0}. Error:{1}",
                cacheChecksumObject,
                error);
        }
    }

    if (copyToLocalStoreLayoutOnly)
    {
        return error;
    }

    if (checkForArchive && ImageModelUtility::IsArchiveFile(cacheObject))
    {
        error = ExtractArchive(cacheObject, localObject, shouldOverwriteCache);
    }
    else
    {
        vector<wstring> cacheObjects;
        cacheObjects.push_back(cacheObject);

        // Copy objects from cache to local target
        error = CopyObjects(cacheObjects, localObjects, skipFiles, flags, checkMarkFiles, timeoutHelper.GetRemainingTime());
        if (!error.IsSuccess())
        {
            WriteWarning(
                ImageStoreLiteral,
                "CopyObjects from cache to local target failed with error {0}",
                error);
        }
    }

    return error;
}

ErrorCode IImageStore::DownloadContent(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    Common::TimeSpan const timeout,
    vector<CopyFlag::Enum> const & flags)
{    
    ASSERT_IFNOT(
        remoteObjects.size() == localObjects.size() && localObjects.size() == flags.size(),
        "There should be a one to one mapping between remoteObjects, localObjects anf flags");

    TimeoutHelper timeouthelper(timeout);
    auto error = ErrorCode::Success();

    if(remoteObjects.size() == 0)
    {
        WriteNoise(ImageStoreLiteral, "DownloadContent has nothing to download. Returning success");
        return error;
    }

    for(size_t i=0; i < remoteObjects.size(); i++)
    {        
        error = DownloadContent(
            remoteObjects[i],
            localObjects[i],
            timeouthelper.GetRemainingTime(),
            L"",
            L"",
            false,
            flags[i],
            false);
        if(!error.IsSuccess()) { return error; }
    }

    return error;
}

ErrorCode IImageStore::DownloadContent(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    Common::TimeSpan const timeout,
    CopyFlag::Enum flag)
{
    vector<CopyFlag::Enum> flags;
    for (size_t i = 0; i < remoteObjects.size(); i++)
    {
        flags.push_back(flag);
    }

    return DownloadContent(remoteObjects, localObjects, timeout, flags);
}                                                                                                               

ErrorCode IImageStore::DownloadContent(
    wstring const & remoteObject,
    wstring const & localObject,
    Common::TimeSpan const timeout,
    CopyFlag::Enum flag)
{
    Trace.WriteNoise(ImageStoreLiteral, "DownloadContent called");
    return DownloadContent(
        remoteObject,
        localObject,
        timeout,
        L"",
        L"",
        false,
        flag,
        false);
}

ErrorCode IImageStore::UploadContent(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    Common::TimeSpan const timeout,
    vector<CopyFlag::Enum> const & flags)
{
    Trace.WriteNoise(ImageStoreLiteral, "UploadContent called");
    ASSERT_IFNOT(
        remoteObjects.size() == localObjects.size() && localObjects.size() == flags.size(),
        "There should be a one to one mapping between remoteObjects, localObjects and flags");

    if(remoteObjects.size() == 0)
    {
        WriteNoise(ImageStoreLiteral, "UploadContent has nothing to upload. Returning success");
        return ErrorCode::Success();
    }

    return OnUploadToStore(
        remoteObjects,
        localObjects,
        flags,
        timeout);
}

ErrorCode IImageStore::UploadContent(
    wstring const & remoteObject,
    wstring const & localObject,
    Common::TimeSpan const timeout,
    CopyFlag::Enum flag)
{
    Trace.WriteNoise(ImageStoreLiteral, "UploadContent called");
    vector<wstring> remoteObjects, localObjects;
    vector<CopyFlag::Enum> flags;
    remoteObjects.push_back(remoteObject);
    localObjects.push_back(localObject);
    flags.push_back(flag);

    return OnUploadToStore(remoteObjects, localObjects, flags, timeout);
}

ErrorCode IImageStore::UploadContent(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    Common::TimeSpan const timeout,
    CopyFlag::Enum flag)
{
    vector<CopyFlag::Enum> flags;
    for (size_t i = 0; i < remoteObjects.size(); i++)
    {
        flags.push_back(flag);
    }

    return UploadContent(remoteObjects, localObjects, timeout, flags);
}

ErrorCode IImageStore::DoesContentExistInCache(
    std::wstring const & object,
    __out bool & objectExists)
{
    objectExists = false;

    if (cacheRoot_.empty())
    {
        return ErrorCodeValue::Success;
    }

    wstring cacheObject = GetCacheFileName(object);
    wstring cacheDirectory = Path::GetDirectoryName(cacheObject);
    if (!Directory::Exists(cacheDirectory))
    {
        return ErrorCodeValue::Success;
    }

    ExclusiveFile file(cacheObject + L".CacheLock");
    auto error = file.Acquire(TimeSpan::FromSeconds(CacheLockAcquireTimeoutInSeconds));

    if (!error.IsSuccess())
    {
        // File download is in progress. Complete with timeout
        Trace.WriteWarning(ImageStoreLiteral, "DoesContentExistInCache failed with error {0} for cache object {1}.", error, cacheObject);
        //IImageStore returns Timeout irrespective of the error. To preserve original functionality we return Timeout.
        if (!error.IsError(ErrorCodeValue::SharingAccessLockViolation))
        {
            return ErrorCodeValue::Timeout;
        }
        return error;
    }

    objectExists = File::Exists(cacheObject) || Directory::Exists(cacheObject);

    return error;
}

ErrorCode IImageStore::DoesContentExist(
    std::vector<std::wstring> const & remoteObjects, 
    Common::TimeSpan const timeout,
    __out std::map<std::wstring, bool> & objectExistsMap)
{
    return OnDoesContentExistInStore(remoteObjects, timeout, objectExistsMap);
}

ErrorCode IImageStore::RemoveFromCache(wstring const & remoteObject)
{
    if(cacheRoot_.empty())
    {
        return ErrorCodeValue::Success;
    }

    wstring cacheObject = GetCacheFileName(remoteObject);
    
    ExclusiveFile file(cacheObject + L".CacheLock");
    auto error = file.Acquire(TimeSpan::FromSeconds(CacheLockAcquireTimeoutInSeconds));

    if (error.IsSuccess())
    {
        FileWriterLock writerLock(cacheObject);    
        error = writerLock.Acquire();
        if(error.IsSuccess())
        {
            if(File::Exists(cacheObject))
            {
                error = File::Delete2(cacheObject);
            }
            else if(Directory::Exists(cacheObject))
            {
                error = Directory::Delete(cacheObject, true);
            }
        }
    }
    else
    {
        //IImageStore returns Timeout irrespective of the error. To preserve original functionality we return Timeout.
        if (!error.IsError(ErrorCodeValue::SharingAccessLockViolation))
        {
            return ErrorCodeValue::Timeout;
        }
    }

    return error;
}

ErrorCode IImageStore::CopyObjects(
    vector<wstring> const & sourceTags,
    vector<wstring> const & destinationTags,
    vector<shared_ptr<vector<wstring>>> const & skipFiles,
    vector<CopyFlag::Enum> const & flags, 
    vector<BOOLEAN> const & checkMarkFiles,
    Common::TimeSpan const timeout)
{
    ASSERT_IFNOT(
        sourceTags.size() == destinationTags.size() && destinationTags.size() == flags.size(),
        "There should be a one to one mapping between local tags and cache tags and flags");

    TimeoutHelper timeoutHelper(timeout);
    for (size_t i = 0; i < sourceTags.size(); i++)
    {
        if (timeoutHelper.GetRemainingTime() <= Common::TimeSpan::Zero)
        {
            return ErrorCodeValue::Timeout;
        }

        bool checkSkipFiles = skipFiles[i] != nullptr && !skipFiles[i]->empty();
        WriteNoise(ImageStoreLiteral, "SourceTag: {0}, DestinationTag: {1}, CheckSkipFiles: {2}", sourceTags[i], destinationTags[i], checkSkipFiles);
        ErrorCode error;
        wstring const & sourceTag = sourceTags[i];
        wstring const & destinationTag = destinationTags[i];

        enum CopyType
        {
            FILE,
            FOLDER
        };
        
        auto copyOperation = [&destinationTag, &flags](wstring sourceTag, CopyType type, size_t ix) -> ErrorCode {
            switch (flags[ix])
            {
            case CopyFlag::Overwrite:
                WriteNoise(ImageStoreLiteral, "{0}::SafeCopy for {1} to {2}", type == CopyType::FILE ? "File" : "Directory", sourceTag, destinationTag);
                return type == CopyType::FILE ?
                    File::SafeCopy(sourceTag, destinationTag, true, true, TimeSpan::FromSeconds(CopyOperationLockAcquireTimeoutInSeconds)) :
                    Directory::SafeCopy(sourceTag, destinationTag, true, TimeSpan::FromSeconds(CopyOperationLockAcquireTimeoutInSeconds));
                break;
            case CopyFlag::Echo:
                WriteNoise(ImageStoreLiteral, "{0}::Echo for {1} to {2}", type == CopyType::FILE ? "File" : "Directory", sourceTag, destinationTag);
                return type == CopyType::FILE ?
                    File::Echo(sourceTag, destinationTag, TimeSpan::FromSeconds(CopyOperationLockAcquireTimeoutInSeconds)) :
                    Directory::Echo(sourceTag, destinationTag, TimeSpan::FromSeconds(CopyOperationLockAcquireTimeoutInSeconds));
                break;
            default:
                Assert::CodingError("Unknown copy flag {0}", flags[ix]);
            }
        };

        bool isDirectory = Directory::Exists(sourceTag);
        if (isDirectory)
        {
            std::vector<std::wstring> files = Directory::GetFiles(sourceTag);

            if (checkMarkFiles[i] && !markFile_.empty())
            {
                wstring const & makeFile = markFile_;
                auto makeFileExists = [&makeFile](wstring sourceTag) -> bool{
                    return sourceTag.rfind(makeFile, sourceTag.length()) != string::npos;
                };

                if (count_if(files.begin(), files.end(), makeFileExists) == 0)
                {
                    Trace.WriteError(ImageStoreLiteral, "Failed because mark file {0} does not exist", makeFile);
                    return ErrorCodeValue::FileNotFound;
                }
            }

            if (checkSkipFiles)
            {
                std::set<wstring> fileSet(files.begin(), files.end());
                std::set<wstring> skipFileSet(skipFiles[i]->begin(), skipFiles[i]->end());
                std::vector<wstring> difference;

                std::string builder = "SkipFiles: ";
                for (std::set<wstring>::iterator file = skipFileSet.begin(); file != skipFileSet.end(); ++file){
                    wstring sFile = *file;
                    string tFile(sFile.begin(), sFile.end());
                    builder.append(tFile);
                    builder.append("\n");
                };
                
                Trace.WriteNoise(ImageStoreLiteral, "SkipFiles", builder);

                std::set_difference(fileSet.begin(), fileSet.end(), skipFileSet.begin(), skipFileSet.end(), std::inserter(difference, difference.end()));
                Trace.WriteNoise(ImageStoreLiteral, "FilesCount: {0}, Difference: {1}", files.size(), difference.size());

                if (difference.size() < files.size())
                {
                    for (wstring& file : files) {
                        error = copyOperation(file, CopyType::FILE, i);
                        if (!error.IsSuccess())
                        {
                            break;
                        }
                    }
                }
                else
                {
                    error = copyOperation(sourceTag, CopyType::FOLDER, i);
                }
            }
            else
            {
                error = copyOperation(sourceTag, CopyType::FOLDER, i);
            }
        }
        else
        {
            error = copyOperation(sourceTag, CopyType::FILE, i);
        }
        
        if (!error.IsSuccess())
        {
            Trace.WriteWarning(ImageStoreLiteral, "Copy failed with error {0}", error);
            return ReplaceErrorCode(error);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode IImageStore::ReadChecksumFile(std::wstring const & fileName, __inout std::wstring & checksumValue)
{
    wstring textW;
    try
    {
        File fileReader; 
        auto error = fileReader.TryOpen(fileName, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
            Trace.WriteError("IImageStore.ReadChecksumFile", fileName, "Failed to open '{0}': error={1}", fileName, error);
            return ErrorCodeValue::FileNotFound;
        }

        int64 fileSize = fileReader.size();
        size_t size = static_cast<size_t>(fileSize);

        string text;
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();

        StringWriter(checksumValue).Write(text);

        StringUtility::TrimWhitespaces(checksumValue);
    }
    catch (std::exception const& e)
    {
        Trace.WriteError("IImageStore.ReadChecksumFile", fileName, "Failed with error {0}", e.what());
        return ErrorCodeValue::InvalidOperation;
    }

    return ErrorCodeValue::Success;
}

ErrorCode IImageStore::GetStoreChecksumValue(wstring const & remoteChecksumFile, __inout wstring & checksum)
{
    wstring tempDir = Path::Combine(cacheRoot_, IImageStore::CacheTempDirectory);
    ErrorCode error;
    if (!Directory::Exists(tempDir))
    {
        error = Directory::Create2(tempDir);
        if (!error.IsSuccess())
        {
            WriteWarning(
                ImageStoreLiteral,
                "Directory::Create2 failed for path {0} with {1}",
                tempDir,
                error);
            return error;
        }
    }
    wstring tempChecksumFile = Path::Combine(IImageStore::CacheTempDirectory, wformatString("{0}.{1}", Guid::NewGuid().ToString(), L"chksm"));
    vector<wstring> remoteObjects;
    vector<wstring> localObjects;
    vector<CopyFlag::Enum> copyFlags;

    remoteObjects.push_back(remoteChecksumFile);
    localObjects.push_back(tempChecksumFile);
    copyFlags.push_back(CopyFlag::Overwrite);

    error = OnDownloadFromStore(remoteObjects, localObjects, copyFlags, ImageStoreServiceConfig::GetConfig().ClientDownloadTimeout);
    WriteTrace(
        error.ToLogLevel(),
        ImageStoreLiteral,
        "Failed to read dowload file from location {0} to location {1}, error {2}",
        remoteChecksumFile,
        tempChecksumFile,
        error);
    if (error.IsSuccess())
    {
        error = ReadChecksumFile(Path::Combine(cacheRoot_, tempChecksumFile), checksum);
        WriteTrace(
            error.ToLogLevel(),
            ImageStoreLiteral,
            "Failed to read checksum file at location {0}, error {1}",
            tempChecksumFile,
            error);
    }
    if (File::Exists(tempChecksumFile))
    {
        auto err = File::Delete2(tempChecksumFile);
        WriteWarning(
            ImageStoreLiteral,
            "File::Delete2 failed for path {0} with {1}",
            tempChecksumFile,
            err);
    }
    return error;
}


ErrorCode IImageStore::ReplaceErrorCode(ErrorCode const & errorCode)
{
    if (errorCode.IsError(ErrorCode::FromWin32Error(ERROR_FILE_NOT_FOUND).ReadValue()) ||
        errorCode.IsError(ErrorCode::FromWin32Error(ERROR_PATH_NOT_FOUND).ReadValue()))
    {
        return ErrorCodeValue::FileNotFound;
    }

    return errorCode;
}

ErrorCode IImageStore::DoesArchiveExistInStore(wstring const & archive, TimeSpan const timeout, __out bool & result)
{
    result = false;

    vector<wstring> remoteArchives;
    remoteArchives.push_back(archive);

    map<wstring, bool> results;
    auto error = OnDoesContentExistInStore(remoteArchives, timeout, results);
    if (!error.IsSuccess())
    {
        WriteWarning(
            ImageStoreLiteral,
            "DoesArchiveExistInStore for {0} failed with error {1}",
            archive,
            error);
        return error;
    }

    result = !results.empty() && results.begin()->second;

    return error;
}

ErrorCode IImageStore::ExtractArchive(wstring const & src, wstring const & dest, bool overwrite)
{
    auto root = Path::GetDirectoryName(dest);
    if (!Directory::Exists(root))
    {
        auto error = Directory::Create2(root);
        if (!error.IsSuccess()) 
        { 
            WriteInfo(ImageStoreLiteral, "ExtractArchive: failed to initialize root directory {0} error={1}", root, error);
            return error; 
        }
    }

    FileWriterLock destLock(dest);

    auto error = destLock.Acquire(TimeSpan::FromSeconds(CacheLockAcquireTimeoutInSeconds));
    if (!error.IsSuccess()) 
    { 
        WriteInfo(ImageStoreLiteral, "ExtractArchive: failed to acquire file lock {0} error={1}", dest, error);
        return error; 
    }

    auto markerFile = Path::Combine(dest, *ArchiveMarkerFileName);
    if (Directory::Exists(dest))
    {
        if (File::Exists(markerFile) && !overwrite)
        {
            return ErrorCodeValue::Success;
        }

        WriteInfo(ImageStoreLiteral, "ExtractArchive: delete {0}", dest);

        error = Directory::Delete(dest, true); // recursive
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    WriteInfo(ImageStoreLiteral, "ExtractArchive: {0} to {1} overwrite={2}", src, dest, overwrite);

    error = Directory::ExtractArchive(src, dest);

    if (error.IsSuccess())
    {
        error = File::Touch(Path::Combine(dest, *ArchiveMarkerFileName));
    }

    return error;
}
