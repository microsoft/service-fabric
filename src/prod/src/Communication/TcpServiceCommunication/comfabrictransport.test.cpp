// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "api/wrappers/ApiWrappers.h"
#include "ComFabricTransportDummyHandler.h"
#include "ComFabricTransportDummyCallbackHandler.h"

namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ComDummyDisposer : public IFabricTransportMessageDisposer, private Common::ComUnknownBase
        {
            DENY_COPY(ComDummyDisposer);

            BEGIN_COM_INTERFACE_LIST(ComDummyDisposer)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportMessageDisposer)
                COM_INTERFACE_ITEM(IID_IFabricTransportMessageDisposer,IFabricTransportMessageDisposer)
                END_COM_INTERFACE_LIST()

        public:
            ComDummyDisposer(){}

            void Dispose(
               ULONG,
                IFabricTransportMessage **
            )
            {
                //no-op
            }

        };
        using namespace Common;
        using namespace Transport;
        using namespace std;
        using namespace Api;
        const StringLiteral ComTestSource("ComFabricTransportTest.Test");

        class ComCallbackWaiter :
            public AutoResetEvent,
            public IFabricAsyncOperationCallback,
            public ComUnknownBase
        {
            COM_INTERFACE_LIST1(
                ComCallbackWaiter,
                IID_IFabricAsyncOperationCallback,
                IFabricAsyncOperationCallback);

        public:
            ComCallbackWaiter() : AutoResetEvent(false) {}

            void STDMETHODCALLTYPE Invoke(__in IFabricAsyncOperationContext *)
            {
                this->Set();
            }
        };

        class ComDumyFabricConnectionEventHandler : public IFabricTransportClientEventHandler,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComDumyFabricConnectionEventHandler);

            BEGIN_COM_INTERFACE_LIST(ComDumyFabricConnectionEventHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportClientEventHandler)
                COM_INTERFACE_ITEM(IID_IFabricServiceConnectionEventHandler, IFabricTransportClientEventHandler)
                END_COM_INTERFACE_LIST()

        public:
            ComDumyFabricConnectionEventHandler() = default;
            virtual HRESULT STDMETHODCALLTYPE OnConnected(
                /* [in] */ LPCWSTR connectionAddress)
            {
                UNREFERENCED_PARAMETER(connectionAddress);

                connectEvent.Set();
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE OnDisconnected(
                /* [in] */ LPCWSTR connectionAddress,
                /* [in] */ HRESULT error)
            {
                UNREFERENCED_PARAMETER(connectionAddress);
                UNREFERENCED_PARAMETER(error);

                disconnectEvent.Set();
                return S_OK;
            }

            bool WaitForConnectEvent(TimeSpan timeout)
            {
                return connectEvent.WaitOne(timeout);
            }

            bool WaitForDisconnectEvent(TimeSpan timeout)
            {
                return disconnectEvent.WaitOne(timeout);
            }
        private:
            ManualResetEvent connectEvent;
            ManualResetEvent disconnectEvent;

        };
        class ComDummyFabricTransportConectionHandler : public IFabricTransportConnectionHandler, private Common::ComUnknownBase
        {
            DENY_COPY(ComDummyFabricTransportConectionHandler);

            BEGIN_COM_INTERFACE_LIST(ComDummyFabricTransportConectionHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricTransportConnectionHandler)
                COM_INTERFACE_ITEM(IID_IFabricServiceConnectionHandler, IFabricTransportConnectionHandler)
                END_COM_INTERFACE_LIST()

        public:
            ComDummyFabricTransportConectionHandler() {};

            virtual HRESULT STDMETHODCALLTYPE BeginProcessConnect(
                /* [in] */ IFabricTransportClientConnection *clientConnection,
                /* [in] */ DWORD timeoutMilliseconds,
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context)
            {
                UNREFERENCED_PARAMETER(timeoutMilliseconds);

                ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
                HRESULT hr = operation->Initialize(ComponentRootSPtr(), callback);
                if (FAILED(hr))
                {
                    return hr;
                }
                ComAsyncOperationContextCPtr baseOperation;
                baseOperation.SetNoAddRef(operation.DetachNoRelease());
                hr = baseOperation->Start(baseOperation);
                if (FAILED(hr))
                {
                    return hr;
                }

                *context = baseOperation.DetachNoRelease();
                clientConnection->QueryInterface(IID_IFabricTransportClientConnection, clientConnection_.VoidInitializationAddress());
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE EndProcessConnect(
                /* [in] */ IFabricAsyncOperationContext *context)
            {
                if (connectEvent.IsSet())
                {
                    return ComUtility::OnPublicApiReturn(E_POINTER);
                }
                connectEvent.Set();
                return ComCompletedAsyncOperationContext::End(context);
            }

            virtual HRESULT STDMETHODCALLTYPE BeginProcessDisconnect(
                /* [in] */ LPCWSTR clientId,
                /* [in] */ DWORD timeoutMilliseconds,
                /* [in] */ IFabricAsyncOperationCallback *callback,
                /* [retval][out] */ IFabricAsyncOperationContext **context)
            {
                UNREFERENCED_PARAMETER(clientId);
                UNREFERENCED_PARAMETER(timeoutMilliseconds);

                ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
                HRESULT hr = operation->Initialize(ComponentRootSPtr(), callback);
                if (FAILED(hr))
                {
                    return hr;
                }
                ComAsyncOperationContextCPtr baseOperation;
                baseOperation.SetNoAddRef(operation.DetachNoRelease());
                hr = baseOperation->Start(baseOperation);
                if (FAILED(hr))
                {
                    return hr;
                }

                *context = baseOperation.DetachNoRelease();

                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE EndProcessDisconnect(
                /* [in] */ IFabricAsyncOperationContext *context)
            {
                disconnectEvent.Set();
                return ComCompletedAsyncOperationContext::End(context);

            }
            ComPointer<IFabricTransportClientConnection> getClientConnection()
            {
                return clientConnection_;
            }

            bool WaitForConnectEvent(TimeSpan timeout)
            {
                return connectEvent.WaitOne(timeout);
            }

            bool WaitForDisconnectEvent(TimeSpan timeout)
            {
                return disconnectEvent.WaitOne(timeout);
            }

        private:
            ManualResetEvent connectEvent;
            ManualResetEvent disconnectEvent;
            ComPointer<IFabricTransportClientConnection> clientConnection_;
        };


        class ComFabricTransportTest
        {
        protected:
            FABRIC_TRANSPORT_SETTINGS  CreateSettings(FABRIC_SECURITY_CREDENTIALS * cred,
                int maxMessageSize);
            void CheckHRESULT(HRESULT hr, HRESULT expectedHr);
            FABRIC_X509_CREDENTIALS CreateSecurityCredential(LPCWSTR *);
            void CreateServiceCommunicationListener(FABRIC_TRANSPORT_SETTINGS const & settings,
                __out IServiceCommunicationListenerPtr & result);
            ComPointer<IFabricTransportClient> CreateClient(wstring serverAddress,
                bool secure,
                IFabricTransportCallbackMessageHandler * notificationHandler,
                IFabricTransportClientEventHandler *connectionEventHandler,
                bool explicitConnect=true);

            ComPointer<IFabricStringResult> CreateAndRegisterServer(LPCWSTR path,
                LPCWSTR ipAddress,
                int port,
                bool secure,
                ComPointer<ComFabricTransportDummyHandler> dummyservice,
                IFabricTransportConnectionHandler* connectionHandler,
                ComPointer<IFabricTransportListener>  & listener
            );

            FABRIC_TRANSPORT_LISTEN_ADDRESS CreateListenerAddress(LPCWSTR path,
                LPCWSTR ipAddress,
                int port);
            ComPointer<IFabricTransportMessage> CreateSendMessage(int bodySize);
            ComPointer<IFabricTransportMessage> CreateReplyMessage(size_t & msgSize,ULONG headerSize=5);


        };

        BOOST_FIXTURE_TEST_SUITE(ComFabricTransportTestSuite, ComFabricTransportTest)

            BOOST_AUTO_TEST_CASE(ClientServiceUnSecuredCommunicationTest)
        {
            Sleep(10000);
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);
            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"Localhost",
                10011,
                false,
                dummyservice,
                nullptr,
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                false,
                nullptr,
                nullptr);
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const * bodybuffers = NULL;
			ULONG bufferCount = 0;
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const *buffer = NULL;
            reply->GetHeaderAndBodyBuffer(&buffer, &bufferCount, &bodybuffers);
            VERIFY_IS_TRUE(buffer->BufferSize == 5);
            
            auto totalsize = 0;
            for (ULONG i = 0; i < bufferCount; i++)
            {
                totalsize += bodybuffers[i].BufferSize;
            }
            VERIFY_IS_TRUE(totalsize == replyMsgSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
        BOOST_AUTO_TEST_CASE(TestForHeaderLessThanDefaultBufferSize)
        {
            //Default Buffer size in Transport Config is 64KB
            Sleep(10000);
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);
            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize, 2 * 1000);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"Localhost",
                10011,
                false,
                dummyservice,
                nullptr,
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                false,
                nullptr,
                nullptr);
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const * bodybuffers = NULL;
			ULONG bufferCount = 0;
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const *buffer = NULL;
			reply->GetHeaderAndBodyBuffer(&buffer, &bufferCount, &bodybuffers);
            VERIFY_IS_TRUE(buffer->BufferSize == 2*1000);
            auto totalsize = 0;
            for (ULONG i = 0; i < bufferCount; i++)
            {
                totalsize += bodybuffers[i].BufferSize;
            }
            VERIFY_IS_TRUE(totalsize == replyMsgSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(TestForHeaderLargerThanDefaultBufferSize)
        {
            Sleep(10000);
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);
            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize, 67 * 1024);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"Localhost",
                10011,
                false,
                dummyservice,
                nullptr,
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                false,
                nullptr,
                nullptr);
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const * bodybuffers = NULL;
			ULONG bufferCount = 0;
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const *buffer = NULL;
			reply->GetHeaderAndBodyBuffer(&buffer, &bufferCount, &bodybuffers);
            VERIFY_IS_TRUE(buffer->BufferSize == 67 * 1024);
          
            auto totalsize = 0;
            for (ULONG i = 0; i < bufferCount; i++)
            {
                totalsize += bodybuffers[i].BufferSize;
            }
            VERIFY_IS_TRUE(totalsize == replyMsgSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
#if !defined(PLATFORM_UNIX)
        BOOST_AUTO_TEST_CASE(ComCientConnectionEventHandlerTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Connection  handler
            ComPointer<ComDummyFabricTransportConectionHandler> connectionhandler = make_com<ComDummyFabricTransportConectionHandler>();
            //Server
            ComPointer<IFabricTransportListener> listener;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"Localhost",
                10012,
                true,
                dummyservice,
                connectionhandler.GetRawPointer(),
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();
            ComPointer<ComFabricTransportDummyCallbackHandler> callbackHandler = make_com<ComFabricTransportDummyCallbackHandler>();

            ComPointer<IFabricTransportMessage>  reply;
            //ConnectionEvent handler
            ComPointer<ComDumyFabricConnectionEventHandler> clientEventhandler = make_com<ComDumyFabricConnectionEventHandler>();

            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                callbackHandler.GetRawPointer(),
                clientEventhandler.GetRawPointer());
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            VERIFY_IS_TRUE(clientEventhandler->WaitForConnectEvent(TimeSpan::FromSeconds(15)), L"Client got Connected");
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            auto callbackChannel = connectionhandler->getClientConnection();
            //Send Notification Message
            ComPointer<IFabricAsyncOperationContext> closecontext;
            waiter->Reset();
            //Close the Listener
            hr = listener->BeginClose(waiter.GetRawPointer(),
                closecontext.InitializationAddress());
            VERIFY_IS_TRUE(waiter->WaitOne(10000));
            hr = listener->EndClose(closecontext.GetRawPointer());
            VERIFY_IS_TRUE(SUCCEEDED(hr), L"Listener got Closed");
            //Send the msg from client to listener
            ComPointer<IFabricTransportMessage>  secondreply;

            ComPointer<IFabricAsyncOperationContext> sendcontext;
            waiter->Reset();
            hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                sendcontext.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(sendcontext.GetRawPointer(), secondreply.InitializationAddress());
            VERIFY_IS_FALSE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(clientEventhandler->WaitForDisconnectEvent(TimeSpan::FromSeconds(2)), L"Client got Disonnected");
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);
        }

        BOOST_AUTO_TEST_CASE(ComServiceNotificationTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Connection  handler
            ComPointer<ComDummyFabricTransportConectionHandler> connectionhandler = make_com<ComDummyFabricTransportConectionHandler>();
            //Server
            ComPointer<IFabricTransportListener> listener;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"localhost",
                10013,
                true,
                dummyservice,
                connectionhandler.GetRawPointer(),
                listener);
            ManualResetEvent recievedEvent;
            ComPointer<ComFabricTransportDummyCallbackHandler>callbackHandler =
                    make_com<ComFabricTransportDummyCallbackHandler>();

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                callbackHandler.GetRawPointer(),
                nullptr);
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            auto callbackConnection = connectionhandler->getClientConnection();
            //Send Notification Message
            ComPointer<IFabricAsyncOperationContext> notificationcontext;
            callbackConnection->Send(
                message.GetRawPointer());

            VERIFY_IS_TRUE(callbackHandler->IsMessageRecieved(TimeSpan::FromSeconds(100)));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
