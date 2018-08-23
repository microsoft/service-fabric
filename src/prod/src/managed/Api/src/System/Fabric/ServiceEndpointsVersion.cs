// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Used to compare two <see cref="System.Fabric.ServiceNotification" /> objects and determine which notification event precedes the other.</para>
    /// </summary>
    public sealed class ServiceEndpointsVersion
    {
        private NativeClient.IFabricServiceEndpointsVersion nativeResult;

        internal ServiceEndpointsVersion(NativeClient.IFabricServiceEndpointsVersion nativeResult)
        {
            this.nativeResult = nativeResult;
        }

        /// <summary>
        /// <para>Compares this version with the version of <paramref name="other" />.</para>
        /// </summary>
        /// <param name="other">
        /// <para>The other version to compare against.</para>
        /// </param>
        /// <returns>
        /// <para>Zero if this and <paramref name="other" /> are equivalent, a negative value if this is less than <paramref name="other" />, and a positive 
        /// value if this is greater than <paramref name="other" />.</para>
        /// </returns>
        public int Compare(ServiceEndpointsVersion other)
        {
            Requires.Argument<ServiceEndpointsVersion>("other", other).NotNull();

            return Utility.WrapNativeSyncInvokeInMTA<int>(
                () =>
                {
                    return this.CompareInternal(other);
                },
                "ServiceEndpointsVersion.Compare");
        }

        internal int CompareInternal(ServiceEndpointsVersion other)
        {
            return this.nativeResult.Compare(other.nativeResult);
        }
    }
}