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
using namespace ImageCache;
using namespace ServiceModel;
using namespace Api;
using namespace Client;
using namespace Query;

StringLiteral const Trace_DeletionManager("DeletionManager");

class DeletionManager::CleanupAppTypeFoldersAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CleanupAppTypeFoldersAsyncOperation)

public:
    CleanupAppTypeFoldersAsyncOperation(
        __in DeletionManager & owner,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        pruneContainerImages_(HostingConfig::GetConfig().PruneContainerImages),
        imageCacheLayout_(owner.hosting_.ImageCacheFolder),
        imageMap_(),
        imageMapLock_()
    {        
    }

    virtual ~CleanupAppTypeFoldersAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupAppTypeFoldersAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ProcessExistingContent(thisSPtr);   
    }

private:
    void ProcessExistingContent(AsyncOperationSPtr const & thisSPtr)
    {        
        // Build a map of AppType to AppIds
        AppTypeInstanceMap appTypeInstanceMap;
        vector<wstring> appInstanceFolders = Directory::GetSubDirectories(owner_.hosting_.DeploymentFolder);        
        for(auto iter = appInstanceFolders.begin(); iter != appInstanceFolders.end(); ++iter)
        {
            ApplicationIdentifier appId;
            auto error = ApplicationIdentifier::FromString(*iter, appId);
            if(!error.IsSuccess()) { continue; }

            if (appId == *ApplicationIdentifier::FabricSystemAppId) { continue; }

            appTypeInstanceMap[appId.ApplicationTypeName].insert(appId);
        }

        appTypeInstanceMap_ = move(appTypeInstanceMap);

        checkpointTime_ = DateTime::Now();

        // Build a map of AppType to content in ImageCache, Shared folder and Application instance folder
        ProcessImageCacheFolder();
        ProcessSharedFolder();
        ProcessApplicationsFolder(appTypeInstanceMap_);

        pendingAppTypeCount_.store(appTypesList_.size());

        for(auto iter = appTypesList_.begin(); iter != appTypesList_.end(); ++iter)
        {
            // Query CM for the provisioned content for each of the
            // AppTypes found on the node
            QueryProvisionedAppTypes(thisSPtr, *iter);
        }

        CheckPendingOperations(thisSPtr, appTypesList_.size());
    }

    void QueryProvisionedAppTypes(AsyncOperationSPtr const & thisSPtr, wstring const & appTypeName)
    {        
        QueryArgumentMap argsMap;
        argsMap.Insert(QueryResourceProperties::ApplicationType::ApplicationTypeName, appTypeName);

        WriteNoise(
            Trace_DeletionManager,
            owner_.Root.TraceId,
            "Begin(ProvisionedAppTypeQuery)");
        auto operation = owner_.hosting_.QueryClient->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetProvisionedApplicationTypePackages),
            argsMap,
            HostingConfig::GetConfig().RequestTimeout,
            [this, appTypeName](AsyncOperationSPtr const& operation) { this->OnGetProvisionedApplicationType(operation, appTypeName); },
            thisSPtr);       
    }

    void OnGetProvisionedApplicationType(AsyncOperationSPtr const & operation, wstring const & appTypeName)
    {        
        QueryResult result;
        wstring queryObStr;
        auto error = owner_.hosting_.QueryClient->EndInternalQuery(operation, result);
        if(!error.IsSuccess())
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "End(ProvisionedAppTypeQuery): ErrorCode={0}",
                error);
            TryComplete(operation->Parent, error);
            return;
        }        

        InternalProvisionedApplicationTypeQueryResult queryResult;
        error = result.MoveItem(queryResult);
        if(!error.IsSuccess())
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "Failed to deserialize ApplicationTypeDependencyResult: ErrorCode={0}",
                error);
            uint64 pendingAppTypeCount = --pendingAppTypeCount_;
            CheckPendingOperations(operation->Parent, pendingAppTypeCount);
            return;
        }

        CleanupAppTypeFolder(operation->Parent, appTypeName, queryResult);
    }

    void CheckPendingOperations(AsyncOperationSPtr const & thisSPtr, uint64 pendingAppTypeCount)
    {
        if (pendingAppTypeCount == 0)
        {
            if (HostingConfig::GetConfig().PruneContainerImages)
            {
                RemoveContainerImages(thisSPtr);
            }
            else
            {
                TryComplete(thisSPtr, ErrorCodeValue::Success);
            }
        }
    }

    void RemoveContainerImages(AsyncOperationSPtr const & thisSPtr)
    {
        vector<wstring> images;
        {
            AcquireReadLock lock(imageMapLock_);
            for (auto it = imageMap_.begin(); it != imageMap_.end(); ++it)
            {
                bool skip = false;
                for (auto iter = owner_.containerImagesToSkip_.begin(); iter != owner_.containerImagesToSkip_.end(); ++iter)
                {
                    if (StringUtility::Contains(it->first, *iter))
                    {
                        skip = true;
                        break;
                    }
                }
                if (!skip)
                {
                    images.push_back(it->first);
                }
            }
        }
        if (!images.empty())
        {
           auto operation = owner_.hosting_.FabricActivatorClientObj->BeginDeleteContainerImages(
                images,
                HostingConfig::GetConfig().RequestTimeout,
                [this](AsyncOperationSPtr const & operation) { this->OnRemoveContainerImagesCompleted(operation, false); },
                thisSPtr);
            OnRemoveContainerImagesCompleted(operation, true);
        }
        else
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

    void OnRemoveContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.hosting_.FabricActivatorClientObj->EndDeleteContainerImages(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DeletionManager,
            owner_.Root.TraceId,
            "Failed to remove container images: ErrorCode={0}",
            error);
        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }

    void CleanupAppTypeFolder(AsyncOperationSPtr const & thisSPtr, wstring const & appTypeName, InternalProvisionedApplicationTypeQueryResult const & dependencyMap)
    {
        // There should be NO write operation on any of the maps after this since there can be multiple threads in parallel

        set<ApplicationIdentifier> appIds;
        auto iter = appTypeInstanceMap_.find(appTypeName);
        if(iter != appTypeInstanceMap_.end()) { appIds = iter->second; }

        ContentSet imageCacheChecksumContent = GetContentSetFromMap(imageCacheChecksumContent_, appTypeName);
        ContentSet imageCacheContent = GetContentSetFromMap(imageCacheContent_, appTypeName);
        ContentSet appInstanceContent = GetContentSetFromMap(appInstanceContent_, appTypeName);
        ContentSet sharedContent = GetContentSetFromMap(sharedContent_, appTypeName);
        
        RemoveUsedCodePackages(appTypeName, dependencyMap.CodePackages, appIds, imageCacheChecksumContent, imageCacheContent, appInstanceContent, sharedContent);
        RemoveUsedConfigPackages(appTypeName, dependencyMap.ConfigPackages, appIds, imageCacheChecksumContent, imageCacheContent, appInstanceContent, sharedContent);
        RemoveUsedDataPackages(appTypeName, dependencyMap.DataPackages, appIds, imageCacheChecksumContent, imageCacheContent, appInstanceContent, sharedContent);
        RemoveUsedServiceManifestFiles(appTypeName, dependencyMap.ServiceManifests, appIds, imageCacheChecksumContent, imageCacheContent, appInstanceContent);

        // important that checksums are deleted first because they indicate integrity of cache
        // if checksum can't be deleted, then the deletion should be aborted
        for (auto const & content : imageCacheChecksumContent)
        {
            auto error = DeleteIfNotModifiedSinceCheckpoint(appTypeName, content, true /* shouldAcquireCacheLock */);
            if (!error.IsSuccess())
            {
                WriteError(
                    Trace_DeletionManager,
                    owner_.Root.TraceId,
                    "Failed to delete checksum file '{0}'. Aborting deletion of AppType: '{1}'. Error: {2}",
                    content,
                    appTypeName,
                    error);
                
                uint64 pendingAppTypeCount = --pendingAppTypeCount_;
                CheckPendingOperations(thisSPtr, pendingAppTypeCount);
                return;
            }
        }
        
        for (auto const & content : imageCacheContent)
        {
            DeleteIfNotModifiedSinceCheckpoint(appTypeName, content, true /* shouldAcquireCacheLock */).ReadValue();
        }

        for (auto const & content : sharedContent)
        {
            DeleteIfNotModifiedSinceCheckpoint(appTypeName, content, false /* shouldAcquireCacheLock */).ReadValue();
        }

        for (auto const & content : appInstanceContent)
        {
            DeleteIfNotModifiedSinceCheckpoint(appTypeName, content, false /* shouldAcquireCacheLock */).ReadValue();
        }

        uint64 pendingAppTypeCount = --pendingAppTypeCount_;
        CheckPendingOperations(thisSPtr, pendingAppTypeCount);
    }

    ContentSet GetContentSetFromMap(AppTypeContentMap const & contentMap, wstring const & appTypeName)
    {
        auto iter = contentMap.find(appTypeName);
        return iter == contentMap.end() ? ContentSet() : iter->second;
    }

    void ConstructContainerImageMap(wstring const & folder)
    {
        vector<wstring> serviceManifestFiles = Directory::GetFiles(folder, L"*.xml", true /*fullPath*/, true /*topDirOnly*/);
        for (auto const & serviceManifestFile : serviceManifestFiles)
        {
            bool result;
            auto error = Parser::IsServiceManifestFile(serviceManifestFile, result);
            if (!error.IsSuccess())
            {
                continue;
            }
            if (result)
            {
                ServiceManifestDescription description;
                error = Parser::ParseServiceManifest(serviceManifestFile, description);
                if (error.IsSuccess())
                {
                    for (auto it = description.CodePackages.begin(); it != description.CodePackages.end(); ++it)
                    {
                        auto containerDescription = it->EntryPoint.ContainerEntryPoint;
                        if (!containerDescription.ImageName.empty())
                        {
                            AcquireWriteLock lock(imageMapLock_);
                            imageMap_[containerDescription.ImageName].insert(serviceManifestFile);
                        }
                    }
                }
            }
        }
    }

    void RemoveUsedServiceManifestFiles(
        wstring const & appTypeName, 
        vector<wstring> const & usedServiceManifests,
        set<ApplicationIdentifier> const & appIds,
        ContentSet & imageCacheChecksumContent,
        ContentSet & imageCacheContent,
        ContentSet & appInstanceContent)
    {
        for (auto const & usedServiceManifest : usedServiceManifests)
        {
            wstring serviceManifestName, serviceManifestVersion;
            auto error = ParseManifestId(usedServiceManifest, serviceManifestName, serviceManifestVersion);
            if(!error.IsSuccess())
            {
                continue;
            }

            wstring imageCacheFile = imageCacheLayout_.GetServiceManifestFile(
                appTypeName,
                serviceManifestName,
                serviceManifestVersion);
            imageCacheContent.erase(imageCacheFile);

            wstring imageCacheChecksumFile = imageCacheLayout_.GetServiceManifestChecksumFile(
                appTypeName,
                serviceManifestName,
                serviceManifestVersion);
            imageCacheChecksumContent.erase(imageCacheChecksumFile);

            //Check if the file is present before Parsing
            if (File::Exists(imageCacheFile))
            {
                ServiceManifestDescription description;
                error = Parser::ParseServiceManifest(imageCacheFile, description);
                if (error.IsSuccess())
                {
                    for (auto it = description.CodePackages.begin(); it != description.CodePackages.end(); ++it)
                    {
                        auto containerDescription = it->EntryPoint.ContainerEntryPoint;
                        if (!containerDescription.ImageName.empty())
                        {
                            AcquireWriteLock lock(imageMapLock_);
                            auto itImage = imageMap_.find(containerDescription.ImageName);
                            if (itImage != imageMap_.end())
                            {
                                imageMap_.erase(itImage);
                            }
                        }
                    }
                }
            }
            else
            {
                WriteNoise(
                    Trace_DeletionManager,
                    owner_.Root.TraceId,
                    "RemoveUsedServiceManifestFiles: File does not exist {0}",
                    imageCacheFile);
            }

            vector<wstring> applicationInstanceFiles;
            for (auto const & appId : appIds)
            {
                wstring path = owner_.hosting_.RunLayout.GetServiceManifestFile(
                    appId.ToString(),
                    serviceManifestName,
                    serviceManifestVersion);
                appInstanceContent.erase(path);
            }
        }
    }

    void RemoveUsedCodePackages(
        wstring const & appTypeName, 
        vector<wstring> const & usedCodePackages, 
        set<ApplicationIdentifier> const & appIds,
        ContentSet & imageCacheChecksumContent,
        ContentSet & imageCacheContent,
        ContentSet & appInstanceContent,
        ContentSet & sharedContent)
    {
        for (auto const & usedCodePackage : usedCodePackages)
        {
            wstring serviceManifestName, packageName, packageVersion;
            auto error = ParsePackageId(usedCodePackage, serviceManifestName, packageName, packageVersion);
            if(!error.IsSuccess())
            {
                continue;
            }

            wstring imageCacheFolder = imageCacheLayout_.GetCodePackageFolder(
                appTypeName, 
                serviceManifestName, 
                packageName, 
                packageVersion);
            imageCacheContent.erase(imageCacheFolder);
            imageCacheContent.erase(ImageModelUtility::GetSubPackageArchiveFileName(imageCacheFolder));

            wstring imageCacheChecksumFile = imageCacheLayout_.GetCodePackageChecksumFile(
                appTypeName, 
                serviceManifestName, 
                packageName, 
                packageVersion);            
            imageCacheChecksumContent.erase(imageCacheChecksumFile);

            wstring sharedFolder = owner_.hosting_.SharedLayout.GetCodePackageFolder(
                appTypeName,
                serviceManifestName,
                packageName,
                packageVersion);
            sharedContent.erase(sharedFolder);
            sharedContent.erase(ImageModelUtility::GetSubPackageArchiveFileName(sharedFolder));

            vector<wstring> applicationInstanceFolders;
            for (auto const & appId : appIds)
            {
                wstring path = owner_.hosting_.RunLayout.GetCodePackageFolder(
                    appId.ToString(),
                    serviceManifestName,
                    packageName,
                    packageVersion,
                    false);
                appInstanceContent.erase(path);

                wstring symbolicLinkPath = owner_.hosting_.RunLayout.GetCodePackageFolder(
                    appId.ToString(),
                    serviceManifestName,
                    packageName,
                    packageVersion,
                    true);
                appInstanceContent.erase(symbolicLinkPath);
            }
        }
    }

    void RemoveUsedConfigPackages(
        wstring const & appTypeName, 
        vector<wstring> const & usedConfigPackages, 
        set<ApplicationIdentifier> const & appIds, 
        ContentSet & imageCacheChecksumContent,
        ContentSet & imageCacheContent,
        ContentSet & appInstanceContent,
        ContentSet & sharedContent)
    {
        for (auto const & usedConfigPackage : usedConfigPackages)
        {
            wstring serviceManifestName, packageName, packageVersion;
            auto error = ParsePackageId(usedConfigPackage, serviceManifestName, packageName, packageVersion);
            if(!error.IsSuccess())
            {
                continue;
            }

            wstring imageCacheFolder = imageCacheLayout_.GetConfigPackageFolder(
                appTypeName, 
                serviceManifestName, 
                packageName, 
                packageVersion);
            imageCacheContent.erase(imageCacheFolder);
            imageCacheContent.erase(ImageModelUtility::GetSubPackageArchiveFileName(imageCacheFolder));

            wstring imageCacheChecksumFile = imageCacheLayout_.GetConfigPackageChecksumFile(
                appTypeName, 
                serviceManifestName, 
                packageName, 
                packageVersion);            
            imageCacheChecksumContent.erase(imageCacheChecksumFile);

            wstring sharedFolder = owner_.hosting_.SharedLayout.GetConfigPackageFolder(
                appTypeName,
                serviceManifestName,
                packageName,
                packageVersion);
            sharedContent.erase(sharedFolder);
            sharedContent.erase(ImageModelUtility::GetSubPackageArchiveFileName(sharedFolder));

            vector<wstring> applicationInstanceFolders;
            for (auto const & appId : appIds)
            {
                wstring path = owner_.hosting_.RunLayout.GetConfigPackageFolder(
                    appId.ToString(),
                    serviceManifestName,
                    packageName,
                    packageVersion,
                    false);
                appInstanceContent.erase(path);

                wstring symbolicLinkPath = owner_.hosting_.RunLayout.GetConfigPackageFolder(
                    appId.ToString(),
                    serviceManifestName,
                    packageName,
                    packageVersion,
                    true);
                appInstanceContent.erase(symbolicLinkPath);
            }
        }
    }

    void RemoveUsedDataPackages(
        wstring const & appTypeName, 
        vector<wstring> const & usedDataPackages, 
        set<ApplicationIdentifier> const & appIds,
        ContentSet & imageCacheChecksumContent,
        ContentSet & imageCacheContent,
        ContentSet & appInstanceContent,
        ContentSet & sharedContent)
    {
        for (auto const & usedDataPackage : usedDataPackages)
        {
            wstring serviceManifestName, packageName, packageVersion;
            auto error = ParsePackageId(usedDataPackage, serviceManifestName, packageName, packageVersion);
            if(!error.IsSuccess())
            {
                continue;
            }

            wstring imageCacheFolder = imageCacheLayout_.GetDataPackageFolder(
                appTypeName, 
                serviceManifestName, 
                packageName, 
                packageVersion);
            imageCacheContent.erase(imageCacheFolder);
            imageCacheContent.erase(ImageModelUtility::GetSubPackageArchiveFileName(imageCacheFolder));

            wstring imageCacheChecksumFile = imageCacheLayout_.GetDataPackageChecksumFile(
                appTypeName, 
                serviceManifestName, 
                packageName, 
                packageVersion);            
            imageCacheChecksumContent.erase(imageCacheChecksumFile);

            wstring sharedFolder = owner_.hosting_.SharedLayout.GetDataPackageFolder(
                appTypeName,
                serviceManifestName,
                packageName,
                packageVersion);
            sharedContent.erase(sharedFolder);
            sharedContent.erase(ImageModelUtility::GetSubPackageArchiveFileName(sharedFolder));

            vector<wstring> applicationInstanceFolders;
            for (auto const & appId : appIds)
            {
                wstring path = owner_.hosting_.RunLayout.GetDataPackageFolder(
                    appId.ToString(),
                    serviceManifestName,
                    packageName,
                    packageVersion,
                    false);
                appInstanceContent.erase(path);

                wstring symbolicLinkPath = owner_.hosting_.RunLayout.GetDataPackageFolder(
                    appId.ToString(),
                    serviceManifestName,
                    packageName,
                    packageVersion,
                    true);
                appInstanceContent.erase(symbolicLinkPath);
            }
        }
    }

    ErrorCode ParsePackageId(wstring const & packageId, __out wstring & serviceManifestName, __out wstring & packageName, __out wstring & packageVersion)
    {
        vector<wstring> tokens;
        StringUtility::Split<wstring>(packageId, tokens, L":", true);

        if(tokens.size() < 3)
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "Failed to parse Id {0}",
                packageId);
            return ErrorCodeValue::InvalidOperation;
        }

        serviceManifestName = tokens[0];
        packageName = tokens[1];
        packageVersion = tokens[2];

        return ErrorCodeValue::Success;
    }

    ErrorCode ParseManifestId(wstring const & packageId, __out wstring & serviceManifestName, __out wstring & serviceManifestVersion)
    {
        vector<wstring> tokens;
        StringUtility::Split<wstring>(packageId, tokens, L":", true);

        if(tokens.size() < 2)
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "Failed to parse Id {0}",
                packageId);
            return ErrorCodeValue::InvalidOperation;
        }

        serviceManifestName = tokens[0];
        serviceManifestVersion = tokens[1];

        return ErrorCodeValue::Success;
    }    

    ErrorCode DeleteIfNotModifiedSinceCheckpoint(wstring const & appTypeName, wstring const & path, bool const shouldAcquireCacheLock)
    {
        ErrorCode error;

        ExclusiveFile cacheLock(path + L".CacheLock");
        if(shouldAcquireCacheLock)
        {
            error = cacheLock.Acquire(TimeSpan::Zero);
            if(!error.IsSuccess())
            {
                return error;
            }
        }

        FileWriterLock writerLock(path);
        error = writerLock.Acquire();
        if(!error.IsSuccess())
        {
            return error;
        }

        if(File::Exists(path))
        {
            DateTime lastWriteTime;
            error = File::GetLastWriteTime(path, lastWriteTime);
            if(!error.IsSuccess())
            {
                return error;
            }

            if(lastWriteTime <= checkpointTime_)
            {
                error = File::Delete2(path, true /*deleteReadOnlyFiles*/);
                if(error.IsSuccess())
                {
                    hostingTrace.ApplicationTypeContentDeletionSuccess(owner_.Root.TraceId, appTypeName, path);
                }
                else
                {
                    hostingTrace.ApplicationTypeContentDeletionFailed(owner_.Root.TraceId, appTypeName, path, error);
                    return error;
                }
            }
        }
        else if(Directory::Exists(path))
        {
            DateTime lastWriteTime;
            error = Directory::GetLastWriteTime(path, lastWriteTime);
            if(!error.IsSuccess())
            {
                return error;
            }

            if(lastWriteTime <= checkpointTime_)
            {
                // important to delete the marker file first because it represents integrity of an extracted zip
                // if it can't be deleted, then maintain integrity by aborting
                wstring markerFilePath = Path::Combine(path, Management::ImageCache::Constants::ArchiveMarkerFileName);
                if(File::Exists(markerFilePath))
                {
                    error = File::Delete2(markerFilePath, true /* deleteReadOnly */);
                    if(error.IsSuccess())
                    {
                        hostingTrace.ApplicationTypeContentDeletionSuccess(owner_.Root.TraceId, appTypeName, markerFilePath);
                    }
                    else
                    {
                        hostingTrace.ApplicationTypeContentDeletionFailed(owner_.Root.TraceId, appTypeName, markerFilePath, error);
                        return error;
                    }
                }

                error = Directory::Delete(path, true /*recursive*/, true /*deleteReadOnlyFiles*/);
                if(error.IsSuccess())
                {
                    hostingTrace.ApplicationTypeContentDeletionSuccess(owner_.Root.TraceId, appTypeName, path);
                }
                else
                {
                    hostingTrace.ApplicationTypeContentDeletionFailed(owner_.Root.TraceId, appTypeName, path, error);
                    return error;
                }
            }
        }

        return ErrorCode::Success();
    }

    void ProcessApplicationsFolder(AppTypeInstanceMap const & appTypeInstanceMap)
    {
        AppTypeContentMap appTypeContentMap;
        for (auto const & appType : appTypeInstanceMap)
        {
            ContentSet contents;
            for (auto const & appId : appType.second)
            {
                wstring appInstanceFolder = Path::Combine(owner_.hosting_.DeploymentFolder, appId.ToString());
                GetContent(appInstanceFolder, contents);
            }

            if(contents.size() > 0)
            {                
                appInstanceContent_.insert(make_pair(appType.first /*appTypeName*/, move(contents)));
                appTypesList_.insert(appType.first /*appTypeName*/);
            }            
        }
    }

    void ProcessSharedFolder()
    {
        wstring sharedFolder = Path::Combine(owner_.hosting_.DeploymentFolder, Constants::SharedFolderName);
        wstring sharedStoreFolder = Path::Combine(sharedFolder, L"Store");

        vector<wstring> appTypeFolders = Directory::GetSubDirectories(sharedStoreFolder, L"*", false, true);
        for (auto const & appTypeFolder : appTypeFolders)
        {                
            wstring appTypeName = appTypeFolder;

            ContentSet contents;
            GetContent(Path::Combine(sharedStoreFolder, appTypeFolder), contents);
            if(contents.size() > 0)
            {                
                sharedContent_.insert(make_pair(appTypeName, move(contents)));
                appTypesList_.insert(appTypeName);
            }            
        }
    }

    void ProcessImageCacheFolder()
    {        
        if(owner_.hosting_.ImageCacheFolder.empty())
        {
            return; // ImageCache is disabled
        }

        wstring imageCacheStoreFolder = Path::Combine(owner_.hosting_.ImageCacheFolder, L"Store");

        AppTypeContentMap imageCacheContent;
        vector<wstring> appTypeImageCacheFolders = Directory::GetSubDirectories(imageCacheStoreFolder, L"*", false /*fullPath*/, true /*topDirOnly*/);
        for (auto const & appTypeFolder : appTypeImageCacheFolders)
        {                
            wstring appTypeName = appTypeFolder;
            wstring imageCacheAppTypeFolder = Path::Combine(imageCacheStoreFolder, appTypeFolder);

            ContentSet contents;
            GetContent(imageCacheAppTypeFolder, contents);
            if(contents.size() > 0)
            {                
                imageCacheContent_.insert(make_pair(appTypeName, move(contents)));
                appTypesList_.insert(appTypeName);
            }

            ContentSet checksumContents;
            GetChecksumContent(imageCacheAppTypeFolder, checksumContents);
            if(checksumContents.size() > 0)
            {                
                imageCacheChecksumContent_.insert(make_pair(appTypeName, move(checksumContents)));
                appTypesList_.insert(appTypeName);
            }

            if (pruneContainerImages_)
            {
                ConstructContainerImageMap(Path::Combine(imageCacheStoreFolder, appTypeFolder));
            }
        }
    }

    // Important that we don't add checksums here. They're a special case and need to be put in their own ContentSet.
    static void GetContent(wstring const & folder, __out ContentSet & contents)
    {
        vector<wstring> serviceManifestFiles = Directory::GetFiles(folder, L"*.xml", true /*fullPath*/, true /*topDirOnly*/);
        for (auto const & serviceManifestFile : serviceManifestFiles)
        {
            if (!File::Exists(serviceManifestFile))
            {
                // We need this because for Package.Current.xml VersionedServicePackage removes the file too.
                // By the time we go through the list, the file maybe deleted.
                continue;
            }

            bool result;
            auto error = Parser::IsServiceManifestFile(serviceManifestFile, result);
            if(!error.IsSuccess())
            {
                continue;
            }
            if(result)
            {
                contents.insert(serviceManifestFile);
            }
        }

        auto archiveFiles = ImageModelUtility::GetArchiveFiles(folder, true /*fullPath*/, true /*topDirOnly*/);
        for (auto const & archiveFile : archiveFiles)
        {
            contents.insert(archiveFile);
        }

        vector<wstring> packageFolders = Directory::GetSubDirectories(folder, L"*", false /*fullPath*/, true /*topDirOnly*/);
        for (auto const & packageFolder : packageFolders)
        {
            if(StringUtility::AreEqualCaseInsensitive(packageFolder, L"apps") ||
                StringUtility::AreEqualCaseInsensitive(packageFolder, L"work") ||
                StringUtility::AreEqualCaseInsensitive(packageFolder, L"temp") ||
                StringUtility::AreEqualCaseInsensitive(packageFolder, L"log") ||
				StringUtility::AreEqualCaseInsensitive(packageFolder, L"settings") ||
                StringUtility::EndsWith<wstring>(packageFolder, L".new") ||
                StringUtility::EndsWith<wstring>(packageFolder, L".isstmp")) {
                continue;
            }
            contents.insert(Path::Combine(folder, packageFolder));
        }        
    }

    static void GetChecksumContent(wstring const & folder, __out ContentSet & contents)
    {
        vector<wstring> checksumFiles = Directory::GetFiles(folder, L"*.checksum", true /*fullPath*/, true /*topDirOnly*/);
        for (auto const & checksumFile : checksumFiles)
        {
            contents.insert(checksumFile);
        }
    }

    
