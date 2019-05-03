// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using Microsoft.ServiceFabric.Data;
    using System.Dynamic;
    using Microsoft.ServiceFabric.Replicator;
    using System.IO;
    using System.Runtime.Serialization.Formatters.Binary;

    /// <summary>
    /// Serializes a wave or a wave feedback (dynamic objects).
    /// </summary>
    internal class DynamicByteConverter : IStateSerializer<DynamicObject>
    {
        public DynamicObject Read(BinaryReader binaryReader)
        {
            int numBytes = binaryReader.ReadInt32();
            byte[] bytes = binaryReader.ReadBytes(numBytes);
            BinaryFormatter bf = new BinaryFormatter();
            using (MemoryStream stream = new MemoryStream(bytes))
            {
                return (DynamicObject)bf.Deserialize(stream);
            }
        }

        public void Write(DynamicObject value, BinaryWriter binaryWriter)
        {
            BinaryFormatter bf = new BinaryFormatter();
            using (MemoryStream stream = new MemoryStream())
            {
                bf.Serialize(stream, value);
                byte[] bytes = stream.ToArray();
                binaryWriter.Write(bytes.Length);
                binaryWriter.Write(bytes);
            }
        }

        public DynamicObject Read(DynamicObject baseValue, BinaryReader reader)
        {
            return this.Read(reader);
        }

        public void Write(DynamicObject currentValue, DynamicObject newValue, BinaryWriter writer)
        {
            this.Write(newValue, writer);
        }
    }
}