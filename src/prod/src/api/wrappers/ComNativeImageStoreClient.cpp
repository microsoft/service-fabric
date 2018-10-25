// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;

StringLiteral const TraceComponent("ComNativeImageStoreClient");

ComNativeImageStoreClient::ComNativeImageStoreClient(INativeImageStoreClientPtr const& clientPtr)
    : client_(clientPtr)
{
}

ComNativeImageStoreClient::~ComNativeImageStoreClient()
{
}

ComPointer<IFabricNativeImageStoreClient> ComNativeImageStoreClient::CreateComNativeImageStoreClient(
    INativeImageStoreClientPtr const & ptr)
{
    return WrapperFactory::create_com_wrapper(ptr);
}

HRESULT ComNativeImageStoreClient::SetSecurityCredentials(
    /* [in] */ FABRIC_SECURITY_CREDENTIALS const* securityCredentials)
{
    Transport::SecuritySettings securitySettings;
    auto error = Transport::SecuritySettings::FromPublicApi(*securityCredentials, securitySettings);
    if(!error.IsSuccess()) {return ComUtility::OnPublicApiReturn(move(error));}

    Common::ErrorCode errorCode = client_->SetSecurity(move(securitySettings));
    if (!errorCode.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(errorCode));
    }

    return S_OK;
}

