// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    internal static class ReplicatorDynamicConstants
    {
        private static int abortBackoffFactor = 2;

        private static int abortDelay = 500;

        private static int groupCommitBackoffFactor = 2;

        private static int groupCommitDelay = 20;

        private static int maxAbortDelay = 5000;

        private static int maxGroupCommitDelay = 1000;

        private static int parallelRecoveryBatchSize = 128;

        internal static int AbortBackoffFactor
        {
            get { return abortBackoffFactor; }

            set { abortBackoffFactor = value; }
        }

        internal static int AbortDelay
        {
            get { return abortDelay; }

            set { abortDelay = value; }
        }

        internal static int GroupCommitBackoffFactor
        {
            get { return groupCommitBackoffFactor; }

            set { groupCommitBackoffFactor = value; }
        }

        internal static int GroupCommitDelay
        {
            get { return groupCommitDelay; }

            set { groupCommitDelay = value; }
        }

        internal static int MaxAbortDelay
        {
            get { return maxAbortDelay; }

            set { maxAbortDelay = value; }
        }

        internal static int MaxGroupCommitDelay
        {
            get { return maxGroupCommitDelay; }

            set { maxGroupCommitDelay = value; }
        }

        internal static int ParallelRecoveryBatchSize
        {
            get { return parallelRecoveryBatchSize; }

            set { parallelRecoveryBatchSize = value; }
        }
    }
}