private:
    set<wstring> appTypesList_;    
    AppTypeInstanceMap appTypeInstanceMap_;
    AppTypeContentMap imageCacheChecksumContent_;    
    AppTypeContentMap imageCacheContent_;    
    AppTypeContentMap sharedContent_;    
    AppTypeContentMap appInstanceContent_;       
    AppTypeContentMap imageMap_;
    bool pruneContainerImages_;
    Common::RwLock imageMapLock_;
    atomic_uint64 pendingAppTypeCount_;
    DateTime checkpointTime_;
    StoreLayoutSpecification imageCacheLayout_;
    DeletionManager & owner_;
};

class DeletionManager::CleanupAppInstanceFoldersAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CleanupAppInstanceFoldersAsyncOperation)

public:
    CleanupAppInstanceFoldersAsyncOperation(
        __in DeletionManager & owner,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner)
    {        
    }

    virtual ~CleanupAppInstanceFoldersAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupAppInstanceFoldersAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & thisSPtr)
    {        
        vector<wstring> appInstanceFolders = Directory::GetSubDirectories(owner_.hosting_.DeploymentFolder);
        
        vector<wstring> appIds;
        for(auto iter = appInstanceFolders.begin(); iter != appInstanceFolders.end(); ++iter)
        {
            ApplicationIdentifier appId;
            auto error = ApplicationIdentifier::FromString(*iter, appId);
            if(!error.IsSuccess()) { continue; }

            if (appId == *ApplicationIdentifier::FabricSystemAppId) { continue; }

            bool contains;
            error = owner_.hosting_.ApplicationManagerObj->Contains(appId, contains);
            if (!error.IsSuccess())  { continue; }

            if(!contains)
            {
                appIds.push_back(*iter);           
            }        
        }

        if(!appIds.empty())
        {
            QueryDeletedApplicationInstances(thisSPtr, move(appIds));
        }
        else
        {
            WriteNoise(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "CleanupAppInstanceFoldersAsyncOperation: No ApplicationInstance folder found.");
            TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }
    }

