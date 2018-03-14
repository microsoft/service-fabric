// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "api/wrappers/ApiWrappers.h"
#include "ComDummyservice.h"

namespace Communication
{
    namespace TcpServiceCommunication
    {
        using namespace Common;
        using namespace Transport;
        using namespace std;
        using namespace Api;
        const StringLiteral ComTestSource("ComFabricServiceCommunicationTest.Test");

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

        class ComDumyFabricConnectionEventHandler : public IFabricServiceConnectionEventHandler,
            private Common::ComUnknownBase
        {
            DENY_COPY(ComDumyFabricConnectionEventHandler);

            BEGIN_COM_INTERFACE_LIST(ComDumyFabricConnectionEventHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceConnectionEventHandler)
                COM_INTERFACE_ITEM(IID_IFabricServiceConnectionEventHandler, IFabricServiceConnectionEventHandler)
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
        class ComDummyFabricServiceConectionHandler : public IFabricServiceConnectionHandler, private Common::ComUnknownBase
        {
            DENY_COPY(ComDummyFabricServiceConectionHandler);

            BEGIN_COM_INTERFACE_LIST(ComDummyFabricServiceConectionHandler)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricServiceConnectionHandler)
                COM_INTERFACE_ITEM(IID_IFabricServiceConnectionHandler, IFabricServiceConnectionHandler)
                END_COM_INTERFACE_LIST()

        public:
            ComDummyFabricServiceConectionHandler() {};

            virtual HRESULT STDMETHODCALLTYPE BeginProcessConnect(
                /* [in] */ IFabricClientConnection *clientConnection,
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
                clientConnection->QueryInterface(IID_IFabricClientConnection, clientConnection_.VoidInitializationAddress());
                return S_OK;
            }

            virtual HRESULT STDMETHODCALLTYPE EndProcessConnect(
                /* [in] */ IFabricAsyncOperationContext *context)
            {
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
            ComPointer<IFabricClientConnection> getClientConnection()
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
            ComPointer<IFabricClientConnection> clientConnection_;
        };


        class ComFabricServiceCommunicationTest
        {
        protected:
            FABRIC_SERVICE_TRANSPORT_SETTINGS  CreateSettings(FABRIC_SECURITY_CREDENTIALS * cred,
                int maxMessageSize);
            void CheckHRESULT(HRESULT hr, HRESULT expectedHr);
            FABRIC_X509_CREDENTIALS CreateSecurityCredential(LPCWSTR *);
            void CreateServiceCommunicationListener(FABRIC_SERVICE_TRANSPORT_SETTINGS const & settings,
                __out IServiceCommunicationListenerPtr & result);
            ComPointer<IFabricServiceCommunicationClient2> CreateClient(wstring serverAddress,
                bool secure,
                IFabricCommunicationMessageHandler * notificationHandler,
                IFabricServiceConnectionEventHandler *connectionEventHandler);

            ComPointer<IFabricStringResult> CreateAndRegisterServer(LPCWSTR path,
                LPCWSTR ipAddress,
                int port,
                bool secure,
                ComPointer<ComDummyService> dummyservice,
                IFabricServiceConnectionHandler* connectionHandler,
                ComPointer<IFabricServiceCommunicationListener>  & listener
            );

            FABRIC_SERVICE_LISTENER_ADDRESS CreateListenerAddress(LPCWSTR path,
                LPCWSTR ipAddress,
                int port);
            ComPointer<IFabricServiceCommunicationMessage> CreateSendMessage(int bodySize);
            ComPointer<IFabricServiceCommunicationMessage> CreateReplyMessage(size_t & msgSize);


        };

        BOOST_FIXTURE_TEST_SUITE(ComFabricServiceCommunicationTestSuite, ComFabricServiceCommunicationTest)

            BOOST_AUTO_TEST_CASE(ClientServiceUnSecuredCommunicationTest)
        {
            Sleep(10000);
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);
            //Message to be send
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricAsyncOperationContext> context;
            //Reply Message
            size_t replyMsgSize;
            ComPointer<IFabricServiceCommunicationMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComDummyService> dummyservice = make_com<ComDummyService>(replymessage);
            //Server
            ComPointer<IFabricServiceCommunicationListener> listener;
            ComPointer<IFabricServiceConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"Localhost",
                10011,
                false,
                dummyservice,
                nullptr,
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricServiceCommunicationMessage>  reply;
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
            auto buffer = reply->Get_Headers();
            VERIFY_IS_TRUE(buffer->BufferSize == 5);
            auto bodybuffer = reply->Get_Body();
            VERIFY_IS_TRUE(bodybuffer->BufferSize == replyMsgSize);
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
            ComPointer<IFabricServiceCommunicationMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComDummyService> dummyservice = make_com<ComDummyService>(replymessage);
            //Connection  handler
            ComPointer<ComDummyFabricServiceConectionHandler> connectionhandler = make_com<ComDummyFabricServiceConectionHandler>();
            //Server
            ComPointer<IFabricServiceCommunicationListener> listener;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"Localhost",
                10012,
                true,
                dummyservice,
                connectionhandler.GetRawPointer(),
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricServiceCommunicationMessage>  reply;
            //ConnectionEvent handler
            ComPointer<ComDumyFabricConnectionEventHandler> clientEventhandler = make_com<ComDumyFabricConnectionEventHandler>();

            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                dummyservice.GetRawPointer(),
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
            ComPointer<IFabricServiceCommunicationMessage>  secondreply;

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
            ComPointer<IFabricServiceCommunicationMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComDummyService> dummyservice = make_com<ComDummyService>(replymessage);
            //Connection  handler
            ComPointer<ComDummyFabricServiceConectionHandler> connectionhandler = make_com<ComDummyFabricServiceConectionHandler>();
            //Server
            ComPointer<IFabricServiceCommunicationListener> listener;
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                L"localhost",
                10013,
                true,
                dummyservice,
                connectionhandler.GetRawPointer(),
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricServiceCommunicationMessage>  reply;
            wstring address = serverAddress->get_String();
            auto client = CreateClient(address,
                true,
                dummyservice.GetRawPointer(),
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
            auto callbackChannel = connectionhandler->getClientConnection();
            //Send Notification Message
            ComPointer<IFabricAsyncOperationContext> notificationcontext;

            waiter->Reset();
            ComPointer<IFabricServiceCommunicationMessage>  notificationreply;

            hr = callbackChannel->BeginRequest(
                message.GetRawPointer(),
                /* [in] */  timeoutMilliseconds,
                /* [in] */ waiter.GetRawPointer(),
                notificationcontext.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(waiter->WaitOne(15000));

            hr = callbackChannel->EndRequest(notificationcontext.GetRawPointer(), notificationreply.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            auto replybuffer = notificationreply->Get_Body();
            VERIFY_IS_TRUE(replybuffer->BufferSize == replyMsgSize);
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
            ComPointer<IFabricServiceCommunicationMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComDummyService> dummyservice = make_com<ComDummyService>(replymessage);
            //Server
            ComPointer<IFabricServiceCommunicationListener> listener;
            ComPointer<IFabricServiceConnectionHandler> connectionHandler;
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
            ComPointer<IFabricServiceCommunicationMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComDummyService> dummyservice = make_com<ComDummyService>(replymessage);
            //Server
            ComPointer<IFabricServiceCommunicationListener> listener;
            ComPointer<IFabricServiceConnectionHandler> connectionHandler;
            auto serverAddress = CreateAndRegisterServer(pathWithDelimiter.c_str(),
                endpoint.GetIpString2().c_str(),
                0,
                true,
                dummyservice, nullptr, listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricServiceCommunicationMessage>  reply;
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
            auto buffer = reply->Get_Headers();
            VERIFY_IS_TRUE(buffer->BufferSize == 5);
            auto bodybuffer = reply->Get_Body();
            VERIFY_IS_TRUE(bodybuffer->BufferSize == replyMsgSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            listener->Abort();
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }
#endif
        BOOST_AUTO_TEST_CASE(InvalidServiceAddressTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricServiceCommunicationListener> listener;
            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();

            Api::ComFabricServiceCommunicationListenerFactory comFactory(ListenerFactory);
            FABRIC_SERVICE_TRANSPORT_SETTINGS settings = CreateSettings(nullptr, 32444);
            auto address = CreateListenerAddress(partitionId.ToString().c_str(), nullptr, 17);

            HRESULT hr = comFactory.CreateFabricServiceCommunicationListener(IID_IFabricServiceCommunicationListener, &settings,
                &address,
                nullptr,
                nullptr,
                listener.InitializationAddress());
            VERIFY_IS_FALSE(hr == S_OK);
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::ArgumentNull));
            Trace.WriteInfo(ComTestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(InvalidServicePathTest)
        {
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricServiceCommunicationListener> listener;
            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();

            Api::ComFabricServiceCommunicationListenerFactory comFactory(ListenerFactory);
            FABRIC_SERVICE_TRANSPORT_SETTINGS settings = CreateSettings(nullptr, 32444);
            auto address = CreateListenerAddress(L"", L"localhost", 16);

            HRESULT hr = comFactory.CreateFabricServiceCommunicationListener(IID_IFabricServiceCommunicationListener, &settings,
                &address,
                nullptr,
                nullptr,
                listener.InitializationAddress());
            VERIFY_IS_FALSE(hr == S_OK);
            auto errorCode = ErrorCode::FromHResult(hr);
            VERIFY_IS_TRUE(errorCode.IsError(ErrorCodeValue::InvalidArgument));
        }

        BOOST_AUTO_TEST_CASE(SettingsValidationFailedTest)
        {
            Trace.WriteInfo(ComTestSource, "Entering {0}", __FUNCTION__);
            auto partitionId = Guid::NewGuid();
            ComPointer<IFabricServiceCommunicationListener> listener;
            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();

            Api::ComFabricServiceCommunicationListenerFactory comFactory(ListenerFactory);
            FABRIC_SERVICE_TRANSPORT_SETTINGS settings = CreateSettings(nullptr, 0);
            auto address = CreateListenerAddress(L"", L"localhost", 15);

            HRESULT hr = comFactory.CreateFabricServiceCommunicationListener(IID_IFabricServiceCommunicationListener, &settings,
                &address,
                nullptr,
                nullptr,
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
            ComPointer<IFabricServiceCommunicationMessage>  replymessage = CreateReplyMessage(replyMsgSize);
            // Dummy Service
            ComPointer<ComDummyService> dummyservice = make_com<ComDummyService>(replymessage);
            //Server
            ComPointer<IFabricServiceCommunicationListener> listener;
            ComPointer<IFabricServiceConnectionHandler> connectionHandler;
            auto ip = L"localhost";
            auto serverAddress = CreateAndRegisterServer(partitionId.ToString().c_str(),
                ip,
                10016,
                true,
                dummyservice,
                nullptr,
                listener);

            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();

            ComPointer<IFabricServiceCommunicationMessage>  reply;
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

            ComPointer<IFabricServiceCommunicationMessage> ComFabricServiceCommunicationTest::CreateSendMessage(int bodySize)
        {
            void * state = nullptr;
            byte * bodyByteBlock = new byte[bodySize];
            byte * headerByteBlock = new byte[10];
            const_buffer bodyBuffer(bodyByteBlock, bodySize);
            TcpServiceMessageHeader headers{ headerByteBlock, 10 };
            vector<Common::const_buffer> buffers;
            buffers.push_back(bodyBuffer);
            auto sendmessgae = Common::make_unique<Message>(
                buffers, [](vector<Common::const_buffer> const &, void *) {}, state);
            sendmessgae->Headers.Add(headers);
            ComPointer<IFabricServiceCommunicationMessage> message = make_com<ComFabricServiceCommunicationMessage,
                IFabricServiceCommunicationMessage>(sendmessgae);
            return message;
        }

        ComPointer<IFabricServiceCommunicationMessage> ComFabricServiceCommunicationTest::CreateReplyMessage(size_t & replyMsgSize)
        {
            byte * replybodyByteBlock1 = new byte[11];
            byte * replyheaderByteBlock = new byte[5];
            TcpServiceMessageHeader replyheaders{ replyheaderByteBlock, 5 };
            const_buffer bodyBuffer1(replybodyByteBlock1, 11);
            replyMsgSize = bodyBuffer1.len;
            vector<Common::const_buffer> replybuffers;
            replybuffers.push_back(bodyBuffer1);
            auto replymsg = Common::make_unique<Message>(
                replybuffers, [](vector<Common::const_buffer> const &, void *) {}, nullptr);
            replymsg->Headers.Add(replyheaders);

            ComPointer<IFabricServiceCommunicationMessage> replymessage = make_com<ComFabricServiceCommunicationMessage,
                IFabricServiceCommunicationMessage>(replymsg);
            return replymessage;
        }

        FABRIC_SERVICE_TRANSPORT_SETTINGS ComFabricServiceCommunicationTest::CreateSettings(FABRIC_SECURITY_CREDENTIALS * securityCredentials,
            int maxMessageSize)
        {
            FABRIC_SERVICE_TRANSPORT_SETTINGS settings = {};
            settings.MaxMessageSize = maxMessageSize;
            settings.SecurityCredentials = securityCredentials;
            settings.OperationTimeoutInSeconds = 60;
            settings.KeepAliveTimeoutInSeconds = 1200;
            settings.MaxConcurrentCalls = 10;
            settings.MaxQueueSize = 100;
            return settings;
        }




        FABRIC_X509_CREDENTIALS ComFabricServiceCommunicationTest::CreateSecurityCredential(LPCWSTR * names)
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

        void ComFabricServiceCommunicationTest::CheckHRESULT(
            HRESULT hr,
            HRESULT expectedHr)
        {
            VERIFY_IS_TRUE(hr == expectedHr,
                wformatString("CheckHRESULT: Expected {0}, got {1}",
                    ErrorCode::FromHResult(expectedHr).ErrorCodeValueToString(),
                    ErrorCode::FromHResult(hr).ErrorCodeValueToString().c_str()).c_str());
        }


        ComPointer<IFabricServiceCommunicationClient2>  ComFabricServiceCommunicationTest::CreateClient(wstring serverAddress,
            bool secure,
            IFabricCommunicationMessageHandler * notificationHandler,
            IFabricServiceConnectionEventHandler *connectionEventHandler)

        {
            auto clientFactoryPtr = Communication::TcpServiceCommunication::ServiceCommunicationClientFactory::Create();
            wstring const remoteName0 = L"WinFabric-Test-SAN1-Alice";
            FABRIC_SERVICE_TRANSPORT_SETTINGS setting;
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

            Api::ComFabricServiceCommunicationClientFactory comFactory(clientFactoryPtr);
            ComPointer<IFabricServiceCommunicationClient> client;


            HRESULT hr = comFactory.CreateFabricServiceCommunicationClient(IID_IFabricServiceCommunicationClient, serverAddress.c_str(),
                &setting,
                notificationHandler,
                connectionEventHandler,
                client.InitializationAddress());

            if (SUCCEEDED(hr)) {
                VERIFY_IS_TRUE(hr == S_OK);
                CheckHRESULT(hr, S_OK);
            }
            ComPointer<IFabricServiceCommunicationClient2> client3;
            client->QueryInterface(IID_IFabricServiceCommunicationClient2,
                client3.VoidInitializationAddress());
            return client3;
        }

        ComPointer<IFabricStringResult> ComFabricServiceCommunicationTest::CreateAndRegisterServer(LPCWSTR path,
            LPCWSTR ipAddress,
            int port,
            bool secure,
            ComPointer<ComDummyService> dummyservice,
            IFabricServiceConnectionHandler*  connectionHandler,
            ComPointer<IFabricServiceCommunicationListener>  & listener
        )
        {

            auto &ListenerFactory = Communication::TcpServiceCommunication::ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();
            wstring const remoteName0 = L"WinFabric-Test-SAN1-Alice";
            ComPointer<ComCallbackWaiter> waiter = make_com<ComCallbackWaiter>();
            FABRIC_X509_CREDENTIALS x509Credentials;
            FABRIC_SECURITY_CREDENTIALS securityCredential;
            FABRIC_SERVICE_TRANSPORT_SETTINGS settings;
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
            Api::ComFabricServiceCommunicationListenerFactory comFactory(ListenerFactory);


            HRESULT hr = comFactory.CreateFabricServiceCommunicationListener(IID_IFabricServiceCommunicationListener, &settings,
                &address, dummyservice.GetRawPointer(),
                connectionHandler,
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

        FABRIC_SERVICE_LISTENER_ADDRESS ComFabricServiceCommunicationTest::CreateListenerAddress(LPCWSTR path, LPCWSTR ipAdddress, int port)
        {
            FABRIC_SERVICE_LISTENER_ADDRESS listenerAddress;
            listenerAddress.IPAddressOrFQDN = ipAdddress;
            listenerAddress.Port = port;
            listenerAddress.Path = path;
            return listenerAddress;
        }

    }
}
