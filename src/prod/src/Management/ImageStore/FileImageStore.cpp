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

StringLiteral const FileImageStoreLiteral("FileImageStore");
GlobalWString FileImageStore::SchemaTag = make_global<wstring>(L"file:");

FileImageStore::FileImageStore(    
    wstring const & rootUri, 
    wstring const & cacheUri,
    wstring const & localRoot,
    ImageStoreAccessDescriptionUPtr && accessDescription)
    : IImageStore(localRoot, cacheUri)
    , rootUri_(RemoveSchemaTag(rootUri))
    , accessDescription_(move(accessDescription))
{    
    markFile_ = std::wstring(L"");
}

wstring FileImageStore::GetRemoteFileName(wstring const & tag)
{
    return Path::Combine(rootUri_, tag);
}

wstring FileImageStore::RemoveSchemaTag(wstring const & uri)
{
    return uri.substr(FileImageStore::SchemaTag->size());
}

ErrorCode FileImageStore::OnRemoveObjectFromStore(
    vector<wstring> const & objects,
    Common::TimeSpan const timeout)
{
    vector<wstring> objectsToDelete;
    for (size_t i = 0; i < objects.size(); i++)
    {
        objectsToDelete.push_back(GetRemoteFileName(objects[i]));
    }

    ImpersonationContextUPtr impersonationContext;
    if(accessDescription_)
    {        
        auto error = accessDescription_->AccessToken->Impersonate(impersonationContext);
        if(!error.IsSuccess()) { return error; }
    }

    return DeleteObjects(objectsToDelete, timeout);
}

ErrorCode FileImageStore::OnDownloadFromStore(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    vector<CopyFlag::Enum> const & flags,
    Common::TimeSpan const timeout)
{           
    TimeoutHelper timeoutHelper(timeout);

    vector<wstring> remoteFileNames;
    vector<wstring> localFileNames;    
    vector<BOOLEAN> checkMakeFiles;
    vector<shared_ptr<vector<wstring>>> skipFiles;

    for(size_t i = 0; i < remoteObjects.size(); i++)
    {
        remoteFileNames.push_back(GetRemoteFileName(remoteObjects[i]));        
        localFileNames.push_back(GetLocalFileNameForDownload(localObjects[i]));   
        checkMakeFiles.push_back(false);
        skipFiles.push_back(nullptr);
    }
    
    if(accessDescription_ && !accessDescription_->HasWriteAccess)
    {                
        unique_ptr<ResourceHolder<wstring>> directoryHolder;        
        auto error = CreateAndAclTempDirectory(
            accessDescription_->Sid,
            GENERIC_READ|GENERIC_WRITE,
            directoryHolder);
        if(!error.IsSuccess()) { return error; }

        vector<wstring> tempLocalFileNames;		
        for(size_t i = 0; i < remoteObjects.size(); i++)
        {
            tempLocalFileNames.push_back(Path::Combine(directoryHolder->Value, remoteObjects[i]));
        }

        // Copy to temp location from the remote store as impersonated user
        {
            ImpersonationContextUPtr impersonationContext;
            error = accessDescription_->AccessToken->Impersonate(impersonationContext);
            if(!error.IsSuccess()) { return error; }

            error = CopyObjects(remoteFileNames, tempLocalFileNames, skipFiles, flags, checkMakeFiles, timeoutHelper.GetRemainingTime());
            if(!error.IsSuccess()) { return error; }
        }        

        // copy from temp location to local destination folder
        return CopyObjects(tempLocalFileNames, localFileNames, skipFiles, flags, checkMakeFiles, timeoutHelper.GetRemainingTime());
    }

    ImpersonationContextUPtr impersonationContext;
    if(accessDescription_)
    {
        // The ImpersonateUser has write access to the local destination
        auto error = accessDescription_->AccessToken->Impersonate(impersonationContext);
        if(!error.IsSuccess()) { return error; }
    }

    return CopyObjects(remoteFileNames, localFileNames, skipFiles, flags, checkMakeFiles, timeoutHelper.GetRemainingTime());
}

