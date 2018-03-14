// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class RequestMessageContext::GetFileFromUploadAsyncOperation
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(GetFileFromUploadAsyncOperation);

    public:

        GetFileFromUploadAsyncOperation(
            HttpServer::RequestMessageContext const & messageContext,
            ULONG fileSize,
            ULONG maxEntityBodyForUploadChunkSize,
            ULONG defaultEntityBodyForUploadChunkSize,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out std::wstring & uniqueFileName);

    protected:

        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);
        void OnCancel();

    private:

        void StartGenerateFile(Common::AsyncOperationSPtr const& operation);
        void OnGenerateFileComplete(Common::AsyncOperationSPtr const& operation);

        HttpServer::RequestMessageContext const & messageContext_;
        std::wstring uniqueFileName_;
        ULONG chunkSize_;
    };
}
