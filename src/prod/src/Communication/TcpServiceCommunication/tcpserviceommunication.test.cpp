// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "dummyservice.h"

namespace Communication
{
    namespace TcpServiceCommunication
    {

        using namespace Common;
        using namespace Transport;
        using namespace std;
        using namespace Api;

        const StringLiteral TestSource("TcpServiceCommunication.Test");

        class DummyServiceConenctionHandler :public  Api::IServiceConnectionHandler, public Common::ComponentRoot
        {
        public:
            DummyServiceConenctionHandler(bool shouldConnect)
                :shouldConnect_(shouldConnect)
            {

            }
            virtual   Common::AsyncOperationSPtr BeginProcessConnect(
                Api::IClientConnectionPtr clientConnection,
                Common::TimeSpan const &,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                clientConnection_ = clientConnection;
                return AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(Common::ErrorCodeValue::Success, callback, parent);

            }

            virtual Common::ErrorCode EndProcessConnect(
                Common::AsyncOperationSPtr const &)
            {
                connectEvent.Set();
                if (shouldConnect_)
                {
                    return ErrorCodeValue::Success;
                }
                else
                {
                    return ErrorCodeValue::CannotConnect;
                }

            }

            virtual Common::AsyncOperationSPtr BeginProcessDisconnect(
                std::wstring const &,
                Common::TimeSpan const &,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
            {
                return AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(Common::ErrorCodeValue::Success, callback, parent);
            }

            virtual Common::ErrorCode EndProcessDisconnect(
                Common::AsyncOperationSPtr const &)
            {
                disconnectEvent.Set();
                return ErrorCodeValue::Success;

            }

            IClientConnectionPtr getClientConnection()
            {
                return clientConnection_;
            }

            bool WaitForConnectEvent(TimeSpan timeout)
            {
                return connectEvent.WaitOne(timeout);
            }

            void ResetConnectEvent()
            {
                connectEvent.Reset();
            }

            bool WaitForDisconnectEvent(TimeSpan timeout)
            {
                return disconnectEvent.WaitOne(timeout);
            }

            ~DummyServiceConenctionHandler()
            {

            }
        private:
            bool shouldConnect_;
            AutoResetEvent connectEvent;
            AutoResetEvent disconnectEvent;
            IClientConnectionPtr clientConnection_;
        };

        class DummyClientConnectionEventHandler :public  Api::IServiceConnectionEventHandler, public Common::ComponentRoot
        {
        public:
            virtual void OnConnected(std::wstring const &)
            {
                connectEvent.Set();
            }

            virtual void OnDisconnected(
                std::wstring const &,
                Common::ErrorCode const &)
            {
                disconnectEvent.Set();
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
            AutoResetEvent connectEvent;
            AutoResetEvent disconnectEvent;
        };
        class TcpServiceCommunicationTests
        {
        protected:
            std::wstring CreateAndOpen(wstring path,
                ULONG port,
                bool secureMode,
                TimeSpan timeout,
                Api::IServiceCommunicationMessageHandlerPtr const &  service,
                Api::IServiceConnectionHandlerPtr const & connectionHandler,
                IServiceCommunicationListenerPtr  & server,
                Common::ErrorCode expectedErrorCode = ErrorCodeValue::Success);

            Common::ErrorCode CreateAndOpenListenerWithNoFactory(wstring path,
                Api::IServiceCommunicationTransportSPtr const & transport,
                Api::IServiceCommunicationMessageHandlerPtr &  service,
                Api::IServiceConnectionHandlerPtr & connectionHandler);

            IServiceCommunicationClientPtr CreateClient(bool secureMode,
                TimeSpan timeout,
                wstring const & serverAddress,
                IServiceCommunicationMessageHandlerPtr messageHandler,
                IServiceConnectionEventHandlerPtr eventhandler);

        };


        BOOST_FIXTURE_TEST_SUITE(TcpServiceCommunicationTestsSuite, TcpServiceCommunicationTests)

