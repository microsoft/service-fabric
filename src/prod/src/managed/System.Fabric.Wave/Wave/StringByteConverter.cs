// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Replicator;
    using System.IO;

    /// <summary>
    /// Converts a string into a byte array.
    /// </summary>
    internal class StringByteConverter : IStateSerializer<string>
    {
        /// <summary>
        /// To byte array.
        /// </summary>
        /// <param name="value">String value.</param>
        /// <returns>Serialized string.</returns>
        public void Write(string value, BinaryWriter binaryWriter)
        {
            if (null == value)
            {
                binaryWriter.Write(true);
            }
            else
            {
                binaryWriter.Write(false);
                binaryWriter.Write(value);
            }
        }

        /// <summary>
        /// From byte array.
        /// </summary>
        /// <param name="bytes">Serialized string.</param>
        /// <returns>Deserialized string.</returns>
        public string Read(BinaryReader binaryReader)
        {
            bool isValueNull = binaryReader.ReadBoolean();

            if (isValueNull)
            {
                return null;
            }

            return binaryReader.ReadString();
        }

        /// <summary>
        /// Converts IEnumerable of byte[] to T
        /// </summary>
        /// <param name="bytes">Serialized version of T (as an list of byte arrays).</param>
        /// <returns>Deserialized version of T.</returns>
        public string Read(string baseValue, BinaryReader reader)
        {
            return this.Read(reader);
        }

        /// <summary>
        /// Converts T to byte array.
        /// </summary>
        /// <param name="value">T to be serialized.</param>
        /// <returns>Serialized version of T.</returns>
        public void Write(string currentValue, string newValue, BinaryWriter binaryWriter)
        {
            this.Write(newValue, binaryWriter);
        }
    }
}