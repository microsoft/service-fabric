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
    using System.Threading;
    using System.Threading.Tasks;

    [SuppressMessage(FxCop.Category.Design, FxCop.Rule.TypesThatOwnDisposableFieldsShouldBeDisposable, Justification = "// TODO: Implement an IDisposable, or ensure cleanup.")]
    internal sealed class TokenValidationServiceBroker : NativeTokenValidationService.IFabricTokenValidationService
    {
        private static readonly InteropApi ValidateAsyncApi = new InteropApi {CopyExceptionDetailsToThreadErrorMessage = true};

        private readonly ITokenValidationService service;

        internal TokenValidationServiceBroker(ITokenValidationService service)
        {
            this.service = service;
        }

        internal ITokenValidationService Service
        {
            get
            {
                return this.service;
            }
        }

        NativeCommon.IFabricAsyncOperationContext NativeTokenValidationService.IFabricTokenValidationService.BeginValidateToken(IntPtr authToken, uint timeoutInMilliseconds, NativeCommon.IFabricAsyncOperationCallback callback)
        {
            string managedToken = NativeTypes.FromNativeString(authToken);
            TimeSpan managedTimeout = TimeSpan.FromMilliseconds(timeoutInMilliseconds);

            return Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => this.ValidateTokenAsync(managedToken, managedTimeout, cancellationToken), callback, "TokenValidationServiceBroker.ValidateTokenAsync", ValidateAsyncApi);
        }

        NativeTokenValidationService.IFabricTokenClaimResult NativeTokenValidationService.IFabricTokenValidationService.EndValidateToken(NativeCommon.IFabricAsyncOperationContext context)
        {
            ClaimDescriptionList claims = AsyncTaskCallInAdapter.End<ClaimDescriptionList>(context);
            return new FabricTokenClaimResult(claims);
        }

        private Task<ClaimDescriptionList> ValidateTokenAsync(string authToken, TimeSpan timeout, CancellationToken cancellationToken)
        {
            return this.service.ValidateTokenAsync(authToken, timeout, cancellationToken);
        }

        NativeTokenValidationService.IFabricTokenServiceMetadataResult NativeTokenValidationService.IFabricTokenValidationService.GetTokenServiceMetadata()
        {
            return new FabricTokenServiceMetadataResult(this.service.GetTokenServiceMetadata());
        }
    }
}