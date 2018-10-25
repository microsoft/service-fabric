// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Threading;

    internal struct AsyncEnumeratorWrapper<TSource> : IEnumerator<TSource>
    {
        private IAsyncEnumerator<TSource> source;
        private TSource current;

        public AsyncEnumeratorWrapper(IAsyncEnumerator<TSource> source)
        {
            this.source = source;
            this.current = default(TSource);
        }

        public TSource Current
        {
            get { return this.current; }
        }

        object IEnumerator.Current
        {
            get { throw new NotImplementedException(); }
        }

        public void Dispose()
        {
        }

        public bool MoveNext()
        {
            if (!this.source.MoveNextAsync(CancellationToken.None).GetAwaiter().GetResult())
            {
                return false;
            }

            this.current = this.source.Current;
            return true;
        }

        public void Reset()
        {
            throw new NotImplementedException();
        }
    }
}