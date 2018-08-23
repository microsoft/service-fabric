// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.Fabric.Common;

    internal static class DictionaryExtensions
    {
        internal static NameValueCollection ToNameValueCollection(this IDictionary<string, string> dictionary)
        {
            dictionary.ThrowIfNull(nameof(dictionary));
            var collection = new NameValueCollection();
            foreach (var entry in dictionary)
            {
                collection.Add(entry.Key, entry.Value);
            }

            return collection;
        }
    }
}