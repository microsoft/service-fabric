// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Client
{
    using System;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport;

    internal class FabricTransportClient : IDisposable
    {
        private NativeServiceCommunication.IFabricServiceCommunicationClient2 nativeClient;
        protected FabricTransportSettings settings;

        public FabricTransportClient(
            FabricTransportSettings transportSettings,
            string connectionAddress,
            IFabricTransportClientConnectionHandler eventHandler,
            IFabricTransportCallbackMessageHandler contract = null)
        {
            this.ConnectionAddress = connectionAddress;
            this.settings = transportSettings;
            Utility.WrapNativeSyncInvokeInMTA(
                () => this.CreateNativeClient(transportSettings, connectionAddress, eventHandler, contract),
                "FabricTransportClient.Create");
        }

        public FabricTransportSettings Settings
        {
            get { return this.settings; }
        }

        public string ConnectionAddress { get; private set; }


        public virtual async Task<FabricTransportReplyMessage> RequestResponseAsync(byte[] header, byte[] requestBody,
            TimeSpan timeout)
        {
            try
            {
                return
                    await
                        Utility.WrapNativeAsyncInvokeInMTA<FabricTransportReplyMessage>(
                            (callback) => this.BeginRequest(header, requestBody, timeout, callback),
                            this.EndRequest,
                            CancellationToken.None,
                            "RequestResponseAsync");
            }
            catch (FabricCannotConnectException)
            {
                //TODO: Remove this check after Bug :1225032 gets resolved
                if (this.IsSecurityMismatch())
                {
                    throw new FabricConnectionDeniedException(SR.Error_ConnectionDenied);
                }
                throw;
            }
            catch (TimeoutException)
            {
                throw new TimeoutException(string.Format(CultureInfo.CurrentCulture, SR.ErrorServiceTooBusy));
            }
        }

        public virtual void SendOneWay(byte[] header, byte[] requestBody)
        {
            NativeServiceCommunication.IFabricServiceCommunicationMessage message =
                new FabricServiceCommunicationMessage(header, requestBody);
            this.nativeClient.SendMessage(message);
        }

        private bool IsSecurityMismatch()
        {
            //Cases where Client using unsecure and service using secure connection.
            if (this.ConnectionAddress.Contains(Helper.Secure) &&
                this.settings.SecurityCredentials.CredentialType.Equals(CredentialType.None))
            {
                return true;
            }
            return false;
        }

        private void CreateNativeClient(
            FabricTransportSettings transportSettings,
            string connectionAddress,
            IFabricTransportClientConnectionHandler eventHandler,
            IFabricTransportCallbackMessageHandler contract)
        {
            var iid = typeof(NativeServiceCommunication.IFabricServiceCommunicationClient2).GetTypeInfo().GUID;
            using (var pin = new PinCollection())
            {
                var nativeTransportSettings = transportSettings.ToNative(pin);
                var messageHandler = new FabricTransportCallbackHandlerBroker(contract);
                var nativeConnectionAddress = pin.AddBlittable(connectionAddress);
                var nativeEventHandler = new FabricTransportNativeClientConnectionEventHandler(eventHandler);
                this.nativeClient =
                    (NativeServiceCommunication.IFabricServiceCommunicationClient2)
                        NativeServiceCommunication.CreateServiceCommunicationClient(
                            ref iid,
                            nativeTransportSettings,
                            nativeConnectionAddress,
                            messageHandler,
                            nativeEventHandler);
            }
        }

        private NativeCommon.IFabricAsyncOperationContext BeginRequest(
            byte[] headers,
            byte[] body,
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinCollection())
            {
                var timeoutInMilliSeconds = Utility.ToMilliseconds(timeout, "timeout");
                NativeServiceCommunication.IFabricServiceCommunicationMessage message =
                    new FabricServiceCommunicationMessage(headers, body);
                return this.nativeClient.BeginRequest(message, timeoutInMilliSeconds, callback);
            }
        }

        private FabricTransportReplyMessage EndRequest(NativeCommon.IFabricAsyncOperationContext context)
        {
            var message = this.nativeClient.EndRequest(context);
            FabricTransportReplyMessage replyMessage = null;
            // Check If there is any ExceptionInfo In Header
            if (message.Get_Headers() != IntPtr.Zero)
            {
                replyMessage = new FabricTransportReplyMessage(true, Helper.Get_Byte(message.Get_Headers()));
            }
            else
            {
                replyMessage = new FabricTransportReplyMessage(false, Helper.Get_Byte(message.Get_Body()));
            }

            GC.KeepAlive(message);
            return replyMessage;
        }

        #region IDisposable Support

        private bool disposedValue = false; // To detect redundant calls


        //Used for Dummy Implemmentation
        protected FabricTransportClient()
        {
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposedValue)
            {
                if (disposing)
                {
                    // No other managed resources to dispose.
                }

                if (this.nativeClient != null)
                {
                    Marshal.FinalReleaseComObject(this.nativeClient);
                }

                this.disposedValue = true;
            }
        }

        public void Abort()
        {
            Utility.WrapNativeSyncInvokeInMTA(() => this.internalAbort(), "Client.Abort");
        }

        private void internalAbort()
        {
            this.nativeClient.Abort();
        }

        ~FabricTransportClient()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion

        public bool IsValid { get; set; }
    }
}