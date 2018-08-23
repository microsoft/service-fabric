// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Common class shared in Microsoft.ServiceFabric.Data.Collections
    /// </summary>
    internal static class Common
    {
        /// <summary>
        /// Try to combine two Uris.
        /// </summary>
        /// <param name="uri1">Uri one.</param>
        /// <param name="uri2">Uri two.</param>
        /// <param name="result">Output Uri.</param>
        /// <returns>True if the operation was successful.</returns>
        internal static bool TryCombine(Uri uri1, Uri uri2, out Uri result)
        {
            result = null;

            var tmp1 = uri1.OriginalString;
            var tmp2 = uri2.OriginalString;

            tmp1 = tmp1.TrimEnd('/');
            tmp2 = tmp2.TrimStart('/');

            return Uri.TryCreate(string.Format("{0}/{1}", tmp1, tmp2), UriKind.Absolute, out result);
        }

        public static IAsyncEnumerable<TSource> ToAsyncEnumerable<TSource>(this IEnumerable<TSource> source)
        {
            if (source == null) throw new ArgumentNullException("source");
            return new AsyncEnumerable<TSource>(source);
        }

        public static async Task<int> GetCountAsync<TSource>(this IAsyncEnumerable<TSource> source)
        {
            var enumerator = source.GetAsyncEnumerator();
            int result = 0;
            while (await enumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
            {
                result += 1;
            }

            return result;
        }
    }

    internal class AsyncEnumerable<TSource> : IAsyncEnumerable<TSource>
    {
        private IEnumerable<TSource> source;

        public AsyncEnumerable(IEnumerable<TSource> source)
        {
            this.source = source;
        }

        public IAsyncEnumerator<TSource> GetAsyncEnumerator()
        {
            return new AsyncEnumerator<TSource>(this.source.GetEnumerator());
        }
    }

    internal struct AsyncEnumerator<TSource> : IAsyncEnumerator<TSource>
    {
        private IEnumerator<TSource> enumerator;
        private static Task<bool> CompletedTrueTask = Task.FromResult(true);
        private static Task<bool> CompletedFalseTask = Task.FromResult(false);

        /// <summary>
        /// Indicates that the enumerator has completed.
        /// </summary>
        private bool isDone;

        private TSource current;

        public AsyncEnumerator(IEnumerator<TSource> enumerator)
        {
            this.enumerator = enumerator;
            this.current = default(TSource);
            this.isDone = false;
        }

        public TSource Current
        {
            get
            {
                if (this.isDone)
                {
                    throw new InvalidOperationException();
                }

                return this.current;
            }
        }

        public void Dispose()
        {
            this.isDone = true;
            this.enumerator.Dispose();
        }

        public Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            if (this.isDone)
            {
                return CompletedFalseTask;
            }

            while (this.enumerator.MoveNext())
            {
                this.current = this.enumerator.Current;
                return CompletedTrueTask;
            }

            this.isDone = true;
            return CompletedFalseTask;
        }

        public void Reset()
        {
            this.isDone = false;
            this.enumerator.Reset();
        }
    }
}