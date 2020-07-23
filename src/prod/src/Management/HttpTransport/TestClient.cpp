// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpClient;
using namespace HttpCommon;

void SendRequests(IHttpClientSPtr client, ComponentRootSPtr const& root);
void StartThreads(IHttpClientSPtr &client, ComponentRootSPtr const& root, vector<unique_ptr<AutoResetEvent>> &threads);
void StartHttpClient(int numberOfThreads);
ByteBufferUPtr GetBytesToSend(string const&);
void WebSocketCloseReceived(shared_ptr<HttpClientWebSocket>);
void WebsocketClient();
void AbortRequestClient();
void ContainerAttach(wstring const & url, wstring const & clientCertThumbprint = wstring());

string requestString = "data store is designed is subtly--and yet profoundly--different than how an object system is designed.Object systems are typically characterized by four basic components : identity, state, behavior and encapsulation.Identity is an implicit concept in most O - O languages, in that a given object has a unique identity that is distinct from its state(the value of its internal fields)--two objects with the same state are still separate and distinct objects, despite being bit - for - bit mirrors of one another.This is the identity vs. equivalence discussion that occurs in languages like C++, C# or Java, where developers must distinguish between \"a == b\" and \"a.equals(b)\".The behavior of an object is fairly easy to see, a collection of operations clients can invoke to manipulate, examine, or interact with objects in some fashion. (This is what distinguishes objects from passive data structures in a procedural language like C.) Encapsulation is a key detail, preventing outside parties from manipulating internal object details, thus providing evolutionary capabilities to the object's interface to clients.3. From this we can derive more interesting concepts, such as type, a formal declaration of object state and behavior, association, allowing types to reference one another through a lightweight reference rather than complete by-value ownership (sometimes called composition), inheritance, the ability to relate one type to another such that the relating type incorporates all of the related type's state and behavior as part of its own, and polymorphism, the ability to substitute an object in where a different type is expected.Relational systems describe a form of knowledge storage and retrieval based on predicate logic and truth statements.In essence, each row within a table is a declaration about a fact in the world, and SQL allows for operator-efficient data retrieval of those facts using predicate logic to create inferences from those facts.[Date04] and[Fussell] define the relational model as characterized by relation, attribute, tuple, relation value and relation variable.A relation is, at its heart, a truth predicate about the world, a statement of facts(attributes) that provide meaning to the predicate.For example, we may define the relation \"PERSON\" as{ SSN, Name, City }, which states that \"there exists a PERSON with a Social Security Number SSN who lives in City and is called Name\".Note that in a relation, attribute ordering is entirely unspecified.A tuple is a truth statement within the context of a relation, a set of attribute values that match the required set of attributes in the relation, such as \"{PERSON SSN='123-45-6789' Name='Catherine Kennedy' City='Seattle'}\".Note that two tuples are considered identical if their relation and attribute values are also identical.A relation value, then, is a combination of a relation and a set of tuples that match that relation, and a relation variable is, like most variables, a placeholder for a given relation, but can change value over time.Thus, a relation variable People can be written to hold the relation{ PERSON }, and consist of the relation value";
string abortString = "Abort request";

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    HeapSetInformation(GetProcessHeap(), HeapEnableTerminationOnCorruption, NULL, 0);
    CommonConfig::GetConfig(); // load configuration
    int numberOfThreads = 30;

    if (argc >= 2)
    {
        if (wcscmp(L"Http", argv[1]) == 0)
        {
            printf("Starting http client\n");
            StartHttpClient(numberOfThreads);
        }
        else if (wcscmp(L"Websocket", argv[1]) == 0)
        {
            printf("Starting websocket client\n");
            WebsocketClient();
        }
        else if (wcscmp(L"AbortRequest", argv[1]) == 0)
        {
            printf("Starting client for abort request\n");
            AbortRequestClient();
        }
        else if (wcscmp(L"ContainerAttach", argv[1]) == 0)
        {
            if (argc < 3)
            {
                //Command line examples:
                //HttpTransport.TestClient.exe ContainerAttach "ws://sfcontainerdev:19007/Nodes/Node03/$/ContainerApi/containers/65b2d3900219/attach/ws?logs=1&stdout=1&stdin=1&stderr=1&stream=1"
                //HttpTransport.TestClient.exe ContainerAttach "ws://sfcontainerdev:19007/Nodes/Node03/$/ContainerApi/containers/65b2d3900219/attach/ws?logs=1&stdout=1&stdin=1&stderr=1&stream=1" D100CA0577F471A4B9525CC561E765C177300979
                printf(".exe ConsoleAttach <wsURL> [ClientCertThumbprint]\n");
                return -1;
            }

            if (argc < 4)
            {
                ContainerAttach(argv[2]);
            }
            else
            {
                ContainerAttach(argv[2], argv[3]);
            }
            return 0;
        }
        else
        {
            printf(".exe <Http>/<WebSocket> \n");
            return -1;
        }
    }
    else
    {
        StartHttpClient(numberOfThreads);
    }

    getchar();

    return 0;
}

