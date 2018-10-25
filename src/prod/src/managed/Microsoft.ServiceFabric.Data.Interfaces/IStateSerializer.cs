// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System.IO;

    /// <summary>
    /// Represents a custom serializer for type <typeparamref name="T"/>.
    /// </summary>
    /// <typeparam name="T">Type to serialize and deserialize.</typeparam>
    /// <remarks>
    /// Use <see cref="IReliableStateManager.TryAddStateSerializer{T}(IStateSerializer{T})"/> to register a custom serializer.
    /// </remarks>
    /// <example>
    /// In this example, implementation of the Read and Write overloads simply call their counterpart overloads.
    /// The currentValue and baseValue parameters are not set by the platform and should be ignored.
    /// <code>
    /// class Order
    /// {
    ///     public byte Warehouse { get; set; }
    ///     public short District { get; set; }
    ///     public int Customer { get; set; }
    ///     public long OrderNumber { get; set; }
    /// }
    ///
    /// class OrderSerializer : IStateSerializer&lt;Order&gt;
    /// {
    ///     void Write(Order value, BinaryWriter writer)
    ///     {
    ///         writer.Write(value.Warehouse);
    ///         writer.Write(value.District);
    ///         writer.Write(value.Customer);
    ///         writer.Write(value.OrderNumber);
    ///     }
    ///
    ///     Order Read(BinaryReader reader)
    ///     {
    ///         Order value = new Order();
    ///
    ///         value.Warehouse = reader.ReadByte();
    ///         value.District = reader.ReadInt16();
    ///         value.Customer = reader.ReadInt32();
    ///         value.OrderNumber = reader.ReadInt64();
    ///
    ///         return value;
    ///     }
    ///
    ///     void Write(Order currentValue, Order newValue, BinaryWriter writer)
    ///     {
    ///         this.Write(newValue, writer);
    ///     }
    ///
    ///     Order Read(Order baseValue, BinaryReader reader)
    ///     {
    ///         return this.Read(reader);
    ///     }
    /// }
    /// </code>
    /// </example>
    public interface IStateSerializer<T>
    {
        /// <summary>
        /// Deserializes from the given <see cref="BinaryReader"/> to <typeparamref name="T"/>.
        /// </summary>
        /// <param name="binaryReader">The <see cref="BinaryReader"/> to deserialize from.</param>
        /// <returns>The deserialized value.</returns>
        /// <remarks>
        /// When accessing the <see cref="BinaryReader"/> base stream, care must be taken when moving the position in the stream.
        /// Reading must begin at the current stream position and end at the current position plus the length of your data.
        /// </remarks>
        T Read(BinaryReader binaryReader);

        /// <summary>
        /// Serializes a value and writes it to the given <see cref="BinaryWriter"/>.
        /// </summary>
        /// <param name="value">The value to serialize.</param>
        /// <param name="binaryWriter">The <see cref="BinaryWriter"/> to serialize to.</param>
        /// <remarks>
        /// When accessing the <see cref="BinaryWriter"/> base stream, care must be taken when moving the position in the stream.
        /// Writing must begin at the current stream position and end at the current position plus the length of your data.
        /// </remarks>
        void Write(T value, BinaryWriter binaryWriter);

        /// <summary>
        /// Deserializes from the given <see cref="BinaryReader"/> to <typeparamref name="T"/>.
        /// </summary>
        /// <param name="baseValue">The base value for the deserialization.</param>
        /// <param name="binaryReader">The <see cref="BinaryReader"/> to deserialize from.</param>
        /// <returns>The deserialized value.</returns>
        /// <remarks>
        /// When accessing the <see cref="BinaryReader"/> base stream, care must be taken when moving the position in the stream.
        /// Reading must begin at the current stream position and end at the current position plus the length of your data.
        /// </remarks>
        T Read(T baseValue, BinaryReader binaryReader);

        /// <summary>
        /// Serializes an object and writes it to the given <see cref="BinaryWriter"/>.
        /// </summary>
        /// <param name="baseValue">The base value for the serialization.</param>
        /// <param name="targetValue">The value to serialize.</param>
        /// <param name="binaryWriter">The <see cref="BinaryWriter"/> to serialize to.</param>
        /// <remarks>
        /// When accessing the <see cref="BinaryWriter"/> base stream, care must be taken when moving the position in the stream.
        /// Writing must begin at the current stream position and end at the current position plus the length of your data.
        /// </remarks>
        void Write(T baseValue, T targetValue, BinaryWriter binaryWriter);
    }
}