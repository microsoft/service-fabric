// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class OverlayNetworkPluginOperations :
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(OverlayNetworkPluginOperations)

    public:
        OverlayNetworkPluginOperations(Common::ComponentRootSPtr const & root);
        virtual ~OverlayNetworkPluginOperations();

        Common::AsyncOperationSPtr BeginUpdateRoutes(
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginAttachContainer(
            OverlayNetworkContainerParametersSPtr const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAttachContainer(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDetachContainer(
            std::wstring containerId,
            std::wstring networkName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDetachContainer(
            Common::AsyncOperationSPtr const & operation);

    private:

#if !defined(PLATFORM_UNIX)
        void GetHttpResponse(
            AsyncOperationSPtr const & thisSPtr,
            HttpClient::IHttpClientRequestSPtr clientRequest);

        void OnGetHttpResponseCompleted(
            AsyncOperationSPtr const & operation,
            HttpClient::IHttpClientRequestSPtr clientRequest,
            bool expectedCompletedSynchronously);
#else
        void ExecutePostRequest(std::unique_ptr<web::http::client::http_client> httpClient, string uri, wstring jsonString, AsyncOperationSPtr const & thisSPtr);
#endif

        class UpdateRoutesAsyncOperation : public AsyncOperation
        {
            DENY_COPY(UpdateRoutesAsyncOperation)

        public:
            UpdateRoutesAsyncOperation(
                __in OverlayNetworkPluginOperations & owner,
                OverlayNetworkRoutingInformationSPtr const & routingInfo,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~UpdateRoutesAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnHttpRequestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            OverlayNetworkPluginOperations & owner_;
            OverlayNetworkRoutingInformationSPtr routingInfo_;
#if !defined(PLATFORM_UNIX)
            HttpClient::IHttpClientSPtr client_;
            HttpClient::IHttpClientRequestSPtr clientRequest_;
#else
            std::unique_ptr<web::http::client::http_client> httpClient_;
#endif
            TimeoutHelper timeoutHelper_;
        };

        class AttachContainerAsyncOperation : public AsyncOperation
        {
            DENY_COPY(AttachContainerAsyncOperation)

        public:
            AttachContainerAsyncOperation(
                __in OverlayNetworkPluginOperations & owner,
                OverlayNetworkContainerParametersSPtr const & params,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~AttachContainerAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnHttpRequestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            OverlayNetworkPluginOperations & owner_;
            OverlayNetworkContainerParametersSPtr params_;
#if !defined(PLATFORM_UNIX)
            HttpClient::IHttpClientSPtr client_;
            HttpClient::IHttpClientRequestSPtr clientRequest_;
#else
            std::unique_ptr<web::http::client::http_client> httpClient_;
#endif
            TimeoutHelper timeoutHelper_;
        };

        class DetachContainerAsyncOperation : public AsyncOperation
        {
            DENY_COPY(DetachContainerAsyncOperation)

        public:
            DetachContainerAsyncOperation(
                __in OverlayNetworkPluginOperations & owner,
                std::wstring containerId,
                std::wstring networkName,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~DetachContainerAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnHttpRequestCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            OverlayNetworkPluginOperations & owner_;
            std::wstring containerId_;
            std::wstring networkName_;
#if !defined(PLATFORM_UNIX)
            HttpClient::IHttpClientSPtr client_;
            HttpClient::IHttpClientRequestSPtr clientRequest_;
#else
            std::unique_ptr<web::http::client::http_client> httpClient_;
#endif
            TimeoutHelper timeoutHelper_;
        };
    };

    class QueryResponse : public Common::IFabricJsonSerializable
    {
    public:

        QueryResponse() {};
        ~QueryResponse() {};

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(QueryResponse::ReturnCodeParameter, ReturnCode)
            SERIALIZABLE_PROPERTY(QueryResponse::MessageParameter, Message)
        END_JSON_SERIALIZABLE_PROPERTIES()

        int ReturnCode;
        wstring Message;
        static Common::WStringLiteral const ReturnCodeParameter;
        static Common::WStringLiteral const MessageParameter;
    };
}