ErrorCode FileImageStore::OnUploadToStore(
    vector<wstring> const & remoteObjects,
    vector<wstring> const & localObjects,
    vector<CopyFlag::Enum> const & flags,
    Common::TimeSpan const timeout)
{
    TimeoutHelper timeoutHelper(timeout);

    vector<wstring> remoteFileNames;
    vector<wstring> localFileNames;
    vector<BOOLEAN> checkMakeFiles;
    vector<shared_ptr<vector<wstring>>> skipFiles;

    for(size_t i = 0; i < remoteObjects.size(); i++)
    {
        remoteFileNames.push_back(GetRemoteFileName(remoteObjects[i]));
        localFileNames.push_back(GetLocalFileNameForUpload(localObjects[i]));
        checkMakeFiles.push_back(false);
        skipFiles.push_back(nullptr);
    }

    if(accessDescription_ && !accessDescription_->HasReadAccess)
    {
        unique_ptr<ResourceHolder<wstring>> directoryHolder;        
        auto error = CreateAndAclTempDirectory(            
            accessDescription_->Sid,
            GENERIC_READ|GENERIC_WRITE,
            directoryHolder);
        if(!error.IsSuccess()) { return error; }

        vector<wstring> tempLocalFileNames;
        for(size_t i = 0; i < remoteObjects.size(); i++)
        {
            tempLocalFileNames.push_back(Path::Combine(directoryHolder->Value, remoteObjects[i]));
        }        

        // Copy to temp location from local source
        error = CopyObjects(localFileNames, tempLocalFileNames, skipFiles, flags, checkMakeFiles, timeoutHelper.GetRemainingTime());
        if(!error.IsSuccess()) { return error; }        
        
        {
            ImpersonationContextUPtr impersonationContext;
            error = accessDescription_->AccessToken->Impersonate(impersonationContext);
            if(!error.IsSuccess()) { return error; }        

            // copy from temp location to remote store as impersonated user
            return CopyObjects(tempLocalFileNames, remoteFileNames, skipFiles, flags, checkMakeFiles, timeoutHelper.GetRemainingTime());
        }        
    }

    ImpersonationContextUPtr impersonationContext;
    if(accessDescription_)
    {
        // The ImpersonateUser has read access to the local source
        auto error = accessDescription_->AccessToken->Impersonate(impersonationContext);
        if(!error.IsSuccess()) { return error; }
    }

    return CopyObjects(localFileNames, remoteFileNames, skipFiles, flags, checkMakeFiles, timeoutHelper.GetRemainingTime());
}

ErrorCode FileImageStore::OnDoesContentExistInStore(
    vector<wstring> const & remoteObjects,
    Common::TimeSpan const timeout,
    __out map<wstring, bool> & objectExistsMap)
{
    TimeoutHelper timeoutHelper(timeout);

    ImpersonationContextUPtr impersonationContext;
    if(accessDescription_)
    {
        auto error = accessDescription_->AccessToken->Impersonate(impersonationContext);
        if(!error.IsSuccess()) { return error; }
    }

    for(auto iter = remoteObjects.begin(); iter != remoteObjects.end(); ++iter)
    {
        if (timeoutHelper.GetRemainingTime() > Common::TimeSpan::Zero)
        {
            wstring fileName = GetRemoteFileName(*iter);
            objectExistsMap[*iter] = Directory::Exists(fileName) || File::Exists(fileName);
        }
        else
        {
            return ErrorCode(ErrorCodeValue::Timeout);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FileImageStore::CreateAndAclTempDirectory(    
    SidSPtr const & sid,
    DWORD const accessMask,
    __out unique_ptr<ResourceHolder<wstring>> & directoryHolder)
{
    wstring directoryName = File::GetTempFileNameW();
    if(!Directory::Exists(directoryName))
    {
        auto error = Directory::Create2(directoryName);
        if(!error.IsSuccess()) { return error; }

        directoryHolder = make_unique<ResourceHolder<wstring>>(
            directoryName,
            [](ResourceHolder<wstring> * thisPtr)
        {
            if(Directory::Exists(thisPtr->Value))
            {
                Directory::Delete(thisPtr->Value, true).ReadValue();
            }
        });

        error = SecurityUtility::UpdateFolderAcl(
            sid,
            directoryName,
            accessMask,
            TimeSpan::Zero);

        if(!error.IsSuccess()) { return error; }
    }
    else
    {
        return ErrorCode(ErrorCodeValue::AlreadyExists);
    }

    return ErrorCodeValue::Success;
}
