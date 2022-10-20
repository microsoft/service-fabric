// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Fabric;
    using System.IO;
    using System.Linq;
    using System.Net.Configuration;
    using System.Reflection;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport.Client;
    using Microsoft.ServiceFabric.FabricTransport.Runtime;
    using Microsoft.ServiceFabric.Services.Remoting.V1;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ServiceCommunicationTest
    {
        [TestMethod]
        [Owner("suchia")]
        public static void OpenListenerTest()
        {
            var service = new DummyService();
            var settings = FabricTransportSettings.GetDefault();
            settings.OperationTimeout = TimeSpan.FromSeconds(4);
            settings.MaxConcurrentCalls = 6;
            settings.MaxMessageSize = 43355;
            settings.MaxQueueSize = 35;
            var listenerAddress = new FabricTransportListenerAddress("localhost", 0, Guid.NewGuid().ToString());
            var remotingRemotingConnectionHandler = new FabricTransportTestConnectionHandler();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                remotingRemotingConnectionHandler);
            var opentask = listener.OpenAsync(CancellationToken.None);
            var address = opentask.Result;
            Assert.IsTrue(address.Length > 0);
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void ReadTransportSettingsFromConfig()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var settings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettings", fileName);
            Assert.IsTrue(settings.OperationTimeout.TotalSeconds == 2);
            Assert.IsTrue(settings.KeepAliveTimeout.TotalSeconds == 1000);
            Assert.IsTrue(settings.MaxConcurrentCalls == 5);
            Assert.IsTrue(settings.MaxQueueSize == 35);

            Assert.IsTrue(settings.SecurityCredentials.CredentialType == CredentialType.X509);
            var x509securityCredentail = (X509Credentials) settings.SecurityCredentials;
            Assert.IsTrue(x509securityCredentail.FindType == X509FindType.FindBySubjectName);
            Assert.IsTrue(x509securityCredentail.FindValue.Equals("CN=WinFabric-Test-SAN1-Alice"));
            Assert.IsTrue(x509securityCredentail.StoreName.Equals(X509Credentials.StoreNameDefault));
            Assert.IsTrue(x509securityCredentail.ProtectionLevel == ProtectionLevel.EncryptAndSign);
            Assert.IsTrue(x509securityCredentail.StoreLocation == StoreLocation.LocalMachine);
            var commonNames = new List<string>() {"WinFabric-Test-SAN1-Alice"};
            Assert.IsTrue(x509securityCredentail.RemoteCommonNames.SequenceEqual(commonNames));
        }

        [TestMethod]
        [Owner("suchia")]
        public static void ReadDefaultTransportSettingsFromConfigWhenSectionIsNotPresent()
        {
            var settings = FabricTransportSettings.GetDefault("Dummy");
            Assert.IsTrue(settings.OperationTimeout.TotalSeconds == TimeSpan.FromMinutes(5).TotalSeconds);
            Assert.IsTrue(settings.KeepAliveTimeout.TotalSeconds == TimeSpan.Zero.TotalSeconds);
            Assert.IsTrue(settings.MaxConcurrentCalls == 16*Environment.ProcessorCount);
            Assert.IsTrue(settings.MaxQueueSize == 10000);
            Assert.IsTrue(settings.SecurityCredentials.CredentialType == CredentialType.None);
        }

        [TestMethod]
        [Owner("suchia")]
        public static void TestReadSettingsUsingTryLoadFrom()
        {
            FabricTransportSettings settings = null;
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var isSucceeded = FabricTransportSettings.TryLoadFrom("TestServiceListenerTransportSettings", out settings,
                fileName);
            Assert.IsTrue(isSucceeded);
            Assert.IsTrue(settings.OperationTimeout.TotalSeconds == 2);
            Assert.IsTrue(settings.KeepAliveTimeout.TotalSeconds == 1000);
            Assert.IsTrue(settings.MaxConcurrentCalls == 5);
            Assert.IsTrue(settings.MaxQueueSize == 35);
            FabricTransportSettings settings2 = null;
            isSucceeded = FabricTransportSettings.TryLoadFrom("TestServiceListener", out settings2, fileName);
            Assert.IsFalse(isSucceeded);
            FabricTransportListenerSettings listenerSettings;
            isSucceeded = FabricTransportListenerSettings.TryLoadFrom("TestServiceListenerTransportSettings",
                out listenerSettings, "Config2");
            Assert.IsFalse(isSucceeded);
        }

        [TestMethod]
        [Owner("suchia")]
        public static void ReadTransportSettingsFromConfigWhenSectionOrFileIsNotPresent()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            try
            {
                var settings = FabricTransportSettings.LoadFrom("TestServiceListener", fileName);
                Assert.Fail("Exception should be thrown as Section is Not Present");
            }
            catch (ArgumentException)
            {
                //Expected Exception
            }
            try
            {
                var settings = FabricTransportSettings.LoadFrom("TestServiceListener");
                Assert.Fail("Exception should be thrown as FileName is Not Present");
            }
            catch (ArgumentException)
            {
                //Expected Exception
            }
        }

        [TestMethod]
        [Owner("suchia")]
        public static void ReadTransportListenerSettingsFromConfigPackage()
        {
            try
            {
                var settings = FabricTransportListenerSettings.LoadFrom("TestServiceListener", "Config1");
                Assert.Fail("Exception should be thrown as ConfigPackage is Not Present");
            }
            catch (ArgumentException)
            {
                //Expected Exception
            }

            try
            {
                var settings = FabricTransportListenerSettings.LoadFrom("TestServiceListener");
                Assert.Fail("Exception should be thrown as Section is Not Present");
            }
            catch (ArgumentException)
            {
                //Expected Exception
            }
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SecuredCommunicationTestWithThumbprints()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var settings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettingsWithThumbprint", fileName);
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 30, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var clientsetting = FabricTransportSettings.LoadFrom("TestTransportClientTransportSettingsWithThumbprint",
                fileName);
            var client = new FabricTransportClient(clientsetting, address, null);
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsFalse(reply.IsException);
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SecuredCommunicationTestWithSecondaryThumbprints()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var settings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettingsWithSecondaryThumbprint", fileName);
            byte[] replybody = { 4, 5 };
            byte[] replyheader = { 8 };
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = { 4, 5 };
            var listenerAddress = new FabricTransportListenerAddress("localhost", 30, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var clientSettings = FabricTransportSettings.LoadFrom("TestTransportClientTransportSettingsWithSecondaryThumbprint", fileName);

            var client = new FabricTransportClient(clientSettings, address, null);
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsFalse(reply.IsException);
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SecuredCommunicationTestWithSubjectName()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var settings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettingsWithSubjectName", fileName);
            byte[] replybody = { 4, 5 };
            byte[] replyheader = { 8 };
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = { 4, 5 };
            var listenerAddress = new FabricTransportListenerAddress("localhost", 30, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var clientSettings = FabricTransportSettings.LoadFrom("TestTransportClientTransportSettingsWithSubjectName", fileName);
            var client = new FabricTransportClient(clientSettings, address, null);
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsFalse(reply.IsException);
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("ratando")]
        public static void SecuredCommunicationTestWithIssuerStore()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var settings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettingsWithIssuerStore", fileName);
            byte[] replybody = { 4, 5 };
            byte[] replyheader = { 8 };
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = { 4, 5 };
            var listenerAddress = new FabricTransportListenerAddress("localhost", 30, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var clientSettings = FabricTransportSettings.LoadFrom("TestTransportClientTransportSettingsWithIssuerStore", fileName);
            var client = new FabricTransportClient(clientSettings, address, null);
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsFalse(reply.IsException);
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void TestWhenClientTimeoutForLongRunningTask()
        {
            var settings = new FabricTransportListenerSettings();
            settings.OperationTimeout = TimeSpan.FromMinutes(1);
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            //Service will drop the requestMessage and client will see timeout for these request
            var service = new DummyService(replyheader, replybody, dropMessage: true);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 31, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, settings.OperationTimeout);
                var reply = taskop.Result;
                Assert.Fail("Request dint fail with Timeout Exception");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.InnerException is TimeoutException);
            }

            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SecuredCommunicationTestWithMismatchSecurity()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var listenersettings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettingsWithThumbprint", fileName);
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 30, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(
                listenersettings,
                listenerAddress,
                service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var settings = FabricTransportSettings.LoadFrom("TestTransportClientTransportSettings");
            var client = new FabricTransportClient(settings, address, null);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail("FabricServerAuthenticationFailedException shuld have been thrown");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.InnerException is FabricServerAuthenticationFailedException , "Exception thrown " + e.Flatten().InnerException);
            }

            //Sending second Request
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail("FabricServerAuthenticationFailedException shuld have been thrown for second request");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.InnerException is FabricServerAuthenticationFailedException);
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SecuredCommunicationTestWithClientUnSecure()
        {
            var fileName = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "ServiceCommunicationTestSettings.xml");
            var listenersettings = FabricTransportSettings.LoadFrom("TestServiceListenerTransportSettings", fileName);
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 30, Guid.NewGuid().ToString());

            //listener is opened with secure transport settings.
            var listener = new FabricTransportListener(
                listenersettings,
                listenerAddress,
                service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var unSecureSettings = new FabricTransportSettings();
            //Client is created with unsecure settings
            var client = new FabricTransportClient(unSecureSettings, address, null);

            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail("FabricConnectionDeniedException should have been thrown");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.InnerException is FabricConnectionDeniedException);
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void OpenListenerWithDefaultSettings()
        {
            var service = new DummyService();
            var settings = FabricTransportSettings.GetDefault();
            var listenerAddress = new FabricTransportListenerAddress("localhost", 0, Guid.NewGuid().ToString());
            var remotingRemotingConnectionHandler = new FabricTransportTestConnectionHandler();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                remotingRemotingConnectionHandler);
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void WhenServiceThrowsException()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            //service throws Exception
            var service = new DummyService(replyheader, replybody, throwsException: true);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 18, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                ThrowfItHasException(reply);
                Assert.Fail("Request should fail with Service thrown Exception");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.Flatten().InnerException is InvalidOperationException);
                Assert.IsTrue(e.Flatten().InnerException.Message.Equals("Exception From Service"));
            }
            catch (Exception e)
            {
                Assert.Fail("Not the Right Type of Exception" + e.InnerException + ":" + e.StackTrace);
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SimpleCommunicationTest()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsFalse(reply.IsException);
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
           listener.CloseAsync(CancellationToken.None).Wait();

        }

        [TestMethod]
        [Owner("suchia")]
        public static void TestWhenServiceThrowsExceptionDuringConnect()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();

            var handler = new FabricTransportTestConnectionHandler(failOnConnection: true);
            var listener = new FabricTransportListener(
                settings,
                listenerAddress,
                service,
                handler);

            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail("Exception Expected As Service throws Exception during Connect");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.Flatten().InnerException is FabricCannotConnectException);
            }

            Assert.IsTrue(handler.count == 1, "Tried to Connect for first connect Request");
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail("For all Request for same client should get same FabricCannotConnectException Exception ");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.Flatten().InnerException is FabricCannotConnectException);
            }
            Assert.IsTrue(handler.count == 1, "Not Tried to Connect, As it should take previous connect Result");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void TestWhenConnectRequestTimedOut()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            //handler with delay as true
            var handler = new FabricTransportTestConnectionHandler(false, true);
            var listener = new FabricTransportListener(
                settings,
                listenerAddress,
                service,
                handler);

            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(15000));
                var reply = taskop.Result;
                Assert.Fail("Client Request dint throw cannotConnect for timing out connect request");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.Flatten().InnerException is FabricCannotConnectException);
            }

            Assert.IsTrue(handler.count == 1, "Once ConnectHandler called");
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail("Client Request dint throw cannotConnect as connect Request failed");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.Flatten().InnerException is FabricCannotConnectException);
            }
            Assert.IsTrue(handler.count == 1, "client dint try to re-connect.");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void TestWhenSendRequestAfterClosingListener()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 121, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            //Open Listener
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            //Send Request
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsFalse(reply.IsException);
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");

            //Close Listener. This should disconnect client
            listener.CloseAsync(CancellationToken.None).Wait();

            //Send Request Again on Same Disconnected Client
            try
            {
                var taskop1 = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply1 = taskop1.Result;
                Assert.Fail("Request dint fail with CannotConnect Exception");
            }
            catch (AggregateException aggregateException)
            {
                var ex = aggregateException.Flatten().InnerException;
                Assert.IsTrue(ex is FabricCannotConnectException);
            }
            //Open Listener on same endpoint
            var listener1 = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task1 = listener1.OpenAsync(CancellationToken.None);
            var address2 = task.Result;
            Assert.IsTrue(address2.Length > 0);
            Assert.IsTrue(address2 == address);

            //Send Request Again on disconnected client.Client won't reconnect after getting disconnected.

            try
            {
                var taskop2 = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply2 = taskop2.Result;
                Assert.Fail("Request dint fail with CannotConnect Exception");
            }
            catch (AggregateException aggregateException)
            {
                var ex = aggregateException.Flatten().InnerException;
                Assert.IsTrue(ex is FabricCannotConnectException);
            }

            listener1.CloseAsync(CancellationToken.None).Wait();
        }


        [TestMethod]
        [Owner("suchia")]
        public static void TestWhenSendRequestonClosedClient()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);

            var sendRequestsTasks = new List<Task>();
            for (var i = 0; i < 100; i++)
            {
                // Some Request will fail with CannotConnect Exception as Client will be in aborted state for some request.
                sendRequestsTasks.Add(Task.Run(() => { SendRequests(client); }));
            }
            var rnd = new Random();
            var number = rnd.Next(0, 5);
            Thread.Sleep(number*100);
            //Abort Client
            client.Abort();
            Task.WaitAll(sendRequestsTasks.ToArray());
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SendMultipleRequestButConnectFiredOnce()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var sendRequests = new List<Task<FabricTransportReplyMessage>>();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(
                settings,
                listenerAddress,
                service,
                new FabricTransportTestConnectionHandler());

            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var eventHandler = new ClientConnectionEvent();
            //EventHandler assert to check client connect fired only once.

            var client = new FabricTransportClient(settings, address, eventHandler);

            var sendRequestsTasks = new List<Task>();
            for (var i = 0; i < 100; i++)
            {
                sendRequestsTasks.Add(
                    Task.Run(
                        () => { SendRequests(client, false); }));
                ;
            }

            Task.WaitAll(sendRequestsTasks.ToArray());
            //Connect Event shoudl have fired once.
            Assert.IsTrue(eventHandler.AsyncConnectEvent.Wait(5000));

            listener.CloseAsync(CancellationToken.None).Wait();
            Thread.Sleep(2000);
            //As Listener Close will trigger Disconnect
            Assert.IsTrue(eventHandler.AsyncDisconnectEvent.Wait(5000));
        }


        [TestMethod]
        [Owner("suchia")]
        public static void ServiceTooBusyExceptionnTestWhenServiceQueueIsFull()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 2, Guid.NewGuid().ToString());
            var listenerSettings = new FabricTransportListenerSettings();
            listenerSettings.MaxQueueSize = 1;
            listenerSettings.MaxConcurrentCalls = 1;
            var listener = new FabricTransportListener(
                listenerSettings,
                listenerAddress,
                service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(new FabricTransportSettings(), address, null);

            var tasks = new List<Task<FabricTransportReplyMessage>>();
            for (var i = 0; i < 5; i++)
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                tasks.Add(taskop);
            }

            for (var i = 0; i < tasks.Count; i++)
            {
                var taskop = tasks[i];
                tasks.RemoveAt(i);
                try
                {
                    var reply = taskop.Result;
                }
                catch (AggregateException ex)
                {
                    Assert.IsTrue(ex.InnerException is FabricTransientException);
                    var transientException = (FabricTransientException) ex.InnerException;
                    Assert.IsTrue(transientException.ErrorCode.Equals(FabricErrorCode.ServiceTooBusy));
                }
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SimpleCommunicationTestWIthInvalidAddress()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 29, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var invalidAdress = "net.tcp://localhost:34002/35437c01-2291-483a-ab1f-f4b8e02938e5";
            try
            {
                var client = new FabricTransportClient(settings, invalidAdress, null);
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply = taskop.Result;
                Assert.Fail(" Request dint fail with FabricInvalidAddressException");
            }
            catch (FabricInvalidAddressException)
            {
                //Expected this Exception
            }
            catch (Exception e)
            {
                Assert.Fail("Not the Right Type of Exception" + e.InnerException + ":" + e.StackTrace);
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void TestListenerDisconnectingClientsDuringClose()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenersettings = new FabricTransportListenerSettings();
            var settings = new FabricTransportSettings();
            var listeners = new Dictionary<string, FabricTransportListener>();
            var connectionHandler = new FabricTransportTestConnectionHandler();

            // Create Multiple listener with same transport (address and port)
            for (var i = 0; i < 15; i++)
            {
                FabricTransportListener listener;
                string address;
                var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
                CreateAndOpenListener(service, listenerAddress, settings, connectionHandler, out listener, out address);
                listeners.Add(address, listener);
            }
            //   SendRequests to each listener
            var clients = new List<FabricTransportClient>();
            foreach (var address in listeners.Keys)
            {
                for (var i = 0; i < 50; i++)
                {
                    var client = new FabricTransportClient(settings, address, null);
                    clients.Add(client);
                    var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                    var reply = taskop.Result;

                    Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
                }
            }

            Assert.IsTrue(connectionHandler.count == 750, "Connected Clients should be 750");

            var tasks = new List<Task>();
            foreach (var listener in listeners.Values)
            {
                var closetask = listener.CloseAsync(CancellationToken.None);
                tasks.Add(closetask);
            }
            Task.WaitAll(tasks.ToArray());
            Assert.IsTrue(connectionHandler.count == 0, "Connected Clients should be zero");
            clients.Clear();
        }


        [TestMethod]
        [Owner("suchia")]
        public static void CallingConnectHandlerEvenforEndpointNotFoundScenario()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var listenersettings = new FabricTransportListenerSettings();
            var settings = new FabricTransportSettings();

            var connectionHandler = new FabricTransportTestConnectionHandler();

            // Create Multiple listener with same transport (address and port)

            FabricTransportListener listener1;
            string address1;
            var listenerAddress = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            CreateAndOpenListener(service, listenerAddress, settings, connectionHandler, out listener1, out address1);


            FabricTransportListener listener2;
            string address2;
            var listenerAddress2 = new FabricTransportListenerAddress("localhost", 25, Guid.NewGuid().ToString());
            CreateAndOpenListener(service, listenerAddress2, settings, connectionHandler, out listener2, out address2);


            //   SendRequests to listener1
            var client1 = new FabricTransportClient(settings, address1, null);
            var taskop1 = client1.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply1 = taskop1.Result;

            Assert.IsTrue(reply1.GetBody().SequenceEqual(replybody), "Client Got Reply");
            // closing listener1 
            var closetask = listener1.CloseAsync(CancellationToken.None);
            closetask.GetAwaiter().GetResult();

            //Creating client2 for sending request to listener1
            var eventHandler = new ClientConnectionEvent();
            var client2 = new FabricTransportClient(settings, address1, eventHandler);

            try
            {
                var taskop2 = client2.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
                var reply2 = taskop2.Result;
                Assert.Fail(
                    "Request should fail with CannotConnect Exception as client is trying to connect t closed listener");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(eventHandler.AsyncConnectEvent.Wait(5000),
                    "For EndpointNotFound scenario , we call connect (This needed for backward compatibility");
                Assert.IsTrue(e.InnerException is FabricEndpointNotFoundException);
            }
            listener2.CloseAsync(CancellationToken.None).Wait();
        }


        [TestMethod]
        [Owner("suchia")]
        public static void SimpleCommunicationWithNoReplyHeaderTest()
        {
            byte[] replybody = {4, 5};
            var header = GetRequestHeader();
            //Service will send reply with null header.
            var service = new DummyService(header: null, body: replybody);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 20, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody), "Client Got Reply");
            listener.CloseAsync(CancellationToken.None).Wait();
        }

