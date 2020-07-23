// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;

shared_ptr<HttpServerImpl> httpServer;

void CreateAndOpen(AutoResetEvent &openEvent, ComponentRootSPtr &root, ULONG);
ErrorCode OnOpenComplete(AsyncOperationSPtr const &operation);
void Close(ComponentRootSPtr &root);
ErrorCode OnCloseComplete(AsyncOperationSPtr const &operation);
string expectedRequestString = "data store is designed is subtly--and yet profoundly--different than how an object system is designed.Object systems are typically characterized by four basic components : identity, state, behavior and encapsulation.Identity is an implicit concept in most O - O languages, in that a given object has a unique identity that is distinct from its state(the value of its internal fields)--two objects with the same state are still separate and distinct objects, despite being bit - for - bit mirrors of one another.This is the identity vs. equivalence discussion that occurs in languages like C++, C# or Java, where developers must distinguish between \"a == b\" and \"a.equals(b)\".The behavior of an object is fairly easy to see, a collection of operations clients can invoke to manipulate, examine, or interact with objects in some fashion. (This is what distinguishes objects from passive data structures in a procedural language like C.) Encapsulation is a key detail, preventing outside parties from manipulating internal object details, thus providing evolutionary capabilities to the object's interface to clients.3. From this we can derive more interesting concepts, such as type, a formal declaration of object state and behavior, association, allowing types to reference one another through a lightweight reference rather than complete by-value ownership (sometimes called composition), inheritance, the ability to relate one type to another such that the relating type incorporates all of the related type's state and behavior as part of its own, and polymorphism, the ability to substitute an object in where a different type is expected.Relational systems describe a form of knowledge storage and retrieval based on predicate logic and truth statements.In essence, each row within a table is a declaration about a fact in the world, and SQL allows for operator-efficient data retrieval of those facts using predicate logic to create inferences from those facts.[Date04] and[Fussell] define the relational model as characterized by relation, attribute, tuple, relation value and relation variable.A relation is, at its heart, a truth predicate about the world, a statement of facts(attributes) that provide meaning to the predicate.For example, we may define the relation \"PERSON\" as{ SSN, Name, City }, which states that \"there exists a PERSON with a Social Security Number SSN who lives in City and is called Name\".Note that in a relation, attribute ordering is entirely unspecified.A tuple is a truth statement within the context of a relation, a set of attribute values that match the required set of attributes in the relation, such as \"{PERSON SSN='123-45-6789' Name='Catherine Kennedy' City='Seattle'}\".Note that two tuples are considered identical if their relation and attribute values are also identical.A relation value, then, is a combination of a relation and a set of tuples that match that relation, and a relation variable is, like most variables, a placeholder for a given relation, but can change value over time.Thus, a relation variable People can be written to hold the relation{ PERSON }, and consist of the relation value";
string abortRequest = "Abort request";

class HandleRequestAsyncOperation : public AsyncOperation
{
public:
    HandleRequestAsyncOperation(
        __in IRequestMessageContextUPtr request,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
        : AsyncOperation(callback, parent)
        , request_(move(request))
    {
    }

    static ErrorCode HandleRequestAsyncOperation::End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<HandleRequestAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        wstring upgradeHeaderValue;
        auto error = request_->GetRequestHeader(HttpConstants::UpgradeHeader, upgradeHeaderValue);
        if (error.IsSuccess())
        {
            HandleWebSocketCommunication(thisSPtr);
            return;
        }

        if (request_->GetVerb() != *HttpConstants::HttpPostVerb)
        {
            ByteBufferUPtr emptyBody;
            printf("Not a POST request\n");
            OnSendResponse(thisSPtr, ErrorCodeValue::InvalidArgument, move(emptyBody));
            return;
        }

        request_->BeginGetMessageBody(
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnGetMessageBodyComplete(operation);
            },
            thisSPtr);
    }

    void OnGetMessageBodyComplete(AsyncOperationSPtr const &operation)
    {
        ByteBufferUPtr body;
        auto error = this->request_->EndGetMessageBody(operation, body);
        if (!error.IsSuccess())
        {
            printf("GetBody failed\n");
            ByteBufferUPtr emptyBody;
            OnSendResponse(operation->Parent, ErrorCodeValue::InvalidArgument, move(emptyBody));
            return;
        }

        string requestString(reinterpret_cast<CHAR *>((*body).data()), body->size());
        if (requestString == abortRequest)
        {
            printf("Aborting request\n");
            AbortRequest(operation->Parent, ErrorCodeValue::Success);
            return;
        }
        else if (requestString != expectedRequestString)
        {
            printf("Request string \"-- %s --\" didnt match what was expected \n", requestString.c_str());
            ByteBufferUPtr emptyBody;
            OnSendResponse(operation->Parent, ErrorCodeValue::InvalidArgument, move(emptyBody));
            return;
        }

        SendResponse(operation->Parent);
    }

    void SendResponse(AsyncOperationSPtr const& thisSPtr)
    {
        string responseString = "<b><i>Hello from Gateway</b></i>";
        ByteBufferUPtr bodyBytes = make_unique<ByteBuffer>();

        bodyBytes->reserve(responseString.length());
        BYTE *byte = (BYTE *)responseString.c_str();
        for (int i = 0; i < responseString.length(); ++i)
        {
            bodyBytes->push_back(byte[i]);
        }

        request_->SetResponseHeader(HttpConstants::ContentTypeHeader, L"text/html");
        OnSendResponse(thisSPtr, ErrorCodeValue::Success, move(bodyBytes));
    }

    void OnSendResponse(AsyncOperationSPtr const &thisSPtr, ErrorCode error, ByteBufferUPtr body)
    {
        request_->BeginSendResponse(
            error,
            move(body),
            [this, error](AsyncOperationSPtr const &operation)
        {
            auto error1 = ErrorCode::FirstError(error, this->request_->EndSendResponse(operation));
            if (!error1.IsSuccess())
            {
                printf("EndSendResponse failed with %d\n", error.ReadValue());
            }

            this->TryComplete(operation->Parent, error1);
        },
            thisSPtr);
    }

    void AbortRequest(AsyncOperationSPtr const &thisSPtr, ErrorCode const &error)
    {
        KMemRef body(0, nullptr);

        request_->BeginSendResponseChunk(
            body,
            true,
            true,
            [this, error](AsyncOperationSPtr const &operation)
        {
            auto error1 = ErrorCode::FirstError(error, this->request_->EndSendResponseChunk(operation));
            if (!error1.IsSuccess())
            {
                printf("Abort failed with %d\n", error.ReadValue());
            }

            this->TryComplete(operation->Parent, error1);
        },
            thisSPtr);
    }

