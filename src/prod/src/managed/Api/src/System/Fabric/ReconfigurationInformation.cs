// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    /// <summary>
    /// Represents information about the replica reconfiguration.
    /// </summary>
    public sealed class ReconfigurationInformation
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.ReconfigurationInformation" /> class.</para>
        /// </summary>
        /// <param name="previousConfigurationRole"></param>
        /// <param name="reconfigurationPhase"></param>
        /// <param name="reconfigurationType"></param>
        /// <param name="reconfigurationStartTimeUtc"></param>
        public ReconfigurationInformation(
            ReplicaRole previousConfigurationRole,
            ReconfigurationPhase reconfigurationPhase,
            ReconfigurationType reconfigurationType,
            DateTime reconfigurationStartTimeUtc)
        {
            this.PreviousConfigurationRole = previousConfigurationRole;
            this.ReconfigurationPhase = reconfigurationPhase;
            this.ReconfigurationType = reconfigurationType;
            this.ReconfigurationStartTimeUtc = reconfigurationStartTimeUtc;
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="ReconfigurationInformation" /> class with default values.</para>
        /// </summary>
        public ReconfigurationInformation()
        {
            this.PreviousConfigurationRole = ReplicaRole.None;
            this.ReconfigurationPhase = ReconfigurationPhase.Unknown;
            this.ReconfigurationType = ReconfigurationType.Unknown;
            this.ReconfigurationStartTimeUtc = DateTime.MinValue;
        }

        /// <summary>
        /// <para>The replica role prior to the reconfiguration. The value would be <see cref="ReplicaRole.Unknown" />if there was no previous reconfiguration.</para>
        /// <value>The previous replica role.</value>
        /// </summary>
        public ReplicaRole PreviousConfigurationRole { get; internal set; }

        /// <summary>
        /// <para>The current phase of the reconfiguration. The value would be <see cref="ReconfigurationPhase.None" /> if no reconfiguration is taking place.</para>
        /// <value>The phase of current ongoing reconfiguration.</value>
        /// </summary>
        public ReconfigurationPhase ReconfigurationPhase
        {
            get;
            private set;
        }

        /// <summary>
        /// <param>The type of the reconfiguration. The value would be <see cref="ReconfigurationType.None" /> if no reconfiguration is taking place.</param>
        /// <value>The type of reconfiguration.</value>
        /// </summary>
        public ReconfigurationType ReconfigurationType
        {
            get;
            private set;
        }

        /// <summary>
        /// <param>The date time when reconfiguration started. The value is represented as DateTime in UTC. 
        /// The vlaue would be <see cref="DateTime.MinValue"/> if no reconfiguration is taking place.</param>
        /// <value>The start date time of reconfiguration in UTC format.</value>
        /// </summary>
        public DateTime ReconfigurationStartTimeUtc
        {
            get;
            private set;
        }
    }
}