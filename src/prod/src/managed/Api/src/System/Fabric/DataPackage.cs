// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>This class represents a data package in the application. A data package consists of static data (that can be upgraded) that is consumed by the application. For more information see https://docs.microsoft.com/azure/service-fabric/service-fabric-application-model</para>
    /// </summary>
    public sealed class DataPackage
    {
        internal DataPackage()
        {
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Description.PackageDescription" /> object 
        /// associated with the <see cref="System.Fabric.DataPackage" />. The package description can be used to access additional information about this package such as the name, version etc. </para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.PackageDescription" /> object 
        /// associated with the <see cref="System.Fabric.DataPackage" />.</para>
        /// </value>
        public DataPackageDescription Description { get; internal set; }

        /// <summary>
        /// <para>Gets the local path of <see cref="System.Fabric.DataPackage" />. This path contains the contents of the data package.</para>
        /// </summary>
        /// <value>
        /// <para>The local path of <see cref="System.Fabric.DataPackage" />.</para>
        /// </value>
        public string Path { get; internal set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe DataPackage CreateFromNative(NativeRuntime.IFabricDataPackage nativePackage)
        {
            ReleaseAssert.AssertIfNull(nativePackage, "nativePackage");
            
            string path = NativeTypes.FromNativeString(nativePackage.get_Path());
            var nativeDescription = *((NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION*)nativePackage.get_Description());
            var description = DataPackageDescription.CreateFromNative(nativeDescription, path);

            var returnValue = new DataPackage()
            {
                Description = description,
                Path = path
            };

            GC.KeepAlive(nativePackage);
            return returnValue;
        }
    }
}