private:
    void QueryDeletedApplicationInstances(AsyncOperationSPtr const & thisSPtr, vector<wstring> && appIds)
    {
        InternalDeletedApplicationsQueryObject queryObj(appIds);
        QueryArgumentMap argsMap;
        wstring queryStr;
        auto error = JsonHelper::Serialize<InternalDeletedApplicationsQueryObject>(queryObj, queryStr);
        if(!error.IsSuccess())
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "Failed to Serialize InternalDeletedApplicationsQueryObject: ErrorCode={0}",
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        argsMap.Insert(L"ApplicationIds", queryStr);

        WriteNoise(
            Trace_DeletionManager,
            owner_.Root.TraceId,
            "Begin(GetDeletedApplicationsListQuery)");
        auto operation = owner_.hosting_.QueryClient->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeletedApplicationsList),
            argsMap,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const& operation) { this->OnGetDeletedApplicationsListCompleted(operation); },
            thisSPtr);
    }

    void OnGetDeletedApplicationsListCompleted(AsyncOperationSPtr const & operation)
    {
        QueryResult result;
        wstring queryObStr;
        auto error = owner_.hosting_.QueryClient->EndInternalQuery(operation, result);
        if(!error.IsSuccess())
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "End(GetDeletedApplicationsListQuery): ErrorCode={0}",
                error);
            TryComplete(operation->Parent, error);
            return;
        }
        
        InternalDeletedApplicationsQueryObject deletedApps;
        error = result.MoveItem(deletedApps);

        if(!error.IsSuccess())
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "Failed to Deserialize DeletedApplicationsQueryObject: ErrorCode={0}",
                error);
            TryComplete(operation->Parent, error);
            return;
        }

      
        if (deletedApps.ApplicationIds.empty())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        DeleteLocalGroupsForAppInstances(deletedApps.ApplicationIds, operation->Parent);
    }

    void DeleteLocalGroupsForAppInstances(vector<wstring> const & applicationIds, AsyncOperationSPtr thisSPtr)
    {
        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginCleanupApplicationSecurityGroup(
            applicationIds,
            HostingConfig::GetConfig().RequestTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnCleanupSecurityPrincipalsCompleted(operation, false); },
            thisSPtr);
        OnCleanupSecurityPrincipalsCompleted(operation, true);
    }

    void OnCleanupSecurityPrincipalsCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        vector<wstring> deletedAppIds;
        auto error = owner_.hosting_.FabricActivatorClientObj->EndCleanupApplicationSecurityGroup(operation, deletedAppIds);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DeletionManager,
            owner_.Root.TraceId,
            "End CleanupSecurityPrincipals: error {0}",
            error);
        DeleteApplicationInstanceFolders(operation->Parent, deletedAppIds);
    }