void WebsocketClient()
{
    auto websocket = make_shared<HttpClientWebSocket>(L"ws://localhost:19007/mytest", L"", HttpUtil::GetKtlAllocator());
    ComponentRootSPtr root = make_shared<ComponentRoot>();

    printf("Opening websocket..\n");
    AutoResetEvent event;
    auto operation = websocket->BeginOpen(
        [websocket]()
        {
            printf("Close callback received");
            WebSocketCloseReceived(websocket);
        },
        [&event](AsyncOperationSPtr const &)
        {
            event.Set();
        },
            root->CreateAsyncOperationRoot());

    event.WaitOne();

    auto error = websocket->EndOpen(operation);
    printf("Open completed with %d\n", error.ReadValue());

    if (!error.IsSuccess())
    {
        return;
    }

    auto requestBody = GetBytesToSend(requestString);
    KBuffer::SPtr buffer;
    KBuffer::Create(static_cast<ULONG>(requestBody->size()), buffer, HttpUtil::GetKtlAllocator());
    byte* buf = (byte *)buffer->GetBuffer();
    for (int i = 0; i < requestBody->size(); ++i)
    {
         *buf = (*requestBody)[i];

    }

    printf("Sending message\n");

    auto contentType = KWebSocket::MessageContentType::Binary;
    operation = websocket->BeginSendFragment(
        buffer,
        static_cast<ULONG>(requestBody->size()),
        true,
        contentType,
        [&event](AsyncOperationSPtr const &)
        {
            event.Set();
        },
        root->CreateAsyncOperationRoot());

    event.WaitOne();

    error = websocket->EndSendFragment(operation);
    printf("Sending message completed with %d\n", error.ReadValue());

    if (!error.IsSuccess())
    {
        return;
    }

    KBuffer::SPtr fragmentBuffer;
    auto status = KBuffer::Create(512, fragmentBuffer, HttpUtil::GetKtlAllocator());
    UNREFERENCED_PARAMETER(status);

    ByteBuffer receivedMessage;

    printf("Receiving message \n");
    while (true)
    {
        operation = websocket->BeginReceiveFragment(
            fragmentBuffer,
            [&event](AsyncOperationSPtr const &)
            {
                event.Set();
            },
            root->CreateAsyncOperationRoot());

        event.WaitOne();

        ULONG bytesReceived;
        bool isFinalFragment;
        KBuffer::SPtr tempBuffer;
        error = websocket->EndReceiveFragment(operation, tempBuffer, &bytesReceived, isFinalFragment, contentType);
        printf("Receive fragment completed with %d, bytes received %d\n", error.ReadValue(), bytesReceived);

        if (!error.IsSuccess())
        {
            return;
        }

        if (contentType != KWebSocket::MessageContentType::Binary)
        {
            printf("Content type not binary\n");
            return;
        }

        byte *fragBuf = (byte*)fragmentBuffer->GetBuffer();
        for (ULONG i = 0; i < bytesReceived; ++i)
        {
            receivedMessage.push_back(*fragBuf);
            fragBuf++;
        }

        if (isFinalFragment)
        {
            break;
        }
    }

    if (receivedMessage.size() != requestBody->size())
    {
        printf("Received message size doesn't match %d expected - %d\n", 
            static_cast<int>(receivedMessage.size()), 
            static_cast<int>(requestBody->size()));
        return;
    }
    else
    {
        printf("Received message of size %d\n", 
            static_cast<int>(receivedMessage.size()));
    }

    printf("Initiating close");
    WebSocketCloseReceived(websocket);
}

