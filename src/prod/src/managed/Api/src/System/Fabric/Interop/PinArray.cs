// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Collections;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;

    internal class PinArray : IPinNode
    {
        private readonly SafePinHandle arrayHandle;
        private readonly IPinNode[] nodes;
        private readonly IntPtr[] pinnedItems;

        public PinArray(IEnumerable enumerable, PinCallback pinCallback)
        {
            if (enumerable != null)
            {
                var count = this.CountItems(enumerable);
                this.nodes = new IPinNode[count];
                this.pinnedItems = new IntPtr[count];
                this.arrayHandle = new SafePinHandle(this.pinnedItems);

                int i = 0;
                foreach (var item in enumerable)
                {
                    this.nodes[i] = pinCallback(item);
                    this.pinnedItems[i] = this.nodes[i].AddrOfPinnedObject();
                    i++;
                }
            }
        }

        public delegate IPinNode PinCallback(object target);

        public int Count
        {
            get
            {
                if (this.nodes == null)
                {
                    return 0;
                }
                else
                {
                    return this.nodes.Length;
                }
            }
        }

        public IntPtr AddrOfPinnedObject()
        {
            if (this.nodes == null)
            {
                return IntPtr.Zero;
            }
            else
            {
                return this.arrayHandle.AddrOfPinnedObject();
            }
        }

        [SuppressMessage(FxCop.Category.Usage, FxCop.Rule.CallGCSuppressFinalizeCorrectly,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        public void Dispose()
        {
            if (this.nodes != null)
            {
                this.arrayHandle.Dispose();

                for (int i = 0; i < this.nodes.Length; i++)
                {
                    this.nodes[i].Dispose();
                }
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.MarkMembersAsStatic,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.RemoveUnusedLocals,
                Justification = "Interop class to be obsoleted after Windows Fabric transition.")]
        private int CountItems(IEnumerable enumerable)
        {
            var collection = enumerable as ICollection;
            if (collection != null)
            {
                return collection.Count;
            }
            else
            {
                int n = 0;
                foreach (var item in enumerable)
                {
                    n++;
                }

                return n;
            }
        }
    }
}