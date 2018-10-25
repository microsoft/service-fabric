// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ContainerLabelDescription
    {
        public ContainerLabelDescription()
        {
        }

        public string Name { get; set; }

        public string Value { get; set; }

        public static unsafe ContainerLabelDescription CreateFromNative(
            NativeTypes.FABRIC_CONTAINER_LABEL_DESCRIPTION nativeDescription)
        {
            return new ContainerLabelDescription
            {
                Name = NativeTypes.FromNativeString(nativeDescription.Name),
                Value = NativeTypes.FromNativeString(nativeDescription.Value)
            };
        }

        public static unsafe List<ContainerLabelDescription> CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerLabelDescription.CreateFromNative() has null pointer.");

            var labelList = new List<ContainerLabelDescription>();

            var nativeList = (NativeTypes.FABRIC_CONTAINER_LABEL_DESCRIPTION_LIST*)nativePtr;

            if (nativeList->Count > 0)
            {
                var items = (NativeTypes.FABRIC_CONTAINER_LABEL_DESCRIPTION*)(nativeList->Items);

                for (var idx = 0; idx < nativeList->Count; ++idx)
                {
                    labelList.Add(CreateFromNative(items[idx]));
                }
            }

            return labelList;
        }
    }
}