HRESULT ComNativeImageStoreClient::UploadContent2(
    LPCWSTR remoteDestination,
    LPCWSTR localSource,
    IFabricNativeImageStoreProgressEventHandler const * progressHandler,
    DWORD timeoutMilliseconds,
    FABRIC_IMAGE_STORE_COPY_FLAG copyFlag)
{
    wstring dest;
    auto hr = StringUtility::LpcwstrToWstring(remoteDestination, false, dest);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    wstring src;
    hr = StringUtility::LpcwstrToWstring(localSource, false, src);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto error = client_->UploadContent(
        dest,
        src,
        WrapperFactory::create_rooted_com_proxy(const_cast<IFabricNativeImageStoreProgressEventHandler*>(progressHandler)),
        this->GetTimeSpan(timeoutMilliseconds),
        Management::ImageStore::CopyFlag::FromPublicApi(copyFlag));
    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComNativeImageStoreClient::UploadContent(
    LPCWSTR remoteDestination,
    LPCWSTR localSource,
    DWORD timeoutMilliseconds,
    FABRIC_IMAGE_STORE_COPY_FLAG copyFlag)
{
    return this->UploadContent2(
        remoteDestination,
        localSource,
        NULL,
        timeoutMilliseconds,
        copyFlag);
}

HRESULT ComNativeImageStoreClient::BeginUploadContent(
    LPCWSTR,
    LPCWSTR,
    DWORD,
    FABRIC_IMAGE_STORE_COPY_FLAG,
    IFabricAsyncOperationCallback *,
    IFabricAsyncOperationContext **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::EndUploadContent(
    IFabricAsyncOperationContext *)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::CopyContent(
    LPCWSTR remoteSource,
    LPCWSTR remoteDestination,
    DWORD timeoutMilliseconds,
    FABRIC_STRING_LIST * skipFiles,
    FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
    BOOLEAN checkMarkFile)
{
    auto skipFileList = static_cast<FABRIC_STRING_LIST*>(skipFiles);
    if (skipFileList == nullptr) { return ErrorCodeValue::InvalidArgument; }

    vector<wstring> nativeSkipFiles;

    for (ULONG ix = 0; ix < skipFileList->Count; ++ix)
    {
        wstring fileName;
        HRESULT hr = StringUtility::LpcwstrToWstring(skipFileList->Items[ix], false, fileName);
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        nativeSkipFiles.push_back(fileName);
    }

    wstring src;
    auto hr = StringUtility::LpcwstrToWstring(remoteSource, false, src);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    wstring dest;
    hr = StringUtility::LpcwstrToWstring(remoteDestination, false, dest);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }    

    auto error = client_->CopyContent(
        src,
        dest,
        this->GetTimeSpan(timeoutMilliseconds),
        shared_ptr<vector<wstring>>(new vector<wstring>(nativeSkipFiles)),
        Management::ImageStore::CopyFlag::FromPublicApi(copyFlag),
        checkMarkFile);
    
    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceComponent, "CopyContent returning error {0}", error);
    }

    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComNativeImageStoreClient::BeginCopyContent(
    LPCWSTR,
    LPCWSTR,
    FABRIC_STRING_LIST *,
    FABRIC_IMAGE_STORE_COPY_FLAG,
    BOOLEAN,
    DWORD,
    IFabricAsyncOperationCallback *,
    IFabricAsyncOperationContext **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::EndCopyContent(
    IFabricAsyncOperationContext *)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::DownloadContent(
    LPCWSTR remoteSource, 
    LPCWSTR localDestination, 
    DWORD timeoutMilliseconds,
    FABRIC_IMAGE_STORE_COPY_FLAG copyFlag)
{
    return this->DownloadContent2(
        remoteSource,
        localDestination,
        NULL,
        timeoutMilliseconds,
        copyFlag);
}

HRESULT ComNativeImageStoreClient::DownloadContent2(
    LPCWSTR remoteSource, 
    LPCWSTR localDestination, 
    IFabricNativeImageStoreProgressEventHandler const * progressHandler,
    DWORD timeoutMilliseconds,
    FABRIC_IMAGE_STORE_COPY_FLAG copyFlag)
{
    wstring dest;
    auto hr = StringUtility::LpcwstrToWstring(localDestination, false, dest);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    wstring src;
    hr = StringUtility::LpcwstrToWstring(remoteSource, false, src);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto error = client_->DownloadContent(
        src,
        dest,
        WrapperFactory::create_rooted_com_proxy(const_cast<IFabricNativeImageStoreProgressEventHandler*>(progressHandler)),
        this->GetTimeSpan(timeoutMilliseconds),
        Management::ImageStore::CopyFlag::FromPublicApi(copyFlag));
    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComNativeImageStoreClient::ListContent(
    LPCWSTR remoteLocation,
    DWORD timeoutMilliseconds,
    IFabricStringListResult ** result)
{
    wstring location;
    auto hr = StringUtility::LpcwstrToWstring(remoteLocation, true, location);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    Management::FileStoreService::StoreContentInfo content;
    auto error = client_->ListContent(
        location,
        true,
        this->GetTimeSpan(timeoutMilliseconds),
        content);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }
    else
    {
        StringCollection innerResult;
        if (content.StoreFiles.size() > 0)
        {
            for (auto ix = 0; ix != content.StoreFiles.size(); ++ix)
            {
                innerResult.push_back(content.StoreFiles[ix].StoreRelativePath);
            }
        }

        return ComStringCollectionResult::ReturnStringCollectionResult(move(innerResult), result);
    }
}

HRESULT ComNativeImageStoreClient::ListPagedContent(
    FABRIC_IMAGE_STORE_LIST_DESCRIPTION const* listDescription,
    DWORD timeoutMilliseconds,
    FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT ** result)
{
    Management::ImageStore::ImageStoreListDescription listDesc;
    ErrorCode error = listDesc.FromPublicApi(*listDescription);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Management::FileStoreService::StorePagedContentInfo content;
    error = client_->ListPagedContent(
        listDesc,
        this->GetTimeSpan(timeoutMilliseconds),
        content);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(move(error));
    }
    else
    {
        vector<wstring> relativePaths;
        if (content.StoreFiles.size() > 0)
        {
            for (auto ix = 0; ix != content.StoreFiles.size(); ++ix)
            {
                relativePaths.push_back(content.StoreFiles[ix].StoreRelativePath);
            }
        }

        auto filePaths = this->heap_.AddItem<FABRIC_STRING_LIST>();
        StringList::ToPublicAPI(this->heap_, relativePaths, filePaths);
        *result = this->heap_.AddItem<FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT>().GetRawPointer();
        (*result)->Files = filePaths.GetRawPointer();
        (*result)->ContinuationToken = this->heap_.AddString(content.ContinuationToken);
        return ComUtility::OnPublicApiReturn(S_OK);
    }
}

HRESULT ComNativeImageStoreClient::ListContentWithDetails(
    __in LPCWSTR remoteLocation,
    __in BOOLEAN isRecursive,
    __in DWORD timeoutMilliseconds,
    __out FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT ** result)
{
    std::wstring location;
    auto hr = StringUtility::LpcwstrToWstring(remoteLocation, true, location);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    Management::FileStoreService::StoreContentInfo content;
    auto error = client_->ListContent(
        location,
        isRecursive,
        this->GetTimeSpan(timeoutMilliseconds),
        content);

    if (!error.IsSuccess())
    {
        Trace.WriteInfo(TraceComponent, "ListContentWithDetails returning error {0}", error);
        return ComUtility::OnPublicApiReturn(move(error));
    }
    else
    {
        *result = this->heap_.AddItem<FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT>().GetRawPointer();
        Common::ReferencePointer<FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST> fileList = this->heap_.AddItem<FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST>();
        Common::ReferencePointer<FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST> folderList = this->heap_.AddItem<FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST>();

        ComConversionUtility::ToPublicList<Management::FileStoreService::StoreFileInfo, FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM, FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST>(
            heap_, move(content.get_StoreFiles()), *fileList);

        ComConversionUtility::ToPublicList<Management::FileStoreService::StoreFolderInfo, FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM, FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST>(
            heap_, move(content.get_StoreFolders()), *folderList);

        (*result)->Files = fileList.GetRawPointer();
        (*result)->Folders = folderList.GetRawPointer();
        return ComUtility::OnPublicApiReturn(S_OK);
    }
}

