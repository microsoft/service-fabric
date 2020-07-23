// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Client
{
    using System;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using SR = Microsoft.ServiceFabric.FabricTransport.SR;

    internal class FabricTransportClient : IDisposable
    {
        private NativeFabricTransport.IFabricTransportClient nativeClient;
        protected FabricTransportSettings settings;

        public FabricTransportClient(
            FabricTransportSettings transportSettings,
            string connectionAddress,
            IFabricTransportClientEventHandler eventHandler,
            IFabricTransportCallbackMessageHandler contract,
            NativeFabricTransport.IFabricTransportMessageDisposer messageMessageDisposer)
        {
            this.ConnectionAddress = connectionAddress;
            this.settings = transportSettings;
            Utility.WrapNativeSyncInvokeInMTA(
                () => this.CreateNativeClient(transportSettings, connectionAddress, eventHandler, contract,messageMessageDisposer),
                "FabricTransportClient.Create");
        }

        public FabricTransportSettings Settings
        {
            get { return this.settings; }
        }

        public bool IsValid { get; set; }
        public string ConnectionAddress { get; private set; }

        public async Task OpenAsync(CancellationToken cancellationToken)
        {
            try
            {
                await   Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.BeginOpen(this.settings.ConnectTimeout, callback),
                    this.EndOpen,
                    cancellationToken,
                    "OpenAsync");
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

        public async Task CloseAsync(CancellationToken cancellationToken)
        {
            try
            {
                await Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.BeginClose(this.settings.ConnectTimeout, callback),
                    this.EndClose,
                    CancellationToken.None,
                    "OpenAsync");
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

        public async Task<FabricTransportMessage> RequestResponseAsync(FabricTransportMessage requestMessage,
            TimeSpan timeout)
        {
            try
            {
                return
                    await
                        Utility.WrapNativeAsyncInvokeInMTA<FabricTransportMessage>(
                            (callback) => this.BeginRequest(requestMessage, timeout, callback),
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

        public virtual void SendOneWay(FabricTransportMessage message)
        {
            NativeFabricTransport.IFabricTransportMessage nativeMessage =
                new NativeFabricTransportMessage(message);
            this.nativeClient.Send(nativeMessage);
        }

    
        private void CreateNativeClient(
            FabricTransportSettings transportSettings,
            string connectionAddress,
            IFabricTransportClientEventHandler eventHandler,
            IFabricTransportCallbackMessageHandler contract,
            NativeFabricTransport.IFabricTransportMessageDisposer messageMessageDisposer)
        {
            var iid = typeof(NativeFabricTransport.IFabricTransportClient).GetTypeInfo().GUID;
            using (var pin = new PinCollection())
            {
                var nativeTransportSettings = transportSettings.ToNativeV2(pin);
                var messageHandler = new FabricTransportCallbackHandlerBroker(contract);
                var nativeConnectionAddress = pin.AddBlittable(connectionAddress);
                var nativeEventHandler = new FabricTransportClientConnectionEventHandlerBroker(eventHandler);
                this.nativeClient =
                    NativeFabricTransport.CreateFabricTransportClient(
                        ref iid,
                        nativeTransportSettings,
                        nativeConnectionAddress,
                        messageHandler,
                        nativeEventHandler,
                        messageMessageDisposer);
            }
        }

        private NativeCommon.IFabricAsyncOperationContext BeginRequest(
            FabricTransportMessage message,
            TimeSpan timeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
                var timeoutInMilliSeconds = Utility.ToMilliseconds(timeout, "timeout");
                NativeFabricTransport.IFabricTransportMessage nativeFabricTransportMessage =
                    new NativeFabricTransportMessage(message);
                return this.nativeClient.BeginRequest(nativeFabricTransportMessage, timeoutInMilliSeconds, callback);
        }

        private FabricTransportMessage EndRequest(NativeCommon.IFabricAsyncOperationContext context)
        {
            var message = this.nativeClient.EndRequest(context);
            var reply = NativeFabricTransportMessage.ToFabricTransportMessage(message);
            GC.KeepAlive(message);
            return reply;
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



        private NativeCommon.IFabricAsyncOperationContext BeginOpen(TimeSpan connectTimeout,
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
          
                var timeoutInMilliSeconds = Utility.ToMilliseconds(connectTimeout, "timeout");
                return this.nativeClient.BeginOpen(timeoutInMilliSeconds, callback);
        }

        private void EndOpen(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeClient.EndOpen(context);
        }

        private NativeCommon.IFabricAsyncOperationContext BeginClose(TimeSpan connectTimeout,
         NativeCommon.IFabricAsyncOperationCallback callback)
        {
            using (var pin = new PinCollection())
            {
                var timeoutInMilliSeconds = Utility.ToMilliseconds(connectTimeout, "timeout");
                return this.nativeClient.BeginClose(timeoutInMilliSeconds, callback);
            }
        }

        private void EndClose(NativeCommon.IFabricAsyncOperationContext context)
        {
            this.nativeClient.EndClose(context);
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

    }
}

