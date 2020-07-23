// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2.Runtime
{
    using System;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.FabricTransport.Runtime;

    internal class FabricTransportListener : IDisposable
    {
        private NativeFabricTransport.IFabricTransportListener nativeListner;

        public FabricTransportListener(
            FabricTransportSettings transportSettings,
            FabricTransportListenerAddress listenerAddress,
            IFabricTransportMessageHandler serviceImplementation,
            IFabricTransportConnectionHandler remotingConnectionHandler)
        {
            //TODO: Remove this address update  after Bug :1225032 gets resolved
            //Update the Address Path if Settings is Secure
            var isNotSecureEndpoint = transportSettings.SecurityCredentials.CredentialType.Equals(CredentialType.None);
            listenerAddress.Path = !isNotSecureEndpoint
                ? string.Format(CultureInfo.InvariantCulture, "{0}-{1}", listenerAddress.Path, Helper.Secure)
                : listenerAddress.Path;
            Utility.WrapNativeSyncInvokeInMTA(
                () =>
                    this.CreateNativeListener(serviceImplementation, transportSettings, listenerAddress,
                        remotingConnectionHandler),
                "FabricTransportListener");
        }

        private void CreateNativeListener(
            IFabricTransportMessageHandler contract,
            FabricTransportSettings transportSettings,
            FabricTransportListenerAddress listenerAddress,
            IFabricTransportConnectionHandler connectionHandler)
        {
            var iid = typeof(NativeFabricTransport.IFabricTransportListener).GetTypeInfo().GUID;

            using (var pin = new PinCollection())
            {
                var nativeTransportSettings = transportSettings.ToNativeV2(pin);
                var nativeListenerAddress = listenerAddress.ToNative(pin);
                var nativeConnectionHandler = new FabricTransportConnectionHandlerBroker(connectionHandler);
                var messageHandler = new FabricTransportMessageHandlerBroker(contract, connectionHandler);
                var nativeFabricTransportMessageDisposer = new NativeFabricTransportMessageDisposer();
                this.nativeListner = NativeFabricTransport.CreateFabricTransportListener(
                    ref iid,
                    nativeTransportSettings,
                    nativeListenerAddress,
                    messageHandler,
                    nativeConnectionHandler,
                    nativeFabricTransportMessageDisposer);
            }
        }

        private NativeCommon.IFabricAsyncOperationContext OpenBeginWrapper(
            NativeCommon.IFabricAsyncOperationCallback callback)
        {
            return this.nativeListner.BeginOpen(callback);
        }

        private string OpenEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return NativeTypes.FromNativeString(this.nativeListner.EndOpen(context));
        }

        #region API

        public Task<string> OpenAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA<string>(
                (callback) => this.OpenBeginWrapper(callback),
                this.OpenEndWrapper,
                cancellationToken,
                "FabricTransportListener.Open");
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return Utility.WrapNativeAsyncInvokeInMTA(
                (callback) => this.nativeListner.BeginClose(callback),
                this.nativeListner.EndClose,
                cancellationToken,
                "FabricTransportListener.Close");
        }

        public void Abort()
        {
            if (this.nativeListner != null)
            {
                Utility.WrapNativeSyncInvokeInMTA(() => this.nativeListner.Abort(), "Listner.Abort");
            }
        }

        #endregion

        #region IDisposable Support

        private bool disposedValue; // To detect redundant calls

        ~FabricTransportListener()
        {
            this.Dispose(false);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposedValue)
            {
                if (disposing)
                {
                    // No other managed resources to dispose.
                }

                if (this.nativeListner != null)
                {
                    Marshal.FinalReleaseComObject(this.nativeListner);
                }

                this.disposedValue = true;
            }
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion
    }
}