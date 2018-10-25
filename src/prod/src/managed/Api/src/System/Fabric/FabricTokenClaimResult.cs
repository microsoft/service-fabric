// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Description;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class FabricTokenClaimResult : NativeTokenValidationService.IFabricTokenClaimResult
    {
        ClaimDescriptionList claimsResult;
        internal FabricTokenClaimResult(ClaimDescriptionList claims)
        {
            this.claimsResult = claims;
        }

        IntPtr  NativeTokenValidationService.IFabricTokenClaimResult.get_Result()
        {
            using (PinCollection pin = new PinCollection())
            {
                return claimsResult.ToNative(pin);
            }
        }
    }
}