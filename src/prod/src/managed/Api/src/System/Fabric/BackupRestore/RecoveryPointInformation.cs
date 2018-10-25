// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    [Serializable]
    internal class RecoveryPointInformation
    {
        public string Location { get; set; }

        public DateTime BackupTime { get; set; }

        public Guid BackupId { get; set; }

        public Guid BackupChainId { get; set; }

        public long BackupIndex { get; set; }
    }
}