#if !defined(PLATFORM_UNIX)
    void DeleteApplicationInstanceFolders(AsyncOperationSPtr thisSPtr, vector<wstring> const & deletedAppIds)
    {
        auto const& config = owner_.hosting_.FabricNodeConfigObj;

        vector<wstring> appFolders;

        auto nodeId = owner_.hosting_.NodeId;
        for (auto iter = deletedAppIds.begin(); iter != deletedAppIds.end(); ++iter)
        {
            wstring appInstanceFolder = Path::Combine(owner_.hosting_.DeploymentFolder, *iter);
            if (Directory::Exists(appInstanceFolder))
            {
                GetRootSymbolicLinkDirectories(*iter, nodeId, config, appFolders);

                appFolders.push_back(appInstanceFolder);
            }

            ApplicationIdentifier appId;
            auto error = ApplicationIdentifier::FromString(*iter, appId);
            TESTASSERT_IFNOT(error.IsSuccess(), "{0} should be valid ApplicationIdentifier", *iter);
            if (!error.IsSuccess())
            {
                continue;
            }

            wstring appInstanceImageCacheFolder = Path::Combine(owner_.hosting_.ImageCacheFolder, L"Store");
            appInstanceImageCacheFolder = Path::Combine(appInstanceImageCacheFolder, appId.ApplicationTypeName);
            appInstanceImageCacheFolder = Path::Combine(appInstanceImageCacheFolder, L"apps");
            appInstanceImageCacheFolder = Path::Combine(appInstanceImageCacheFolder, appId.ToString());
            if (Directory::Exists(appInstanceImageCacheFolder))
            {
                error = Directory::Delete(appInstanceImageCacheFolder, true, true/*deleteReadOnlyFiles*/);
                WriteTrace(
                    error.ToLogLevel(),
                    Trace_DeletionManager,
                    owner_.Root.TraceId,
                    "Failed to delete folder {0}, error {1}, proceeding with other folders",
                    appInstanceImageCacheFolder,
                    error);
            }
        }

        for (auto iter = appFolders.begin(); iter != appFolders.end(); ++iter)
        {
            auto error = Directory::Delete(*iter, true, true /*deleteReadOnlyFiles*/);
            if (error.IsSuccess())
            {
                hostingTrace.ApplicationInstanceFolderDeletionSuccess(owner_.Root.TraceId, *iter);
            }
            else
            {
                hostingTrace.ApplicationInstanceFolderDeletionFailed(owner_.Root.TraceId, *iter, error);
            }
        }

        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
#else
    void DeleteApplicationInstanceFolders(AsyncOperationSPtr thisSPtr, vector<wstring> const & deletedAppIds)
    {
        Common::FabricNodeConfig const& config = owner_.hosting_.FabricNodeConfigObj;

        vector<wstring> appFolders;

        auto nodeId = owner_.hosting_.NodeId;

        for (auto iter = deletedAppIds.begin(); iter != deletedAppIds.end(); ++iter) {

            wstring appInstanceFolder = Path::Combine(owner_.hosting_.DeploymentFolder, *iter);
            if (Directory::Exists(appInstanceFolder)) {
            
                GetRootSymbolicLinkDirectories(*iter, nodeId, config, appFolders);

                appFolders.push_back(appInstanceFolder);

            }

            ApplicationIdentifier appId;
            auto error = ApplicationIdentifier::FromString(*iter, appId);
            TESTASSERT_IFNOT(error.IsSuccess(), "{0} should be valid ApplicationIdentifier", *iter);
            if (!error.IsSuccess()) {
                continue;
            }

            wstring appInstanceImageCacheFolder = Path::Combine(owner_.hosting_.ImageCacheFolder, L"Store");
            appInstanceImageCacheFolder = Path::Combine(appInstanceImageCacheFolder, appId.ApplicationTypeName);
            appInstanceImageCacheFolder = Path::Combine(appInstanceImageCacheFolder, L"apps");
            appInstanceImageCacheFolder = Path::Combine(appInstanceImageCacheFolder, appId.ToString());
            if (Directory::Exists(appInstanceImageCacheFolder)) {
                appFolders.push_back(appInstanceImageCacheFolder);
            }
        }
        auto operation = owner_.hosting_.FabricActivatorClientObj->BeginDeleteApplicationFolder(
                appFolders,
                HostingConfig::GetConfig().RequestTimeout,
                [this](AsyncOperationSPtr const & operation) { this->OnDeleteFoldersCompleted(operation, false); },
                thisSPtr);
        OnDeleteFoldersCompleted(operation, true);
    }

    void OnDeleteFoldersCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        vector<wstring> deletedAppFolders;
        auto error = owner_.hosting_.FabricActivatorClientObj->EndDeleteApplicationFolder(operation, deletedAppFolders);
        WriteTrace(
                error.ToLogLevel(),
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "End DeleteApplicationFolders: error {0}",
                error);
        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }
