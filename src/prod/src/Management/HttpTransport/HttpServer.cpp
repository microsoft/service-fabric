// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#endif

using namespace std;
using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;
using namespace ServiceModel;
using namespace Transport;

#if defined(PLATFORM_UNIX)
using namespace std;
using namespace web;
using namespace utility;
using namespace http;
using namespace web::http::experimental::listener;
#endif

StringLiteral const TraceType("HttpServer");
StringLiteral const TraceTypeLifeCycle("HttpServerLifeCycle");

#if !defined(PLATFORM_UNIX)
class LookasideAllocator : public std::enable_shared_from_this<LookasideAllocator>
{
    DENY_COPY(LookasideAllocator)
public:
    static ErrorCode Create(
        __in InterlockedBufferPool &bufferPool,
        __in InterlockedPoolPerfCountersSPtr &perfCounters,
        __out shared_ptr<LookasideAllocator> &allocator)
    {
        allocator = shared_ptr<LookasideAllocator>(new LookasideAllocator());

        return allocator->Initialize(bufferPool, perfCounters);
    }

    __declspec(property(get = get_InnerAllocator)) KAllocator & InnerAllocator;

    inline KAllocator & get_InnerAllocator()
    {
        return bufferPoolAllocator_;
    }

private:
    LookasideAllocator() 
        : allocations_(0)
    {}

    ErrorCode Initialize(__in InterlockedBufferPool &bufferPool, __in InterlockedPoolPerfCountersSPtr &perfCounters)
    {
        // capture a self reference so the allocator stays alive.
        thisSPtr_ = this->shared_from_this();

        auto hr = bufferPoolAllocator_.Initialize(
            &bufferPool,
            [this](PVOID, size_t)
            {
                ++allocations_;
            },
            [this](PVOID)
            {
                // This operates on the assumption that after getting to 0 allocs
                // there will not be any more allocs. This is valid for our use case here.
                auto currentAllocs = --allocations_;
                if (currentAllocs == 0)
                {
                    thisSPtr_ = nullptr;
                }
            },
            perfCounters);

        if (hr != S_OK)
        {
            thisSPtr_ = nullptr;
        }

        return ErrorCode::FromHResult(hr);
    }

    InterlockedBufferPoolAllocator bufferPoolAllocator_;
    atomic_long allocations_;
    shared_ptr<LookasideAllocator> thisSPtr_;
};

class KHttpServerRequestWrapper : public KHttpServerRequest, public KHttp_IOCP_Handler
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpServerRequestWrapper)

public:

    static ErrorCode Create(
        __in KSharedPtr<KHttpServer> &Parent,
        __in KAllocator &allocator,
        __out KHttpServerRequestWrapper::SPtr &request)
    {
        KHttpServerRequestWrapper::SPtr tmp = _new(KTL_TAG_HTTP, allocator) KHttpServerRequestWrapper(Parent);
        if (!tmp)
        {
            return ErrorCodeValue::OutOfMemory;
        }

        auto status = tmp->Initialize();
        if (!NT_SUCCESS(status))
        {
            return ErrorCode::FromNtStatus(status);
        }

        request = Ktl::Move(tmp);

        return ErrorCodeValue::Success;
    }

    NTSTATUS StartRead(
        KAsyncContextBase* const parentAsync,
        __in KAsyncContextBase::CompletionCallback completion)
    {
        auto status = __super::StartRead(parentAsync, completion);
        return status;
    }

    VOID
        IOCP_Completion(
        __in ULONG error,
        __in ULONG bytesTransferred) override;

protected:

    KHttpServerRequestWrapper(
        __in KSharedPtr<KHttpServer> Parent)
        : KHttpServerRequest(Parent)
    {
        _Overlapped._Handler = this;
    }
};

KHttpServerRequestWrapper::~KHttpServerRequestWrapper()
{
}

VOID KHttpServerRequestWrapper::IOCP_Completion(
    __in ULONG error,
    __in ULONG bytesTransferred)
{
    IOCP_Completion_Impl(error, bytesTransferred);
}

class KHttpServerWrapper : public KHttpServer
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpServerWrapper)

