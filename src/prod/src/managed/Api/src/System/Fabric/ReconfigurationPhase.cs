// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// Represents replica reconfiguration phase.
    /// </summary>
    public enum ReconfigurationPhase
    {
        /// <summary>
        /// Invalid reconfiguration phase.
        /// </summary>
        Unknown = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_INVALID,

        /// <summary>
        /// There is no reconfiguration currently in progress.
        /// </summary>
        None = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_NONE,

        /// <summary>
        /// The reconfiguration is transferring data from the previous primary to the new primary.
        /// </summary>
        Phase0 = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_ZERO,

        /// <summary>
        /// The reconfiguration is querying the replica set for the progress.
        /// </summary>
        Phase1 = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_ONE,

        /// <summary>
        /// The reconfiguration is ensuring that data from the current primary is present in a majority of the replica set.
        /// </summary>
        Phase2 = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_TWO,

        /// <summary>
        /// For internal use only.
        /// </summary>
        Phase3 = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_THREE,

        /// <summary>
        /// For internal use only.
        /// </summary>
        Phase4 = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_PHASE_FOUR,

        /// <summary>
        /// For internal use only.
        /// </summary>
        AbortPhaseZero = NativeTypes.FABRIC_RECONFIGURATION_PHASE.FABRIC_RECONFIGURATION_ABORT_PHASE_ZERO
    }
}