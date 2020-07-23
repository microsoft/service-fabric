// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    using Microsoft.ServiceFabric.Data;
    #region using directives

    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.ReliableMessaging;
    using System.Fabric.ReliableMessaging.Session;
    using System.IO;

    #endregion

    #region PartitionKind

    /// <summary>
    /// Classifies all service partition Kind.
    /// 
    /// </summary>
    public enum PartitionKind
    {
        /// <summary>
        /// 
        /// </summary>
        Singleton = 1001,

        /// <summary>
        /// 
        /// </summary>
        Numbered = 2001,

        /// <summary>
        /// 
        /// </summary>
        Named = 3001
    }

    #endregion

    /// <summary>
    /// Represents a service (partition).
    /// </summary>
    [Serializable]
    public class PartitionKey : IComparable<PartitionKey>, IEquatable<PartitionKey>
    {
        #region properties

        private string serviceInstanceString;

        private IntegerPartitionKeyRange partitionRange;

        /// <summary>
        /// 
        /// </summary>
        public PartitionKind Kind { get; private set; }

        /// <summary>
        /// 
        /// </summary>
        public Uri ServiceInstanceName { get; private set; }

        /// <summary>
        /// 
        /// </summary>
        public string PartitionName { get; private set; }

        /// <summary>
        /// 
        /// </summary>
        public IntegerPartitionKeyRange PartitionRange
        {
            get { return this.partitionRange; }
        }

        /// <summary>
        /// 
        /// </summary>
        public long PartitionNumber
        {
            get { return this.Kind == PartitionKind.Numbered ? this.partitionRange.IntegerKeyLow : 0; }
        }

        #endregion

        #region cstor

        /// <summary>
        /// Creates a new PartitionKey specifing its service instance name and set the partition kind to default to 
        /// singleton.
        /// </summary>
        /// <param name="serviceInstanceName">Service name of the role</param>
        public PartitionKey(Uri serviceInstanceName)
        {
            //Set service and partition info.
            this.ServiceInstanceName = serviceInstanceName;
            this.serviceInstanceString = this.ServiceInstanceName.OriginalString;
            this.Kind = PartitionKind.Singleton;
        }

        /// <summary>
        /// Creates a new PartitionKey specifing its service instance name and set the partition kind to numbered.
        /// </summary>
        /// <param name="serviceInstanceName">Service name of the role</param>
        /// <param name="partitionNumber">Service partition number</param>
        public PartitionKey(Uri serviceInstanceName, long partitionNumber)
        {
            //Set service and partition info.
            this.ServiceInstanceName = serviceInstanceName;
            this.serviceInstanceString = this.ServiceInstanceName.OriginalString;
            this.partitionRange = new IntegerPartitionKeyRange {IntegerKeyLow = partitionNumber, IntegerKeyHigh = partitionNumber};
            this.Kind = PartitionKind.Numbered;
        }

        /// <summary>
        /// Creates a new PartitionKey specifing its service instance name, partition range and numbered kind.
        /// </summary>
        /// <param name="serviceInstanceName">Service name of the role</param>
        /// <param name="lowKey">Partition range - Low key</param>
        /// <param name="highKey">Partition range - High key</param>
        public PartitionKey(Uri serviceInstanceName, long lowKey, long highKey)
        {
            //Set service and partition info.
            this.ServiceInstanceName = serviceInstanceName;
            this.serviceInstanceString = this.ServiceInstanceName.OriginalString;
            this.partitionRange = new IntegerPartitionKeyRange {IntegerKeyLow = lowKey, IntegerKeyHigh = highKey};
            this.Kind = PartitionKind.Numbered;
        }

        /// <summary>
        /// Creates a new PartitionKey specifing its service instance name and partition name
        /// </summary>
        /// <param name="serviceInstanceName">Service name of the role</param>
        /// <param name="partitionName">Partition name of the role</param>
        public PartitionKey(Uri serviceInstanceName, string partitionName)
        {
            //Set service and partition info.
            this.ServiceInstanceName = serviceInstanceName;
            this.serviceInstanceString = this.ServiceInstanceName.OriginalString;
            this.PartitionName = partitionName;
            this.Kind = PartitionKind.Named;
        }

        #endregion

        /// <summary>
        /// Encoded byte count is variable as it would need to be determined by exploring
        /// the partition type(Kind) and partition info along with the fixed size of the service instance info.
        /// </summary>
        internal int EncodingByteCount
        {
            get
            {
                var encoder = new StringEncoder();
                var svcNameByteLength = encoder.GetEncodingByteCount(this.serviceInstanceString);

                switch (this.Kind)
                {
                    case PartitionKind.Singleton:
                        // kind + svcNameEncoding
                        return sizeof(int) + svcNameByteLength;
                    case PartitionKind.Numbered:
                        // kind + svcNameEncoding + partitionNumber
                        return sizeof(int) + (2*sizeof(long)) + svcNameByteLength;
                    case PartitionKind.Named:
                        // kind + svcNameEncoding + partitionNameEncoding
                        var partitionNameByteLength = encoder.GetEncodingByteCount(this.PartitionName);
                        return sizeof(int) + svcNameByteLength + partitionNameByteLength;
                }

                Diagnostics.Assert(false, "Unknown partition kind");
                return 0;
            }
        }

        /// <summary>
        /// Check if absolute URI
        /// </summary>
        /// <param name="serviceInstanceName">Service Instance name to be verified.</param>
        private void CheckServiceInstanceValidity(Uri serviceInstanceName)
        {
            serviceInstanceName.ThrowIfNull("serviceInstanceName");

            if (!serviceInstanceName.IsAbsoluteUri)
            {
                throw new ArgumentException(SR.Error_PartitionKey_ServiceInstanceName, "serviceInstanceName");
            }
        }

        /// <summary>
        /// Decode the Partition key from the given stream
        /// </summary>
        /// <param name="binaryReader">BinaryReader to deserialize from</param>
        /// <returns>Decoded Partition Key</returns>
        internal static PartitionKey Decode(BinaryReader binaryReader)
        {
            // Decode PartitionKey.Kind
            var kind = (PartitionKind) binaryReader.ReadInt32();

            // Decode Service Instance Name
            var serviceInstanceString = binaryReader.ReadString();
            var serviceInstanceName = new Uri(serviceInstanceString);

            // Decode the Partition Info (Name, range ex.)
            PartitionKey result = null;
            switch (kind)
            {
                case PartitionKind.Singleton:
                    result = new PartitionKey(serviceInstanceName);
                    break;
                case PartitionKind.Named:
                    var partitionName = binaryReader.ReadString();
                    result = new PartitionKey(serviceInstanceName, partitionName);
                    break;
                case PartitionKind.Numbered:
                    var lowKey = binaryReader.ReadInt64();

                    var highKey = binaryReader.ReadInt64();

                    result = new PartitionKey(serviceInstanceName, lowKey, highKey);
                    break;
            }

            return result;
        }

        /// <summary>
        /// Overrides the Hash function for this object. 
        /// return Bitwise exclusive-OR column by column of
        /// serviceInstanceName^Partition Info
        /// 
        /// </summary>
        /// <returns>
        /// A hash code for the current 
        ///     <see>
        ///         <cref>T:System.Fabric.Messaging.Stream.PartitionKey</cref>
        ///     </see>
        /// </returns>
        public override int GetHashCode()
        {
            var svcNameCode = this.ServiceInstanceName.GetHashCode();

            switch (this.Kind)
            {
                case PartitionKind.Named:
                    return svcNameCode ^ this.PartitionName.GetHashCode();
                case PartitionKind.Numbered:
                    return svcNameCode ^ this.partitionRange.IntegerKeyLow.GetHashCode() ^ this.partitionRange.IntegerKeyHigh.GetHashCode();
                default:
                    Diagnostics.Assert(this.Kind == PartitionKind.Singleton, "PartitionKey has unknown kind in PartitionKey.GetHashCode");
                    return svcNameCode;
            }
        }

        /// <summary>
        /// Compares the current object with another object of the same type.
        /// </summary>
        /// <returns>
        /// A value that indicates the relative order of the objects being compared. 
        /// The return value has the following meanings: Value Meaning Less than zero This object is less than the 
        /// <paramref name="other"/> parameter.Zero This object is equal to <paramref name="other"/>. Greater than zero This object is greater than <paramref name="other"/>. 
        /// </returns>
        /// <param name="other">An object to compare with this object.</param>
        public int CompareTo(PartitionKey other)
        {
            if (other == null)
            {
                return 1;
            }

            if (this.Kind != other.Kind)
            {
                return this.Kind.CompareTo(other.Kind);
            }

            // TODO: revisit Uri equality -- may not be compatible with the unmanaged definition
            var svcInstanceCompare = Uri.Compare(
                this.ServiceInstanceName,
                other.ServiceInstanceName,
                UriComponents.AbsoluteUri,
                UriFormat.UriEscaped,
                StringComparison.Ordinal);

            if (svcInstanceCompare != 0)
            {
                return svcInstanceCompare;
            }

            var result = 0;

            switch (this.Kind)
            {
                case PartitionKind.Singleton:
                    break;
                case PartitionKind.Named:
                    result = String.Compare(this.PartitionName, other.PartitionName, System.StringComparison.Ordinal);
                    break;
                case PartitionKind.Numbered:
                    result = this.PartitionRange.CompareTo(other.PartitionRange);
                    break;
                default:
                    Diagnostics.Assert(false, string.Format("{0} unknown partition kind", this));
                    break;
            }

            return result;
        }

        /// <summary>
        /// Indicates whether the current object is equal to another object of the same type.
        /// </summary>
        /// <returns>
        /// true if the current object is equal to the <paramref name="other"/> parameter; otherwise, false.
        /// </returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(PartitionKey other)
        {
            return this.CompareTo(other) == 0;
        }

        /// <summary>
        /// Encode and write to the memory stream.
        /// 
        /// PartitionType + ServiceInstanceName + PartitionName(/range).
        /// 
        /// TODO:Check if there is enough room in the stream
        /// </summary>
        /// <param name="writer">BinaryWriter to serialize to.</param>
        internal void Encode(BinaryWriter writer)
        {
            // write kind field
            writer.Write((int) this.Kind);

            // write the service instance name size and content
            var encoder = new StringEncoder(this.serviceInstanceString);
            encoder.Encode(writer);

            switch (this.Kind)
            {
                case PartitionKind.Singleton:
                    break;
                case PartitionKind.Numbered:
                    // write partition number
                    writer.Write(this.partitionRange.IntegerKeyLow);
                    writer.Write(this.partitionRange.IntegerKeyHigh);
                    break;
                case PartitionKind.Named:
                    encoder = new StringEncoder(this.PartitionName);
                    encoder.Encode(writer);
                    break;
            }
        }

        /// <summary>
        /// One other specialized PropertyKey.toString() method.
        /// 
        /// TODO: Check if ToString() can suffice for this need.
        /// </summary>
        /// <param name="suffix">Append this suffix</param>
        /// <returns>PublishedPropertyName as string</returns>
        internal string PublishedPropertyName(string suffix)
        {
            var prefix = string.Format("StreamPropertyName:/{0}", this.Kind.ToString());
            string result = null;

            switch (this.Kind)
            {
                case PartitionKind.Singleton:
                    result = prefix;
                    break;
                case PartitionKind.Named:
                    result = string.Format("{0}/[Name = {1}]", prefix, this.PartitionName);
                    break;
                case PartitionKind.Numbered:
                    result = string.Format("{0}/[Range = {1}:{2}]", prefix, this.partitionRange.IntegerKeyLow, this.partitionRange.IntegerKeyHigh);
                    break;
                default:
                    Diagnostics.Assert(false, "Unknown partition kind in PartitionKey.ToString()");
                    break;
            }

            return result + "/" + suffix;
        }

        /// <summary>
        /// String representation of the PartitionKey
        /// </summary>
        /// <returns>String representation of the PartitionKey</returns>
        public override string ToString()
        {
            string result = null;
            var prefix = string.Format(" Service = {0}", this.serviceInstanceString);

            switch (this.Kind)
            {
                case PartitionKind.Singleton:
                    result = prefix;
                    break;
                case PartitionKind.Named:
                    result = string.Format("{0} Name = {1}", prefix, this.PartitionName);
                    break;
                case PartitionKind.Numbered:
                    result = string.Format("{0} Range = {1}:{2}", prefix, this.partitionRange.IntegerKeyLow, this.partitionRange.IntegerKeyHigh);
                    break;
                default:
                    Diagnostics.Assert(false, "Unknown partition kind in PartitionKey.ToString()");
                    break;
            }

            return result ?? string.Empty;
        }
    }

    internal class PartitionKeyComparer : IComparer<PartitionKey>
    {
        // Summary:
        //     Compares two objects and returns a value indicating whether one is less than,
        //     equal to, or greater than the other.
        //
        // Parameters:
        //   x:
        //     The first key to compare.
        //
        //   y:
        //     The second key to compare.
        //
        // Returns:
        //     Compares service instance name first.  Then if the same service instance compares partition kind.  Finally if the same kind then does a custom comparison.

        public int Compare(PartitionKey x, PartitionKey y)
        {
            if (x == null)
            {
                return y == null ? 0 : 1;
            }
            return x.CompareTo(y);
        }
    }

    internal class PartitionKeyEqualityComparer : IEqualityComparer<PartitionKey>
    {
        public int GetHashCode(PartitionKey key)
        {
            key.ThrowIfNull("PartitionKeyEqualityComparer.GetHashCode: key");

            return key.GetHashCode();
        }

        public bool Equals(PartitionKey key1, PartitionKey key2)
        {
            if (key1 == null)
            {
                return key2 == null;
            }

            return key1.Equals(key2);
        }
    }

    internal class UriEqualityComparer : IEqualityComparer<Uri>
    {
        public int GetHashCode(Uri uri)
        {
            uri.ThrowIfNull("UriEqualityComparer.GetHashCode: uri");

            return uri.GetHashCode();
        }

        public static bool AreEqual(Uri uri1, Uri uri2)
        {
            if (uri1 == null)
            {
                return uri2 == null;
            }

            return 0 == Uri.Compare(
                uri1,
                uri2,
                UriComponents.AbsoluteUri,
                UriFormat.UriEscaped,
                StringComparison.Ordinal);
        }

        public bool Equals(Uri uri1, Uri uri2)
        {
            return AreEqual(uri1, uri2);
        }
    }
}