void WebSocketCloseReceived(shared_ptr<HttpClientWebSocket> clientWebSocketSPtr)
{
    KSharedBufferStringA::SPtr clientCloseReason;
    KSharedBufferStringA::Create(clientCloseReason, "going away", HttpUtil::GetKtlAllocator());

    AutoResetEvent event;
    auto statusCode = KWebSocket::CloseStatusCode::GoingAway;
    auto operation = clientWebSocketSPtr->BeginClose(
        statusCode,
        clientCloseReason,
        [&event](AsyncOperationSPtr const &)
        {
            event.Set();
        },
        AsyncOperationSPtr());

    event.WaitOne();

    auto error = clientWebSocketSPtr->EndClose(operation);

    if (!error.IsSuccess())
    {
        printf("Close failed with %d\n", error.ReadValue());
        return;
    }

    printf("Close Succeeded\n");
}

void StartHttpClient(int numberOfThreads)
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    IHttpClientSPtr client;

    auto error = HttpClientImpl::CreateHttpClient(
        L"TestClient",
        *root,
        client);

    if (!error.IsSuccess())
    {
        printf("Creating client failed with %S\n", error.ErrorCodeValueToString().c_str());
        return;
    }

    vector<unique_ptr<AutoResetEvent>> threads;
    for (int i = 0; i < numberOfThreads; i++)
    {
        threads.push_back(make_unique<AutoResetEvent>());
    }

    Stopwatch stopWatch;
    stopWatch.Start();
    StartThreads(client, root, threads);

    HANDLE handles[MAXIMUM_WAIT_OBJECTS];

    for (int i = 0; i < threads.size(); ++i)
    {
        handles[i] = (*threads[i]).GetHandle();
    }

    ::DWORD status = ::WaitForMultipleObjects(static_cast<DWORD>(threads.size()), handles, TRUE, INFINITE);

    stopWatch.Stop();

    if (status < WAIT_OBJECT_0 + threads.size())
        printf("Succeeded... \n");
    else
        printf("failed \n");

    printf("Total time elapsed %ld seconds\n", static_cast<long>(TimeSpan::FromMilliseconds((double)stopWatch.ElapsedMilliseconds).TotalSeconds()));
}

void StartThreads(IHttpClientSPtr &client1, ComponentRootSPtr const& root, vector<unique_ptr<AutoResetEvent>> &threads)
{
    UNREFERENCED_PARAMETER(client1);

    printf("Starting %d threads ... \n", static_cast<int>(threads.size()));
    TimeSpan timeout = TimeSpan::FromMinutes(1);

    for (int i = 0; i < threads.size(); ++i)
    {
        AutoResetEvent & threadEvent = *threads[i];
        IHttpClientSPtr client;

        HttpClientImpl::CreateHttpClient(
            L"TestClient",
            *root,
            client);

        Threadpool::Post(
            [&threadEvent, client, &root]()
        {
            SendRequests(client, root);
            threadEvent.Set();
        });
    }

    printf("Started %d threads ... \n", static_cast<int>(threads.size()));
}

