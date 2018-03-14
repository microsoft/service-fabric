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

StringLiteral TraceType_ImageStoreAccessDescription= "ImageStoreAccessDescription";

ImageStoreAccessDescription::ImageStoreAccessDescription(
    Common::AccessTokenSPtr const & accessToken,
    Common::SidSPtr const & sid,
    bool const hasReadAccess,
    bool const hasWriteAccess): accessToken_(accessToken),
    sid_(sid),
    hasReadAccess_(hasReadAccess),
    hasWriteAccess_(hasWriteAccess)
{
    ASSERT_IF(!accessToken, "accessToken should not be null");
    ASSERT_IF(!sid, "sid should not be null");
}

ImageStoreAccessDescription::~ImageStoreAccessDescription() { }

ErrorCode ImageStoreAccessDescription::Create(
    AccessTokenSPtr const & imageStoreAccessToken,
    wstring const & localCache,
    wstring const & localRoot,
    __out ImageStoreAccessDescriptionUPtr & imageStoreAccessDescription)
{
    ASSERT_IF(!imageStoreAccessToken, "imageStoreAccessToken should not be null");

    SidSPtr imageStoreAccessSid;        
    auto error = imageStoreAccessToken->GetUserSid(imageStoreAccessSid);
    if(!error.IsSuccess()) { return error; }        

    bool hasReadAccess;
    if(!localRoot.empty())
    {
        error = CheckAccess(
            localRoot,
            imageStoreAccessToken,
            GENERIC_READ,
            hasReadAccess);
        if(!error.IsSuccess()) { return error; }        
    }
    else
    {        
        hasReadAccess = false;
    }

    bool hasWriteAccess;
    wstring writePath = localCache.empty() ? localRoot : localCache;
    if(!writePath.empty())
    {
        error = CheckAccess(
            writePath,
            imageStoreAccessToken,
            GENERIC_WRITE,
            hasWriteAccess);
        if(!error.IsSuccess()) { return error; }        
    }
    else
    {        
        hasWriteAccess = false;
    }

    imageStoreAccessDescription = make_unique<ImageStoreAccessDescription>(
        imageStoreAccessToken,
        imageStoreAccessSid,
        hasReadAccess,
        hasWriteAccess);

    return ErrorCodeValue::Success;
}

ErrorCode ImageStoreAccessDescription::CheckAccess(
    std::wstring const & path,
    AccessTokenSPtr const & accessToken,
    DWORD const accessMask,
    __out bool & hasAccess)
{
#if !defined(PLATFORM_UNIX)
    wstring tempFileName = Path::Combine(path, File::GetTempFileNameW());
    HANDLE handle = ::CreateFile(
        tempFileName.c_str(),
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL);    

    if(handle == INVALID_HANDLE_VALUE)
    {
        return ErrorCode::TraceReturn(
            ErrorCode::FromWin32Error(::GetLastError()),
            TraceTaskCodes::ImageStore,
            TraceType_ImageStoreAccessDescription,
            "CreateFile");
    }

    ResourceHolder<HANDLE> fileHolder(
        move(handle),
        [tempFileName] (ResourceHolder<HANDLE> * thisPtr)
    {
        ::CloseHandle(thisPtr->Value);
        if(File::Exists(tempFileName)) { File::Delete2(tempFileName); }
    });

    {
        ImpersonationContextUPtr impersonationContext;
        auto error = accessToken->Impersonate(impersonationContext);
        if(!error.IsSuccess()) { return error; }  

        DWORD shareOption = FILE_SHARE_READ;         
        if((accessMask & GENERIC_WRITE) == GENERIC_WRITE) 
        { 
            shareOption = shareOption | FILE_SHARE_WRITE; 
        }        

        HANDLE accessCheckHandle = ::CreateFile(
            tempFileName.c_str(),
            accessMask,
            shareOption,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if(accessCheckHandle == INVALID_HANDLE_VALUE)
        {
            error = ErrorCode::FromWin32Error(::GetLastError());
            if(error.IsError(ErrorCodeValue::AccessDenied))
            {
                hasAccess = false;
            }
            else
            {
                TraceWarning(
                    TraceTaskCodes::ImageStore,
                    TraceType_ImageStoreAccessDescription,
                    "Failed to determine if the SecurityPrincipal has access to the path. Path:{0}, AccessMask:{1}, Error:{2}",                
                    path,
                    accessMask,
                    error);
                return error;
            }
        }
        else
        {
            ::CloseHandle(accessCheckHandle);
            hasAccess = true;
        }
    }
#else
    hasAccess = true;
#endif

    return ErrorCodeValue::Success;
}
