// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para> Represents the current version of the partition in Service Fabric. </para>
    /// </summary>
    /// <remarks>
    /// <para>An Epoch is a configuration number for the partition as a whole. 
    /// When the configuration of the replica set changes, for example when the Primary replica changes, 
    /// the operations that are replicated from the new Primary replica are said to be a new Epoch 
    /// from the ones which were sent by the old Primary replica. 
    /// The fact that the Primary has changed is not directly visible to Secondary replicas, 
    /// which are usually unaffected by the failure that affected the original Primary replica. 
    /// To track that the Primary replica has changed has to be communicated to the Secondary replica. 
    /// This communication occurs via the <see cref="System.Fabric.IStateProvider.UpdateEpochAsync(System.Fabric.Epoch,System.Int64,System.Threading.CancellationToken)" /> method. 
    /// Most services can ignore the details of the inner fields of the Epoch as it is usually sufficient to know that the Epoch has changed 
    /// and to compare Epochs to determine relative ordering of operations and events in the system. 
    /// Comparison operations are provided for this purpose.</para>
    /// </remarks>
    [Serializable]
    public struct Epoch : IEquatable<Epoch>, IComparable<Epoch>
    {
        private long dataLossNumber;
        private long configurationNumber;

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Epoch" /> class with the specified 
        /// data loss number and configuration number.</para>
        /// </summary>
        /// <param name="dataLossNumber">
        /// <para>An <see cref="System.Int64" /> representing an increasing value which is updated 
        /// whenever data loss is suspected.</para>
        /// </param>
        /// <param name="configurationNumber">
        /// <para>An <see cref="System.Int64" /> representing an increasing value that is updated whenever 
        /// the configuration of this replica set changes.</para>
        /// </param>
        public Epoch(long dataLossNumber, long configurationNumber)
        {
            this.dataLossNumber = dataLossNumber;
            this.configurationNumber = configurationNumber;
        }

        /// <summary>
        /// <para>Gets the current data loss number in this <see cref="System.Fabric.Epoch" />.</para>
        /// </summary>
        /// <value>
        /// <para>Returns an <see cref="System.Int64" /> representing the current data loss number.</para>
        /// </value>
        /// <remarks>
        /// <para>The data loss number property is an increasing value which is updated 
        /// whenever data loss is suspected, as when loss of a quorum of replicas in the replica set 
        /// that includes the Primary replica.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.DataLossVersion)]
        public long DataLossNumber
        {
            get
            {
                return this.dataLossNumber;
            }

            set
            {
                this.dataLossNumber = value;
            }
        }

        /// <summary>
        /// <para>Gets or sets the current configuration number property in this <see cref="System.Fabric.Epoch" />.</para>
        /// </summary>
        /// <value>
        /// <para>Returns an <see cref="System.Int64" /> representing the configuration number.</para>
        /// </value>
        /// <remarks>
        /// <para>The configuration number is an increasing value that is updated whenever the configuration 
        /// of this replica set changes. The services are informed of the current configuration number 
        /// only when <see cref="System.Fabric.IReplicator.UpdateEpochAsync(Epoch, Threading.CancellationToken)" />
        /// method is called as a result of an attempt to change the Primary replica of the replica set.</para>
        /// </remarks>
        [JsonCustomization(PropertyName = JsonPropertyNames.ConfigurationVersion)]
        public long ConfigurationNumber
        {
            get
            {
                return this.configurationNumber;
            }

            set
            {
                this.configurationNumber = value;
            }
        }

        /// <summary>
        /// <para>Determines whether two specified <see cref="System.Fabric.Epoch" /> objects have the same value.</para>
        /// </summary>
        /// <param name="left">
        /// <para>The left <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <param name="right">
        /// <para>The right <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of <paramref name="left" /> is the same as the value of <paramref name="right" />; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator ==(Epoch left, Epoch right)
        {
            return left.Equals(right);
        }

        /// <summary>
        /// <para>Determines whether two specified <see cref="System.Fabric.Epoch" /> objects have different values.</para>
        /// </summary>
        /// <param name="left">
        /// <para>The left <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <param name="right">
        /// <para>The right <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of <paramref name="left" /> is different than the value of <paramref name="right" />; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator !=(Epoch left, Epoch right)
        {
            return !(left == right);
        }

        /// <summary>
        /// <para>Determines whether one specified <see cref="System.Fabric.Epoch" /> object is less than another specified <see cref="System.Fabric.Epoch" /> object.</para>
        /// </summary>
        /// <param name="left">
        /// <para>The left <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <param name="right">
        /// <para>The right <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of <paramref name="left" /> is less than the value of <paramref name="right" />; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator <(Epoch left, Epoch right)
        {
            return left.CompareTo(right) < 0;
        }

        /// <summary>
        /// <para>Determines whether one specified <see cref="System.Fabric.Epoch" /> object is less than or equal to another specified <see cref="System.Fabric.Epoch" /> object.</para>
        /// </summary>
        /// <param name="left">
        /// <para>The left <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <param name="right">
        /// <para>The right <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of <paramref name="left" /> is less than or equal to the value of <paramref name="right" />; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator <=(Epoch left, Epoch right)
        {
            return left.CompareTo(right) <= 0;
        }

        /// <summary>
        /// <para>Determines whether one specified <see cref="System.Fabric.Epoch" /> object is greater than another specified <see cref="System.Fabric.Epoch" /> object.</para>
        /// </summary>
        /// <param name="left">
        /// <para>The left <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <param name="right">
        /// <para>The right <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of <paramref name="left" /> is greater than the value of <paramref name="right" />; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator >(Epoch left, Epoch right)
        {
            return left.CompareTo(right) > 0;
        }

        /// <summary>
        /// <para>Determines whether one specified <see cref="System.Fabric.Epoch" /> object is greater than or equal to another specified <see cref="System.Fabric.Epoch" /> object.</para>
        /// </summary>
        /// <param name="left">
        /// <para>The left <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <param name="right">
        /// <para>The right <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the value of <paramref name="left" /> is greater than or equal to the value of <paramref name="right" />; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public static bool operator >=(Epoch left, Epoch right)
        {
            return left.CompareTo(right) >= 0;
        }

        /// <summary>
        /// <para>Determines whether the specified object is equal to the current object.</para>
        /// </summary>
        /// <param name="obj">
        /// <para>The object to compare with the current object.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the specified object is equal to the current object; 
        ///     otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public override bool Equals(object obj)
        {
            if (obj == null || typeof(Epoch) != obj.GetType())
            {
                return false;
            }

            Epoch other = (Epoch)obj;
            return this.Equals(other);
        }

        /// <summary>
        /// <para>Serves as a hash function for the <see cref="System.Fabric.Epoch" /> type.</para>
        /// </summary>
        /// <returns>
        /// <para>A <see cref="System.Int32" /> representing a hash code for the current <see cref="System.Fabric.Epoch" />..</para>
        /// </returns>
        public override int GetHashCode()
        {
            return this.ConfigurationNumber.GetHashCode() + (int)this.DataLossNumber;
        }

        /// <summary>
        /// <para>Determines whether the specified <see cref="System.Fabric.Epoch" /> object is equal to the current <see cref="System.Fabric.Epoch" /> object.</para>
        /// </summary>
        /// <param name="other">
        /// <para>The object to compare with the current <see cref="System.Fabric.Epoch" /> object.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the specified <see cref="System.Fabric.Epoch" /> object is equal to the current <see cref="System.Fabric.Epoch" /> object; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Equals(Epoch other)
        {
            return (this.ConfigurationNumber == other.ConfigurationNumber) && (this.DataLossNumber == other.DataLossNumber);
        }

        /// <summary>
        /// <para>Compares this <see cref="System.Fabric.Epoch" /> object to the specified <paramref name="other" /><see cref="System.Fabric.Epoch" /> object.</para>
        /// </summary>
        /// <param name="other">
        /// <para>The <see cref="System.Fabric.Epoch" /> object to compare.</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Int32" />.</para>
        /// </returns>
        public int CompareTo(Epoch other)
        {
            if (this.Equals(other))
            {
                return 0;
            }

            if (this.DataLossNumber < other.DataLossNumber || (this.DataLossNumber == other.DataLossNumber && this.ConfigurationNumber < other.ConfigurationNumber))
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }

        internal static unsafe Epoch FromNative(IntPtr epochIntPtr)
        {
            var nativeEpoch = (NativeTypes.FABRIC_EPOCH*)epochIntPtr;
            var epoch = new Epoch(nativeEpoch->DataLossNumber, nativeEpoch->ConfigurationNumber);

            return epoch;
        }

        internal static unsafe Epoch FromNative(NativeTypes.FABRIC_EPOCH nativeEpoch)
        {
            var epoch = new Epoch(nativeEpoch.DataLossNumber, nativeEpoch.ConfigurationNumber);

            return epoch;
        }

        internal void ToNative(out NativeTypes.FABRIC_EPOCH native)
        {
            native.DataLossNumber = this.DataLossNumber;
            native.ConfigurationNumber = this.ConfigurationNumber;
            native.Reserved = IntPtr.Zero;
        }
    }
}