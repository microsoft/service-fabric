// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides information about the executable entry point.</para>
    /// </summary>
    public sealed class ContainerHostEntryPointDescription : EntryPointDescription
    {
        internal ContainerHostEntryPointDescription()
            : base(CodePackageEntryPointKind.Container)
        {
            this.ImageName = string.Empty;
            this.Commands = string.Empty;
            this.EntryPoint = string.Empty;
        }

        /// <summary>
        /// <para>Gets or sets the Container Image name as specified in the service manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The Container Image name as specified in the service manifest.</para>
        /// </value>
        public string ImageName { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the commands passed to the container as specified in the service manifest. </para>
        /// </summary>
        /// <value>
        /// <para>The commands passed to the container as specified in the service manifest.</para>
        /// </value>
        public string Commands { get; internal set; }

        /// <summary>
        /// <para>Gets the entry point for the Contianer.</para>
        /// </summary>
        /// <value>
        /// <para>The entry point for the Contianer.</para>
        /// </value>
        public string EntryPoint { get; internal set; }

        /// <summary>
        /// <para>Gets the string representation of this entry point.</para>
        /// </summary>
        /// <returns>
        /// <para>The string representation of this entry point.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                @"ContinerHost(ImageName={0}, Commands={1}, EntryPoint={2})",
                this.ImageName,
                this.Commands,
                this.EntryPoint);
        }

        internal static new unsafe ContainerHostEntryPointDescription CreateFromNative(IntPtr entryPointDescriptionPtr)
        {
            NativeTypes.FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION* nativeEntryPointDescription = (NativeTypes.FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION*)entryPointDescriptionPtr;

            ContainerHostEntryPointDescription entryPointDescription = new ContainerHostEntryPointDescription
            {
                ImageName = NativeTypes.FromNativeString(nativeEntryPointDescription->ImageName),
                Commands = NativeTypes.FromNativeString(nativeEntryPointDescription->Commands),
                EntryPoint = NativeTypes.FromNativeString(nativeEntryPointDescription->EntryPoint)
            };

            return entryPointDescription;
        }
    }
}