#endif

    // Get all the app instance directories in the JBOD directory with symbolic links from the app instance work folder
    // ex: FooJBODDir:\directoryName\NodeId\%AppInstance%_App%App_Type%
    void GetRootSymbolicLinkDirectories(wstring const& appInstanceId, wstring const& nodeId, Common::FabricNodeConfig const& config, __inout vector<wstring> & appFolders)
    {
        wstring rootDeployedfolder;
        for (auto iter = config.LogicalApplicationDirectories.begin(); iter != config.LogicalApplicationDirectories.end(); ++iter)
        {
            rootDeployedfolder = Path::Combine(iter->second, nodeId);
            rootDeployedfolder = Path::Combine(rootDeployedfolder, appInstanceId);
            appFolders.push_back(rootDeployedfolder);
        }
    }

private:
    DeletionManager & owner_;
};

class DeletionManager::CleanupAsyncOperation
    : public AsyncOperation,
    protected TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(CleanupAsyncOperation)

public:
    CleanupAsyncOperation(
        __in DeletionManager & owner,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , continuousFailureCount_(0)
    {        
    }

    virtual ~CleanupAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CleanupAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.RegisterCleanupAsyncOperation(thisSPtr);
        if(!error.IsSuccess())
        {
            WriteWarning(
                Trace_DeletionManager,
                owner_.Root.TraceId,
                "RegisterCleanupAsyncOperation failed with {0}",
                error);
            TryComplete(thisSPtr, error);
            return;
        }

        CleanupApplicationInstanceFolders(thisSPtr);   
    }

    void OnCompleted()
    {
        TimerSPtr snap;
        {
            AcquireExclusiveLock lock(timerLock_);
            snap = retryTimer_;
        }

        if (snap)
        {
            snap->Cancel();
        }
    }