void SendRequests(IHttpClientSPtr client, ComponentRootSPtr const& root)
{
    IHttpClientRequestSPtr clientRequest;
    printf("Starting 5000 requests\n");

    TimeSpan timeout = TimeSpan::FromMinutes(1);

    for (int i = 0; i < 5000; ++i)
    {
        auto error = client->CreateHttpRequest(
                L"http://localhost:19007/mytest",
                timeout,
                timeout,
                timeout,
                HttpUtil::GetKtlAllocator(),
                clientRequest);

        if (!error.IsSuccess())
        {
            Assert::CodingError("Creating client request failed with {0}", error);
        }

        clientRequest->SetVerb(*HttpConstants::HttpPostVerb);

        AutoResetEvent event;

        ByteBufferUPtr requestBody = GetBytesToSend(requestString);
        
        clientRequest->SetRequestHeader(*HttpConstants::ContentTypeHeader, L"application/text");

        auto operation = clientRequest->BeginSendRequest(
            move(requestBody),
            [&event](AsyncOperationSPtr const &)
            {
                event.Set();
            },
            root->CreateAsyncOperationRoot());

        event.WaitOne();
        ULONG winHttpError;
        error = clientRequest->EndSendRequest(operation, &winHttpError);

        if (!error.IsSuccess())
        {
            if (winHttpError == ERROR_WINHTTP_CANNOT_CONNECT)
            {
                printf("Client request failed because it cannot connect to the server\n");
                continue;
                //Assert::CodingError("Client request failed because it cannot connect to the server");
            }
            else if (winHttpError == ERROR_WINHTTP_TIMEOUT)
            {
                Assert::CodingError("Client request failed because the server didnt respond on time");
            }
            else if (winHttpError == ERROR_WINHTTP_CONNECTION_ERROR)
            {
                printf("Client request failed because of connection error\n");
                continue;
            }

            Assert::CodingError("Client request failed with winHttpError - {0}", winHttpError);
        }

        operation = clientRequest->BeginGetResponseBody(
            [&event](AsyncOperationSPtr const &)
            {
                event.Set();
            },
            root->CreateAsyncOperationRoot());

        event.WaitOne();
        ByteBufferUPtr body;
        error = clientRequest->EndGetResponseBody(operation, body);

        if (error.IsError(ErrorCodeValue::NotFound) || !error.IsSuccess())
        {
            USHORT statusCode;
            wstring description;
            clientRequest->GetResponseStatusCode(&statusCode, description);
            Assert::CodingError("Client request failed with {0}", statusCode);
        }

        string message(reinterpret_cast<CHAR *>((*body).data()), body->size());
        if (message != "<b><i>Hello from Gateway</b></i>")
        {
            USHORT statusCode;
            wstring description;
            clientRequest->GetResponseStatusCode(&statusCode, description);

            Assert::CodingError("Invalid message from server - got {0}", message);
        }
    }
    printf("Finishing 1000 requests\n");
}

void AbortRequestClient()
{
    ComponentRootSPtr root = make_shared<ComponentRoot>();
    IHttpClientSPtr client;

    auto error = HttpClientImpl::CreateHttpClient(
        L"AbortRequestClient",
        *root,
        client);

    if (!error.IsSuccess())
    {
        printf("Creating client failed with %S\n", error.ErrorCodeValueToString().c_str());
        return;
    }

    IHttpClientRequestSPtr clientRequest;
    TimeSpan timeout = TimeSpan::FromMinutes(1);

    error = client->CreateHttpRequest(
        L"http://localhost:19007/mytest",
        timeout,
        timeout,
        timeout,
        HttpUtil::GetKtlAllocator(),
        clientRequest);

    if (!error.IsSuccess())
    {
        Assert::CodingError("Creating client request failed with {0}", error);
    }

    clientRequest->SetVerb(*HttpConstants::HttpPostVerb);

    AutoResetEvent event;

    ByteBufferUPtr requestBody = GetBytesToSend(abortString);

    clientRequest->SetRequestHeader(*HttpConstants::ContentTypeHeader, L"application/text");

    auto operation = clientRequest->BeginSendRequest(
        move(requestBody),
        [&event](AsyncOperationSPtr const &)
        {
            event.Set();
        },
        root->CreateAsyncOperationRoot());

    event.WaitOne();
    ULONG winHttpError;
    error = clientRequest->EndSendRequest(operation, &winHttpError);

    if (!error.IsSuccess())
    {
        if (winHttpError == ERROR_WINHTTP_CANNOT_CONNECT)
        {
            printf("Client request failed because it cannot connect to the server\n");
            return;
        }
        else if (winHttpError == ERROR_WINHTTP_TIMEOUT)
        {
            Assert::CodingError("Client request failed because the server didnt respond on time");
            return;
        }
        else if (winHttpError == ERROR_WINHTTP_CONNECTION_ERROR)
        {
            printf("Client request failed because of connection error\n");
            return;
        }
        Assert::CodingError("Client request failed with winHttpError - {0}", winHttpError);
    }

    operation = clientRequest->BeginGetResponseBody(
        [&event](AsyncOperationSPtr const &)
        {
            event.Set();
        },
        root->CreateAsyncOperationRoot());

    event.WaitOne();
    ByteBufferUPtr body;
    error = clientRequest->EndGetResponseBody(operation, body);

    if (error.IsError(ErrorCodeValue::NotFound) || !error.IsSuccess())
    {
        USHORT statusCode;
        wstring description;
        clientRequest->GetResponseStatusCode(&statusCode, description);
        Assert::CodingError("Client request failed with {0}", statusCode);
    }

    printf("Reached the end of AbortClient");
}

