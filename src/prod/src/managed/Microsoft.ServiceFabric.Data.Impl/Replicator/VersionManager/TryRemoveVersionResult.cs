// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// This is for internal use only.
    /// Encapsulates the output of the TryRemoveVersionAsync
    /// </summary>
    public struct TryRemoveVersionResult : IEquatable<TryRemoveVersionResult>
    {
        /// <summary>
        /// 
        /// </summary>
        public bool CanBeRemoved;

        /// <summary>
        /// 
        /// </summary>
        public ISet<long> EnumerationSet;

        /// <summary>
        /// 
        /// </summary>
        public ISet<EnumerationCompletionResult> EnumerationCompletionNotifications;

        internal TryRemoveVersionResult(
            bool canBeRemoved,
            ISet<long> enumerationSet,
            ISet<EnumerationCompletionResult> enumerationCompletionNotifications)
        {
            this.CanBeRemoved = canBeRemoved;
            this.EnumerationSet = enumerationSet;
            this.EnumerationCompletionNotifications = enumerationCompletionNotifications;
        }

        /// <summary>
        /// Indicates whether the current object is equal to another object of the same type.
        /// </summary>
        /// <returns>
        /// true if the current object is equal to the <paramref name="other"/> parameter; otherwise, false.
        /// </returns>
        /// <param name="other">An object to compare with this object.</param>
        public bool Equals(TryRemoveVersionResult other)
        {
            if (this.CanBeRemoved != other.CanBeRemoved)
            {
                return false;
            }

            if (null == this.EnumerationSet)
            {
                if (null != other.EnumerationSet)
                {
                    return false;
                }
            }
            else
            {
                if (null == other.EnumerationSet)
                {
                    return false;
                }

                if (false == this.EnumerationSet.SetEquals(other.EnumerationSet))
                {
                    return false;
                }
            }

            if (null == this.EnumerationCompletionNotifications)
            {
                if (null != other.EnumerationCompletionNotifications)
                {
                    return false;
                }
            }
            else
            {
                if (null == other.EnumerationCompletionNotifications)
                {
                    return false;
                }

                if (other.EnumerationCompletionNotifications.Count != this.EnumerationCompletionNotifications.Count)
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Indicates whether this instance and a specified object are equal.
        /// </summary>
        /// <returns>
        /// true if <paramref name="obj"/> and this instance are the same type and represent the same value; otherwise, false.
        /// </returns>
        /// <param name="obj">Another object to compare to. </param><filterpriority>2</filterpriority>
        public override bool Equals(object obj)
        {
            var isRightType = obj is TryRemoveVersionResult;

            if (isRightType == false)
            {
                return false;
            }

            return this.Equals((TryRemoveVersionResult) obj);
        }

        /// <summary>
        /// Returns the hash code for this instance.
        /// </summary>
        /// <returns>
        /// A 32-bit signed integer that is the hash code for this instance.
        /// </returns>
        /// <filterpriority>2</filterpriority>
        public override int GetHashCode()
        {
            var result = this.CanBeRemoved.GetHashCode();

            if (this.EnumerationSet != null)
            {
                result = result ^ this.EnumerationSet.GetHashCode();
            }

            if (this.EnumerationCompletionNotifications != null)
            {
                result = result ^ this.EnumerationCompletionNotifications.GetHashCode();
            }

            return result;
        }
    }
}