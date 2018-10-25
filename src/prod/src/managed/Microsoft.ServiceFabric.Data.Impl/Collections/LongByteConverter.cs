// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Globalization;
    using System.IO;

    /// <summary>
    /// Long byte converter.
    /// </summary>
    internal class LongByteConverter : IStateSerializer<long>
    {
        /// <summary>
        /// Serializes in to binary writer.
        /// </summary>
        /// <param name="value">Value to serialize.</param>
        /// <param name="binaryWriter">The binary Writer.</param>
        public void Write(long value, BinaryWriter binaryWriter)
        {
            binaryWriter.Write(value);
        }

        /// <summary>
        /// De-serializes to object.
        /// </summary>
        /// <param name="binaryReader">The binary reader.</param>
        /// <returns>De-serialized value.</returns>
        public long Read(BinaryReader binaryReader)
        {
            if (null == binaryReader)
            {
                throw new ArgumentNullException(string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "bytes"));
            }

            return binaryReader.ReadInt64();
        }

        /// <summary>
        /// De-serializes to object.
        /// </summary>
        /// <param name="baseValue">Base value for the de-serialization.</param>
        /// <param name="binaryReader">The binary reader.</param>
        /// <returns>De-serialized value.</returns>
        public long Read(long baseValue, BinaryReader binaryReader)
        {
            return this.Read(binaryReader);
        }

        /// <summary>
        /// Serializes in to binary writer.
        /// </summary>
        /// <param name="baseValue">Base value for the serialization.</param>
        /// <param name="targetValue">Value to serialize.</param>
        /// <param name="binaryWriter">The binary Writer.</param>
        public void Write(long baseValue, long targetValue, BinaryWriter binaryWriter)
        {
            this.Write(targetValue, binaryWriter);
        }
    }
}