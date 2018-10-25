// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System.Collections.Generic;
    using System.Fabric.Interop;

    /// <summary>
    /// Represents the metadata used to acquire authentication tokens from Azure Active Directory.
    /// </summary>
    public sealed class AzureActiveDirectoryMetadata
    {
        internal AzureActiveDirectoryMetadata()
        {
        }

        /// <summary>
        /// Gets a value indicating the Azure Active Directory login instance endpoint (for ADAL initialization).
        /// </summary>
        /// <value>
        /// Returns the login instance endpoint.
        /// </value>
        public string LoginEndpoint { get; internal set; }

        /// <summary>
        /// Gets a value indicating the authority location to acquire a token from.
        /// </summary>
        /// <value>
        /// Returns the authority.
        /// </value>
        public string Authority { get; internal set; }

        /// <summary>
        /// Gets a value indicating the tenant ID of the relevant Azure Active Directory tenant.
        /// </summary>
        /// <value>
        /// Returns the tenant ID.
        /// </value>
        public string TenantId { get; internal set; }

        /// <summary>
        /// Gets a value indicating the cluster application resource.
        /// </summary>
        /// <value>
        /// Returns the cluster application resource.
        /// </value>
        public string ClusterApplication { get; internal set; }

        /// <summary>
        /// Gets a value indicating the client ID to use when acquiring tokens for the cluster application resource.
        /// </summary>
        /// <value>
        /// Returns the client ID.
        /// </value>
        public string ClientApplication { get; internal set; }

        /// <summary>
        /// Gets a value indicating the client redirect URI.
        /// </summary>
        /// <value>
        /// Returns the client redirect URI.
        /// </value>
        public string ClientRedirect { get; internal set; }

        internal static unsafe AzureActiveDirectoryMetadata FromNative(IntPtr nativePtr)
        {
            var casted = (NativeTypes.FABRIC_CLAIMS_RETRIEVAL_METADATA*)nativePtr;

            if (casted->Kind == NativeTypes.FABRIC_CLAIMS_RETRIEVAL_METADATA_KIND.FABRIC_CLAIMS_RETRIEVAL_METADATA_KIND_AAD)
            {
                var castedAad = (NativeTypes.FABRIC_AAD_CLAIMS_RETRIEVAL_METADATA*)casted->Value;

                var result = new AzureActiveDirectoryMetadata();

                result.Authority = NativeTypes.FromNativeString(castedAad->Authority);
                result.TenantId = NativeTypes.FromNativeString(castedAad->TenantId);
                result.ClusterApplication = NativeTypes.FromNativeString(castedAad->ClusterApplication);
                result.ClientApplication = NativeTypes.FromNativeString(castedAad->ClientApplication);
                result.ClientRedirect = NativeTypes.FromNativeString(castedAad->ClientRedirect);

                if (castedAad->Reserved != IntPtr.Zero)
                {
                    var castedEx1 = (NativeTypes.FABRIC_AAD_CLAIMS_RETRIEVAL_METADATA_EX1*)castedAad->Reserved;

                    result.LoginEndpoint = NativeTypes.FromNativeString(castedEx1->LoginEndpoint);
                }

                return result;
            }
            else
            {
                return null;
            }
        }

        internal static IDictionary<string, string> GetDefaultAzureActiveDirectoryConfigurations()
        {
            var result = NativeTokenValidationService.GetDefaultAzureActiveDirectoryConfigurations();
            return NativeTypes.FromNativeStringMap(result);
        }
    }
}