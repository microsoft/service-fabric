// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Collections.ObjectModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Security;

    // For pinning multiple objects.
    //
    // This can be used on its own or as a base class.
    //
    // NOTE: When used as an IPinNode, this assumes that the last object pinned is the root.  If
    // that is not the case, the subclass should override AddrOfPinnedObject to return the correct
    // one.
    internal class PinCollection : Collection<IPinNode>, IPinNode
    {
        public IntPtr AddBlittable(object item)
        {
            if (item != null)
            {
                var pin = new PinBlittable(item);
                this.Add(pin);
                return pin.AddrOfPinnedObject();
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        public IntPtr AddObject(IPinNode pin)
        {
            if (pin != null)
            {
                this.Add(pin);
                return pin.AddrOfPinnedObject();
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        /// <summary>
        /// Use this method instead of AddBlittable() if empty strings should also be converted to null.
        /// Use AddBlittable() method if empty string should not be converted to null.
        /// </summary>
        /// <param name="pin">Object to pin.</param>
        /// <returns>Pointer to object.</returns>
        public IntPtr AddObject(string pin)
        {
            if (string.IsNullOrEmpty(pin))
            {
                return IntPtr.Zero;
            }
            else
            {
                return this.AddBlittable(pin);
            }
        }

        public IntPtr AddObject(Uri uri)
        {
            if (uri != null)
            {
                var pin = PinBlittable.Create(uri);
                this.Add(pin);
                return pin.AddrOfPinnedObject();
            }
            else
            {
                return IntPtr.Zero;
            }
        }

        public IntPtr AddObject(SecureString secureString)
        {
            return this.AddObject(new SecureStringPinNode(secureString));
        }

        public virtual IntPtr AddrOfPinnedObject()
        {
            if (this.Count > 0)
            {
                return this[this.Count - 1].AddrOfPinnedObject();
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
            foreach (var pin in this)
            {
                pin.Dispose();
            }

            this.Clear();
        }
    }
}