public:
    static ErrorCode Create(
        unique_ptr<InterlockedBufferPool> &&bufferPool,
        InterlockedPoolPerfCountersSPtr &allocatorPerfCounters,
        __out KHttpServer::SPtr &httpServer)
    {           
        KHttpServer::SPtr tmp = _new(KTL_TAG_HTTP, HttpUtil::GetKtlAllocator()) KHttpServerWrapper(move(bufferPool), allocatorPerfCounters);
        if (!tmp)
        {
            return ErrorCodeValue::OutOfMemory;
        }

        auto status = tmp->Status();
        if (!NT_SUCCESS(status))
        {
            return ErrorCode::FromNtStatus(status);
        }

        httpServer = tmp;
        return ErrorCodeValue::Success;
    }

    NTSTATUS PostRead() override;

    VOID MyReadCompletion(
        KAsyncContextBase* const parent,
        __in KAsyncContextBase& op)
    {
        __super::ReadCompletion(parent, op);
    }

private:

    KHttpServerWrapper(unique_ptr<InterlockedBufferPool> &&bufferPool, InterlockedPoolPerfCountersSPtr &allocatorPerfCounters);

    unique_ptr<InterlockedBufferPool> bufferPool_;
    uint activeListeners_;
    InterlockedPoolPerfCountersSPtr allocatorPerfCounters_;
};

KHttpServerWrapper::KHttpServerWrapper(
    unique_ptr<InterlockedBufferPool> &&bufferPool,
    InterlockedPoolPerfCountersSPtr &allocatorPerfCounters)
    : bufferPool_(move(bufferPool))
    , allocatorPerfCounters_(allocatorPerfCounters)
{
}

NTSTATUS KHttpServerWrapper::PostRead()
{
    KHttpServerRequestWrapper::SPtr Req;
    shared_ptr<LookasideAllocator> allocatorSPtr;
    auto error = LookasideAllocator::Create(*bufferPool_, allocatorPerfCounters_, allocatorSPtr);
    if (!error.IsSuccess())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KSharedPtr<KHttpServer> khttpServerSPtr(dynamic_cast<KHttpServer*>(this));
    error = KHttpServerRequestWrapper::Create(khttpServerSPtr, allocatorSPtr->InnerAllocator, Req);
    if (!error.IsSuccess())
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    auto status = Req->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    CompletionCallback callback(this, &KHttpServerWrapper::MyReadCompletion);

    return Req->StartRead(this, callback);;
}

KHttpServerWrapper::~KHttpServerWrapper()
{

}
#endif

class HttpServerImpl::OpenAsyncOperation : public TimedAsyncOperation
{
public:
    OpenAsyncOperation(
        HttpServerImpl &owner,
        TimeSpan &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        auto error = owner_.Initialize();

#if !defined(PLATFORM_UNIX)

        EventRegisterMicrosoft_Windows_KTL();

        owner_.KHttpBeginOpen(
            owner_.listenAddress_,
            owner_.numberOfParallelRequests_,
            owner_.credentialKind_,
            KHttpServer::RequestHandler(&owner_, &HttpServerImpl::OnRequestReceived),
            KHttpServer::RequestHeaderHandler(&owner_, &HttpServerImpl::OnHeaderReadComplete),
            [this](AsyncOperationSPtr const& operation)
            {
                auto error = this->owner_.KHttpEndOpen(operation);
                TryComplete(operation->Parent, error);
            },
            thisSPtr);
#else
        HttpServerImpl & owner = owner_;
        owner_.LHttpBeginOpen(
            owner_.listenAddress_,
            owner_.credentialKind_,
            [&owner](http_request & message)
        {
            owner.OnRequestReceived(message);
        },
            [this](AsyncOperationSPtr const& operation)
        {
            auto error = this->owner_.LHttpEndOpen(operation);
            TryComplete(operation->Parent, error);
        },
            thisSPtr);
#endif
    }

private:
    HttpServerImpl &owner_;
};

class HttpServerImpl::CloseAsyncOperation : public TimedAsyncOperation
{
public:
    CloseAsyncOperation(
        HttpServerImpl &owner,
        TimeSpan &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
#if !defined(PLATFORM_UNIX)
        owner_.KHttpBeginClose(
            [this](AsyncOperationSPtr const& operation)
            {
                auto error = this->owner_.KHttpEndClose(operation);
                EventUnregisterMicrosoft_Windows_KTL();
                TryComplete(operation->Parent, error);
            },
            thisSPtr);
#else
        owner_.LHttpBeginClose(
            [this](AsyncOperationSPtr const& operation)
        {
            auto error = this->owner_.LHttpEndClose(operation);
            TryComplete(operation->Parent, error);
        },
            thisSPtr);
#endif
    }

private:
    HttpServerImpl &owner_;
};

