// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    internal static class Extensions
    {
        internal static IEnumerable<TSource> ToEnumerable<TSource>(this IAsyncEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            return new AsyncEnumerableWrapper<TSource>(source);
        }

        internal static int Count<TSource>(this IAsyncEnumerable<TSource> source)
        {
            int count = 0;
            using (var asyncEnumerator = source.GetAsyncEnumerator())
            {
                while (asyncEnumerator.MoveNextAsync(CancellationToken.None).GetAwaiter().GetResult())
                {
                    count++;
                }
            }

            return count;
        }

        internal static async Task<int> CountAsync<TSource>(this IAsyncEnumerable<TSource> source)
        {
            int count = 0;
            using (var asyncEnumerator = source.GetAsyncEnumerator())
            {
                while (await asyncEnumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                {
                    count++;
                }
            }

            return count;
        }
    }
}