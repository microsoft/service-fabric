// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ContainerImageDescription
    {
        internal ContainerImageDescription()
        {
        }

        internal string ImageName { get; set; }

        internal RepositoryCredentialDescription RepositoryCredential { get; set; }

        internal bool UseDefaultRepositoryCredentials { get; set; }

        internal bool UseTokenAuthenticationCredentials { get; set; }

        internal static unsafe ContainerImageDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerImageDescription.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_CONTAINER_IMAGE_DESCRIPTION*)nativePtr);

            return CreateFromNative(nativeDescription);
        }

        internal static unsafe ContainerImageDescription CreateFromNative(
            NativeTypes.FABRIC_CONTAINER_IMAGE_DESCRIPTION nativeDescription)
        {
            var containerImageDescription =  new ContainerImageDescription
            {
                ImageName = NativeTypes.FromNativeString(nativeDescription.ImageName),
                RepositoryCredential = RepositoryCredentialDescription.CreateFromNative(nativeDescription.RepositoryCredential),
            };

            if (nativeDescription.Reserved != null)
            {
                var nativeParametersEx1 = *((NativeTypes.FABRIC_CONTAINER_IMAGE_DESCRIPTION_EX1*)nativeDescription.Reserved);
                containerImageDescription.UseDefaultRepositoryCredentials = NativeTypes.FromBOOLEAN(nativeParametersEx1.UseDefaultRepositoryCredentials);

                if(nativeParametersEx1.Reserved != null)
                {
                    var nativeParametersEx2 = *((NativeTypes.FABRIC_CONTAINER_IMAGE_DESCRIPTION_EX2*)nativeParametersEx1.Reserved);
                    containerImageDescription.UseTokenAuthenticationCredentials = NativeTypes.FromBOOLEAN(nativeParametersEx2.UseTokenAuthenticationCredentials);
                }
            }

            return containerImageDescription;
        }

        internal static unsafe List<ContainerImageDescription> CreateFromNativeList(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerImageDescription.CreateFromNativeList() has null pointer.");

            var imageList = new List<ContainerImageDescription>();

            var nativeList = (NativeTypes.FABRIC_CONTAINER_IMAGE_DESCRIPTION_LIST*)nativePtr;

            if(nativeList->Count > 0)
            {
                var items = (NativeTypes.FABRIC_CONTAINER_IMAGE_DESCRIPTION*)(nativeList->Items);

                for (var idx = 0; idx < nativeList->Count; ++idx)
                {
                    imageList.Add(CreateFromNative(items[idx]));
                }
            }

            return imageList;
        }
    }
}