// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    /// <summary>
    /// A record that captures the user's input on the action to allow for a particular tenant job.
    /// </summary>
    /// TODO, do we need the interface?
    internal sealed class AllowActionRecord : IAllowActionRecord
    {
        public Guid JobId { get; private set; }

        public uint? UD { get; private set; }

        public DateTimeOffset CreationTime { get; private set; }

        public ActionType Action { get; private set; }

        public AllowActionRecord(Guid jobId, uint? ud, ActionType action)
        {
            JobId = jobId;
            UD = ud;
            Action = action;
            CreationTime = DateTimeOffset.UtcNow;
        }

        public override string ToString()
        {
            var text = "{0}/{1}/{2}/{3:o}".ToString(JobId, UD, Action, CreationTime);
            return text;
        }
    }
}