ByteBufferUPtr GetBytesToSend(string const &requestBodyString)
{
    ByteBufferUPtr requestBody = make_unique<ByteBuffer>();;
    BYTE *byte = (BYTE *)requestBodyString.c_str();
    for (int i = 0; i < requestBodyString.length(); ++i)
    {
        requestBody->push_back(byte[i]);
    }

    return requestBody;
}

static bool detachSequenceReceived = false;

ByteBufferUPtr ContainerAttach_GetInput()
{
    static bool ctrlpReceived = false;

    ByteBufferUPtr requestBody = make_unique<ByteBuffer>();
    auto ch = (char)_getch();

    if (ch == 17 /*ctrl-q*/)
    {
        if (ctrlpReceived)
        {
            detachSequenceReceived = true;
        }
    }
    else
    {
        ctrlpReceived = (ch == 16 /*ctrl-p*/);
    }

    requestBody->push_back(ch);
    return requestBody;
}

DWORD ContainerAttach_GetDwordFromBytes(byte const *b)
{
    return (b[3]) | (b[2] << 8) | (b[1] << 16) | (b[0] << 24);
}

void ContainerAttach_ProcessIncomingStream(ByteBuffer const & body)
{
    if (body.empty()) return;

    int index = 0;
    while (index < body.size())
    {
        auto b = body[index];

        if (b == 0 || b == 1 || b == 2) // check for docker stream headers in non-tty mode
        {
            DWORD size = ContainerAttach_GetDwordFromBytes(&(body[index + 4]));
            string str((char *)(&body[index + 8]), size);
            if (b == 2)
            {
                std::cerr << str;
            }
            else if (b == 0)
            {
                std::cout << str;
            }
            else if (b == 1)
            {
                std::cout << str;
            }

            index = index + 8 + size;
            continue;
        }

        //tty mode
        index++;
        if (b == 0x1b)
        {
            b = body[index++];
            if (b == 0x5d /* ']' */)
            {
                // skip OSC sequenece
                while ((index < body.size()) && (body[index++] != 0x7));
            }
            else if (b == 0x5b /* '[' */)
            {
                // skip escape sequenece
                while (index < body.size())
                {
                    auto ch = (char)(body[index++]);
                    if ((ch == 'm') || (ch == 'K') || (ch == 'H') || (ch == 'f') || (ch == 'l') || (ch == 'h'))
                    {
                        break;
                    }
                }
            }
            else
            {
                std::cout << "unknown escape code: " << hex << b << endl;
            }
        }
        else
        {
            std::cout << b;
        }
    }
}

BOOLEAN ValidateServerCertificate(PVOID, PCCERT_CONTEXT)
{
    return TRUE;
}

