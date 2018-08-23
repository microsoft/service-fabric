// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;

    /// <summary>
    /// State machine of role, apply context and drain state
    /// </summary>
    internal class RoleContextDrainState
    {
        private IStatefulServicePartition partition;

        public RoleContextDrainState()
        {
            Reuse(null);
        }

        public ReplicaRole ReplicaRole
        {
            get; private set;
        }

        public DrainingStream DrainingStream
        {
            get; private set;
        }

        public ApplyContext ApplyRedoContext
        {
            get; private set;
        }

        public bool IsClosing
        {
            get; private set;
        }

        public void Reuse(IStatefulServicePartition partition)
        {
            this.partition = partition;
            this.ReplicaRole = ReplicaRole.Unknown;
            this.DrainingStream = DrainingStream.Invalid;
            this.ApplyRedoContext = ApplyContext.Invalid;
            this.IsClosing = false;
        }

        public void ReportPartitionFault()
        {
            if (!this.IsClosing)
            {
                this.IsClosing = true;

                Utility.Assert(this.partition != null, "Partition must not be null while reporting fault");

                // Only report transient faults as permanent can cause data loss
                this.partition.ReportFault(FaultType.Transient);
            }
        }

        public void OnClosing()
        {
            this.IsClosing = true;
        }

        public void OnRecovery()
        {
            Utility.Assert(
                this.DrainingStream == DrainingStream.Invalid,
                "OnRecovery: this.DrainingStream == DrainingStream.Invalid. It is {0}",
                this.DrainingStream);

            Utility.Assert(
                this.ReplicaRole == ReplicaRole.Unknown,
                "OnRecovery: this.ReplicaRole == ReplicaRole.Unknown. It is {0}",
                this.ReplicaRole);

            DrainingStream = DrainingStream.RecoveryStream;
            ApplyRedoContext = ApplyContext.RecoveryRedo;
        }

        public void OnRecoveryCompleted()
        {
            Utility.Assert(
                this.DrainingStream == DrainingStream.RecoveryStream,
                "OnRecoveryCompleted: this.DrainingStream == DrainingStream.Recovery. It is {0}",
                this.DrainingStream);

            Utility.Assert(
                this.ReplicaRole == ReplicaRole.Unknown,
                "OnRecoveryCompleted: this.ReplicaRole == ReplicaRole.Unknown. It is {0}",
                this.ReplicaRole);

            Utility.Assert(
                this.ApplyRedoContext == ApplyContext.RecoveryRedo,
                "OnRecoveryCompleted: this.ApplyRedoContext == ApplyContext.RecoveryRedo. It is {0}",
                this.ApplyRedoContext);

            DrainingStream = DrainingStream.Invalid;
            ApplyRedoContext = ApplyContext.Invalid;
        }

        public void OnDrainState()
        {
            Utility.Assert(
                this.DrainingStream == DrainingStream.Invalid,
                "OnDrainState: this.DrainingStream == DrainingStream.Invalid. It is {0}",
                this.DrainingStream);

            Utility.Assert(
                this.ReplicaRole == ReplicaRole.IdleSecondary,
                "OnDrainState: this.ReplicaRole == ReplicaRole.IdleSecondary. It is {0}",
                this.ReplicaRole);

            DrainingStream = DrainingStream.StateStream;
            ApplyRedoContext = ApplyContext.Invalid;
        }

        public void OnDrainCopy()
        {
            Utility.Assert(
                this.DrainingStream == DrainingStream.Invalid || this.DrainingStream == DrainingStream.StateStream,
                "OnDrainCopy: this.DrainingStream == DrainingStream.Invalid || this.DrainingStream == DrainingStream.StateStream. It is {0}",
                this.DrainingStream);

            Utility.Assert(
                this.ReplicaRole == ReplicaRole.IdleSecondary,
                "OnDrainCopy: this.ReplicaRole == ReplicaRole.IdleSecondary. It is {0}",
                this.ReplicaRole);

            DrainingStream = DrainingStream.CopyStream;
            ApplyRedoContext = ApplyContext.SecondaryRedo;
        }

        public void OnDrainReplication()
        {
            Utility.Assert(
                this.DrainingStream == DrainingStream.CopyStream || this.DrainingStream == DrainingStream.StateStream || this.DrainingStream == DrainingStream.Invalid,
                "OnDrainReplication: this.DrainingStream == DrainingStream.Invalid || this.DrainingStream == DrainingStream.StateStream. It is {0}",
                this.DrainingStream);

            Utility.Assert(
                this.ReplicaRole == ReplicaRole.IdleSecondary || this.ReplicaRole == ReplicaRole.ActiveSecondary,
                "OnDrainReplication: this.ReplicaRole == ReplicaRole.IdleSecondary|ActiveSecondary. It is {0}",
                this.ReplicaRole);

            DrainingStream = DrainingStream.ReplicationStream;
            ApplyRedoContext = ApplyContext.SecondaryRedo;
        }

        private void BecomePrimary()
        {
            // U->P will have invalid draining stream
            // S/I->P will have replication as the respective drain stream
            Utility.Assert(
                this.DrainingStream == DrainingStream.Invalid || this.DrainingStream == DrainingStream.ReplicationStream || this.DrainingStream == DrainingStream.CopyStream,
                "BecomePrimary: his.DrainingStream == DrainingStream.Invalid || this.DrainingStream == DrainingStream.ReplicationStream || this.DrainingStream == DrainingStream.CopyStream. It is {0}",
                this.DrainingStream);

            this.ReplicaRole = ReplicaRole.Primary;
            this.ApplyRedoContext = ApplyContext.PrimaryRedo;
            this.DrainingStream = DrainingStream.Primary;
        }

        private void BecomeActiveSecondary()
        {
            Utility.Assert(
                this.ReplicaRole == ReplicaRole.IdleSecondary || this.ReplicaRole == ReplicaRole.Primary,
                "BecomeActiveSecondary: this.ReplicaRole == ReplicaRole.IdleSecondary || this.ReplicaRole == ReplicaRole.Primary. It is {0}",
                this.ReplicaRole);

            this.ReplicaRole = ReplicaRole.ActiveSecondary;
        }

        public void ChangeRole(ReplicaRole newRole)
        {
            if (this.ReplicaRole == ReplicaRole.ActiveSecondary)
            {
                Utility.Assert(newRole != ReplicaRole.IdleSecondary, "ChangeRole: S->I not allowed");
            }

            if (this.ReplicaRole == ReplicaRole.IdleSecondary &&
                newRole == ReplicaRole.ActiveSecondary)
            {
                BecomeActiveSecondary();
            }
            else if (newRole == ReplicaRole.Primary)
            {
                BecomePrimary();
            }
            else
            {
                this.ReplicaRole = newRole;
                this.ApplyRedoContext = ApplyContext.Invalid;
                this.DrainingStream = DrainingStream.Invalid;
            }
        }
    }
}