// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Common;

    internal class FinishTaskCommand : InfrastructureServiceCommand
    {
        private static readonly TraceType TraceType = new TraceType("FinishTaskCommand");

        private readonly string taskId;
        private long instanceId;

        public FinishTaskCommand(string taskId, long instanceId)
        {
            this.taskId = taskId;
            this.instanceId = instanceId;
        }

        public override string TaskId
        {
            get { return this.taskId; }
        }

        public override long InstanceId
        {
            get { return this.instanceId; }
        }

        public static FinishTaskCommand TryParseCommand(Queue<string> tokens)
        {
            if (tokens.Count <= 0) 
            {
                Trace.WriteWarning(TraceType, "Command missing Task Id");
                return null; 
            }

            string taskId = tokens.Dequeue();

            return new FinishTaskCommand(taskId, 0);
        }

        public void TryUpdateInstanceId(long instanceId)
        {
            if (this.instanceId < instanceId)
            {
                this.instanceId = instanceId;
            }
        }
    }
}