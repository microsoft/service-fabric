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
    /// <para>Describes an application name.</para>
    /// </summary>
    public sealed class ApplicationNameResult
    {
        internal ApplicationNameResult()
        {
        }

        /// <summary>
        /// <para>Gets the name of the application.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public Uri ApplicationName { get; internal set; }

        internal static unsafe ApplicationNameResult CreateFromNativeResult(
            NativeClient.IFabricGetApplicationNameResult result)
        {
            var nativePtr = result.get_ApplicationName();
            var retval = FromNative((NativeTypes.FABRIC_APPLICATION_NAME_QUERY_RESULT*)nativePtr);

            GC.KeepAlive(result);
            return retval;
        }

        private static unsafe ApplicationNameResult FromNative(
            NativeTypes.FABRIC_APPLICATION_NAME_QUERY_RESULT* nativeResultItem)
        {
            var item = new ApplicationNameResult();

            item.ApplicationName = NativeTypes.FromNativeUri(nativeResultItem->ApplicationName);
           
            return item;
        }
    }
}