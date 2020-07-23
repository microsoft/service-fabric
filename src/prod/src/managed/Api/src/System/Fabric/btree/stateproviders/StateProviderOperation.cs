// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Data.Replicator;
    using System.Threading.Tasks;

    /// <summary>
    /// Base state provider operation.
    /// </summary>
    public abstract class StateProviderOperation
    {
        #region Instance Members

        /// <summary>
        /// Atomic group id for this operation.
        /// </summary>
        long atomicGroupId = FabricReplicatorEx.InvalidAtomicGroupId;

        /// <summary>
        /// Operation sequence number.
        /// </summary>
        long sequenceNumber = FabricReplicatorEx.InvalidSequenceNumber;

        /// <summary>
        /// Data required to redo this operation.
        /// </summary>
        IOperationData redoInfo;

        /// <summary>
        /// Data required to undo this operation.
        /// </summary>
        IOperationData undoInfo;

        /// <summary>
        /// 
        /// </summary>
        IOperationData serviceMetadata;

        #endregion

        #region Properties

        /// <summary>
        /// Atomic group id for this operation.
        /// </summary>
        public long AtomicGroupId
        {
            get
            {
                return this.atomicGroupId;
            }
            set
            {
                this.atomicGroupId = value;
            }
        }

        /// <summary>
        /// Operation sequence number.
        /// </summary>
        public long SequenceNumber
        {
            get
            {
                return this.sequenceNumber;
            }
            set
            {
                this.sequenceNumber = value;
            }
        }

        /// <summary>
        /// Data required to redo this operation.
        /// </summary>
        public IOperationData RedoInfo
        {
            get
            {
                return this.redoInfo;
            }
            set
            {
                this.redoInfo = value;
            }
        }

        /// <summary>
        /// Data required to undo this operation.
        /// </summary>
        public IOperationData UndoInfo
        {
            get
            {
                return this.undoInfo;
            }
            set
            {
                this.undoInfo = value;
            }
        }

        /// <summary>
        /// Metadata associated to this operation.
        /// </summary>
        public IOperationData ServiceMetadata
        {
            get
            {
                return this.serviceMetadata;
            }
            set
            {
                this.serviceMetadata = value;
            }
        }

        #endregion
    }
}