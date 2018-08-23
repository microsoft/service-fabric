// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// This class will be a helper class to generate random entities.
    /// This class adds new test parameters which are relevant to RandomSession.
    /// </summary>
    class RandomExtensions
    {
        private readonly Dictionary<string, uint> nameSuffixCounter = new Dictionary<string, uint>();
        private Random random;

        public RandomExtensions(Random random)
        {
            this.random = random;
        }

        public uint NextUint(uint max)
        {
            return (uint)this.random.Next((int)max);
        }

        public uint NextUint(uint min, uint max)
        {
            return (uint)this.random.Next((int)min, (int)max);
        }

        public bool ChooseFirstSet(int size1, int size2)
        {
            int arb = this.random.Next(size1 + size2);
            return arb < size1;
        }

        /// <summary>
        /// Returns a randomly chosen item from given collection.
        /// </summary>
        /// <typeparam name="T">type of items in collection.</typeparam>
        /// <param name="items">Input collection of items from an item will be chosen</param>
        /// <returns>Returns an item at randomly chosen index if collection non-empty.</returns>
        public T ChooseRandomItemOrDefault<T>(ICollection<T> items)
        {
            if (items == null)
            {
                return default(T);
            }
            else
            {
                return items.ElementAtOrDefault(this.random.Next(items.Count));
            }
        }
    }
}