private:
    void CleanupApplicationInstanceFolders(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<CleanupAppInstanceFoldersAsyncOperation>(
            owner_,
            [this](AsyncOperationSPtr const& operation) { this->OnCleanupAppInstanceFolders(operation, false); },
            thisSPtr);
        this->OnCleanupAppInstanceFolders(operation, true);
    }

    void OnCleanupAppInstanceFolders(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = CleanupAppInstanceFoldersAsyncOperation::End(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DeletionManager,
            owner_.Root.TraceId,
            "End(CleanupAppInstanceFoldersAsyncOperation): Error:{0}",
            error);

        if(!error.IsSuccess())
        {
            continuousFailureCount_++;
            if(IsRetryable(error))
            {
                error = TryScheduleRetry(
                    operation->Parent, 
                    [this](AsyncOperationSPtr const & thisSPtr) { this->CleanupApplicationInstanceFolders(thisSPtr); });
                if(error.IsSuccess())
                {
                    return;
                }
            }

            CompleteCleanupAsyncOperation(operation->Parent, error);
            return;
        }

        // reset continous failure count
        continuousFailureCount_.store(0);

        CleanupAppTypeFolders(operation->Parent);
    }

    void CleanupAppTypeFolders(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<CleanupAppTypeFoldersAsyncOperation>(
            owner_,
            [this](AsyncOperationSPtr const& operation) { this->OnCleanupAppTypeFolders(operation, false); },
            thisSPtr);
        this->OnCleanupAppTypeFolders(operation, true);
    }

    void OnCleanupAppTypeFolders(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = CleanupAppTypeFoldersAsyncOperation::End(operation);
        WriteTrace(
            error.ToLogLevel(),
            Trace_DeletionManager,
            owner_.Root.TraceId,
            "End(CleanupAppTypeFoldersAsyncOperation): Error:{0}",
            error);

        if(!error.IsSuccess())
        {
            continuousFailureCount_++;
            if(IsRetryable(error))
            {
                error = TryScheduleRetry(
                    operation->Parent, 
                    [this](AsyncOperationSPtr const & thisSPtr) { this->CleanupAppTypeFolders(thisSPtr); });
                if(error.IsSuccess())
                {
                    return;
                }
            }
        }

        CompleteCleanupAsyncOperation(operation->Parent, error);
    }
    
    void CompleteCleanupAsyncOperation(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
    {        
        if(TryStartComplete())
        {
            owner_.UnregisterCleanupAsyncOperation();
            FinishComplete(thisSPtr, error);
        }
    }

    bool IsRetryable(ErrorCode const & error)
    {
        if(error.ReadValue() == ErrorCodeValue::OperationCanceled ||
            error.ReadValue() == ErrorCodeValue::ObjectClosed)
        {
            return false;
        }

        return true;
    }

    typedef function<void(AsyncOperationSPtr const &)> RetryCallback;

    ErrorCode TryScheduleRetry(AsyncOperationSPtr const & thisSPtr, RetryCallback const & callback)
    {
        TimeSpan delay = HostingConfig::GetConfig().CacheCleanupBackoffInternval;
        if (continuousFailureCount_.load() > HostingConfig::GetConfig().CacheCleanupMaxContinuousFailures)
        {
            return ErrorCodeValue::OperationFailed;
        }

        {            
            if (!this->InternalIsCompleted)
            {
                AcquireExclusiveLock lock(timerLock_);

                retryTimer_ = Timer::Create(
                    "Hosting.DeleteManagerRetry",
                    [this, thisSPtr, callback](TimerSPtr const & timer)
                    {
                        timer->Cancel();
                        callback(thisSPtr);
                    });
                retryTimer_->Change(delay);
            }
        }

        return ErrorCodeValue::Success;
    }    

private:
    DeletionManager & owner_;

    atomic_uint64 continuousFailureCount_;

    ExclusiveLock timerLock_;
    TimerSPtr retryTimer_;
};

