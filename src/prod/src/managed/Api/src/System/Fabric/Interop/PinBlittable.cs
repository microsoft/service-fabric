// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;

    internal class PinBlittable : IPinNode
    {
        private readonly SafePinHandle handle;

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUnusedPrivateFields,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        private readonly object item;
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUnusedPrivateFields,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        private readonly IntPtr address;

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.DoNotInitializeUnnecessarily,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        public PinBlittable(object item)
        {
            if (item != null)
            {
                this.handle = new SafePinHandle(item);
            }
            else
            {
                this.handle = null;
            }

            this.item = item;
            this.address = this.AddrOfPinnedObject();
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.DoNotInitializeUnnecessarily,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        public PinBlittable(Uri uri) : this(uri.OriginalString)
        {
        }

        public static PinBlittable Create(object item)
        {
            return new PinBlittable(item);
        }

        public static PinBlittable Create(Uri uri)
        {
            return new PinBlittable(uri);
        }

        public IntPtr AddrOfPinnedObject()
        {
            if (this.handle != null)
            {
                return this.handle.AddrOfPinnedObject();
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        [SuppressMessage(FxCop.Category.Usage, FxCop.Rule.CallGCSuppressFinalizeCorrectly,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        public void Dispose()
        {
            if (this.handle != null)
            {
                this.handle.Dispose();
            }
        }
    }
}