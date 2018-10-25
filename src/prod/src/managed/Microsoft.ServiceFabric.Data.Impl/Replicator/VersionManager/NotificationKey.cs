// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;

    /// <summary>
    /// Struct for notification key.
    /// </summary>
    internal struct NotificationKey : IEquatable<NotificationKey>
    {
        /// <summary>
        /// State Provider Id of the state provider for whom the notification is registered.
        /// </summary>
        public readonly long StateProviderId;

        /// <summary>
        /// Snapshot sequence number of the notification.
        /// </summary>
        public readonly long VisibilityNumber;

        /// <summary>
        /// Initializes a new instance of the <see cref="NotificationKey"/> struct.
        /// </summary>
        /// <param name="stateProviderId">The state provider id.</param>
        /// <param name="visibilityNumber">The snapshot sequence number.</param>
        internal NotificationKey(long stateProviderId, long visibilityNumber)
        {
            this.StateProviderId = stateProviderId;
            this.VisibilityNumber = visibilityNumber;
        }

        public bool Equals(NotificationKey other)
        {
            return this.StateProviderId == other.StateProviderId && this.VisibilityNumber == other.VisibilityNumber;
        }

        public override bool Equals(object obj)
        {
            if (false == obj is NotificationKey)
            {
                return false;
            }

            return this.Equals((NotificationKey) obj);
        }

        public override int GetHashCode()
        {
            return this.StateProviderId.GetHashCode() ^ this.VisibilityNumber.GetHashCode();
        }
    }
}