// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ContainerVolumeDescription
    {
        internal ContainerVolumeDescription()
        {
        }

        internal string Source { get; set; }

        internal string Destination { get; set; }

        internal string Driver { get; set; }

        internal bool IsReadOnly { get; set; }

        internal List<ContainerDriverOptionDescription> DriverOpts { get; set; }

        internal static unsafe ContainerVolumeDescription CreateFromNative(
            NativeTypes.FABRIC_CONTAINER_VOLUME_DESCRIPTION nativeDescription)
        {
            return new ContainerVolumeDescription
            {
                Source = NativeTypes.FromNativeString(nativeDescription.Source),
                Destination = NativeTypes.FromNativeString(nativeDescription.Destination),
                Driver = NativeTypes.FromNativeString(nativeDescription.Driver),
                IsReadOnly = NativeTypes.FromBOOLEAN(nativeDescription.IsReadOnly),
                DriverOpts = ContainerDriverOptionDescription.CreateFromNativeList(nativeDescription.DriverOpts)
            };
        }

        internal static unsafe List<ContainerVolumeDescription> CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerVolumeDescription.CreateFromNative() has null pointer.");

            var volumeList = new List<ContainerVolumeDescription>();

            var nativeList = (NativeTypes.FABRIC_CONTAINER_VOLUME_DESCRIPTION_LIST*)nativePtr;

            if (nativeList->Count > 0)
            {
                var items = (NativeTypes.FABRIC_CONTAINER_VOLUME_DESCRIPTION*)(nativeList->Items);

                for (var idx = 0; idx < nativeList->Count; ++idx)
                {
                    volumeList.Add(CreateFromNative(items[idx]));
                }
            }

            return volumeList;
        }
    }
}