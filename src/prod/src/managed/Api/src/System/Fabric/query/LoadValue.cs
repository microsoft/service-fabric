// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    public sealed class LoadValue
    {
        public string Name { get; internal set; }

        public int Value { get; internal set; }

        public DateTime LastReportedUtc { get; internal set; }

        internal static unsafe LoadValue CreateFromNative(NativeTypes.FABRIC_LOAD_VALUE nativeResultItem)
        {
            return new LoadValue
            {
                Name = NativeTypes.FromNativeString(nativeResultItem.Name),
                LastReportedUtc = NativeTypes.FromNativeFILETIME(nativeResultItem.LastReportedUtc),
                Value = (int)nativeResultItem.Value
            };
        }

        internal static unsafe IList<LoadValue> CreateFromNativeList(NativeTypes.FABRIC_LOAD_VALUE_LIST *list)
        {
            var rv = new List<LoadValue>();

            var nativeArray = (NativeTypes.FABRIC_LOAD_VALUE*)list->Items;
            for (int i = 0; i < list->Count; i++)
            {
                var nativeItem = *(nativeArray + i);
                rv.Add(LoadValue.CreateFromNative(nativeItem));
            }

            return rv;
        }
    }
}