HRESULT ComNativeImageStoreClient::ListPagedContentWithDetails(
    __in const FABRIC_IMAGE_STORE_LIST_DESCRIPTION * listDescription,
    __in DWORD timeoutMilliseconds,
    __out FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT ** result)
{
    Management::ImageStore::ImageStoreListDescription listDesc;
    ErrorCode error = listDesc.FromPublicApi(*listDescription);
    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    Trace.WriteInfo(
        TraceComponent,
        "ListContentWithDetails RemoteLocation:{0}, ContinuationToken:{1}, IsRecursive:{2}",
        listDesc.RemoteLocation,
        listDesc.ContinuationToken,
        listDesc.IsRecursive);

    Management::FileStoreService::StorePagedContentInfo content;
    error = client_->ListPagedContent(
        listDesc,
        this->GetTimeSpan(timeoutMilliseconds),
        content);

    if (!error.IsSuccess())
    {
        return ComUtility::OnPublicApiReturn(error.ToHResult());
    }
    else
    {
        *result = this->heap_.AddItem<FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT>().GetRawPointer();
        Common::ReferencePointer<FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST> fileList = this->heap_.AddItem<FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST>();
        Common::ReferencePointer<FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST> folderList = this->heap_.AddItem<FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST>();

        ComConversionUtility::ToPublicList<Management::FileStoreService::StoreFileInfo, FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM, FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST>(
            heap_, move(content.get_StoreFiles()), *fileList);

        ComConversionUtility::ToPublicList<Management::FileStoreService::StoreFolderInfo, FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM, FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST>(
            heap_, move(content.get_StoreFolders()), *folderList);

        (*result)->Files = fileList.GetRawPointer();
        (*result)->Folders = folderList.GetRawPointer();
        (*result)->ContinuationToken = this->heap_.AddString(content.ContinuationToken);
        return ComUtility::OnPublicApiReturn(S_OK);
    }
}

HRESULT ComNativeImageStoreClient::BeginListContent(
    LPCWSTR,
    BOOLEAN,
    DWORD,
    IFabricAsyncOperationCallback *,
    IFabricAsyncOperationContext **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::EndListContent(
    IFabricAsyncOperationContext *,
    FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::BeginListPagedContent(
    const FABRIC_IMAGE_STORE_LIST_DESCRIPTION *,
    DWORD,
    IFabricAsyncOperationCallback *,
    IFabricAsyncOperationContext **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::EndListPagedContent(
    IFabricAsyncOperationContext *,
    FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::DoesContentExist(
    LPCWSTR remoteLocation, 
    DWORD timeoutMilliseconds,
    BOOLEAN * result)
{
    if (result == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

    wstring location;
    auto hr = StringUtility::LpcwstrToWstring(remoteLocation, false, location);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    bool innerResult = false;
    auto error = client_->DoesContentExist(
        location, 
        this->GetTimeSpan(timeoutMilliseconds),
        innerResult);

    if (!error.IsSuccess()) { return ComUtility::OnPublicApiReturn(move(error)); }

    *result = (innerResult ? TRUE : FALSE);

    return S_OK;
}

HRESULT ComNativeImageStoreClient::DeleteContent(
    LPCWSTR remoteLocation,
    DWORD timeoutMilliseconds)
{
    wstring location;
    auto hr = StringUtility::LpcwstrToWstring(remoteLocation, false, location);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    auto error = client_->DeleteContent(location, this->GetTimeSpan(timeoutMilliseconds));
    return ComUtility::OnPublicApiReturn(move(error));
}

HRESULT ComNativeImageStoreClient::BeginDeleteContent(
    LPCWSTR,
    DWORD,
    IFabricAsyncOperationCallback *,
    IFabricAsyncOperationContext **)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

HRESULT ComNativeImageStoreClient::EndDeleteContent(
    IFabricAsyncOperationContext *)
{
    //to be implemented at the item #7202109
    return ComUtility::OnPublicApiReturn(ErrorCodeValue::NotImplemented);
}

Common::TimeSpan ComNativeImageStoreClient::GetTimeSpan(DWORD timeoutMilliseconds)
{
    return timeoutMilliseconds == INFINITE ? Common::TimeSpan::MaxValue : TimeSpan::FromMilliseconds(static_cast<double>(timeoutMilliseconds));
}