AsyncOperationSPtr HttpServerImpl::OnBeginOpen(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceTypeLifeCycle, "BeginOpen");
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode HttpServerImpl::OnEndOpen(AsyncOperationSPtr const & asyncOperation)
{
    auto error = HttpServerImpl::OpenAsyncOperation::End(asyncOperation);
    WriteInfo(TraceTypeLifeCycle, "Open completed with {0}", error);
    return error;
}

AsyncOperationSPtr HttpServerImpl::OnBeginClose(
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    WriteInfo(TraceTypeLifeCycle, "BeginClose");
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode HttpServerImpl::OnEndClose(AsyncOperationSPtr const & asyncOperation)
{
    auto error = CloseAsyncOperation::End(asyncOperation);
    WriteInfo(TraceTypeLifeCycle, "Close completed with {0}", error);
    return error;
}

void HttpServerImpl::OnAbort()
{
    // TODO
}

#if !defined(PLATFORM_UNIX)
AsyncOperationSPtr HttpServerImpl::KHttpBeginOpen(
    __in std::wstring const& url,
    __in uint activeListeners,
    __in Transport::SecurityProvider::Enum credentialKind,
    __in KHttpServer::RequestHandler requestHandler,
    __in KHttpServer::RequestHeaderHandler requestHeaderHandler,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<OpenKtlAsyncServiceOperation>(
        url,
        activeListeners,
        credentialKind,
        requestHandler,
        requestHeaderHandler,
        httpServer_,
        callback,
        parent);
}

ErrorCode HttpServerImpl::KHttpEndOpen(AsyncOperationSPtr const &operation)
{
    return OpenKtlAsyncServiceOperation::End(operation);
}

AsyncOperationSPtr HttpServerImpl::KHttpBeginClose(
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<CloseKtlAsyncServiceOperation>(
        httpServer_,
        callback,
        parent);
}

ErrorCode HttpServerImpl::KHttpEndClose(AsyncOperationSPtr const &operation)
{
    return CloseKtlAsyncServiceOperation::End(operation);
}

#else

Common::AsyncOperationSPtr HttpServerImpl::LHttpBeginOpen(
    __in std::wstring const& url,
    __in Transport::SecurityProvider::Enum credentialKind,
    __in LHttpServer::RequestHandler requestHandler,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<OpenLinuxAsyncServiceOperation>(
        url,
        credentialKind,
        requestHandler,
        httpServer_,
        callback,
        parent
        );
}

Common::ErrorCode HttpServerImpl::LHttpEndOpen(
    __in Common::AsyncOperationSPtr const& operation)
{
    return OpenLinuxAsyncServiceOperation::End(operation);
}

Common::AsyncOperationSPtr HttpServerImpl::LHttpBeginClose(
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<CloseLinuxAsyncServiceOperation>(httpServer_, callback, parent);
}

Common::ErrorCode HttpServerImpl::LHttpEndClose(
    __in Common::AsyncOperationSPtr const& operation)
{
    return CloseLinuxAsyncServiceOperation::End(operation);
}

#endif

ErrorCode HttpServerImpl::RegisterHandler(
    __in std::wstring const& urlSuffix,
    __in IHttpRequestHandlerSPtr const& handler)
{
    PrefixTree::Node &node = prefixTreeUPtr_->RootRef;
    StringCollection tokenizedSuffix;

    //
    // Ths suffix passed in shouldnt contain query string portion - '?<..>'
    //
    StringUtility::Split<wstring>(urlSuffix, tokenizedSuffix, *HttpConstants::QueryStringDelimiter);
    if (tokenizedSuffix.size() != 1)
    {
        return ErrorCodeValue::InvalidAddress;
    }

    tokenizedSuffix.clear();
    StringUtility::Split<wstring>(urlSuffix, tokenizedSuffix, *HttpConstants::SegmentDelimiter);
    ULONG currentIndex = 0;

    AcquireWriteLock writeLock(prefixTreeLock_);

    WriteNoise(TraceType, "Registering Handler for suffix - {0}", urlSuffix);

    auto error = ParseUrlSuffixAndUpdateTree(node, tokenizedSuffix, currentIndex, handler);

    return error;
}

ErrorCode HttpServerImpl::UnRegisterHandler(__in wstring const& urlSuffix, __in IHttpRequestHandlerSPtr const& handler)
{
    //
    // TODO: Handler unregistration.
    //
    UNREFERENCED_PARAMETER(urlSuffix);
    UNREFERENCED_PARAMETER(handler);
    return ErrorCode::Success();
}

ErrorCode HttpServerImpl::Initialize()
{
#if !defined(PLATFORM_UNIX)
    ErrorCode error;
    if (useLookasideAllocator_)
    {
        auto perfCounters = InterlockedPoolPerfCounters::CreateInstance(L"Http", Guid::NewGuid().ToString());

        unique_ptr<InterlockedBufferPool> bufferPool = make_unique<InterlockedBufferPool>();
        bufferPool->Initialize(
            allocatorSettings_.InitialNumberOfAllocations, // having this to be twice the numberOfParallelRequests_ is a good value
            allocatorSettings_.PercentExtraAllocationsToMaintain,
            allocatorSettings_.AllocationBlockSizeInKb);

        error = KHttpServerWrapper::Create(move(bufferPool), perfCounters, httpServer_);
    }
    else
    {
        error = ErrorCode::FromNtStatus(KHttpServer::Create(HttpUtil::GetKtlAllocator(), httpServer_, TRUE));
    }

    if (!error.IsSuccess()) { return error; }
#else
    LHttpServer::Create(httpServer_);
#endif

    return ErrorCodeValue::Success;
}

#if !defined(PLATFORM_UNIX)
void HttpServerImpl::OnRequestReceived(
    KHttpServerRequest::SPtr request)
{
    IRequestMessageContextUPtr reqMessageContextUPtr;
    auto error = this->CreateAndInitializeMessageContext(request, reqMessageContextUPtr);
    if (error.IsSuccess())
    {
        DispatchRequest(move(reqMessageContextUPtr));
    }
    else
    {
        WriteInfo(
            TraceType,
            "Invalid request message context - {0}, continuing listen",
            error);
    }
}

void HttpServerImpl::OnHeaderReadComplete(
    __in KHttpServerRequest::SPtr request,
    __out KHttpServer::HeaderHandlerAction &action)
{
    //
    // Update the header action before calling OnRequestReceived. This is because
    // this action reference is checked when a read is issued the before this
    // function returns.
    //
    action = KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead;

    OnRequestReceived(request);
}

ErrorCode HttpServerImpl::CreateAndInitializeMessageContext(
    __in KHttpServerRequest::SPtr &request,
    __out IRequestMessageContextUPtr &result)
{
    unique_ptr<RequestMessageContext> reqMessageContextUPtr = make_unique<RequestMessageContext>(request);

    auto error = reqMessageContextUPtr->TryParseRequest();

    if (error.IsSuccess())
    {
        result = std::move(reqMessageContextUPtr);
    }

    return error;
}

#else

void HttpServerImpl::OnRequestReceived(http_request & request)
{
    IRequestMessageContextUPtr reqMessageContextUPtr;
    auto error = CreateAndInitializeMessageContext(request, reqMessageContextUPtr);
    if (error.IsSuccess())
    {
        DispatchRequest(move(reqMessageContextUPtr));
    }
    else
    {
        WriteInfo(
            TraceType,
            "Invalid request message context - {0}, continuing listen",
            error);
    }
}

ErrorCode HttpServerImpl::CreateAndInitializeMessageContext(
    __in http_request & request,
    __out IRequestMessageContextUPtr &result)
{
    RequestMessageContextUPtr reqMessageContextUPtr = make_unique<RequestMessageContext>(request);

    auto error = reqMessageContextUPtr->TryParseRequest();

    if (error.IsSuccess())
    {
        result = std::move(reqMessageContextUPtr);
    }

    return error;
}

#endif

void HttpServerImpl::DispatchRequest(__in IRequestMessageContextUPtr messageContext)
{
    StringCollection tokenizedSuffix;
    StringCollection pathSegments;
    wstring suffix = messageContext->GetSuffix();
    IHttpRequestHandlerSPtr handler;
    ULONG token = 0;

    WriteNoise(TraceType, "Dispatching Request for suffix - {0}", suffix);

    StringUtility::Split<wstring>(suffix, tokenizedSuffix, *HttpConstants::QueryStringDelimiter);
    if (tokenizedSuffix.size() != 0)
    {
        StringUtility::Split<wstring>(tokenizedSuffix[0], pathSegments, *HttpConstants::SegmentDelimiter);
    }

    ErrorCode error;
    {
        AcquireReadLock readLock(prefixTreeLock_);
        error = SearchTree(prefixTreeUPtr_->Root, pathSegments, token, handler);
    }

    if (!error.IsSuccess())
    {
        WriteInfo(TraceType, "No Handler found for suffix - {0}", suffix);
        IRequestMessageContextSPtr messageContextSPtr = std::shared_ptr<IRequestMessageContext>(std::move(messageContext));

        ByteBufferUPtr emptyBody;
        AsyncOperationSPtr operation = messageContextSPtr->BeginSendResponse(
            error,
            move(emptyBody),
            [messageContextSPtr](AsyncOperationSPtr const& operation)
        {
            messageContextSPtr->EndSendResponse(operation);
        },
            this->Root.CreateAsyncOperationRoot());

        return;
    }

    AsyncOperationSPtr operation = handler->BeginProcessRequest(
        move(messageContext),
        [handler](AsyncOperationSPtr const& operation)
    {
        handler->EndProcessRequest(operation);
    },
        this->Root.CreateAsyncOperationRoot());
}

ErrorCode HttpServerImpl::ParseUrlSuffixAndUpdateTree(
    __inout PrefixTreeNode &currentNode,
    __in StringCollection const& tokenizedSuffix,
    __inout ULONG &currentIndex,
    __in IHttpRequestHandlerSPtr const& handler)
{
    if (currentIndex == tokenizedSuffix.size())
    {
        if (!currentNode.Data.handler_)
        {
            currentNode.DataRef.handler_ = handler;
            return ErrorCode::Success();
        }
        else
        {
            WriteInfo(
                TraceType,
                "Handler already registered for token - {0}",
                currentNode.Data.token_);

            return ErrorCodeValue::AlreadyExists;
        }
    }

    for (auto index = 0; index < currentNode.Children.size(); ++index)
    {
        PrefixTreeNode &node = currentNode.ChildrenRef[index];
        if (StringUtility::AreEqualCaseInsensitive(node.Data.token_, tokenizedSuffix[currentIndex]))
        {
            currentIndex++;
            return ParseUrlSuffixAndUpdateTree(node, tokenizedSuffix, currentIndex, handler);
        }
    }

    //
    // Special case '*' in suffix as match all.
    //
    if (StringUtility::AreEqualCaseInsensitive(L"*", tokenizedSuffix[currentIndex]))
    {
        currentNode.ChildrenRef.push_back(CreateNode(tokenizedSuffix, currentIndex, handler));
    }
    else
    {
        currentNode.ChildrenRef.insert(currentNode.ChildrenRef.begin(), CreateNode(tokenizedSuffix, currentIndex, handler));
    }

    return ErrorCode::Success();
}

ErrorCode HttpServerImpl::SearchTree(
    __in PrefixTreeNode const &currentNode,
    __in Common::StringCollection const& tokenizedSuffix,
    __inout ULONG &suffixIndex,
    __out IHttpRequestHandlerSPtr &handler)
{
    //
    // Searches for the handler which matches the url suffix by the longest prefix match rule.
    //
    ErrorCode error(ErrorCodeValue::NotFound);
    IHttpRequestHandlerSPtr currentHandler = currentNode.Data.handler_;
    ULONG index = 0;

    if (suffixIndex == tokenizedSuffix.size())
    {
        if (currentHandler)
        {
            handler = currentHandler;
            return ErrorCode::Success();
        }
        return error;
    }

    while (index < currentNode.Children.size())
    {
        PrefixTreeNode const& node = currentNode.Children[index];
        if ((StringUtility::AreEqualCaseInsensitive(node.Data.token_, tokenizedSuffix[suffixIndex])) ||
            (StringUtility::AreEqualCaseInsensitive(node.Data.token_, wstring(L"*"))))
        {
            suffixIndex++;
            error = SearchTree(node, tokenizedSuffix, suffixIndex, handler);
            break;
        }
        else
        {
            index++;
        }
    }

    if (error.IsError(ErrorCodeValue::NotFound))
    {
        if (currentHandler)
        {
            handler = currentHandler;
            error = ErrorCode::Success();
        }
    }

    return error;
}

HttpServerImpl::PrefixTreeNode HttpServerImpl::CreateNode(
    __in StringCollection const& tokenizedSuffix,
    __in ULONG &currentIndex,
    __in IHttpRequestHandlerSPtr const& handler)
{
    PrefixTreeNode node = PrefixTreeNode(NodeData(tokenizedSuffix[currentIndex]));

    PrefixTreeNode *currentNode = &node;
    currentIndex++;

    while (currentIndex < tokenizedSuffix.size())
    {
        PrefixTreeNode childNode = PrefixTreeNode(NodeData(tokenizedSuffix[currentIndex]));
        currentNode->ChildrenRef.push_back(childNode);
        currentNode = &currentNode->ChildrenRef[0];
        currentIndex++;
    }

    currentNode->DataRef.handler_ = handler;

    return move(node);
}
