//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.SecretStore;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Linq;

    public partial class FabricClient
    {
        internal sealed class SecretStoreClient
        {
            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricSecretStoreClient nativeClient;

            internal SecretStoreClient(FabricClient fabricClient, NativeClient.IFabricSecretStoreClient nativeClient)
            {
                this.fabricClient = fabricClient;
                this.nativeClient = nativeClient;
            }

            #region GetSecretsAsync()

            public Task<Secret[]> GetSecretsAsync(
                SecretReference[] secretReferences,
                bool includeValue,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return Utility.WrapNativeAsyncInvokeInMTA(
                   (callback) => this.GetSecretsAsyncBegin(
                       secretReferences,
                       includeValue,
                       timeout,
                       callback),
                   this.GetSecretsAsyncEnd,
                   cancellationToken,
                   "SecretStoreClient.GetSecretsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetSecretsAsyncBegin(
                 SecretReference[] secretReferences,
                bool includeValue,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClient.BeginGetSecrets(
                        SecretReference.ToNativeArray(pin, secretReferences),
                        NativeTypes.ToBOOLEAN(includeValue),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Secret[] GetSecretsAsyncEnd(NativeCommon.IFabricAsyncOperationContext context)
            {
                var result = this.nativeClient.EndGetSecrets(context);

                return Secret.FromNativeArray(result.get_Secrets());
            }

            #endregion

            #region SetSecretsAsync()

            public Task<SecretReference[]> SetSecretsAsync(
                Secret[] secrets,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return Utility.WrapNativeAsyncInvokeInMTA(
                   (callback) => this.SetSecretsAsyncBegin(
                       secrets,
                       timeout,
                       callback),
                   this.SetSecretsAsyncEnd,
                   cancellationToken,
                   "SecretStoreClient.SetSecretsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext SetSecretsAsyncBegin(
                Secret[] secrets,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClient.BeginSetSecrets(
                        Secret.ToNativeArray(pin, secrets),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private SecretReference[] SetSecretsAsyncEnd(NativeCommon.IFabricAsyncOperationContext context)
            {
                var result = this.nativeClient.EndSetSecrets(context);

                return SecretReference.FromNativeArray(result.get_SecretReferences());
            }

            #endregion

            #region RemoveSecretsAsync()

            public Task<SecretReference[]> RemoveSecretsAsync(
                SecretReference[] secretReferences,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return Utility.WrapNativeAsyncInvokeInMTA(
                   (callback) => this.RemoveSecretsAsyncBegin(
                       secretReferences,
                       timeout,
                       callback),
                   this.RemoveSecretsAsyncEnd,
                   cancellationToken,
                   "SecretStoreClient.RemoveSecretsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext RemoveSecretsAsyncBegin(
                SecretReference[] secretReferences,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClient.BeginRemoveSecrets(
                        SecretReference.ToNativeArray(pin, secretReferences),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private SecretReference[] RemoveSecretsAsyncEnd(NativeCommon.IFabricAsyncOperationContext context)
            {
                var result = this.nativeClient.EndRemoveSecrets(context);

                return SecretReference.FromNativeArray(result.get_SecretReferences());
            }

            #endregion

            #region GetSecretVersionsAsync()

            public Task<SecretReference[]> GetSecretVersionsAsync(
                SecretReference[] secretReferences,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return Utility.WrapNativeAsyncInvokeInMTA(
                   (callback) => this.GetSecretVersionsAsyncBegin(
                       secretReferences,
                       timeout,
                       callback),
                   this.GetSecretVersionsAsyncEnd,
                   cancellationToken,
                   "SecretStoreClient.GetSecretVersionsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetSecretVersionsAsyncBegin(
                SecretReference[] secretReferences,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClient.BeginGetSecretVersions(
                        SecretReference.ToNativeArray(pin, secretReferences),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private SecretReference[] GetSecretVersionsAsyncEnd(NativeCommon.IFabricAsyncOperationContext context)
            {
                var result = this.nativeClient.EndGetSecretVersions(context);

                return SecretReference.FromNativeArray(result.get_SecretReferences());
            }

            #endregion
        }
    }
}
