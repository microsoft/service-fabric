// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Reserved for future use.</para>
    /// </summary>
    public class DllHostHostedDllDescription
    {
        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <param name="kind">
        /// <para>Reserved for future use.</para>
        /// </param>
        protected DllHostHostedDllDescription(DllHostHostedDllKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Description.DllHostHostedDllKind" />.</para>
        /// </value>
        public DllHostHostedDllKind Kind { get; private set; }

        internal static unsafe DllHostHostedDllDescription CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION* nativeDescription = (NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_DESCRIPTION*)nativeDescriptionPtr;
            switch (nativeDescription->Kind)
            {
                case NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_UNMANAGED:
                    return DllHostHostedUnmanagedDllDescription.CreateFromNative(nativeDescription->Value);
                case NativeTypes.FABRIC_DLLHOST_HOSTED_DLL_KIND.FABRIC_DLLHOST_HOSTED_DLL_KIND_MANAGED:
                    return DllHostHostedManagedDllDescription.CreateFromNative(nativeDescription->Value);
                default:
                    throw new ArgumentException(StringResources.Error_HostedDllKindUnknown);
            }
        }
    }

    /// <summary>
    /// <para>Reserved for future use.</para>
    /// </summary>
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Small subclass.")]
    public sealed class DllHostHostedUnmanagedDllDescription : DllHostHostedDllDescription
    {
        internal DllHostHostedUnmanagedDllDescription()
            : base(DllHostHostedDllKind.Unmanaged)
        {
        }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string DllName { get; internal set; }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                "{0}({1})",
                this.Kind,
                this.DllName);
        }

        internal static unsafe new DllHostHostedUnmanagedDllDescription CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            NativeTypes.FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION* nativeDescription =
                (NativeTypes.FABRIC_DLLHOST_HOSTED_UNMANAGED_DLL_DESCRIPTION*)nativeDescriptionPtr;

            DllHostHostedUnmanagedDllDescription description = new DllHostHostedUnmanagedDllDescription();
            description.DllName = NativeTypes.FromNativeString(nativeDescription->DllName);

            return description;
        }
    }

    /// <summary>
    /// <para>Reserved for future use.</para>
    /// </summary>
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Small subclass.")]
    public sealed class DllHostHostedManagedDllDescription : DllHostHostedDllDescription
    {
        internal DllHostHostedManagedDllDescription()
            : base(DllHostHostedDllKind.Managed)
        {
        }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string AssemblyName { get; internal set; }

        /// <summary>
        /// <para>Reserved for future use.</para>
        /// </summary>
        /// <returns>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </returns>
        public override string ToString()
        {
            return string.Format(
                System.Globalization.CultureInfo.InvariantCulture,
                "{0}({1})",
                this.Kind,
                this.AssemblyName);
        }

        internal static unsafe new DllHostHostedManagedDllDescription CreateFromNative(IntPtr nativeDescriptionPtr)
        {
            NativeTypes.FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION* nativeDescription = (NativeTypes.FABRIC_DLLHOST_HOSTED_MANAGED_DLL_DESCRIPTION*)nativeDescriptionPtr;

            DllHostHostedManagedDllDescription description = new DllHostHostedManagedDllDescription();
            description.AssemblyName = NativeTypes.FromNativeString(nativeDescription->AssemblyName);

            return description;
        }
    }
}