// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the possible start folder types.</para>
    /// </summary>
    public enum ExeHostWorkingFolder
    {
        /// <summary>
        /// <para>Do not use.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_INVALID,
        /// <summary>
        /// <para>Indicates that the start folder is the application’s work folder.</para>
        /// </summary>
        Work = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_WORK,
        /// <summary>
        /// <para>Indicates that the start folder is the Code Package’s folder.</para>
        /// </summary>
        CodePackage = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_CODE_PACKAGE,
        /// <summary>
        /// <para>Indicates that the start folder is from where the executable / program that is specified in the ExeHost is loaded. This folder can differ 
        /// from the Code Package folder if there are multiple levels in the Code Package folder and a specified program is from one of the nested folders.</para>
        /// </summary>
        CodeBase = NativeTypes.FABRIC_EXEHOST_WORKING_FOLDER.FABRIC_EXEHOST_WORKING_FOLDER_CODE_BASE
    }
}