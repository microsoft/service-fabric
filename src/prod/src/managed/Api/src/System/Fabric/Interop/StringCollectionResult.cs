// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;

    internal class StringCollectionResult : ItemList<string>
    {

        private StringCollectionResult(): this(false)
        {
        }

        private StringCollectionResult(bool mayContainDuplicates) : base(false, mayContainDuplicates)
        {
        }

        internal static StringCollectionResult FromNative(NativeCommon.IFabricStringListResult innerCollection)
        {
            return FromNative(innerCollection, false);
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static StringCollectionResult FromNative(NativeCommon.IFabricStringListResult innerCollection, bool mayContainDuplicates)
        {
            var result = new StringCollectionResult(mayContainDuplicates);

            if (innerCollection != null)
            {
                uint count;
                unsafe
                {
                    var fabricStringArrayPointer = (char**)innerCollection.GetStrings(out count);
                    for (int i = 0; i < count; i++)
                    {
                        var itemPtr = (IntPtr)(fabricStringArrayPointer + i);
                        result.Add(NativeTypes.FromNativeStringPointer(itemPtr));
                    }
                }
            }

            GC.KeepAlive(innerCollection);
            return result;
        }
    }
}