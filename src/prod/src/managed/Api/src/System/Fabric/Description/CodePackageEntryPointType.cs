// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the possible types of entry points.</para>
    /// </summary>
    public enum CodePackageEntryPointKind
    {
        /// <summary>
        /// <para>Indicates that the HOST Type is invalid. Do not use.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_INVALID,
        
        /// <summary>
        /// <para>Indicates that the HOST Type is None.</para>
        /// </summary>
        None = NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_NONE,
        
        /// <summary>
        /// <para>Indicates that the HOST Type is EXE. </para>
        /// </summary>
        Exe = NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_EXEHOST,

        /// <summary>
        /// <para>Indicates that the HOST Type is DLL. </para>
        /// </summary>
        DllHost = NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_DLLHOST,

        /// <summary>
        /// <para>Indicates that the HOST Type is CONTAINER. </para>
        /// </summary>
        Container = NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND.FABRIC_CODE_PACKAGE_ENTRY_POINT_KIND_CONTAINERHOST
    }
}