// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.IO;


    /// <summary>
    /// A class that needs to be serialized/deserialized to/from a byte array should inherit from this base class
    /// </summary>
    [Serializable]
    public abstract class ByteSerializable : IByteSerializable
    {
        private const string TraceType = "ByteSerializable";

        /// <summary>
        /// This is the default implementation to convert an object into a byte array
        /// </summary>
        /// <returns>A byte array</returns>
        public virtual byte[] ToBytes()
        {
            try
            {
                using (MemoryStream mem = new MemoryStream())
                using (BinaryWriter bw = new BinaryWriter(mem))
                {
                    this.Write(bw);
                    return mem.ToArray();
                }
            }
            catch(Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "Exception '{0}' occurred while serializing '{1}'.", e.ToString(), this.GetType());
                throw;
            }
        }

        /// <summary>
        /// This is the default implementation to covert an object back from a byte array
        /// </summary>
        /// <param name="data">Byte array representation of the object</param>
        public virtual void FromBytes(byte[] data)
        {
            try
            {
                using (BinaryReader br = new BinaryReader(new MemoryStream(data)))
                {
                    this.Read(br);
                }
            }
            catch(Exception e)
            {
                TestabilityTrace.TraceSource.WriteError(TraceType, "Exception '{0}' occurred while de-serializing '{1}'.", e.ToString(), this.GetType());
                throw;
            }
        }

        /// <summary>
        /// This method must be implemented by derived classes so that they can be converted into a byte array
        /// </summary>
        /// <param name="bw">A BinaryWriter object</param>
        public abstract void Write(BinaryWriter bw);

        /// <summary>
        /// This method must be implemented by derived classes so that they can be converted back from a byte array
        /// </summary>
        /// <param name="br">A BinaryReader object</param>
        public abstract void Read(BinaryReader br);
    }
}