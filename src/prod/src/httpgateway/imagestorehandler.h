// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    //
    // REST Handler for INativeImageStoreClient API's 
    //  * INativeImageStoreClient - ListContent()
    //  * INativeImageStoreClient - DeleteContent()
    //  * INativeImageStoreClient - CopyContent()
    //  * INativeImageStoreClient - UploadContent()
    //  * INativeImageStoreClient - UploadChunk()
    //  * INativeImageStoreClient - ListUploadSession()
    //  * INativeImageStoreClient - DeleteUploadSession()
    //  * INativeImageStoreClient - CommitUploadSession()

    class ImageStoreHandler
        : public RequestHandlerBase
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(ImageStoreHandler)

    public:

        explicit ImageStoreHandler(__in HttpGatewayImpl & server);
        virtual ~ImageStoreHandler();

        Common::ErrorCode Initialize();
        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            __in HttpServer::IRequestMessageContextUPtr request,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

    private:

        class ImageStoreHandlerAsyncOperation 
            : public RequestHandlerBase::HandlerAsyncOperation
        {
            DENY_COPY(ImageStoreHandlerAsyncOperation);
            
        public:
            ImageStoreHandlerAsyncOperation(
                RequestHandlerBase & owner,
                HttpServer::IRequestMessageContextUPtr messageContext,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);

            __declspec(property(get = get_UniqueFileName)) std::wstring const & UniqueFileName;
            __declspec(property(get = get_StartPosition)) uint64 StartPosition;
            __declspec(property(get = get_EndPosition)) uint64 EndPosition;
            __declspec(property(get = get_FileSize)) uint64 FileSize;

            std::wstring const & get_UniqueFileName() const { return uniqueFileName_; }
            uint64 get_StartPosition() const { return startPosition_; }
            uint64 get_EndPosition() const { return endPosition_; }
            uint64 get_FileSize() const { return fileSize_; }

            virtual void GetRequestBody(Common::AsyncOperationSPtr const& thisSPtr) override;
        
        private:
            void OnGetFileFromUploadComplete(
                Common::AsyncOperationSPtr const& operation, 
                bool expectedCompletedSynchronously);
            
            std::wstring uniqueFileName_;
            uint64 startPosition_;
            uint64 endPosition_;
            uint64 fileSize_;
            bool chunkedUpload_;
        };

        void ListContentFromRootPath(Common::AsyncOperationSPtr const& thisSPtr);
        void ListContentByRelativePath(Common::AsyncOperationSPtr const& thisSPtr);
        void OnListContentComplete(
            Common::AsyncOperationSPtr const& operation, 
            bool expectedCompletedSynchronously);
        void OnListPagedContentComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);

        void DeleteContent(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteContentComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);

        void CopyContent(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCopyContentComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);
   
        
#if defined(PLATFORM_UNIX)
        void UploadFileInLinux(Common::AsyncOperationSPtr const& thisSPtr);
#else
        void UploadFile(Common::AsyncOperationSPtr const& thisSPtr);
#endif
        void OnUploadFileComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);

#if !defined(PLATFORM_UNIX)
        void UploadChunk(Common::AsyncOperationSPtr const& thisSPtr);
        void OnUploadChunkComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);

        void ListUploadSessionBySessionId(Common::AsyncOperationSPtr const& thisSPtr);
        void ListUploadSessionByRelativePath(Common::AsyncOperationSPtr const& thisSPtr);
        void OnListUploadSessionComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);

        void DeleteUploadSession(Common::AsyncOperationSPtr const& thisSPtr);
        void OnDeleteUploadSessionComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);

        void CommitUploadSession(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCommitUploadSessionComplete(
            Common::AsyncOperationSPtr const& operation,
            bool expectedCompletedSynchronously);
#endif

        template<typename T>
        inline void HandleListContentResult(
            T& object, 
            __in HttpGateway::RequestHandlerBase::HandlerAsyncOperation* handlerOperation,
            __in Common::AsyncOperationSPtr const & operation)
        {
            ByteBufferUPtr bufferUPtr;
            Common::ErrorCode error;
            if (object.StoreFiles.size() > 0 || object.StoreFolders.size() > 0)
            {
                error = handlerOperation->Serialize(object, bufferUPtr);
                if (!error.IsSuccess())
                {
                    handlerOperation->OnError(operation->Parent, error);
                    return;
                }

                handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
            }
            else
            {
                UriArgumentParser argumentParser(handlerOperation->Uri);
                wstring relativePath;
                error = argumentParser.TryGetImageStoreRelativePath(relativePath);

                if (!error.IsSuccess())
                {
                    handlerOperation->OnError(operation->Parent, error);
                    return;
                }

                if (!relativePath.empty())
                {
                    ///ListContent API always return success error code no matter whether the path is existed or not
                    ///That design need to be changed in the future to return more accurate code 
                    ///Here the second call is to verify the existence of the path 
                    bool isExists;
                    auto &client = handlerOperation->FabricClient;
                    error = client.NativeImageStoreClient->DoesContentExist(relativePath, handlerOperation->Timeout, isExists);
                    if (!error.IsSuccess())
                    {
                        handlerOperation->OnError(operation->Parent, error);
                        return;
                    }

                    if (!isExists)
                    {
                        if (handlerOperation->Uri.ApiVersion < Constants::V62ApiVersion)
                        {
                            handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNotFound, *Constants::StatusDescriptionNotFound);
                        }
                        else
                        {
                            handlerOperation->OnError(operation->Parent, Constants::StatusNotFound, *Constants::StatusDescriptionNotFound);
                        }
                        return;
                    }
                }
                if (handlerOperation->Uri.ApiVersion < Constants::V62ApiVersion)
                {
                    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, *Constants::StatusDescriptionNoContent);
                }
                else
                {
                    handlerOperation->OnError(operation->Parent, Constants::StatusNoContent, *Constants::StatusDescriptionNoContent);
                }
            }
        }
    };
}
