// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes an service name.</para>
    /// </summary>
    public sealed class ServiceNameResult
    {
        internal ServiceNameResult()
        {
        }

        /// <summary>
        /// <para>Gets the name of the service.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ServiceName { get; internal set; }

        internal static unsafe ServiceNameResult CreateFromNativeResult(
            NativeClient.IFabricGetServiceNameResult result)
        {
            var nativePtr = result.get_ServiceName();
            var retval = FromNative((NativeTypes.FABRIC_SERVICE_NAME_QUERY_RESULT*)nativePtr);

            GC.KeepAlive(result);
            return retval;
        }

        private static unsafe ServiceNameResult FromNative(
            NativeTypes.FABRIC_SERVICE_NAME_QUERY_RESULT* nativeResultItem)
        {
            var item = new ServiceNameResult();

            item.ServiceName = NativeTypes.FromNativeUri(nativeResultItem->ServiceName);
           
            return item;
        }
    }
}