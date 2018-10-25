// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class FabricTokenServiceMetadataResult : NativeTokenValidationService.IFabricTokenServiceMetadataResult
    {
        TokenServiceMetadata  serviceMetadata;
        internal FabricTokenServiceMetadataResult(TokenServiceMetadata serviceMetadata)
        {
            this.serviceMetadata = serviceMetadata;
        }

        IntPtr  NativeTokenValidationService.IFabricTokenServiceMetadataResult.get_Result()
        {
            NativeTypes.FABRIC_TOKEN_SERVICE_METADATA tokenServiceMetadata;
            using (PinCollection pin = new PinCollection())
            {
                this.serviceMetadata.ToNative(pin, out tokenServiceMetadata);
                return pin.AddBlittable(tokenServiceMetadata);
            }
        }
    }
}