// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // 21131800-f7d0-4734-b364-611502f9a107
    static const GUID CLSID_ComNativeImageStoreClient = 
        {0x21131800, 0xf7d0, 0x4734, {0xb3, 0x64, 0x61, 0x15, 0x02f, 0x9, 0xa1, 0x07}};

    class ComNativeImageStoreClient :
        public IFabricNativeImageStoreClient3,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComNativeImageStoreClient)

        BEGIN_COM_INTERFACE_LIST(ComNativeImageStoreClient)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricNativeImageStoreClient)
            COM_INTERFACE_ITEM(IID_IFabricNativeImageStoreClient,IFabricNativeImageStoreClient)
            COM_INTERFACE_ITEM(IID_IFabricNativeImageStoreClient2,IFabricNativeImageStoreClient2)
            COM_INTERFACE_ITEM(IID_IFabricNativeImageStoreClient3,IFabricNativeImageStoreClient3)
            COM_INTERFACE_ITEM(CLSID_ComNativeImageStoreClient, ComNativeImageStoreClient)
        END_COM_INTERFACE_LIST()

    public:
        ComNativeImageStoreClient(INativeImageStoreClientPtr const& clientPtr);
        virtual ~ComNativeImageStoreClient();

        static Common::ComPointer<IFabricNativeImageStoreClient> CreateComNativeImageStoreClient(INativeImageStoreClientPtr const &);

        //
        // IFabricNativeImageStoreClient methods.
        //
        //

        HRESULT STDMETHODCALLTYPE SetSecurityCredentials(
            /* [in] */ FABRIC_SECURITY_CREDENTIALS const* securityCredentials);

        HRESULT STDMETHODCALLTYPE UploadContent(
            /* [in] */ LPCWSTR remoteDestination, 
            /* [in] */ LPCWSTR localSource, 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

        HRESULT STDMETHODCALLTYPE UploadContent2(
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ LPCWSTR localSource,
            /* [in] */ IFabricNativeImageStoreProgressEventHandler const * progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

        HRESULT STDMETHODCALLTYPE BeginUploadContent(
            __in  /* [in] */ LPCWSTR remoteDestination,
            __in  /* [in] */ LPCWSTR localSource,
            __in  /* [in] */ DWORD timeout,
            __in  /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            __in  /* [in]*/	IFabricAsyncOperationCallback * callback,
            __out /* [out,retval]*/ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndUploadContent(
            __in /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE CopyContent(
            /* [in] */ LPCWSTR remoteSource,
            /* [in] */ LPCWSTR remoteDestination,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_STRING_LIST *skipFiles,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            /* [in] */ BOOLEAN checkMarkFile);

        HRESULT STDMETHODCALLTYPE BeginCopyContent(
            __in /* [in] */ LPCWSTR remoteSource,
            __in /* [in] */ LPCWSTR remoteDestination,
            __in /* [in] */ FABRIC_STRING_LIST *skipFiles,
            __in /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
            __in /* [in] */ BOOLEAN checkMarkFile,
            __in /* [in] */ DWORD timeoutMilliseconds,
            __in /* [in]*/	IFabricAsyncOperationCallback * callback,
            __out /* [out,retval]*/ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndCopyContent(
            __in /* [in] */ IFabricAsyncOperationContext *context);

        HRESULT STDMETHODCALLTYPE DownloadContent(
            /* [in] */ LPCWSTR remoteSource, 
            /* [in] */ LPCWSTR localDestination, 
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

        HRESULT STDMETHODCALLTYPE DownloadContent2(
            /* [in] */ LPCWSTR remoteSource, 
            /* [in] */ LPCWSTR localDestination, 
            /* [in] */ IFabricNativeImageStoreProgressEventHandler const * progressHandler,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [in] */ FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

        HRESULT STDMETHODCALLTYPE ListContent(
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [out,retval] */  IFabricStringListResult ** result);

        HRESULT STDMETHODCALLTYPE ListPagedContent(
            /* [in] */  FABRIC_IMAGE_STORE_LIST_DESCRIPTION const* listDescription,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [out,retval] */  FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT ** result);

        HRESULT STDMETHODCALLTYPE ListContentWithDetails(
            __in /* [in] */ LPCWSTR remoteLocation,
            __in /* [in]*/  BOOLEAN isRecursive,
            __in /* [in] */ DWORD timeoutMilliseconds,
            __out /* [out,retval] */  FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT ** result);

        HRESULT STDMETHODCALLTYPE ListPagedContentWithDetails(
            /* [in] */ FABRIC_IMAGE_STORE_LIST_DESCRIPTION const* listDescription,
            __in /* [in] */ DWORD timeoutMilliseconds,
            __out /* [out,retval] */  FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT ** result);

        HRESULT STDMETHODCALLTYPE BeginListContent(
            __in /* [in] */ LPCWSTR remoteLocation,
            __in /* [in] */ BOOLEAN isRecursive,
            __in /* [in] */ DWORD timeoutMilliseconds,
            __in /* [in] */	IFabricAsyncOperationCallback * callback,
            /* [out,retval]*/ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndListContent(
            /* [in] */ IFabricAsyncOperationContext *context, 
            __out /* [out,retval] */  FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT ** result);

        HRESULT STDMETHODCALLTYPE BeginListPagedContent(
            /* [in] */ const FABRIC_IMAGE_STORE_LIST_DESCRIPTION * listDescription,
            __in /* [in] */ DWORD timeoutMilliseconds,
            __in /* [in] */	IFabricAsyncOperationCallback * callback,
            /* [out,retval]*/ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndListPagedContent(
            /* [in] */ IFabricAsyncOperationContext *context,
            __out /* [out,retval] */  FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT ** result);

        HRESULT STDMETHODCALLTYPE DoesContentExist(
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds,
            /* [out,retval] */ BOOLEAN * result);

        HRESULT STDMETHODCALLTYPE DeleteContent(
            /* [in] */ LPCWSTR remoteLocation,
            /* [in] */ DWORD timeoutMilliseconds);

        HRESULT STDMETHODCALLTYPE BeginDeleteContent(
            __in /* [in] */ LPCWSTR remoteLocation,
            __in /* [in] */ DWORD timeoutMilliseconds,
            __in /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out,retval]*/ IFabricAsyncOperationContext **context);

        HRESULT STDMETHODCALLTYPE EndDeleteContent(
            __in /* [in] */ IFabricAsyncOperationContext *context);

    private:
        Common::TimeSpan GetTimeSpan(DWORD timeoutMilliseconds);
        INativeImageStoreClientPtr client_;
        Common::ScopedHeap heap_;
   };
}