private:
    void HandleWebSocketCommunication(AsyncOperationSPtr const &thisSPtr)
    {
        auto status = KBuffer::Create(1024, receiveBuffer_, HttpUtil::GetKtlAllocator());
        status = KStringA::Create(ktlSubProtocolAnsiString_, HttpUtil::GetKtlAllocator(), "", TRUE);

        requestAsyncOperation_ = thisSPtr;
        webSocketHandler_ = make_shared<HttpServerWebSocket>(*request_, *ktlSubProtocolAnsiString_);
        webSocketHandler_->BeginOpen(
            [this]()
            {
                this->WebSocketCloseReceived();
            },
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnWebSocketOpenComplete(operation);
            },
            thisSPtr);
    }

    void OnWebSocketOpenComplete(AsyncOperationSPtr const &operation)
    {
        auto error = webSocketHandler_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            printf("WebSocket upgrade failed with %d\n", error.ReadValue());
            TryComplete(operation->Parent, error);
            return;
        }

        
        printf("websocket upgrade complete.. starting receive \n");
        DoReceive(operation->Parent);
    }

    void DoReceive(AsyncOperationSPtr const &thisSPtr)
    {
        webSocketHandler_->BeginReceiveFragment(
            receiveBuffer_,
            [this](AsyncOperationSPtr operation)
            {
                this->OnReceiveComplete(operation);
            },
            thisSPtr);
    }

    void OnReceiveComplete(AsyncOperationSPtr const &operation)
    {
        bool isFinalFragment;
        KWebSocket::MessageContentType contentType;
        ULONG bytesReceived;
        KBuffer::SPtr buffer;
        auto error = webSocketHandler_->EndReceiveFragment(
            operation, 
            buffer, 
            &bytesReceived, 
            isFinalFragment, 
            contentType);

        if (!error.IsSuccess())
        {
            printf("Receive failed %d", error.ReadValue());
            CloseWebSocket(KWebSocket::CloseStatusCode::ProtocolError, "receive failure");
            return;
        }

        DoSend(operation->Parent, bytesReceived, isFinalFragment, contentType);
    }

    void DoSend(AsyncOperationSPtr const &thisSPtr, ULONG bytesToSend, bool isFinalFragment, KWebSocket::MessageContentType contentType)
    {
        webSocketHandler_->BeginSendFragment(
            receiveBuffer_,
            bytesToSend,
            isFinalFragment,
            contentType,
            [this](AsyncOperationSPtr operation)
        {
            this->OnSendComplete(operation);
        },
            thisSPtr);
    }

    void OnSendComplete(AsyncOperationSPtr const &operation)
    {
        auto error = webSocketHandler_->EndSendFragment(operation);
        if (!error.IsSuccess())
        {
            printf("Send failed %d \n", error.ReadValue());
            CloseWebSocket(KWebSocket::CloseStatusCode::ProtocolError, "send failure");
            return;
        }

        DoReceive(operation->Parent);
    }

    void WebSocketCloseReceived()
    {
        printf("Received Close\n");
        KWebSocket::CloseStatusCode statusCode;
        KSharedBufferStringA::SPtr clientCloseReason;
        auto error = webSocketHandler_->GetRemoteCloseStatus(statusCode, clientCloseReason);
        if (error.IsSuccess())
        {
            CloseWebSocket(statusCode, clientCloseReason);
        }
        else
        {
            printf("Getting remote close status failed - %d", error.ReadValue());
        }
    }

    void CloseWebSocket(KWebSocket::CloseStatusCode statusCode, string reason)
    {
        KSharedBufferStringA::SPtr clientCloseReason;
        KSharedBufferStringA::Create(clientCloseReason, reason.c_str(), HttpUtil::GetKtlAllocator());
        CloseWebSocket(statusCode, clientCloseReason);
    }

    void CloseWebSocket(KWebSocket::CloseStatusCode statusCode, KSharedBufferStringA::SPtr clientCloseReason)
    {
        AsyncOperationSPtr parent;
        {
            AcquireReadLock readLock(requestAsyncOperationLock_);
            if (requestAsyncOperation_)
            {
                parent = requestAsyncOperation_;
            }
        }

        if (parent)
        {
            printf("Sending close %d --- %s\n", statusCode, clientCloseReason->operator char *());
            webSocketHandler_->BeginClose(
                statusCode,
                clientCloseReason,
                [this](AsyncOperationSPtr const &operation)
                {
                    this->OnCloseComplete(operation);
                },
                parent);
        }
        else
        {
            printf("Close already sent\n");
        }
    }

    void OnCloseComplete(AsyncOperationSPtr const &operation)
    {
        auto error = webSocketHandler_->EndClose(operation);
        printf("Close completed - %d", error.ReadValue());
    
        TryComplete(operation->Parent, error);
        {
            AcquireReadLock readLock(requestAsyncOperationLock_);
            if (requestAsyncOperation_)
            {
                requestAsyncOperation_.reset();
            }
        }
    }

    RwLock requestAsyncOperationLock_;
    AsyncOperationSPtr requestAsyncOperation_;
    IWebSocketHandlerSPtr webSocketHandler_;
    IRequestMessageContextUPtr request_;
    KBuffer::SPtr receiveBuffer_;
    KStringA::SPtr ktlSubProtocolAnsiString_;
};