        BOOST_AUTO_TEST_CASE(OpenListenerWithSamePort)
        {
            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent openEvent(false);

            wstring address;
            address += Common::wformatString("{0}:{1}", L"localhost", 1002);
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(
                servicePtr.get(),
                servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto settings = make_unique<ServiceCommunicationTransportSettings>();
            IServiceCommunicationTransportSPtr transportSPtr;
            ErrorCode error = ServiceCommunicationTransport::Create(move(settings),
                address,
                transportSPtr);
            transportSPtr->Open();
            //Open Another Transport with same port.

            IServiceCommunicationTransportSPtr transportSPtr2;
            auto settings2 = make_unique<ServiceCommunicationTransportSettings>();

            ErrorCode error1 = ServiceCommunicationTransport::Create(move(settings2),
                address,
                transportSPtr2);


            auto errorcode = CreateAndOpenListenerWithNoFactory(partitionId.ToString(),
                transportSPtr2,
                service,
                handler);

            VERIFY_IS_TRUE(errorcode.IsError(ErrorCodeValue::AddressAlreadyInUse));

            //open another listener with same transport

            auto errorcode1 = CreateAndOpenListenerWithNoFactory(partitionId.ToString(),
                transportSPtr2,
                service,
                handler);
            VERIFY_IS_TRUE(errorcode1.IsError(ErrorCodeValue::ObjectClosed));

        }

        BOOST_AUTO_TEST_CASE(OpenListenerWithSamePortUsingFactory)
        {
            wstring address;
            address += Common::wformatString("{0}:{1}", L"localhost", 1004);
            auto partitionId = Guid::NewGuid();
            auto servicePtr = make_shared<DummyService>();
            IServiceCommunicationTransportSPtr transportSPtr;
            auto settings = make_unique<ServiceCommunicationTransportSettings>();
            //Open Transport 
            ErrorCode error = ServiceCommunicationTransport::Create(move(settings),
                address,
                transportSPtr);
            transportSPtr->Open();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            ErrorCode alreadyExist = ErrorCodeValue::AddressAlreadyInUse;
            auto settings2 = make_unique<ServiceCommunicationTransportSettings>();
            Api::IServiceConnectionHandlerPtr handler;
            Api::IServiceCommunicationListenerPtr server;
            //try to Create Listener on same port as last transport and got error as AddressAlreadyInUse
            auto endpointAddress = CreateAndOpen(
                partitionId.ToString(),
                1004,
                false,
                TimeSpan::FromSeconds(0),
                service,
                handler,
                server,
                alreadyExist);

            //transport close
            transportSPtr->Close();
            Api::IServiceCommunicationListenerPtr server2;
            auto settings3 = make_unique<ServiceCommunicationTransportSettings>();
            ErrorCode success = ErrorCodeValue::Success;
            //try to create listener on same port after transport is closed. This should succeed
            auto endpointAddress2 = CreateAndOpen(
                partitionId.ToString(),
                1004,
                false,
                TimeSpan::FromSeconds(0),
                service,
                handler,
                server2,
                success);
        }


        BOOST_AUTO_TEST_CASE(SimpleTcpTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();

            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent closeEvent(false);

            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10005, false, TimeSpan::FromSeconds(0), service, handler, server);

            //Checking endpointAddress is Compatible with Uri
            Uri uri;
            bool uriParsingSucceded = Uri::TryParse(endpointAddress, uri);
            VERIFY_IS_TRUE(uriParsingSucceded);

            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, IServiceCommunicationMessageHandlerPtr(), IServiceConnectionEventHandlerPtr());

            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const &)
            {
                isMessageHandled.Set();
            },
                client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(10)));
            MessageUPtr reply = make_unique<Message>();
            ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
            VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");

