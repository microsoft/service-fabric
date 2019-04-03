// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Util
{
    using System;

    /// <summary>
    /// Define a simple Max heap interface.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public interface IMaxHeap<T> where T : IComparable<T>
    {
        int ItemCount { get; }

        T ExtractMax();

        T PeekMax();

        void Insert(T item);

        bool IsEmpty();
    }
}