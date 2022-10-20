// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Text;
    using System.Fabric.Data.Common;

    /// <summary>
    /// Provides serialization for unicode string data type.
    /// </summary>
    public class ByteArrayKeyBitConverter : IKeyBitConverter<byte[]>
    {
        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(byte[] key)
        {
            return key;
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public byte[] FromByteArray(byte[] bytes)
        {
            return bytes;
        }
    }

    /// <summary>
    /// Provides serialization for unicode string data type.
    /// </summary>
    public class UnicodeStringKeyBitConverter : IKeyBitConverter<string>
    {
        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(string key)
        {
            return Encoding.Unicode.GetBytes(key);
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public string FromByteArray(byte[] bytes)
        {
            return Encoding.Unicode.GetString(bytes);
        }
    }

    /// <summary>
    /// Provides serialization for GUID data type.
    /// </summary>
    public class GuidKeyBitConverter : IKeyBitConverter<Guid>
    {
        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(Guid key)
        {
            return key.ToByteArray();
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public Guid FromByteArray(byte[] bytes)
        {
            return new Guid(bytes);
        }
    }

    /// <summary>
    /// Provides serialization for long data type.
    /// </summary>
    public class UInt64KeyBitConverter : IKeyBitConverter<UInt64>
    {
        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(UInt64 key)
        {
            return BitConverter.GetBytes(key);
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public UInt64 FromByteArray(byte[] bytes)
        {
            return BitConverter.ToUInt64(bytes, 0);
        }
    }

    /// <summary>
    /// Provides serialization for int data type.
    /// </summary>
    public class Int32KeyBitConverter : IKeyBitConverter<Int32>
    {
        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(Int32 key)
        {
            return BitConverter.GetBytes(key);
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public Int32 FromByteArray(byte[] bytes)
        {
            return BitConverter.ToInt32(bytes, 0);
        }
    }

    /// <summary>
    /// Provides serialization for long data type.
    /// </summary>
    public class Int64KeyBitConverter : IKeyBitConverter<Int64>
    {
        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(Int64 key)
        {
            return BitConverter.GetBytes(key);
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public Int64 FromByteArray(byte[] bytes)
        {
            return BitConverter.ToInt64(bytes, 0);
        }
    }

    /// <summary>
    /// Provides serialization for datetime data type.
    /// </summary>
    public class DateTimeKeyBitConverter : IKeyBitConverter<DateTime>
    {
        /// <summary>
        /// A datetime is a long.
        /// </summary>
        Int64KeyBitConverter int64BitConverter = new Int64KeyBitConverter();

        /// <summary>
        /// Serialization to array of bytes.
        /// </summary>
        /// <param name="key">Key to serialize.</param>
        /// <returns></returns>
        public byte[] ToByteArray(DateTime key)
        {
            return int64BitConverter.ToByteArray(key.Ticks);
        }

        /// <summary>
        /// Deserialization from array of bytes.
        /// </summary>
        /// <param name="bytes"></param>
        /// <returns></returns>
        public DateTime FromByteArray(byte[] bytes)
        {
            return new DateTime(int64BitConverter.FromByteArray(bytes));
        }
    }
}