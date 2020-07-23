// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    
    internal sealed class ContainerDriverOptionDescription
    {
        internal ContainerDriverOptionDescription()
        {
        }

        internal string Name { get; set; }

        internal string Value { get; set; }

        internal bool IsEncrypted { get; set; }

        internal string Type { get; set; }

        internal static unsafe ContainerDriverOptionDescription CreateFromNative(
            NativeTypes.FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION nativeDescription)
        {
            var driverOptionDesc = new ContainerDriverOptionDescription
            {
                Name = NativeTypes.FromNativeString(nativeDescription.Name),
                Value = NativeTypes.FromNativeString(nativeDescription.Value),
                IsEncrypted = NativeTypes.FromBOOLEAN(nativeDescription.IsEncrypted)
            };

            if (nativeDescription.Reserved != null)
            {
                var nativeDescriptionEx = *((NativeTypes.FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION_EX1*)nativeDescription.Reserved);
                driverOptionDesc.Type = NativeTypes.FromNativeString(nativeDescriptionEx.Type);
            }

            return driverOptionDesc;
        }

        internal static unsafe List<ContainerDriverOptionDescription> CreateFromNativeList(IntPtr nativeListPtr)
        {
            ReleaseAssert.AssertIfNot(
                nativeListPtr != IntPtr.Zero,
                "ContainerDriverOptionDescription.CreateFromNativeList() has null pointer.");

            var driverOptionList = new List<ContainerDriverOptionDescription>();

            var nativeList = (NativeTypes.FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION_LIST*)nativeListPtr;

            if(nativeList->Count > 0)
            {
                var items = (NativeTypes.FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION*)(nativeList->Items);

                for (int i = 0; i < nativeList->Count; ++i)
                {
                    driverOptionList.Add(CreateFromNative(items[i]));
                }
            }            

            return driverOptionList;
        }
    }
}
