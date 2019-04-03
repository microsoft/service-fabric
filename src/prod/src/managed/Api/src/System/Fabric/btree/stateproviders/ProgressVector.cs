// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using System.Text;

    /// <summary>
    /// Encapsulates progress vector computations.
    /// </summary>
    public sealed class ProgressVector
    {
        #region Instance Members

        /// <summary>
        /// The progress vector is a list of state versions.
        /// </summary>
        List<StateVersion> progress = new List<StateVersion>();

        #endregion

        /// <summary>
        /// Accessor to the progress vector.
        /// </summary>
        public IList<StateVersion> Progress 
        { 
            get 
            { 
                return this.progress; 
            } 
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="bw"></param>
        internal void ToBinary(BinaryWriter bw)
        {
            bw.Write(this.progress.Count);
            foreach (StateVersion r in this.progress)
            {
                r.ToBinary(bw);
            }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="br"></param>
        /// <returns></returns>
        internal static ProgressVector Read(BinaryReader br)
        {
            int count = br.ReadInt32();
            if (0 == count)
            {
                return null;
            }
            ProgressVector pv = new ProgressVector();
            for (int i = 0; i < count; i++)
            {
                StateVersion sv = StateVersion.Read(br);
                pv.progress.Add(sv);
            }
            return pv;
        }
        /// <summary>
        /// Deserialization support.
        /// </summary>
        /// <param name="bytes">Byte representation of the progress vector.</param>
        /// <returns></returns>
        public static ProgressVector FromBytes(byte[] value)
        {
            using (MemoryStream mem = new MemoryStream(value))
            {
                BinaryReader br = new BinaryReader(mem);
                return ProgressVector.Read(br);
            }
        }
        /// <summary>
        /// Serialization support.
        /// </summary>
        /// <returns></returns>
        public byte[] GetBytes()
        {
            if (null == this.progress)
            {
                return null;
            }

            using (MemoryStream mem = new MemoryStream())
            {
                BinaryWriter bw = new BinaryWriter(mem);
                this.ToBinary(bw);
                return mem.ToArray();
            }
        }
    }
}