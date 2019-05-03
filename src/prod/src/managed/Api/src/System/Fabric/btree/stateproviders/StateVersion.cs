// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.IO;
    using System.Text;
    using System.Fabric;
    /// <summary>
    /// 
    /// </summary>
    public sealed class StateVersion : IComparable<StateVersion>, IEquatable<StateVersion>
    {
        /// <summary>
        /// Consructor.
        /// </summary>
        public StateVersion() { }
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="epoch"></param>
        /// <param name="lsn"></param>
        public StateVersion(Epoch epoch, long lsn)
        {
            this.epoch = epoch;
            this.lsn = lsn;
        }

        #region Instance members

        /// <summary>
        /// Epoch of this state verison.
        /// </summary>
        Epoch epoch;
        /// <summary>
        /// The sequence number for previous epoch.
        /// </summary>
        long lsn;

        #endregion

        /// <summary>
        /// 
        /// </summary>
        public Epoch Epoch { get { return this.epoch; } }
        /// <summary>
        /// 
        /// </summary>
        public long PreviousEpochLastSequenceNumber { get { return this.lsn; } set { this.lsn = value; } }
        /// <summary>
        /// Initial state version.
        /// </summary>
        public static StateVersion Zero = new StateVersion(new Epoch(0, 0), 0);
        /// <summary>
        /// 
        /// </summary>
        /// <param name="bw"></param>
        internal void ToBinary(BinaryWriter bw)
        {
            bw.Write(epoch.DataLossNumber);
            bw.Write(epoch.ConfigurationNumber);
            bw.Write(lsn);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="br"></param>
        /// <returns></returns>
        internal static StateVersion Read(BinaryReader br)
        {
            StateVersion r = new StateVersion();
            long ed = br.ReadInt64();
            long ec = br.ReadInt64();
            r.epoch = new Epoch(ed, ec);
            r.lsn = br.ReadInt64();
            return r;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        public static StateVersion FromBytes(byte[] value)
        {
            using (MemoryStream mem = new MemoryStream(value))
            {
                BinaryReader br = new BinaryReader(mem);
                return StateVersion.Read(br);
            }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public byte[] GetBytes()
        {
            using (MemoryStream mem = new MemoryStream())
            {
                BinaryWriter bw = new BinaryWriter(mem);
                this.ToBinary(bw);
                return mem.ToArray();
            }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="dv1"></param>
        /// <param name="dv2"></param>
        /// <returns></returns>
        public static bool operator ==(StateVersion dv1, StateVersion dv2)
        {
            return Equals(dv1, dv2);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="dv1"></param>
        /// <param name="dv2"></param>
        /// <returns></returns>
        public static bool operator !=(StateVersion dv1, StateVersion dv2)
        {
            return Equals(dv1, dv2);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(object obj)
        {
            if (obj == null)
                return false;
            StateVersion dv = obj as StateVersion;
            if (dv == null)
                return false;
            return Equals(this, dv);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        public bool Equals(StateVersion other)
        {
            return Equals(this, other);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public override int GetHashCode()
        {
            return (int)(epoch.GetHashCode() << 5 ^ this.lsn);
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder("\"{(");
            sb.Append(this.epoch.DataLossNumber);
            sb.Append(", ");
            sb.Append(this.epoch.ConfigurationNumber);
            sb.Append("), ");
            sb.Append(this.lsn);
            sb.Append("}\"");
            return sb.ToString();
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        public int CompareTo(StateVersion other)
        {
            if (null == other)
            {
                throw new ArgumentNullException("other");
            }
            if (this.epoch < other.Epoch)
                return -1;
            if (this.epoch > other.Epoch)
                return 1;
            if (this.lsn < other.PreviousEpochLastSequenceNumber)
                return -1;
            if (this.lsn > other.PreviousEpochLastSequenceNumber)
                return 1;
            return 0;
        }
    }
}