void ContainerAttach(wstring const & url, wstring const & clientCertThumbprint)
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
    KFinally([=] { SetConsoleMode(hStdin, mode); });

    CertContextUPtr clientCert;
    if (!clientCertThumbprint.empty())
    {
        X509FindValue::SPtr findValue;
        auto err = X509FindValue::Create(X509FindType::FindByThumbprint, clientCertThumbprint, findValue);
        if (!err.IsSuccess())
        {
            wcout << "Failed to convert '" << clientCertThumbprint << "' to thumbprint: " << err.ErrorCodeValueToString() << endl;
            exit(1);
        }

        err = CryptoUtility::GetCertificate(
            X509StoreLocation::CurrentUser,
            X509Default::StoreName(),
            findValue,
            clientCert);

        if (!err.IsSuccess())
        {
            wcout << "Failed to load certificate '" << clientCertThumbprint << "' : " << err.ErrorCodeValueToString() << endl;
            exit(1);
        }
    }

    KHttpClientWebSocket::ServerCertValidationHandler handler(nullptr, ValidateServerCertificate);

    auto websocket = make_shared<HttpClientWebSocket>(
        url,
        L"",
        HttpUtil::GetKtlAllocator(),
        4096,
        4096,
        5000, // 5 seconds
        10000, // 10 seconds - WINHTTP_OPTION_WEB_SOCKET_CLOSE_TIMEOUT
        nullptr,
        clientCert.get(),
        handler);

    ComponentRootSPtr root = make_shared<ComponentRoot>();
    {
        wcout << "[opening] " << url;
        AutoResetEvent openCompleteEvent;
        auto operation = websocket->BeginOpen(
            [websocket]() { WebSocketCloseReceived(websocket); },
            [&openCompleteEvent](AsyncOperationSPtr const &) { openCompleteEvent.Set(); },
            root->CreateAsyncOperationRoot());

        openCompleteEvent.WaitOne();

        auto error = websocket->EndOpen(operation);
        if (!error.IsSuccess())
        {
            printf("\n[failed] error: 0x%x\n", error.ReadValue());
            return;
        }

        printf("\n\n");
    }

    Threadpool::Post([websocket, root]
    {
        auto contentType = KWebSocket::MessageContentType::Binary;

        for (;;)
        {
            KBuffer::SPtr fragmentBuffer;
            auto status = KBuffer::Create(512, fragmentBuffer, HttpUtil::GetKtlAllocator());
            KInvariant(SUCCEEDED(status));
            ByteBuffer receivedMessage;

            for (;;)
            {
                AutoResetEvent receiveCompleteEvent;
                auto operation = websocket->BeginReceiveFragment(
                    fragmentBuffer,
                    [&receiveCompleteEvent](AsyncOperationSPtr const &)
                {
                    receiveCompleteEvent.Set();
                },
                    root->CreateAsyncOperationRoot());

                receiveCompleteEvent.WaitOne();

                ULONG bytesReceived;
                bool isFinalFragment;
                KBuffer::SPtr tempBuffer;
                auto error = websocket->EndReceiveFragment(operation, tempBuffer, &bytesReceived, isFinalFragment, contentType);
                if (!error.IsSuccess())
                {
                    return;
                }

                byte *fragBuf = (byte*)fragmentBuffer->GetBuffer();
                for (ULONG i = 0; i < bytesReceived; ++i)
                {
                    receivedMessage.push_back(*fragBuf);
                    fragBuf++;
                }

                if (isFinalFragment)
                {
                    break;
                }
            }

            ContainerAttach_ProcessIncomingStream(receivedMessage);
        }
    });

    for (;;)
    {
        auto requestBody = ContainerAttach_GetInput();
        uint toSend = (uint)(requestBody->size());
        KBuffer::SPtr buffer;
        KBuffer::Create(toSend, buffer, HttpUtil::GetKtlAllocator());

        byte* buf = (byte *)buffer->GetBuffer();
        memcpy_s(buf, requestBody->size(), requestBody->data(), requestBody->size());

        auto contentType = KWebSocket::MessageContentType::Binary;
        AutoResetEvent sendCompleteEvent;

        auto operation = websocket->BeginSendFragment(
            buffer,
            toSend,
            true,
            contentType,
            [&sendCompleteEvent](AsyncOperationSPtr const &) { sendCompleteEvent.Set(); },
            root->CreateAsyncOperationRoot());

        sendCompleteEvent.WaitOne();
        auto error = websocket->EndSendFragment(operation);
        if (!error.IsSuccess())
        {
            return;
        }

        if (detachSequenceReceived)
        {
            printf("\n[detatched]\n");
            break;
        }
    };
}