/// ********************************************************************************************************************
// DeletionManager Implementation
//

DeletionManager::DeletionManager(
    ComponentRoot const & root, 
    __in HostingSubsystem & hosting)
    : RootedObject(root)
    , hosting_(hosting)
    , cacheCleanupScanTimer_()
    , cleanupAsyncOperation_()
    , lock_()
{
    StringUtility::Split<wstring>(HostingConfig::GetConfig().ContainerImagesToSkip, containerImagesToSkip_, L"|");
}

DeletionManager::~DeletionManager()
{
}

ErrorCode DeletionManager::OnOpen()
{
    TimeSpan interval = HostingConfig::GetConfig().CacheCleanupScanInterval;

    if(interval  < TimeSpan::MaxValue &&
        interval.TotalMilliseconds() > 0)
    {
        double randomStartTime;
        Random rand;
        int64 timeRange;

        if(interval < TimeSpan::MaxValue/2)
        {
            timeRange = interval.TotalMilliseconds();
        }
        else
        {
            timeRange = interval.TotalMilliseconds()/2;
        }
        randomStartTime = ((rand.NextDouble() *  timeRange) +  timeRange);
        this->ScheduleCleanup(TimeSpan::FromMilliseconds(randomStartTime));
    }
    else
    {
        WriteInfo(
            Trace_DeletionManager,
            Root.TraceId,
            "DeletionManager cleanup disabled because CacheCleanupScanInterval is set to {0}",
            interval);
    }
    WriteInfo(
        Trace_DeletionManager,
        Root.TraceId,
        "DeletionManager opened");
    return ErrorCodeValue::Success;
}

