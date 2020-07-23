// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class GetFileFromUploadAsyncOperation
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(GetFileFromUploadAsyncOperation);

    public:

        GetFileFromUploadAsyncOperation(
            __in HttpServer::IRequestMessageContext & messageContext,
            ULONG fileSize,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out std::wstring & uniqueFileName);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancel();

    private:

        void StartRead(Common::AsyncOperationSPtr const& operation);
        void OnReadChunkComplete(
            Common::AsyncOperationSPtr const& operation, 
            bool expectedCompletedSynchronously);
      
        HttpServer::IRequestMessageContext & messageContext_;
        std::wstring uniqueFileName_;
        ULONG chunkSize_;
        std::vector<byte> buffer_;
        KMemRef memRef_;
    };
}
