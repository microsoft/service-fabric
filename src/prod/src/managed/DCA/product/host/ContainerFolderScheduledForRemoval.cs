// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca
{
    // class to represent Container Log folders scheduled for deletion by Container Manager.
    // It holds information of when it was scheduled for deletion to allow controlling folder removal wait times.
    internal class ContainerFolderScheduledForRemoval
    {
        private const string TraceType = "ContainerFolderScheduledForRemoval";
        private int deletionTimeTickCount;

        public ContainerFolderScheduledForRemoval(string containerLogFullPath)
        {
            this.deletionTimeTickCount = System.Environment.TickCount;
            this.ContainerLogFullPath = containerLogFullPath;
        }

        public string ContainerLogFullPath { get; }

        public bool IsProcessingWaitTimeExpired(TimeSpan maxWaitTime)
        {
            int currentTickCount = System.Environment.TickCount;
            TimeSpan elapsed = TimeSpan.FromTicks((long)(currentTickCount - this.deletionTimeTickCount) * TimeSpan.TicksPerMillisecond);

            if (elapsed > maxWaitTime)
            {
                return true;
            }

            return false;
        }
    }
}