ErrorCode DeletionManager::OnClose()
{    
    TimerSPtr cleanupTimer;
    AsyncOperationSPtr cleanupAsyncOperation;
    {
        AcquireExclusiveLock grab(lock_);
        cleanupTimer = move(cacheCleanupScanTimer_);
        cleanupAsyncOperation = move(cleanupAsyncOperation_);
    }

    if(cleanupTimer)
    {
        cleanupTimer->Cancel();
    }

    if(cleanupAsyncOperation)
    {
        cleanupAsyncOperation->Cancel();
    }
    WriteInfo(
        Trace_DeletionManager,
        Root.TraceId,
        "DeletionManager closed");
    return ErrorCodeValue::Success;
}

void DeletionManager::OnAbort()
{    
    OnClose();
}

ErrorCode DeletionManager::RegisterCleanupAsyncOperation(Common::AsyncOperationSPtr const & asyncOperation)
{
    {
        AcquireExclusiveLock grab(lock_);
        if(this->State.Value == FabricComponentState::Opened)
        {
            cleanupAsyncOperation_ = asyncOperation;
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::ObjectClosed;
        }
    }
}

void DeletionManager::UnregisterCleanupAsyncOperation()
{
    AcquireExclusiveLock grab(lock_);
    cleanupAsyncOperation_.reset();
}

void DeletionManager::ScheduleCleanup(TimeSpan const & delay)
{
    TimerSPtr cleanupTimer;
    auto root = Root.CreateComponentRoot();
    cleanupTimer = Timer::Create(
        "Hosting.DeleteManager.Cleanup",
        [this, root](TimerSPtr const & timer) 
        { 
            timer->Cancel();
            this->StartCleanup(); 
        }, 
        false);

    bool cancelTimer = false;
    {
        AcquireExclusiveLock lock(lock_);
        if(this->State.Value == FabricComponentState::Opened || 
            this->State.Value == FabricComponentState::Opening)
        {
            cacheCleanupScanTimer_ = move(cleanupTimer);
        }
        else
        {
            cancelTimer = true;
        }
    }

    if(cancelTimer == true)
    {
        cleanupTimer->Cancel();
    }
    else
    {
        WriteInfo(
        Trace_DeletionManager,
        Root.TraceId,
        "Scheduling cleanup to run after {0}",
        delay);
        cacheCleanupScanTimer_->Change(delay);
    }
}

void DeletionManager::StartCleanup()
{
    WriteInfo(
        Trace_DeletionManager,
        Root.TraceId,
        "Begin(CleanupAsyncOperation)");

    auto operation = AsyncOperation::CreateAndStart<CleanupAsyncOperation>(
        *this,
        [this](AsyncOperationSPtr const& operation) { this->OnCleanupCompleted(operation); },
        this->Root.CreateAsyncOperationRoot());
}

void DeletionManager::OnCleanupCompleted(AsyncOperationSPtr const & operation)
{
    auto error = CleanupAsyncOperation::End(operation);
    WriteInfo(
        Trace_DeletionManager,
        Root.TraceId,
        "End(CleanupAsyncOperation): Error:{0}",
        error);

    this->ScheduleCleanup(HostingConfig::GetConfig().CacheCleanupScanInterval);
}