class TestRequestHandler : public IHttpRequestHandler
{
public:
    TestRequestHandler()
        : requestCount_(0)
    {}

    AsyncOperationSPtr BeginProcessRequest(
        __in IRequestMessageContextUPtr request,
        AsyncCallback const& callback,
        AsyncOperationSPtr const& parent)
    {
        return AsyncOperation::CreateAndStart<HandleRequestAsyncOperation>(move(request), callback, parent);
    }

    ErrorCode EndProcessRequest(AsyncOperationSPtr const& operation)
    {
        requestCount_++;
        if ((requestCount_ % 1000) == 0)
        {
            printf("Processed %d requests...\n", requestCount_);
        }

        return HandleRequestAsyncOperation::End(operation);
    }

private:
    ULONG requestCount_;
};

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
    CommonConfig::GetConfig(); // load configuration
    ULONG numberOfParallelRequests = 100;

    if (argc == 2)
    {
        numberOfParallelRequests = _wtoi(argv[1]);
    }

    AutoResetEvent openEvent;
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    Threadpool::Post([numberOfParallelRequests, &openEvent, &root]()
    {
        CreateAndOpen(openEvent, root, numberOfParallelRequests);
    });

    openEvent.WaitOne();

    IHttpRequestHandlerSPtr handler = make_shared<TestRequestHandler>();

    httpServer->RegisterHandler(L"/", handler);

    printf("Press any key to terminate..\n");
    wchar_t singleChar;
    wcin.getline(&singleChar, 1);

    Close(root);

    httpServer.reset();
}

void CreateAndOpen(AutoResetEvent &openEvent, ComponentRootSPtr &root, ULONG numberOfParallelRequests)
{
    httpServer = make_shared<HttpServerImpl>(
            *root,
            L"http://+:19007/mytest",
            numberOfParallelRequests,
            Transport::SecurityProvider::None);

    httpServer->BeginOpen(
        TimeSpan::FromMinutes(2),
        [&openEvent](AsyncOperationSPtr const &operation)
        {
            OnOpenComplete(operation);
            openEvent.Set();
        });
}

ErrorCode OnOpenComplete(AsyncOperationSPtr const & operation)
{
    ErrorCode e = httpServer->EndOpen(operation);
    if (e.IsSuccess())
    {
        printf("Open successful ...\n");
    }
    else
    {
        wprintf(L"Open failed - %s \n", e.ErrorCodeValueToString().c_str());
    }

    return e;
}

void Close(ComponentRootSPtr &root)
{
    if (!httpServer)
    {
        return;
    }

    HttpServerImpl & httpServerImpl = (HttpServerImpl&)(*httpServer);
    AutoResetEvent closeEvent;

    httpServerImpl.BeginClose(
        TimeSpan::FromMinutes(2),
        [&closeEvent, &httpServerImpl](AsyncOperationSPtr const &operation)
        {
            auto error = httpServerImpl.EndClose(operation);
            closeEvent.Set();
        },
        root->CreateAsyncOperationRoot());


    closeEvent.WaitOne();
}
