// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System;

    public class ClusterStreamingId
    {
        #region Constructors

        public ClusterStreamingId(long sequenceId)
        {
            if (sequenceId < 0)
            {
                throw new ArgumentOutOfRangeException("sequenceId", "Sequence id must be non negative");
            }

            this.SequenceId = sequenceId;
        }

        #endregion

        public long SequenceId { get; set; }

        #region Overrides

        public override string ToString()
        {
            return string.Format("{0}", this.SequenceId);
        }

        #endregion
    }
}