#endif
        BOOST_AUTO_TEST_CASE(NonZeroPortListenAddressTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            wstring pathWithDelimiter = Common::wformatString("{0}{1}{2}{3}", L"3298479847", Constants::ListenAddressPathDelimiter, "454546", Constants::ListenAddressPathDelimiter);
            Endpoint endpoint;
            auto error = TcpTransportUtility::GetFirstLocalAddress(endpoint, ResolveOptions::Enum::IPv4);

            //Message to be send
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(pathWithDelimiter.c_str(),
                endpoint.GetIpString2().c_str(),
                10014,
                false,
                dummyservice, nullptr, listener);

            wstring address = serverAddress->get_String();
            wstring shouldbeAddress = Common::wformatString("{0}:{1}+{2}", endpoint.GetIpString2(), 10014, pathWithDelimiter);
            VERIFY_IS_TRUE(StringUtility::AreEqualCaseInsensitive(address, shouldbeAddress));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }

#if !defined(PLATFORM_UNIX)
        BOOST_AUTO_TEST_CASE(SimpleClientServiceCommunicationTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            wstring pathWithDelimiter = Common::wformatString("{0}{1}{2}{3}", L"3298479847", Constants::ListenAddressPathDelimiter, "454546", Constants::ListenAddressPathDelimiter);

            //Message to be send
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            Endpoint endpoint;
            auto error = TcpTransportUtility::GetFirstLocalAddress(endpoint, ResolveOptions::Enum::IPv6);
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(pathWithDelimiter.c_str(),
                endpoint.GetIpString2().c_str(),
                0,
                true,
                dummyservice, nullptr, listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                nullptr,
                nullptr);
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(SUCCEEDED(hr), L"Client got reply");
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const * bodybuffers = NULL;
			ULONG bufferCount = 0;
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const *buffer = NULL;
			reply->GetHeaderAndBodyBuffer(&buffer, &bufferCount, &bodybuffers);
            VERIFY_IS_TRUE(buffer->BufferSize == 5);
            auto totalsize = 0;
            for (ULONG i = 0; i < bufferCount; i++)
            {
                totalsize += bodybuffers[i].BufferSize;
            }
            VERIFY_IS_TRUE(totalsize == replyMsgSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
#endif


#if !defined(PLATFORM_UNIX)
        BOOST_AUTO_TEST_CASE(ClientCloseAsynOperationTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            wstring pathWithDelimiter = Common::wformatString("{0}{1}{2}{3}", L"3298479847", Constants::ListenAddressPathDelimiter, "454546", Constants::ListenAddressPathDelimiter);

            //Message to be send
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            Endpoint endpoint;
            auto error = TcpTransportUtility::GetFirstLocalAddress(endpoint, ResolveOptions::Enum::IPv6);
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(pathWithDelimiter.c_str(),
                endpoint.GetIpString2().c_str(),
                0,
                true,
                dummyservice, nullptr, listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                nullptr,
                nullptr);
            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(SUCCEEDED(hr), L"Client got reply");
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const * bodybuffers = NULL;
			ULONG bufferCount = 0;
			FABRIC_TRANSPORT_MESSAGE_BUFFER  const *buffer = NULL;
			reply->GetHeaderAndBodyBuffer(&buffer, &bufferCount, &bodybuffers);
            VERIFY_IS_TRUE(buffer->BufferSize == 5);
            auto totalsize = 0;
            for (ULONG i = 0; i < bufferCount; i++)
            {
                totalsize += bodybuffers[i].BufferSize;
            }
            VERIFY_IS_TRUE(totalsize == replyMsgSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            //Close Client
            ComPointer<IFabricAsyncOperationContext> context1;
            waiter->Reset();
             hr = client->BeginClose(
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context1.InitializationAddress());

            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndClose(context1.GetRawPointer());
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            //Send Rquest Again
            ComPointer<IFabricAsyncOperationContext> context2;
            ComPointer<IFabricTransportMessage>  reply1;
            waiter->Reset();
             hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context2.InitializationAddress());

            
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context2.GetRawPointer(), reply1.InitializationAddress());
            VERIFY_IS_TRUE(hr==FABRIC_E_CANNOT_CONNECT, L"Client got Error");

            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
#endif


#if !defined(PLATFORM_UNIX)
        BOOST_AUTO_TEST_CASE(SendMessageBeforeExplicitConnectTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            wstring pathWithDelimiter = Common::wformatString("{0}{1}{2}{3}", L"3298479847", Constants::ListenAddressPathDelimiter, "454546", Constants::ListenAddressPathDelimiter);

            //Message to be send
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            Endpoint endpoint;
            auto error = TcpTransportUtility::GetFirstLocalAddress(endpoint, ResolveOptions::Enum::IPv6);
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<ComDummyFabricTransportConectionHandler> connectionHandler =
                make_com<ComDummyFabricTransportConectionHandler>();
            auto serverAddress = CreateAndRegisterServer(pathWithDelimiter.c_str(),
                endpoint.GetIpString2().c_str(),
                0,
                true,
                dummyservice, connectionHandler.GetRawPointer(), listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                nullptr,
                nullptr,
                false);
            VERIFY_IS_FALSE(connectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(10)));

            auto message = CreateSendMessage(2);
            DWORD timeoutMilliseconds = 5000;
      
            ComPointer<IFabricAsyncOperationContext> context1;
            waiter->Reset();

            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context1.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context1.GetRawPointer(), reply.InitializationAddress());
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(hr==FABRIC_E_CANNOT_CONNECT, L"Client got Cannot Connect Error");
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
#endif
        BOOST_AUTO_TEST_CASE(InvalidServiceAddressTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricTransportListener> listener;
            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();

            Api::ComFabricTransportListenerFactory comFactory(ListenerFactory);
            FABRIC_TRANSPORT_SETTINGS settings = CreateSettings(nullptr, 32444);
            auto address = CreateListenerAddress(partitionId.ToString().c_str(), nullptr, 17);
            ComPointer<ComDummyDisposer> dummyDisposer = make_com<ComDummyDisposer>();
            HRESULT hr = comFactory.CreateFabricTransportListener(IID_IFabricTransportListener, &settings,
                &address,
                nullptr,
                nullptr,
                dummyDisposer.GetRawPointer(),
                listener.InitializationAddress());
            VERIFY_IS_FALSE(hr == S_OK);
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::ArgumentNull));
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(InvalidServicePathTest)
        {
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricTransportListener> listener;
            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();

            Api::ComFabricTransportListenerFactory comFactory(ListenerFactory);
            FABRIC_TRANSPORT_SETTINGS settings = CreateSettings(nullptr, 32444);
            auto address = CreateListenerAddress(L"", L"localhost", 16);
            ComPointer<ComDummyDisposer> dummyDisposer = make_com<ComDummyDisposer>();

            HRESULT hr = comFactory.CreateFabricTransportListener(IID_IFabricTransportListener, &settings,
                &address,
                nullptr,
                nullptr,
                dummyDisposer.GetRawPointer(),
                listener.InitializationAddress());
            VERIFY_IS_FALSE(hr == S_OK);
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::InvalidArgument));
        }

        BOOST_AUTO_TEST_CASE(SettingsValidationFailedTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricTransportListener> listener;
            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();

            Api::ComFabricTransportListenerFactory comFactory(ListenerFactory);
            FABRIC_TRANSPORT_SETTINGS settings = CreateSettings(nullptr, 0);
            auto address = CreateListenerAddress(L"", L"localhost", 15);
            ComPointer<ComDummyDisposer> dummyDisposer = make_com<ComDummyDisposer>();

            HRESULT hr = comFactory.CreateFabricTransportListener(IID_IFabricTransportListener, &settings,
                &address,
                nullptr,
                nullptr,
                dummyDisposer.GetRawPointer(),
                listener.InitializationAddress());
            VERIFY_IS_FALSE(hr == S_OK);
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::InvalidArgument));
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);
        }
