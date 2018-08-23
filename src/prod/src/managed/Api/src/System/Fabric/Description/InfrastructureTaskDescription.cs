// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Interop;

    internal sealed class InfrastructureTaskDescription
    {
        public InfrastructureTaskDescription(
            string taskId,
            long instanceId,
            ReadOnlyCollection<NodeTaskDescription> nodeTasks)
            : this(Guid.Empty, taskId, instanceId, nodeTasks)
        {
        }

        public InfrastructureTaskDescription(
            Guid sourcePartitionId,
            string taskId,
            long instanceId,
            ReadOnlyCollection<NodeTaskDescription> nodeTasks)
        {
            this.SourcePartitionId = sourcePartitionId;
            this.TaskId = taskId;
            this.InstanceId = instanceId;

            // Avoiding null initialization as native serializer does not handle null sometimes.
            this.NodeTasks = nodeTasks ?? new List<NodeTaskDescription>().AsReadOnly();
        }

        public Guid SourcePartitionId  { get; private set; }

        public string TaskId { get; private set; }

        public long InstanceId { get; private set; }

        public ReadOnlyCollection<NodeTaskDescription> NodeTasks { get; private set; }

        private InfrastructureTaskDescription()
        {
            this.NodeTasks = new List<NodeTaskDescription>().AsReadOnly();
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION();

            nativeDescription.SourcePartitionId = this.SourcePartitionId;

            if (this.TaskId != null)
            {
                nativeDescription.TaskId = pinCollection.AddObject(this.TaskId);
            }
            else
            {
                nativeDescription.TaskId = IntPtr.Zero;
            }

            nativeDescription.InstanceId = this.InstanceId;

            nativeDescription.NodeTasks = ToNativeNodeTasks(pinCollection, this.NodeTasks);

            return pinCollection.AddBlittable(nativeDescription);
        }

        internal static unsafe InfrastructureTaskDescription FromNative(IntPtr nativePtr)
        {
            var casted = (NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION*)nativePtr;

            var managed = new InfrastructureTaskDescription();
            managed.SourcePartitionId = casted->SourcePartitionId;
            managed.TaskId = NativeTypes.FromNativeString(casted->TaskId);
            managed.InstanceId = casted->InstanceId;
            managed.NodeTasks = FromNativeNodeTasks(casted->NodeTasks);

            return managed;
        }

        private static IntPtr ToNativeNodeTasks(PinCollection pinCollection, ReadOnlyCollection<NodeTaskDescription> nodeTasks)
        {
            var nativeArray = new NativeInfrastructureService.FABRIC_NODE_TASK_DESCRIPTION[nodeTasks.Count];

            for (int ix = 0; ix < nodeTasks.Count; ++ix)
            {
                nativeArray[ix].NodeName = pinCollection.AddObject(nodeTasks[ix].NodeName);
                nativeArray[ix].TaskType = ToNativeTaskType(nodeTasks[ix].TaskType);
            }

            var nativeList = new NativeInfrastructureService.FABRIC_NODE_TASK_DESCRIPTION_LIST();
            nativeList.Count = (uint)nodeTasks.Count;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }

        private static unsafe ReadOnlyCollection<NodeTaskDescription> FromNativeNodeTasks(IntPtr nativePtr)
        {
            var castedPtr = (NativeInfrastructureService.FABRIC_NODE_TASK_DESCRIPTION_LIST*)nativePtr;

            var count = castedPtr->Count;
            var result = new List<NodeTaskDescription>();

            var arrayPtr = (NativeInfrastructureService.FABRIC_NODE_TASK_DESCRIPTION*)castedPtr->Items;
            for (int ix = 0; ix < count; ++ix)
            {
                NativeInfrastructureService.FABRIC_NODE_TASK_DESCRIPTION item = *(arrayPtr + ix);

                var nodeTask = new NodeTaskDescription();
                nodeTask.NodeName = NativeTypes.FromNativeString(item.NodeName);
                nodeTask.TaskType = FromNativeTaskType(item.TaskType);
                    
                result.Add(nodeTask);
            }

            return new ReadOnlyCollection<NodeTaskDescription>(result);
        }

        private static NativeInfrastructureService.FABRIC_NODE_TASK_TYPE ToNativeTaskType(NodeTask taskType)
        {
            switch (taskType)
            {
                case NodeTask.Restart:
                case NodeTask.Relocate:
                case NodeTask.Remove:
                    return (NativeInfrastructureService.FABRIC_NODE_TASK_TYPE)taskType;

                default:
                    return NativeInfrastructureService.FABRIC_NODE_TASK_TYPE.FABRIC_NODE_TASK_TYPE_INVALID;
            }
        }

        private static NodeTask FromNativeTaskType(NativeInfrastructureService.FABRIC_NODE_TASK_TYPE taskType)
        {
            switch (taskType)
            {
                case NativeInfrastructureService.FABRIC_NODE_TASK_TYPE.FABRIC_NODE_TASK_TYPE_RESTART:
                case NativeInfrastructureService.FABRIC_NODE_TASK_TYPE.FABRIC_NODE_TASK_TYPE_RELOCATE:
                case NativeInfrastructureService.FABRIC_NODE_TASK_TYPE.FABRIC_NODE_TASK_TYPE_REMOVE:
                    return (NodeTask)taskType;

                default:
                    return NodeTask.Invalid;
            }
        }
    }
}