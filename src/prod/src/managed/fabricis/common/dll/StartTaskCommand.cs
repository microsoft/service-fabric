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

    internal class StartTaskCommand : InfrastructureServiceCommand
    {
        private static readonly TraceType TraceType = new TraceType("StartTaskCommand");

        private readonly InfrastructureTaskDescription taskDescription;

        public StartTaskCommand(InfrastructureTaskDescription description)
        {
            this.taskDescription = description;
        }

        public InfrastructureTaskDescription TaskDescription
        {
            get { return this.taskDescription; }
        }

        public override string TaskId 
        { 
            get { return this.TaskDescription.TaskId; } 
        }

        public override long InstanceId
        {
            get { return this.TaskDescription.InstanceId; }
        }

        public static StartTaskCommand TryParseCommand(Guid partitionId, Queue<string> tokens)
        {
            if (tokens.Count <= 0) 
            {
                Trace.WriteWarning(TraceType, "Command missing Task Id");
                return null; 
            }

            string taskId = tokens.Dequeue();

            if (tokens.Count <= 0) 
            {
                Trace.WriteWarning(TraceType, "Command missing Task Timeout");
                return null; 
            }

            var nodeTasks = new List<NodeTaskDescription>();

            while (tokens.Count > 0)
            {
                var nodeTask = new NodeTaskDescription { NodeName = tokens.Dequeue() };

                if (tokens.Count <= 0) 
                {
                    Trace.WriteWarning(TraceType, "Command missing Node Task Type");
                    return null; 
                }

                string command = tokens.Dequeue();

                if (command == InfrastructureServiceCommand.RestartCommand)
                {
                    nodeTask.TaskType = NodeTask.Restart;
                }
                else if (command == InfrastructureServiceCommand.RelocateCommand)
                {
                    nodeTask.TaskType = NodeTask.Relocate;
                }
                else if (command == InfrastructureServiceCommand.RemoveCommand)
                {
                    nodeTask.TaskType = NodeTask.Remove;
                }
                else
                {
                    Trace.WriteWarning(TraceType, "Invalid Node Task Type '{0}'", command);
                    return null;
                }

                nodeTasks.Add(nodeTask);
            }

            var description = new InfrastructureTaskDescription(
                partitionId,
                taskId,
                DateTime.UtcNow.Ticks,
                new ReadOnlyCollection<NodeTaskDescription>(nodeTasks));

            return new StartTaskCommand(description);
        }
    }
}