// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;

    internal static class RandomUtility
    {
        public static int NextFromTimestamp(int maxValue)
        {
            ThrowIf.OutOfRange(maxValue, 1, int.MaxValue, "maxValue");
            return ((int)Stopwatch.GetTimestamp() & 0x7FFFFFFF) % maxValue;
        }

        public static T NextFromTimestamp<T>(T[] values)
        {
            ThrowIf.Null(values, "values");
            return values[RandomUtility.NextFromTimestamp(values.Length)];
        }

        public static void Shuffle<T>(IList<T> list, Random random)
        {
            ThrowIf.Null(list, "list");
            ThrowIf.Null(random, "random");
            for (int i = 0; i < list.Count - 1; i++)
            {
                int r = random.Next(i, list.Count);
                RandomUtility.Swap(list, i, r);
            }
        }

        private static void Swap<T>(IList<T> list, int indexA, int indexB)
        {
            T temp = list[indexA];
            list[indexA] = list[indexB];
            list[indexB] = temp;
        }
    }
}


#pragma warning restore 1591