#if !defined(PLATFORM_UNIX)
        BOOST_AUTO_TEST_CASE(MessageTooLargeTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricTransportMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComFabricTransportDummyHandler> dummyservice = make_com<ComFabricTransportDummyHandler>(replymessage);
            //Server
            ComPointer<IFabricTransportListener> listener;
            ComPointer<IFabricTransportConnectionHandler> connectionHandler;
            auto ip = L"localhost";
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                ip,
                10016,
                true,
                dummyservice,
                nullptr,
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricTransportMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                nullptr,
                nullptr);
            auto message = CreateSendMessage(35465657);
            DWORD timeoutMilliseconds = 5000;
            //Send Messages
            auto hr = client->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                context.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));
            hr = client->EndRequest(context.GetRawPointer(), reply.InitializationAddress());
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::MessageTooLarge), L"Message Too Large");
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);
        }
#endif
        BOOST_AUTO_TEST_SUITE_END()

            ComPointer<IFabricTransportMessage> ComFabricTransportTest::CreateSendMessage(int bodySize)
        {
            void * state = nullptr;
            byte * bodyByteBlock = new byte[bodySize];
            byte * headerByteBlock = new byte[10];
            const_buffer bodyBuffer(bodyByteBlock, bodySize);
            FabricTransportMessageHeader headers{ 10 };
            vector<Common::const_buffer> buffers;
            const_buffer headerBuffer(headerByteBlock, 10);
            buffers.push_back(headerBuffer);
            buffers.push_back(bodyBuffer);
            auto sendmessgae = Common::make_unique<Message>(
                buffers, [](vector<Common::const_buffer> const &, void *) {}, state);
            sendmessgae->Headers.Add(headers);
            ComPointer<IFabricTransportMessage> message = make_com<ComFabricTransportMessage,
                IFabricTransportMessage>(move(sendmessgae));
            return message;
        }

        ComPointer<IFabricTransportMessage> ComFabricTransportTest::CreateReplyMessage(size_t & replyMsgSize,
            ULONG headerSize)
        {
            byte * replybodyByteBlock1 = new byte[2];
            byte * replybodyByteBlock2 = new byte[9];

            byte * replyheaderByteBlock = new byte[headerSize];
            FabricTransportMessageHeader replyheaders{ headerSize };
            const_buffer headerBuffer1(replyheaderByteBlock, headerSize);
            const_buffer bodyBuffer1(replybodyByteBlock1, 2);
            const_buffer bodyBuffer2(replybodyByteBlock2, 9);
            replyMsgSize = bodyBuffer1.len + bodyBuffer2.len;
            vector<Common::const_buffer> replybuffers;
            replybuffers.push_back(headerBuffer1);
            replybuffers.push_back(bodyBuffer1);
            replybuffers.push_back(bodyBuffer2);
            auto replymsg = Common::make_unique<Message>(
                replybuffers, [](vector<Common::const_buffer> const &, void *) {}, nullptr);
            replymsg->Headers.Add(replyheaders);

            ComPointer<IFabricTransportMessage> replymessage = make_com<ComFabricTransportMessage,
                IFabricTransportMessage>(move(replymsg));
            return replymessage;
        }

        FABRIC_TRANSPORT_SETTINGS ComFabricTransportTest::CreateSettings(FABRIC_SECURITY_CREDENTIALS * securityCredentials,
            int maxMessageSize)
        {
            FABRIC_TRANSPORT_SETTINGS settings = {};
            settings.MaxMessageSize = maxMessageSize;
            settings.SecurityCredentials = securityCredentials;
            settings.OperationTimeoutInSeconds = 60;
            settings.KeepAliveTimeoutInSeconds = 1200;
            settings.MaxConcurrentCalls = 10;
            settings.MaxQueueSize = 100;
            return settings;
        }




        FABRIC_X509_CREDENTIALS ComFabricTransportTest::CreateSecurityCredential(LPCWSTR * names)
        {
            FABRIC_X509_CREDENTIALS x509Credentials = {};
            x509Credentials.FindType = FABRIC_X509_FIND_TYPE_FINDBYSUBJECTNAME;
            x509Credentials.FindValue = (void*)L"CN=WinFabric-Test-SAN1-Alice";
            x509Credentials.StoreName = X509Default::StoreName().c_str();
            x509Credentials.ProtectionLevel = FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN;
            x509Credentials.StoreLocation = FABRIC_X509_STORE_LOCATION_LOCALMACHINE;
            x509Credentials.AllowedCommonNameCount = 1;
            x509Credentials.AllowedCommonNames = names;
            return x509Credentials;
        }

        void ComFabricTransportTest::CheckHRESULT(
            HRESULT hr,
            HRESULT expectedHr)
        {
            VERIFY_IS_TRUE(hr == expectedHr,
                wformatString("CheckHRESULT: Expected {0}, got {1}",
                    ErrorCode::FromHResult(expectedHr).ErrorCodeValueToString(),
                    ErrorCode::FromHResult(hr).ErrorCodeValueToString().c_str()).c_str());
        }


        ComPointer<IFabricTransportClient>  ComFabricTransportTest::CreateClient(wstring serverAddress,
            bool secure,
            IFabricTransportCallbackMessageHandler * notificationHandler,
            IFabricTransportClientEventHandler *connectionEventHandler,
            bool explicitConnect)

        {
            wstring const remoteName0 = L"WinFabric-Test-SAN1-Alice";
            FABRIC_TRANSPORT_SETTINGS setting;
            FABRIC_X509_CREDENTIALS x509Credentials;
            FABRIC_SECURITY_CREDENTIALS securityCredential;
            LPCWSTR * remoteNames = nullptr;
            if (secure)
            {
                remoteNames = new LPCWSTR[1];
                remoteNames[0] = remoteName0.c_str();
                x509Credentials = CreateSecurityCredential(remoteNames);
                securityCredential = { FABRIC_SECURITY_CREDENTIAL_KIND_X509, &x509Credentials };
                setting = CreateSettings(&securityCredential, 32444);
            }
            else {
                setting = CreateSettings(nullptr, 32444);
            }
            ComPointer<ComDummyDisposer> dummyDisposer = make_com<ComDummyDisposer>();

            auto &comFactory = Api::ComFabricTransportClientFactory::GetComFabricTransportClientFactory(dummyDisposer.GetRawPointer());
            ComPointer<IFabricTransportClient> client;

            HRESULT hr = comFactory.CreateFabricTransportClient(IID_IFabricTransportClient, serverAddress.c_str(),
                &setting,
                notificationHandler,
                connectionEventHandler,
                client.InitializationAddress());

            if (SUCCEEDED(hr)) {
                VERIFY_IS_TRUE(hr == S_OK);
                CheckHRESULT(hr, S_OK);
            }
            ComPointer<IFabricTransportClient> client3;
            client->QueryInterface(IID_IFabricTransportClient,
                client3.VoidInitializationAddress());
            if (explicitConnect)
            {
                ComPointer<IFabricAsyncOperationContext> context;
                ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();
                DWORD timeoutMilliseconds = 5000;

                hr = client3->BeginOpen(
                    /* [in] */  timeoutMilliseconds,
                    /* [in] */ waiter.GetRawPointer(),
                    context.InitializationAddress());

                VERIFY_IS_TRUE(waiter->WaitOne(15000));

                hr = client3->EndOpen(context.GetRawPointer());

                VERIFY_IS_TRUE(SUCCEEDED(hr));
            }
            return client3;
        }

        ComPointer<IFabricStringResult> ComFabricTransportTest::CreateAndRegisterServer(LPCWSTR path,
            LPCWSTR ipAddress,
            int port,
            bool secure,
            ComPointer<ComFabricTransportDummyHandler> dummyservice,
            IFabricTransportConnectionHandler*  connectionHandler,
            ComPointer<IFabricTransportListener>  & listener
        )
        {

            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();
            wstring const remoteName0 = L"WinFabric-Test-SAN1-Alice";
            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();
            FABRIC_X509_CREDENTIALS x509Credentials;
            FABRIC_SECURITY_CREDENTIALS securityCredential;
            FABRIC_TRANSPORT_SETTINGS settings;
            LPCWSTR * remoteNames = nullptr;
            if (secure)
            {
                remoteNames = new LPCWSTR[1];
                remoteNames[0] = remoteName0.c_str();
                x509Credentials = CreateSecurityCredential(remoteNames);
                securityCredential = { FABRIC_SECURITY_CREDENTIAL_KIND_X509, &x509Credentials };
                settings = CreateSettings(&securityCredential, 32444);
            }
            else
            {
                settings = CreateSettings(nullptr, 32444);
            }
            auto address = CreateListenerAddress(path, ipAddress, port);
            Api::ComFabricTransportListenerFactory comFactory(ListenerFactory);

            ComPointer<ComDummyDisposer> dummyDisposer = make_com<ComDummyDisposer>();

            HRESULT hr = comFactory.CreateFabricTransportListener(IID_IFabricTransportListener, &settings,
                &address, 
                dummyservice.GetRawPointer(),
                connectionHandler,
                dummyDisposer.GetRawPointer(),
                listener.InitializationAddress());
            VERIFY_IS_TRUE(hr == S_OK, wformatString("Create Listener Resullt {0}", hr).c_str());

            ComPointer<IFabricStringResult> reply;
            ComPointer<IFabricAsyncOperationContext> context;

            hr = listener->BeginOpen(waiter.GetRawPointer(),
                context.InitializationAddress());
            hr = listener->EndOpen(context.GetRawPointer(), reply.InitializationAddress());
            if (remoteNames != nullptr)
            {
                delete[] remoteNames;
            }
            VERIFY_IS_TRUE(hr == S_OK);
            return reply;
        }

        FABRIC_TRANSPORT_LISTEN_ADDRESS ComFabricTransportTest::CreateListenerAddress(LPCWSTR path, LPCWSTR ipAdddress, int port)
        {
            FABRIC_TRANSPORT_LISTEN_ADDRESS listenerAddress;
            listenerAddress.IPAddressOrFQDN = ipAdddress;
            listenerAddress.Port = port;
            listenerAddress.Path = path;
            return listenerAddress;
        }

    }
}

