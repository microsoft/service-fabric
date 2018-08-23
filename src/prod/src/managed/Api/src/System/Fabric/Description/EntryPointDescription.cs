// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Describes an entry point that is associated with a code package.</para>
    /// </summary>
    public abstract class EntryPointDescription
    {
        /// <summary>
        /// <para>Creates and initializes an <see cref="System.Fabric.Description.EntryPointDescription.Kind" /> object with the specified 
        /// <see cref="System.Fabric.Description.CodePackageEntryPointKind" />.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>A <see cref="System.Fabric.Description.CodePackageEntryPointKind" /> specifying the type of entry point.</para>
        /// </param>
        protected EntryPointDescription(CodePackageEntryPointKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Indicates an entry point type.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.CodePackageEntryPointKind" />.</para>
        /// </value>
        public CodePackageEntryPointKind Kind { get; internal set; }

        internal static unsafe EntryPointDescription CreateFromNative(IntPtr entryPointDescriptionPtr)
        {
            NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION* nativeEntryPointDescription = (NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_DESCRIPTION*)entryPointDescriptionPtr;

            switch (nativeEntryPointDescription->Kind)
            {
                case NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST:
                    return ExeHostEntryPointDescription.CreateFromNative(nativeEntryPointDescription->Value);
                case NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_DLLHOST:
                    return DllHostEntryPointDescription.CreateFromNative(nativeEntryPointDescription->Value);
                case NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_CONTAINERHOST:
                    return ContainerHostEntryPointDescription.CreateFromNative(nativeEntryPointDescription->Value);
                default:
                    AppTrace.TraceSource.WriteError("EntryPointDescription.CreateFromNative", "Unknown code package entry point type : {0}", nativeEntryPointDescription->Kind);
                    ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownCodePackageEntryType_Formatted, nativeEntryPointDescription->Kind));
                    return null;
            }
        }
    }
}