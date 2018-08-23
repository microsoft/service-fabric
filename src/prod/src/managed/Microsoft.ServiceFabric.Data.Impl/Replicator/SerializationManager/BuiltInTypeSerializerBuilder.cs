// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Data;
    using System;

    /// <summary>
    /// Class to construct <see cref="BuiltInTypeSerializer{T}"/> instances
    /// </summary>
    internal class BuiltInTypeSerializerBuilder
    {
        /// <summary>
        /// Returns an instance of <see cref="BuiltInTypeSerializer{T}"/> where
        /// T is the data type passed in as parameter. The instance is returned
        /// as an object instance.
        /// </summary>
        /// <typeparam name="T"> Data type to be serialized </typeparam>
        /// <returns> Instance of BuiltInTypeSerializer </returns>
        internal static BuiltInTypeSerializer<T> MakeBuiltInTypeSerializer<T>()
        {
            var dataType = typeof(T);

            if (dataType.Equals(typeof(Guid)))
            {
                return
                    new BuiltInTypeSerializer<Guid>(
                        (writer, value) => writer.Write(value.ToByteArray()),
                        (reader) => { return new Guid(reader.ReadBytes(16)); }) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(bool)))
            {
                return
                    new BuiltInTypeSerializer<bool>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadBoolean()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(byte)))
            {
                return
                    new BuiltInTypeSerializer<byte>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadByte()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(sbyte)))
            {
                return
                    new BuiltInTypeSerializer<sbyte>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadSByte()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(char)))
            {
                return
                    new BuiltInTypeSerializer<char>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadChar()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(decimal)))
            {
                return
                    new BuiltInTypeSerializer<decimal>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadDecimal()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(double)))
            {
                return
                    new BuiltInTypeSerializer<double>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadDouble()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(float)))
            {
                return
                    new BuiltInTypeSerializer<float>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadSingle()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(int)))
            {
                return
                    new BuiltInTypeSerializer<int>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadInt32()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(uint)))
            {
                return
                    new BuiltInTypeSerializer<uint>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadUInt32()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(long)))
            {
                return
                    new BuiltInTypeSerializer<long>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadInt64()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(ulong)))
            {
                return
                    new BuiltInTypeSerializer<ulong>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadUInt64()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(short)))
            {
                return
                    new BuiltInTypeSerializer<short>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadInt16()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(ushort)))
            {
                return
                    new BuiltInTypeSerializer<ushort>(
                        (writer, value) => writer.Write(value),
                        (reader) => reader.ReadUInt16()) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(string)))
            {
                return new BuiltInTypeSerializer<string>(
                    (writer, value) =>
                    {
                        if (value == null)
                        {
                            writer.Write(false);
                        }
                        else
                        {
                            writer.Write(true);
                            writer.Write(value);
                        }
                    },
                    (reader) =>
                    {
                        string value = null;
                        var hasValue = reader.ReadBoolean();
                        if (hasValue)
                        {
                            value = reader.ReadString();
                        }

                        return value;
                    }) as BuiltInTypeSerializer<T>;
            }
            else if (dataType.Equals(typeof(byte[])))
            {
                return new BuiltInTypeSerializer<byte[]>(
                    (writer, value) =>
                    {
                        if (value == null)
                        {
                            writer.Write((int) -1);
                        }
                        else
                        {
                            writer.Write((int) value.Length);
                            writer.Write(value);
                        }
                    },
                    (reader) =>
                    {
                        var length = reader.ReadInt32();
                        if (length < 0)
                        {
                            return null;
                        }

                        return reader.ReadBytes(length);
                    }) as BuiltInTypeSerializer<T>;
            }
            else
            {
                throw new ArgumentException(string.Format(SR.Error_BuiltInType_Expected, dataType.FullName));
            }
        }
    }
}