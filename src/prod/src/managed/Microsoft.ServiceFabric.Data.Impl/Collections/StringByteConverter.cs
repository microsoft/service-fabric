// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.IO;

    /// <summary>
    /// String Byte Converter
    /// </summary>
    internal class StringByteConverter : IStateSerializer<string>
    {
        /// <summary>
        /// Serializes in to binary writer.
        /// </summary>
        /// <param name="value">Value to serialize.</param>
        /// <param name="binaryWriter">The binary Writer.</param>
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
        /// De-serializes to object.
        /// </summary>
        /// <param name="binaryReader">The binary reader.</param>
        /// <returns>De-serialized value.</returns>
        public string Read(BinaryReader binaryReader)
        {
            var isValueNull = binaryReader.ReadBoolean();

            if (isValueNull)
            {
                return null;
            }

            return binaryReader.ReadString();
        }

        /// <summary>
        /// De-serializes to object.
        /// </summary>
        /// <param name="baseValue">Base value for the de-serialization.</param>
        /// <param name="binaryReader">The binary reader.</param>
        /// <returns>De-serialized value.</returns>
        public string Read(string baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        /// <summary>
        /// Serializes in to binary writer.
        /// </summary>
        /// <param name="baseValue">Base value for the serialization.</param>
        /// <param name="targetValue">Value to serialize.</param>
        /// <param name="binaryWriter">The binary Writer.</param>
        public void Write(string baseValue, string targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }
    }
}