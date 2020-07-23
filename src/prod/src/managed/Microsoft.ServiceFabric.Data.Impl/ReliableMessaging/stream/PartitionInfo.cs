// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    #region using directives

    using System.Collections.Generic;
    using System.Fabric.Common;

    #endregion

    /// <summary>
    /// Represents a service (partition)  Info. data like end point, era ex.
    /// </summary>
    [Serializable]
    public class PartitionInfo : IComparable<PartitionInfo>, IEquatable<PartitionInfo>
    {
        #region properties

        /// <summary>
        /// 
        /// </summary>
        public string EndPoint { get; set; }

        /// <summary>
        /// 
        /// </summary>
        public Guid Era { get; set; }

        #endregion

        #region cstor

        /// <summary>
        /// Creates an empty instance of PartitionInfo
        /// Parameterless constructor required for decode/serialization of object instance.
        /// </summary>
        internal PartitionInfo()
        {
        }

        /// <summary>
        /// Creates an instance of PartitionInfo
        /// Partition End point and Era will change if the primary replica moves to a new replica, 
        /// its also possible that the Era alone will change, if the replica is reset at the same location.
        /// </summary>
        /// <param name="endPoint">Partition End point</param>
        /// <param name="era">Partition Era</param>
        internal PartitionInfo(string endPoint, Guid era)
        {
            this.EndPoint = endPoint;
            this.Era = era;
        }

        #endregion

        /// <summary>
        /// Overrides the Hash function for this object. 
        /// return Bitwise exclusive-OR column by column of
        /// EndPoint^Era Info
        /// 
        /// </summary>
        /// <returns>
        /// A hash code for the current 
        ///     <see>
        ///         <cref>T:System.Fabric.Messaging.Stream.PartitionBody</cref>
        ///     </see>
        /// </returns>
        public override int GetHashCode()
        {
            return this.EndPoint.GetHashCode() ^ this.Era.GetHashCode();
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
        public int CompareTo(PartitionInfo other)
        {
            if (other == null)
            {
                return 1;
            }

            if (this.EndPoint != other.EndPoint)
            {
                return String.Compare(this.EndPoint, other.EndPoint, StringComparison.Ordinal);
            }

            return this.Era.CompareTo(other.Era);
        }

        /// <summary>
        /// Indicates whether the current object is equal to another object of the same type.
        /// </summary>
        /// <returns>
        /// true if the current object is equal to the <paramref name="other"/> parameter; otherwise, false.
        /// </returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(PartitionInfo other)
        {
            return this.CompareTo(other) == 0;
        }

        /// <summary>
        /// String representation of the PartitionInfo
        /// </summary>
        /// <returns>String representation of the PartitionInfo</returns>
        public override string ToString()
        {
            return string.Format(" EndPoint = {0} Era = {1}", this.EndPoint, this.Era.ToString());
        }
    }

    internal class PartitionInfoComparer : IComparer<PartitionInfo>
    {
        // Summary:
        //     Compares two objects and returns a value indicating whether one is less than,
        //     equal to, or greater than the other.
        //
        // Parameters:
        //   x:
        //     The first info to compare.
        //
        //   y:
        //     The second info to compare.
        //
        public int Compare(PartitionInfo x, PartitionInfo y)
        {
            return x.CompareTo(y);
        }
    }

    internal class PartitionInfoEqualityComparer : IEqualityComparer<PartitionInfo>
    {
        public int GetHashCode(PartitionInfo info)
        {
            info.ThrowIfNull("Info");

            return info.GetHashCode();
        }

        public bool Equals(PartitionInfo body1, PartitionInfo body2)
        {
            if (body1 == null)
            {
                return body2 == null;
            }

            return body1.Equals(body2);
        }
    }
}