#if false //Max message size checking is only meaningful for secure mode, re-enable this in secure mode
        [TestMethod]
        [Owner("suchia")]
        public static void MessageTooLargeTest()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            var body = new byte[500000];
            for (var i = 0; i < 500000; i++)
            {
                body[i] = (byte) i;
            }
            var listenerAddress = new FabricTransportListenerAddress("localhost", 15, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            settings.MaxMessageSize = 1;
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(10));
                var result = taskop.Result;
                Assert.Fail("Request dint fail with FabricMessageTooLarge Exception ");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.InnerException is FabricMessageTooLargeException);
            }
            catch (Exception e)
            {
                Assert.Fail("Not the Right Type of Exception" + e.InnerException + ":" + e.StackTrace);
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

#endif

        [TestMethod]
        [Owner("suchia")]
        public static void ServiceCommunicationCannotConnectTest()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var settings = FabricTransportSettings.GetDefault();
            var address = "localhost:62613+39c610ce-f63d-45e3-8a7b-eb4ae77dd349";
            //Creating client for  any random address.
            var client = new FabricTransportClient(settings, address, null);
            var eventHandler = new ClientConnectionEvent();
            var nativeeventHandler = new FabricTransportNativeClientConnectionEventHandler(eventHandler);
            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(10));
                var result = taskop.Result;
                Assert.Fail("Request dint fail with FabricCannotConnect Exception ");
            }
            catch (AggregateException e)
            {
                Assert.IsFalse(eventHandler.AsyncConnectEvent.Wait(5000));
                Assert.IsTrue(e.InnerException is FabricCannotConnectException);
            }
            catch (Exception e)
            {
                Assert.Fail("Not the Right Type of Exception" + e.InnerException + ":" + e.StackTrace);
            }
        }

        [TestMethod]
        [Owner("suchia")]
        public static void ServiceCommunicationFabricEndpointNotFoundTest()
        {
            byte[] replybody = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            byte[] body = {4, 5};
            var settings = FabricTransportSettings.GetDefault();
            var listenerAddress = new FabricTransportListenerAddress("localhost", 19, Guid.NewGuid().ToString());
            var listener = new FabricTransportListener(settings,
                listenerAddress, service,
                new FabricTransportTestConnectionHandler());
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var address1 = address.Split('+')[0] + "+1234";
            //Creaing Client for same ip address as opened listener but different service path
            var client = new FabricTransportClient(settings, address1, null);

            try
            {
                var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(100));
                var result = taskop.Result;
                Assert.Fail("This should throw FabricEndpointNotFound Exception ");
            }
            catch (AggregateException e)
            {
                Assert.IsTrue(e.InnerException is FabricEndpointNotFoundException, "Exception :" + e.InnerException);
            }
            catch (Exception e)
            {
                Assert.Fail("Not the Right Type of Exception" + e.InnerException + ":" + e.StackTrace);
            }
            listener.CloseAsync(CancellationToken.None).Wait();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SimpleNotificationAsyncTest()
        {
            byte[] replybody = {4, 5};
            byte[] replynotification = {4, 5};
            byte[] notificationByte = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            var callback = new DummyCallBackImplementation(replynotification);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 10, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var remotingRemotingConnectionHandler = new FabricTransportTestConnectionHandler();
            //Create and Open Listener
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                remotingRemotingConnectionHandler);
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null, callback);
            //Register For Notification
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000));
            var reply = taskop.Result;
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody));
            //SendNotification
            var notificationTask = service.SendNotificationAsync();
            var replymessage = notificationTask.Result;
            Assert.IsTrue(replymessage.SequenceEqual(replynotification), "Client Got Notification");
            client.Abort();
            listener.Abort();
        }

        [TestMethod]
        [Owner("suchia")]
        public static void SimpleNotificationOneWayTest()
        {
            byte[] replybody = {4, 5};
            byte[] replynotification = {4, 5};

            byte[] notificationByte = {4, 5};
            byte[] replyheader = {8};
            var header = GetRequestHeader();
            var service = new DummyService(replyheader, replybody);
            var callback = new DummyCallBackImplementation(replynotification);
            byte[] body = {4, 5};
            var listenerAddress = new FabricTransportListenerAddress("localhost", 15, Guid.NewGuid().ToString());
            var settings = FabricTransportSettings.GetDefault();
            var remotingRemotingConnectionHandler = new FabricTransportTestConnectionHandler();
            //Create and Open Listener
            var listener = new FabricTransportListener(settings, listenerAddress, service,
                remotingRemotingConnectionHandler);
            var task = listener.OpenAsync(CancellationToken.None);
            var address = task.Result;
            Assert.IsTrue(address.Length > 0);
            var client = new FabricTransportClient(settings, address, null, callback);
            //Register For Notification
            var taskop = client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(4000));
            service.AsyncEvent.Set();
            var reply = taskop.Result;
            Assert.IsTrue(reply.GetBody().SequenceEqual(replybody));
            //SendNotification
            service.SendNotificationOneWay();

            Assert.IsTrue(callback.AsyncEvent.WaitOne(TimeSpan.FromMilliseconds(5000)));
            Assert.IsTrue(replybody.SequenceEqual(callback.GetNotificationMessage()), "Client Got Notification");
            listener.CloseAsync(CancellationToken.None).Wait();
            client.Abort();
        }

        private static byte[] GetRequestHeader()
        {
            return new byte[10];
        }

        private static void ThrowfItHasException(FabricTransportReplyMessage replyMessage)
        {
            Assert.IsTrue(replyMessage.IsException);
            Exception e;
            var isDeserialzied =
                RemoteExceptionInformation.ToException(new RemoteExceptionInformation(replyMessage.GetBody()), out e);
            if (isDeserialzied)
            {
                throw new AggregateException(e);
            }
            else
            {
                throw new ArgumentException(@"failed to deserialize and get remote exception",
                    "remoteExceptionInformation");
            }
        }


        private static void CreateAndOpenListener(
            DummyService service,
            FabricTransportListenerAddress listenerAddress,
            FabricTransportSettings settings,
            FabricTransportTestConnectionHandler connectionHandler,
            out FabricTransportListener listener,
            out string address)
        {
            listener = new FabricTransportListener(
                settings,
                listenerAddress,
                service,
                connectionHandler);
            var task = listener.OpenAsync(CancellationToken.None);
            address = task.Result;
            Assert.IsTrue(address.Length > 0);
        }

        private static void SendRequests(FabricTransportClient client, bool ExceptionExpected = true)
        {
            byte[] body = {4, 5};
            var header = GetRequestHeader();
            var sendRequests = new List<Task<FabricTransportReplyMessage>>();

            for (var i = 0; i < 100; i++)
            {
                sendRequests.Add(client.RequestResponseAsync(header, body, TimeSpan.FromSeconds(1000)));
            }
            for (var i = 0; i < sendRequests.Count; i++)
            {
                try
                {
                    var res = sendRequests[i].Result;
                }
                catch (AggregateException e)
                {
                    if (!ExceptionExpected)
                    {
                        throw;
                    }
                    var ex = e.Flatten().InnerException;
                    Assert.IsTrue(ex is FabricCannotConnectException, e.Message);
                }
            }
        }
    }


  internal class DummyCallBackImplementation : IFabricTransportCallbackMessageHandler
    {
        private readonly byte[] replybody;
        private byte[] notificationMsg;

        public DummyCallBackImplementation(byte[] reply)
        {
            this.replybody = reply;
            this.AsyncEvent = new ManualResetEvent(false);
        }

        public ManualResetEvent AsyncEvent { get; private set; }


        public byte[] GetNotificationMessage()
        {
            return this.notificationMsg;
        }

        public Task<byte[]> RequestResponseAsync(byte[] messageHeaders, byte[] requestBody)
        {
            return Task.Factory.StartNew<byte[]>(() => { return this.replybody; });
        }

        public void OneWayMessage(byte[] messageHeaders, byte[] requestBody)
        {
            this.notificationMsg = requestBody;
            this.AsyncEvent.Set();
        }
    }

    internal class DummyService : IFabricTransportMessageHandler
    {
        private FabricTransportRequestContext requestContext;
        private readonly byte[] header;
        private readonly byte[] body;

        private readonly bool throwsException;
        private readonly bool dropMessage;

        public DummyService(bool throwsException = false, bool dropMessage = false)
        {
            this.throwsException = throwsException;
            this.dropMessage = dropMessage;
        }

        public DummyService(byte[] header, byte[] body, bool throwsException = false, bool dropMessage = false)
        {
            this.AsyncEvent = new ManualResetEventSlim();
            this.AsyncEvent.Reset();
            this.header = header;
            this.body = body;
            this.throwsException = throwsException;
            this.dropMessage = dropMessage;
        }

        public ManualResetEventSlim AsyncEvent { get; private set; }


        private static FabricTransportReplyMessage CrateExceptionMessage()
        {
            var remoteExceptionInformation =
                RemoteExceptionInformation.FromException(new InvalidOperationException("Exception From Service"));
            return new FabricTransportReplyMessage(true, remoteExceptionInformation.Data);
        }


        public Task<byte[]> SendNotificationAsync()
        {
            return this.requestContext.GetCallbackClient().RequestResponseAsync(this.header, this.body);
        }

        public void SendNotificationOneWay()
        {
            this.requestContext.GetCallbackClient().OneWayMessage(this.header, this.body);
        }

        public Task<FabricTransportReplyMessage> RequestResponseAsync(FabricTransportRequestContext requestContext,
            byte[] messageHeaders, byte[] requestBody)
        {
            return Task.Factory.StartNew<FabricTransportReplyMessage>(
                () =>
                {
                    if (this.throwsException)
                    {
                        return CrateExceptionMessage();
                    }
                    if (this.dropMessage)
                    {
                        Thread.Sleep(TimeSpan.FromMinutes(5));
                    }
                    this.requestContext = requestContext;
                    return new FabricTransportReplyMessage(false, this.body);
                });
        }


        public void HandleOneWay(FabricTransportRequestContext requestContext, byte[] messageHeaders, byte[] requestBody)
        {
            ;
        }

        private static void ThrowException()
        {
            throw new InvalidOperationException("Exception From Service");
        }
    }

    public class FabricTransportTestConnectionHandler : IFabricTransportConnectionHandler
    {
        public int count = 0;
        private readonly Object thisLock = new Object();
        private readonly bool failOnConnection;
        private readonly bool isDelay;
		private FabricTransportCallbackClient callback;
		
        public FabricTransportTestConnectionHandler(bool failOnConnection = false, bool isdelay = false)
        {
            this.failOnConnection = failOnConnection;
            this.isDelay = isdelay;
        }

        Task IFabricTransportConnectionHandler.ConnectAsync(
            FabricTransportCallbackClient fabricTransportServiceRemotingCallback, TimeSpan timeout)
        {
            lock (this.thisLock)
                this.count++;
			this.callback = fabricTransportServiceRemotingCallback;
            if (this.failOnConnection)
            {
                throw new InvalidEnumArgumentException();
            }
            if (this.isDelay)
            {
//More than default connect Timeout(5 sec)
                return Task.Delay(TimeSpan.FromSeconds(7));
            }

            return Task.FromResult(true);
        }

        Task IFabricTransportConnectionHandler.DisconnectAsync(string clientId, TimeSpan timeout)
        {
            lock (this.thisLock)
                this.count--;
            return Task.FromResult(true);
        }

        FabricTransportCallbackClient IFabricTransportConnectionHandler.GetCallBack(string clientId)
        {
           return this.callback;
        }
    }

    internal class ClientConnectionEvent : IFabricTransportClientConnectionHandler
    {
        public ClientConnectionEvent()
        {
            this.AsyncConnectEvent = new ManualResetEventSlim();
            this.AsyncDisconnectEvent = new ManualResetEventSlim();
        }

        public void OnConnected()
        {
            Assert.IsFalse(this.AsyncConnectEvent.IsSet, "Connect Event not yet fired");
            this.AsyncConnectEvent.Set();
        }

        public ManualResetEventSlim AsyncConnectEvent { get; set; }
        public ManualResetEventSlim AsyncDisconnectEvent { get; set; }


        public void OnDisconnected()
        {
            Assert.IsTrue(this.AsyncConnectEvent.IsSet, "Connect Fired Once");
            Assert.IsFalse(this.AsyncDisconnectEvent.IsSet, "Disconnect Not Fired yet");
            this.AsyncDisconnectEvent.Set();
        }
    }
}