// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    internal sealed class DescriptionExtension
    {
        // Needed for serialization. Native side has this property
        internal string Key { get; set; }

        // Needed for serialization. Native side has this property
        internal string Value { get; set; }

        internal static unsafe Tuple<string, string> CreateFromNative(IntPtr nativeRaw)
        {
            NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION native = *(NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION*)nativeRaw;

            string name = NativeTypes.FromNativeString(native.Name);
            string value = NativeTypes.FromNativeString(native.Value);

            if (string.IsNullOrEmpty(name))
            {
                throw new ArgumentException(
                    StringResources.Error_ServiceTypeExtensionNameNullOrEmpty,
                    "nativeRaw");
            }

            AppTrace.TraceSource.WriteNoise("DescriptionExtension.CreateFromNative", "Extension {0} with value {1}", name, value);

            return Tuple.Create(name, value);
        }

        internal static unsafe IEnumerable<Tuple<string, string>> CreateFromNativeList(IntPtr extensionListPtr)
        {
            NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST* nativeExtensions = (NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION_LIST*)extensionListPtr;

            var descriptionExtensions = new List<Tuple<string, string>>();

            for (int i = 0; i < nativeExtensions->Count; i++)
            {
                IntPtr nativeExtension = (IntPtr)(((NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_EXTENSION*)nativeExtensions->Items) + i);

                descriptionExtensions.Add(DescriptionExtension.CreateFromNative(nativeExtension));
            }

            return descriptionExtensions;
        }
    }
}