            auto closeoperation = server->BeginClose(
                [&](AsyncOperationSPtr const & closeoperation)
            {
                ErrorCode endRequestErrorCode = server->EndClose(closeoperation);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Closing The Connection");
                closeEvent.Set();
            }, Common::AsyncOperationSPtr());
            VERIFY_IS_TRUE(closeEvent.WaitOne(TimeSpan::FromSeconds(3)));
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(OpeningDifferentListenerWithSamePathTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();

            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent openEvent(false);

            IServiceCommunicationListenerPtr server1;
            IServiceCommunicationListenerPtr server2;

            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler1;
            auto endpointAddress1 = CreateAndOpen(partitionId.ToString(), 10006, false, TimeSpan::FromSeconds(0), service, handler1, server1);
            Api::IServiceConnectionHandlerPtr handler2;
            ErrorCode alreadyexist = ErrorCodeValue::AlreadyExists;

            auto endpointAddress2 = CreateAndOpen(partitionId.ToString(), 10006, false, TimeSpan::FromSeconds(0), service, handler2, server2, alreadyexist);

            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress1, IServiceCommunicationMessageHandlerPtr(), IServiceConnectionEventHandlerPtr());

            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(10),
                [&](AsyncOperationSPtr const &)
            {
                isMessageHandled.Set();
            },
                client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(15)));
            MessageUPtr reply;
            ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
            VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
            server1->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(OpeningSameListenerAgainTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();

            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent openEvent(false);

            IServiceCommunicationListenerPtr server;

            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress1 = CreateAndOpen(partitionId.ToString(), 10007, false, TimeSpan::FromSeconds(0), service, handler, server);

            auto operation = server->BeginOpen(
                [&](AsyncOperationSPtr const &)
            {
                openEvent.Set();
            },
                Common::AsyncOperationSPtr());

            VERIFY_IS_TRUE(openEvent.WaitOne(TimeSpan::FromSeconds(5)));
            wstring endopintAddress2;

            auto errorcode = server->EndOpen(operation, endopintAddress2);
            VERIFY_IS_FALSE(errorcode.IsSuccess(), L"Opening Listener Second Time Failed");

            //  Send request from client to server
            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress1, IServiceCommunicationMessageHandlerPtr(), IServiceConnectionEventHandlerPtr());
            auto sendoperation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const &)
            {
                isMessageHandled.Set();
            },
                client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(10)));
            MessageUPtr reply;
            ErrorCode endRequestErrorCode = client->EndRequest(sendoperation, reply);
            VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(AbortAfterClosingListenerTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent closeEvent(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10008, false, TimeSpan::FromSeconds(0), service, handler, server);

            auto closeoperation = server->BeginClose(
                [&](AsyncOperationSPtr const & closeoperation)
            {
                ErrorCode endRequestErrorCode = server->EndClose(closeoperation);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Closing The Connection");
                closeEvent.Set();
            }, Common::AsyncOperationSPtr());

            VERIFY_IS_TRUE(closeEvent.WaitOne(TimeSpan::FromSeconds(5)));
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }


        BOOST_AUTO_TEST_CASE(AbortListenerTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent closeEvent(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10008, false, TimeSpan::FromSeconds(0), service, handler, server);
            //Abort the listener
            server->AbortListener();
            //Try to create transport on same port to verify if abort cleanup happens
            wstring address;
            address += Common::wformatString("{0}:{1}", L"localhost", 10008);
            auto settings = make_unique<ServiceCommunicationTransportSettings>();
            IServiceCommunicationTransportSPtr transportSPtr;
            ServiceCommunicationTransport::Create(move(settings),
                address,
                transportSPtr);
            auto error = transportSPtr->Open();
            VERIFY_IS_TRUE(error.IsSuccess());
            transportSPtr->Close();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(TransportSecuredSettingsTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);
            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent closeEvent(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10009, true, TimeSpan::FromSeconds(0), service, handler, server);
            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(true, TimeSpan::FromSeconds(3), endpointAddress, IServiceCommunicationMessageHandlerPtr(), IServiceConnectionEventHandlerPtr());
            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(10),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                isMessageHandled.Set();
            }, client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(20)));

            auto closeoperation = server->BeginClose(
                [&](AsyncOperationSPtr const & closeoperation)
            {
                ErrorCode endRequestErrorCode = server->EndClose(closeoperation);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Closing The Connection");
                closeEvent.Set();
            }, Common::AsyncOperationSPtr());

            VERIFY_IS_TRUE(closeEvent.WaitOne(TimeSpan::FromSeconds(5)));
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);
        }

        BOOST_AUTO_TEST_CASE(SimpleNotificationTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent isNotificationHandled(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            auto connectionHandler = make_shared<DummyServiceConenctionHandler>(true);
            auto connectionHandlerPtr = RootedObjectPointer<IServiceConnectionHandler>(connectionHandler.get(), connectionHandler->CreateComponentRoot());
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10010, false, TimeSpan::FromSeconds(0), service, connectionHandlerPtr, server);
            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, service, IServiceConnectionEventHandlerPtr());
            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                isMessageHandled.Set();
            }, client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(10)));
            auto callbackChannel = connectionHandler->getClientConnection();
            MessageUPtr   notificationmsg = make_unique<Message>();
            auto sendnotificationoperation = callbackChannel->BeginRequest(
                move(notificationmsg), TimeSpan::FromSeconds(3),
                [&](AsyncOperationSPtr const & sendnotificationoperation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = callbackChannel->EndRequest(sendnotificationoperation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Service got reply ");
                isNotificationHandled.Set();
            }, callbackChannel.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isNotificationHandled.WaitOne(TimeSpan::FromSeconds(10)));
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(SimpleOneWayNotificationTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent isNotificationHandled(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            auto messageHandlerPtr = make_shared<DummyService>();
            auto messageHandler = RootedObjectPointer<IServiceCommunicationMessageHandler>(messageHandlerPtr.get(), messageHandlerPtr->CreateComponentRoot());
            auto connectionHandler = make_shared<DummyServiceConenctionHandler>(true);
            auto connectionHandlerPtr = RootedObjectPointer<IServiceConnectionHandler>(connectionHandler.get(), connectionHandler->CreateComponentRoot());
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10011, false, TimeSpan::FromSeconds(0), service, connectionHandlerPtr, server);
            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, messageHandler, IServiceConnectionEventHandlerPtr());
            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                isMessageHandled.Set();
            }, client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(10)));
            auto callbackChannel = connectionHandler->getClientConnection();
            MessageUPtr   notificationmsg = make_unique<Message>();
            auto errorCode = callbackChannel->SendOneWay(move(notificationmsg));
            VERIFY_IS_TRUE(errorCode.IsSuccess());
            VERIFY_IS_TRUE(messageHandlerPtr->WaitForNotificationRecieved(TimeSpan::FromSeconds(10)), L"Notification Recieved");
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(ClientConnectionEventHandlerTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();

            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent closeEvent(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10001, false, TimeSpan::FromSeconds(0), service, handler, server);
            MessageUPtr   msg = make_unique<Message>();
            auto connectionHandler = make_shared<DummyClientConnectionEventHandler>();
            auto connectionHandlerPtr = RootedObjectPointer<IServiceConnectionEventHandler>(connectionHandler.get(), connectionHandler->CreateComponentRoot());
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, IServiceCommunicationMessageHandlerPtr(), connectionHandlerPtr);
            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(3),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                //Connect Event Fired
                VERIFY_IS_TRUE(connectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(3)));
                isMessageHandled.Set();
            }, client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(10)));
            auto closeoperation = server->BeginClose(
                [&](AsyncOperationSPtr const & closeoperation)
            {
                ErrorCode endRequestErrorCode = server->EndClose(closeoperation);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Closing The Connection");
                closeEvent.Set();
            }, Common::AsyncOperationSPtr());


            VERIFY_IS_TRUE(closeEvent.WaitOne(TimeSpan::FromSeconds(3)));
            //Sending Message After CLosing Listener
            isMessageHandled.Reset();
            msg = make_unique<Message>();
            auto sendoperation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(3),
                [&](AsyncOperationSPtr const & sendoperation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(sendoperation, reply);
                //Disconnect Event Fired
                VERIFY_IS_TRUE(connectionHandler->WaitForDisconnectEvent(TimeSpan::FromSeconds(2)), L"Disconnect Event Fired");
                isMessageHandled.Set();

            }, client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(15)));
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(ServiceFailsConnectionTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);
            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            AutoResetEvent isNotificationHandled(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            auto clientConnectionHandler = make_shared<DummyClientConnectionEventHandler>();
            auto clientConnectionHandlerPtr = RootedObjectPointer<IServiceConnectionEventHandler>(clientConnectionHandler.get(), clientConnectionHandler->CreateComponentRoot());
            //error while connecting
            auto connectionHandler = make_shared<DummyServiceConenctionHandler>(false);
            auto connectionHandlerPtr = RootedObjectPointer<IServiceConnectionHandler>(connectionHandler.get(), connectionHandler->CreateComponentRoot());

            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10002, false, TimeSpan::FromSeconds(0), service, connectionHandlerPtr, server);

            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, service, clientConnectionHandlerPtr);

            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(10),
                [&](AsyncOperationSPtr const & operation)
            {
                VERIFY_IS_TRUE(connectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(2)), L"Service Connect Even Fired");
                VERIFY_IS_FALSE(clientConnectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(2)), L"Client Connect Even not Fired");
                VERIFY_IS_FALSE(connectionHandler->WaitForDisconnectEvent(TimeSpan::FromSeconds(2)), L"Disconnect Event Not Fired as Service was not connected");
                VERIFY_IS_FALSE(clientConnectionHandler->WaitForDisconnectEvent(TimeSpan::FromSeconds(2)), L"Disconnect Event not Fired as Client was connected");
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                Trace.WriteInfo(TestSource, "endRequestErrorCode: {0}", endRequestErrorCode);
                VERIFY_IS_TRUE(endRequestErrorCode.IsError(ErrorCodeValue::ServiceCommunicationCannotConnect));
                isMessageHandled.Set();
            },
                client.get_Root()->CreateAsyncOperationRoot());


            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(20)));

            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(ServiceConnectionHandlerTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            AutoResetEvent isNotificationHandled(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());

            auto connectionHandler = make_shared<DummyServiceConenctionHandler>(true);
            auto connectionHandlerPtr = RootedObjectPointer<IServiceConnectionHandler>(connectionHandler.get(), connectionHandler->CreateComponentRoot());

            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10003, false, TimeSpan::FromSeconds(0), service, connectionHandlerPtr, server);

            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, service, IServiceConnectionEventHandlerPtr());

            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                isMessageHandled.Set();
            }, client.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(connectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(10)));

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(15)));

            //Closing the client 
            client->AbortClient();
            //Service Sending Notification to close client
            auto callbackChannel = connectionHandler->getClientConnection();
            MessageUPtr   notificationmsg = make_unique<Message>();
            auto sendnotificationoperation = callbackChannel->BeginRequest(
                move(notificationmsg), TimeSpan::FromSeconds(3),
                [&](AsyncOperationSPtr const & sendnotificationoperation)
            {
                VERIFY_IS_TRUE(connectionHandler->WaitForDisconnectEvent(TimeSpan::FromSeconds(3)));
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = callbackChannel->EndRequest(sendnotificationoperation, reply);
                VERIFY_IS_FALSE(endRequestErrorCode.IsSuccess());
                isNotificationHandled.Set();
            }, server.get_Root()->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(isNotificationHandled.WaitOne(TimeSpan::FromSeconds(10)));
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_CASE(ConnectCallOnceForSameClientTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            AutoResetEvent isNotificationHandled(false);
            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());

            auto connectionHandler = make_shared<DummyServiceConenctionHandler>(true);
            auto connectionHandlerPtr = RootedObjectPointer<IServiceConnectionHandler>(connectionHandler.get(), connectionHandler->CreateComponentRoot());

            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10004, false, TimeSpan::FromSeconds(0), service, connectionHandlerPtr, server);

            MessageUPtr   msg = make_unique<Message>();
            auto client = CreateClient(false, TimeSpan::FromSeconds(0), endpointAddress, service, IServiceConnectionEventHandlerPtr());

            //  Send request from client to listener
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                isMessageHandled.Set();
            }, Common::AsyncOperationSPtr());

            VERIFY_IS_TRUE(connectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(10)));

            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(15)));

            //Reset Events
            isMessageHandled.Reset();
            connectionHandler->ResetConnectEvent();
            msg = make_unique<Message>();
            //  Sending Second request from same client to same listener

            operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(5),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Client got reply ");
                isMessageHandled.Set();
            }, client.get_Root()->CreateAsyncOperationRoot());


            VERIFY_IS_FALSE(connectionHandler->WaitForConnectEvent(TimeSpan::FromSeconds(10)));
            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(15)));

            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }
        BOOST_AUTO_TEST_CASE(ServerDisconnectTest)
        {
            Trace.WriteInfo(TestSource, "Entering {0}", __FUNCTION__);

            auto partitionId = Guid::NewGuid();
            Common::AutoResetEvent isMessageHandled(false);
            Common::AutoResetEvent closeEvent(false);

            IServiceCommunicationListenerPtr server;
            auto servicePtr = make_shared<DummyService>();
            auto service = RootedObjectPointer<IServiceCommunicationMessageHandler>(servicePtr.get(), servicePtr->CreateComponentRoot());
            Api::IServiceConnectionHandlerPtr handler;
            auto endpointAddress = CreateAndOpen(partitionId.ToString(), 10015, false, TimeSpan::FromSeconds(0), service, handler, server);

            MessageUPtr   msg = make_unique<Message>();

            auto client = CreateClient(false, TimeSpan::FromSeconds(3), endpointAddress, IServiceCommunicationMessageHandlerPtr(), IServiceConnectionEventHandlerPtr());
            //Close the Listener
            auto closeoperation = server->BeginClose(
                [&](AsyncOperationSPtr const & closeoperation)
            {

                ErrorCode endRequestErrorCode = server->EndClose(closeoperation);
                VERIFY_IS_TRUE(endRequestErrorCode.IsSuccess(), L"Closing The Connection");

                closeEvent.Set();

            }, Common::AsyncOperationSPtr());

            VERIFY_IS_TRUE(closeEvent.WaitOne(TimeSpan::FromSeconds(5)));

            //  Send request from client to server
            auto operation = client->BeginRequest(
                move(msg), TimeSpan::FromSeconds(3),
                [&](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply = make_unique<Message>();
                ErrorCode endRequestErrorCode = client->EndRequest(operation, reply);
                VERIFY_IS_TRUE(endRequestErrorCode.IsError(ErrorCodeValue::ServiceCommunicationCannotConnect), L"Cannot Connect  :");
                isMessageHandled.Set();
            },
                client.get_Root()->CreateAsyncOperationRoot());


            VERIFY_IS_TRUE(isMessageHandled.WaitOne(TimeSpan::FromSeconds(10)));
            client->AbortClient();
            server->AbortListener();
            Trace.WriteInfo(TestSource, "Leaving {0}", __FUNCTION__);

        }

        BOOST_AUTO_TEST_SUITE_END()


            IServiceCommunicationClientPtr TcpServiceCommunicationTests::CreateClient(bool secureMode, TimeSpan defaultTimeout, wstring const &  serverAddress, IServiceCommunicationMessageHandlerPtr messageHandler, IServiceConnectionEventHandlerPtr eventhandler)
        {
            auto factory = ServiceCommunicationClientFactory::Create();
            ErrorCode error;
            AutoResetEvent openEvent;
            IServiceCommunicationClientPtr client;

            if (secureMode) {
                SecuritySettings securitySettings;
                error = SecuritySettings::CreateNegotiateClient(
                    TransportSecurity().LocalWindowsIdentity(),
                    securitySettings);
                VERIFY_IS_TRUE(error.IsSuccess());

                auto settings = make_unique<ServiceCommunicationTransportSettings>(433354, 0, 5, securitySettings, defaultTimeout, defaultTimeout, defaultTimeout);
                error = factory->CreateServiceCommunicationClient(serverAddress,
                    settings,
                    messageHandler,
                    eventhandler,
                    client);
                VERIFY_IS_TRUE(error.IsSuccess());
            }
            else
            {
                auto settings = make_unique<ServiceCommunicationTransportSettings>();
                error = factory->CreateServiceCommunicationClient(serverAddress,
                    settings,
                    messageHandler,
                    eventhandler,
                    client);
                VERIFY_IS_TRUE(error.IsSuccess());
            }

            return client;
        }

        wstring TcpServiceCommunicationTests::CreateAndOpen(wstring path,
            ULONG port,
            bool secureMode,
            TimeSpan defaultTimeout,
            Api::IServiceCommunicationMessageHandlerPtr const & service,
            Api::IServiceConnectionHandlerPtr const & connectionHandler,
            IServiceCommunicationListenerPtr & server,
            Common::ErrorCode expectedErrorCode)
        {

            auto& factory = ServiceCommunicationListenerFactory::GetServiceCommunicationListenerFactory();
            wstring address;
            address += Common::wformatString("{0}:{1}", L"localhost", port);

            ErrorCode error;
            AutoResetEvent openEvent;
            if (secureMode)
            {
                SecuritySettings securitySettings;
                error = SecuritySettings::CreateNegotiateServer(L"", securitySettings);
                VERIFY_IS_TRUE(error.IsSuccess());

                auto settings = make_unique<ServiceCommunicationTransportSettings>(10, 0, 5, securitySettings, defaultTimeout, defaultTimeout, defaultTimeout);
                error = factory.CreateServiceCommunicationListener(move(settings),
                    address,
                    path,
                    service,
                    connectionHandler,
                    server);
                VERIFY_IS_TRUE(error.IsSuccess());
            }
            else
            {
                auto settings = make_unique<ServiceCommunicationTransportSettings>();
                error = factory.CreateServiceCommunicationListener(move(settings),
                    address,
                    path,
                    service,
                    connectionHandler,
                    server);
                VERIFY_IS_TRUE(error.IsSuccess());
            }

            auto operation = server->BeginOpen(
                [&](AsyncOperationSPtr const &)
                {
                    openEvent.Set();
                },
                Common::AsyncOperationSPtr());
            VERIFY_IS_TRUE(openEvent.WaitOne(TimeSpan::FromSeconds(3)));
            wstring endopintAddress;
            auto errorcode = server->EndOpen(operation, endopintAddress);
            VERIFY_IS_TRUE(errorcode.IsError(expectedErrorCode.ReadValue()));
            return endopintAddress;
        }


        Common::ErrorCode TcpServiceCommunicationTests::CreateAndOpenListenerWithNoFactory(wstring path,
            Api::IServiceCommunicationTransportSPtr const & transport,
            Api::IServiceCommunicationMessageHandlerPtr &  service,
            Api::IServiceConnectionHandlerPtr & connectionHandler)
        {
            ServiceCommunicationListenerSPtr listenerSPtr = make_shared<ServiceCommunicationListener>(transport);

            IServiceConnectionHandlerPtr connectionHandlerPtr;
            Common::AutoResetEvent openEvent(false);

            listenerSPtr->CreateAndSetDispatcher(service,
                connectionHandler,
                path);

            auto operation = listenerSPtr->BeginOpen([&](AsyncOperationSPtr const &)
            {
                openEvent.Set();
            },
                Common::AsyncOperationSPtr());

            VERIFY_IS_TRUE(openEvent.WaitOne(TimeSpan::FromSeconds(3)));
            wstring endopintAddress;
            return listenerSPtr->EndOpen(operation, endopintAddress);


        }
    }
}
