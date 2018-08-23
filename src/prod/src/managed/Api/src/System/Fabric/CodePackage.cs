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
    /// <para>Represents the code package definition.</para>
    /// </summary>
    public sealed class CodePackage
    {
        internal CodePackage()
        {
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Description.CodePackageDescription" /> 
        /// for the <see cref="System.Fabric.CodePackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.CodePackageDescription" /> for the <see cref="System.Fabric.CodePackage" />.</para>
        /// </value>
        public CodePackageDescription Description { get; internal set; }

        /// <summary>
        /// <para>Gets the path to the <see cref="System.Fabric.CodePackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The path to the <see cref="System.Fabric.CodePackage" />.</para>
        /// </value>
        public string Path { get; internal set; }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Description.RunAsPolicyDescription" /> object 
        /// associated with Setup EntryPoint in <see cref="System.Fabric.CodePackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.RunAsPolicyDescription" /> object 
        /// associated with Setup EntryPoint in <see cref="System.Fabric.CodePackage" />.</para>
        /// </value>
        public RunAsPolicyDescription SetupEntryPointRunAsPolicy { get; internal set; }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Description.RunAsPolicyDescription" /> 
        /// associated with Main EntryPoint in the <see cref="System.Fabric.CodePackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.RunAsPolicyDescription" /> 
        /// associated with Main EntryPoint in the <see cref="System.Fabric.CodePackage" />.</para>
        /// </value>
        public RunAsPolicyDescription EntryPointRunAsPolicy { get; internal set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe CodePackage CreateFromNative(NativeRuntime.IFabricCodePackage nativePackage)
        {
            ReleaseAssert.AssertIfNull(nativePackage, "nativePackage");
            
            string path = NativeTypes.FromNativeString(nativePackage.get_Path());
            var nativeDescription = *((NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION*)nativePackage.get_Description());
            var description = CodePackageDescription.CreateFromNative(nativeDescription, path);

            var returnValue = new CodePackage()
            {
                Description = description,
                Path = path
            };

            var nativePackage2 = (NativeRuntime.IFabricCodePackage2)nativePackage;

            if (nativePackage2.get_SetupEntryPointRunAsPolicy() != IntPtr.Zero)
            {
                returnValue.SetupEntryPointRunAsPolicy = RunAsPolicyDescription.CreateFromNative(
                    *(NativeTypes.FABRIC_RUNAS_POLICY_DESCRIPTION*)nativePackage2.get_SetupEntryPointRunAsPolicy());
            }

            if (nativePackage2.get_EntryPointRunAsPolicy() != IntPtr.Zero)
            {
                returnValue.EntryPointRunAsPolicy = RunAsPolicyDescription.CreateFromNative(
                    *(NativeTypes.FABRIC_RUNAS_POLICY_DESCRIPTION*)nativePackage2.get_EntryPointRunAsPolicy());
            }

            GC.KeepAlive(nativePackage2);
            GC.KeepAlive(nativePackage);

            return returnValue;
        }
    }
}