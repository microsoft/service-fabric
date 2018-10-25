// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System;
    using System.Collections;
    using System.Diagnostics;

    /// <summary>
    /// BloomFilter is a probabilistic data structure which can deterministically tell if an item is not present in a set.
    /// Compared to hash table it occupies less memory. Actual size depends on desired false positive rate.
    /// This implementation is based on article at https://blogs.msdn.microsoft.com/devdev/2006/01/23/literate-programming/
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class BloomFilter<T>
    {
        int _bitSize, _numberOfHashes, _setSize;
        BitArray _bitArray;

        #region Constructors
        /// <summary>
        /// Initializes the bloom filter and sets the optimal number of hashes. 
        /// </summary>
        /// <param name="bitSize">Size of the bloom filter in bits (m)</param>
        /// <param name="setSize">Size of the set (n)</param>
        // rahku- TODO change to long
        public BloomFilter(int bitSize, int setSize) : this(bitSize, setSize, OptimalNumberOfHashes(bitSize, setSize))
        {
        }

        /// <summary>
        /// Initializes the bloom filter and sets the optimal number of hashes and bitSize. 
        /// </summary>
        /// <param name="setSize">Size of the set (n)</param>
        /// <param name="falsePosRate">Desired false positive rate</param>
        public BloomFilter(int setSize, float falsePosRate) : this(OptimalBloomFilterSize(setSize, falsePosRate), setSize)
        {
        }

        /// <summary>
        /// Initializes the bloom filter with a manual number of hashes.
        /// </summary>
        /// <param name="bitSize">Size of the bloom filter in bits (m)</param>
        /// <param name="setSize">Size of the set (n)</param>
        /// <param name="numberOfHashes">Number of hashing functions (k)</param>
        public BloomFilter(int bitSize, int setSize, int numberOfHashes)
        {
            if (bitSize <= 0)
                throw new ArgumentException("Argument must be greater than 0", "bitSize");

            if (setSize <= 0)
                throw new ArgumentException("Argument must be greater than 0", "setSize");

            if (numberOfHashes <= 0)
                throw new ArgumentException("Argument must be greater than 0", "numberOfHashes");

            _bitSize = bitSize;
            _bitArray = new BitArray(bitSize);
            _setSize = setSize;
            _numberOfHashes = numberOfHashes;
        }
        #endregion

        #region Properties
        /// <summary>
        /// Number of hashing functions (k)
        /// </summary>
        public int NumberOfHashes
        {
            get
            {
                return _numberOfHashes;
            }
        }

        /// <summary>
        /// Size of the set (n)
        /// </summary>
        public int SetSize
        {
            get
            {
                return _setSize;
            }
        }

        /// <summary>
        /// Size of the bloom filter in bits (m)
        /// </summary>
        public int BitSize
        {
            get
            {
                return _bitSize;
            }
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Adds an item to the bloom filter.
        /// </summary>
        /// <param name="item">Item to be added</param>
        public void Add(T item)
        {
            int h1 = Hash(item);
            int h2 = Hash2(item);

            unchecked
            {
                h1 = (int)(((uint)h1) % _bitSize);
                h2 = (int)(((uint)h2) % _bitSize);
            }

            for (int i = 0; i < _numberOfHashes; i++)
            {
                _bitArray[h1] = true;

                unchecked
                {
                    h1 = (int)((uint)(h1 + h2) % _bitSize);
                    h2 = (int)((uint)(h2 + i) % _bitSize);
                }
            }
        }

        /// <summary>
        /// Checks whether an item is probably in the set. False positives 
        /// are possible, but false negatives are not.
        /// </summary>
        /// <param name="item">Item to be checked</param>
        /// <returns>True if the set probably contains the item</returns>
        public bool Contains(T item)
        {
            int h1 = Hash(item);
            int h2 = Hash2(item);

            unchecked
            {
                h1 = (int)(((uint)h1) % _bitSize);
                h2 = (int)(((uint)h2) % _bitSize);
            }

            for (int i = 0; i < _numberOfHashes; i++)
            {
                if (!_bitArray[h1])
                    return false;

                unchecked
                {
                    h1 = (int)((uint)(h1 + h2) % _bitSize);
                    h2 = (int)((uint)(h2 + i) % _bitSize);
                }
            }

            return true;
        }

        /// <summary>
        /// Computes the probability of encountering a false positive.
        /// </summary>
        /// <returns>Probability of a false positive</returns>
        public double FalsePositiveProbability()
        {
            return Math.Pow((1 - Math.Exp(-_numberOfHashes * _setSize / (double)_bitSize)), _numberOfHashes);
        }
        #endregion

        #region Private Methods
        /// <summary>
        /// Hashing function for an object
        /// </summary>
        /// <param name="item">Any object</param>
        /// <returns>Hash of that object</returns>
        private int Hash(T item)
        {
            return item.GetHashCode();
        }

        private int Hash2(T item)
        {
            int hashcode = item.GetHashCode();
            byte[] bytes = BitConverter.GetBytes(hashcode);
            return MurmurHash(bytes, hashcode);
        }

        /// <summary>
        /// Implementation of MurmurHash3 algorithm
        /// Based on c++ implementation at https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
        /// MurmurHash is a fast hashing algorithm for hash-based lookup
        /// </summary>
        /// <param name="data"></param>
        /// <param name="seed"></param>
        /// <returns></returns>
        private static int MurmurHash(byte[] data, int seed)
        {
            const uint c1 = 0xcc9e2d51;
            const uint c2 = 0x1b873593;

            uint h1 = (uint)seed;
            uint k1 = 0;
            int offset = 0;

            var chunk = new ArraySegment<byte>(data, offset, (data.Length - offset) > 4 ? 4:(data.Length - offset));
            while (chunk.Count > 0)
            {
                offset += chunk.Count;
                switch (chunk.Count)
                {
                    case 4:
                        /* Get four bytes from the input into an uint */
                        k1 = (uint)
                            (chunk.Array[chunk.Offset + 0]
                            | chunk.Array[chunk.Offset + 1] << 8
                            | chunk.Array[chunk.Offset + 2] << 16
                            | chunk.Array[chunk.Offset + 3] << 24);

                        /* bitmagic hash */
                        k1 *= c1;
                        k1 = rotl32(k1, 15);
                        k1 *= c2;

                        h1 ^= k1;
                        h1 = rotl32(h1, 13);
                        h1 = h1 * 5 + 0xe6546b64;
                        break;
                    case 3:
                        k1 = (uint)
                            (chunk.Array[chunk.Offset + 0]
                            | chunk.Array[chunk.Offset + 1] << 8
                            | chunk.Array[chunk.Offset + 2] << 16);
                        k1 *= c1;
                        k1 = rotl32(k1, 15);
                        k1 *= c2;
                        h1 ^= k1;
                        break;
                    case 2:
                        k1 = (uint)
                            (chunk.Array[chunk.Offset + 0]
                            | chunk.Array[chunk.Offset + 1] << 8);
                        k1 *= c1;
                        k1 = rotl32(k1, 15);
                        k1 *= c2;
                        h1 ^= k1;
                        break;
                    case 1:
                        k1 = (uint)(chunk.Array[chunk.Offset + 0]);
                        k1 *= c1;
                        k1 = rotl32(k1, 15);
                        k1 *= c2;
                        h1 ^= k1;
                        break;

                }
                chunk = new ArraySegment<byte>(data, offset, (data.Length - offset) > 4 ? 4 : (data.Length - offset));
            }

            // finalization, magic chants to wrap it all up
            h1 ^= (uint)data.Length;
            h1 = fmix(h1);

            unchecked //ignore overflow
            {
                return (int)h1;
            }
        }

        private static uint rotl32(uint x, byte r)
        {
            return (x << r) | (x >> (32 - r));
        }

        private static uint fmix(uint h)
        {
            h ^= h >> 16;
            h *= 0x85ebca6b;
            h ^= h >> 13;
            h *= 0xc2b2ae35;
            h ^= h >> 16;
            return h;
        }

        /// <summary>
        /// Calculates the optimal number of hashes based on bloom filter
        /// bit size and set size.
        /// </summary>
        /// <param name="bitSize">Size of the bloom filter in bits (m)</param>
        /// <param name="setSize">Size of the set (n)</param>
        /// <returns>The optimal number of hashes</returns>
        private static int OptimalNumberOfHashes(int bitSize, int setSize)
        {
            if (setSize <= 0)
                throw new ArgumentException("Argument must be greater than 0", "setSize");

            var result = (int)Math.Ceiling((bitSize / setSize) * Math.Log(2.0));
            if (result <= 0)
                return 1;
            else
                return result;
        }

        /// <summary>
        /// From https://www.safaribooksonline.com/library/view/mapreduce-design-patterns/9781449341954/apa.html
        /// </summary>
        /// <param name="setSize"></param>
        /// <param name="falsePosRate"></param>
        /// <returns></returns>
        public static int OptimalBloomFilterSize(int setSize, float falsePosRate)
        {
            if (falsePosRate <= 0.0f || falsePosRate >= 1.0f)
                throw new ArgumentException("Argument must be greater than 0 and less than 1", "falsePosRate");
            try
            {
                return Convert.ToInt32(-setSize * (float)Math.Log(falsePosRate)
                            / Math.Pow(Math.Log(2), 2));
            }
            catch (OverflowException)
            {
                return Int32.MaxValue;
            }
        }
        #endregion
    }
}
