// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "SimpleHttpServer.h"

namespace HttpServer
{
    class HttpServerImpl
        : public Common::AsyncFabricComponent
        , public Common::RootedObject
        , public IHttpServer
        , public Common::TextTraceComponent<Common::TraceTaskCodes::HttpGateway>
    {
        DENY_COPY(HttpServerImpl)

    public:

        HttpServerImpl(
            Common::ComponentRoot const &root,
            std::wstring const& listenAddress,
            ULONG numberOfParallelRequests,
            Transport::SecurityProvider::Enum credentialKind)
            : AsyncFabricComponent()
            , RootedObject(root)
            , listenAddress_(listenAddress)
            , prefixTreeUPtr_(std::make_unique<PrefixTree>(std::make_unique<PrefixTreeNode>(NodeData(listenAddress_))))
            , numberOfParallelRequests_(numberOfParallelRequests)
            , credentialKind_(credentialKind)
            , useLookasideAllocator_(false)
        {
        }

#if !defined(PLATFORM_UNIX)
        HttpServerImpl(
            Common::ComponentRoot const &root,
            std::wstring const& listenAddress,
            ULONG numberOfParallelRequests,
            Transport::SecurityProvider::Enum credentialKind,
            LookasideAllocatorSettings &&allocatorSettings)
            : AsyncFabricComponent()
            , RootedObject(root)
            , listenAddress_(listenAddress)
            , prefixTreeUPtr_(std::make_unique<PrefixTree>(std::make_unique<PrefixTreeNode>(NodeData(listenAddress_))))
            , numberOfParallelRequests_(numberOfParallelRequests)
            , useLookasideAllocator_(true)
            , credentialKind_(credentialKind)
            , allocatorSettings_(std::move(allocatorSettings))
        {
        }
#endif

        //
        // IHttpServer methods
        //

        ~HttpServerImpl() { }

        Common::ErrorCode RegisterHandler(
            __in std::wstring const& urlSuffix,
            __in IHttpRequestHandlerSPtr const& handler);

        Common::ErrorCode UnRegisterHandler(
            __in std::wstring const& urlSuffix,
            __in IHttpRequestHandlerSPtr const& handler);

    protected:
        //
        // AsyncFabricComponent methods.
        //

        Common::AsyncOperationSPtr OnBeginOpen(
            __in Common::TimeSpan timeout,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndOpen(__in Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr OnBeginClose(
            __in Common::TimeSpan timeout,
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode OnEndClose(__in Common::AsyncOperationSPtr const & asyncOperation);

        void OnAbort();

    private:

        Common::ErrorCode Initialize();

#if !defined(PLATFORM_UNIX)

        //
        // Ktl Http server initialization methods
        //

        Common::AsyncOperationSPtr KHttpBeginOpen(
            __in std::wstring const& url,
            __in uint activeListeners,
            __in Transport::SecurityProvider::Enum credentialKind,
            __in KHttpServer::RequestHandler requestHandler,
            __in KHttpServer::RequestHeaderHandler requestHeaderHandler,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode KHttpEndOpen(
            __in Common::AsyncOperationSPtr const& operation);

        Common::AsyncOperationSPtr KHttpBeginClose(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode KHttpEndClose(
            __in Common::AsyncOperationSPtr const& operation);

        void OnRequestReceived(__in KHttpServerRequest::SPtr request);

        void OnHeaderReadComplete(__in KHttpServerRequest::SPtr request, __out KHttpServer::HeaderHandlerAction &action);

        Common::ErrorCode CreateAndInitializeMessageContext(
            __in KHttpServerRequest::SPtr &request,
            __out IRequestMessageContextUPtr &messageContext);

        class AsyncKtlServiceOperationBase;
        class OpenKtlAsyncServiceOperation;
        class CloseKtlAsyncServiceOperation;

#else
        Common::AsyncOperationSPtr LHttpBeginOpen(
            __in std::wstring const& url,
            __in Transport::SecurityProvider::Enum credentialKind,
            __in LHttpServer::RequestHandler requestHandler,
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode LHttpEndOpen(
            __in Common::AsyncOperationSPtr const& operation);

        Common::AsyncOperationSPtr LHttpBeginClose(
            __in Common::AsyncCallback const& callback,
            __in Common::AsyncOperationSPtr const& parent);

        Common::ErrorCode LHttpEndClose(
            __in Common::AsyncOperationSPtr const& operation);

        void OnRequestReceived(web::http::http_request & message);
        Common::ErrorCode CreateAndInitializeMessageContext(web::http::http_request & request, IRequestMessageContextUPtr &result);

        class LinuxAsyncServiceBaseOperation;
        class OpenLinuxAsyncServiceOperation;
        class CloseLinuxAsyncServiceOperation;
#endif

        void DispatchRequest(__in IRequestMessageContextUPtr messageContext);

        class CreateAndOpenAsyncOperation;
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        //
        // Prefix tree to keep the URL suffixes registered by various handlers in an
        // ordered format to easily dispatch requests to the handler with the longest 
        // matching prefix.
        // 
        Common::RwLock prefixTreeLock_;

        struct NodeData
        {
            NodeData(std::wstring const &token)
            : token_(token)
            , handler_(nullptr)
            {
            }

            NodeData(std::wstring const &token, IHttpRequestHandlerSPtr handler)
                : token_(token)
                , handler_(handler)
            {
            }

            std::wstring token_;
            IHttpRequestHandlerSPtr handler_;
        };

        typedef Common::Tree<NodeData> PrefixTree;
        typedef PrefixTree::Node PrefixTreeNode;

        Common::ErrorCode ParseUrlSuffixAndUpdateTree(
            __inout PrefixTreeNode &currentNode,
            __in Common::StringCollection const& tokenizedSuffix,
            __in ULONG &currentIndex,
            __in IHttpRequestHandlerSPtr const&handler);

        PrefixTreeNode CreateNode(
            __in Common::StringCollection const& tokenixedSuffix,
            __in ULONG &currentIndex,
            __in IHttpRequestHandlerSPtr const&handler);

        Common::ErrorCode SearchTree(
            __in PrefixTreeNode const &currentNode,
            __in Common::StringCollection const& tokenizedSuffix,
            __in ULONG &currentIndex,
            __out IHttpRequestHandlerSPtr &handler);

        std::wstring listenAddress_;
        std::unique_ptr<PrefixTree> prefixTreeUPtr_;

#if !defined(PLATFORM_UNIX)
        KHttpServer::SPtr httpServer_;
        HttpServer::LookasideAllocatorSettings allocatorSettings_;
#else
        LHttpServer::SPtr httpServer_;
#endif

        ULONG numberOfParallelRequests_;
        Transport::SecurityProvider::Enum credentialKind_;
        bool useLookasideAllocator_;
    };
}
