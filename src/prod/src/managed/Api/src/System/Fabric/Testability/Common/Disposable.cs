// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System;

    internal static class Disposable
    {
        public static TDisposable Guard<TDisposable>(Func<TDisposable> constructor, Action<TDisposable> body) where TDisposable : class, IDisposable
        {
            ThrowIf.Null(constructor, "constructor");
            ThrowIf.Null(body, "body");

            TDisposable tempItem = null;
            TDisposable item = null;
            try
            {
                tempItem = constructor();
                body(tempItem);
                item = tempItem;
                tempItem = null;
            }
            finally
            {
                if (tempItem != null)
                {
                    tempItem.Dispose();
                }
            }

            return item;
        }
    }
}

#pragma warning restore 1591