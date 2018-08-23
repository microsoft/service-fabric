// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.ObjectModel;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    internal enum InfrastructureTaskState
    {
        Invalid = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_INVALID,
        PreProcessing = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_PROCESSING,
        PreAckPending = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACK_PENDING,
        PreAcked = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACKED,
        PostProcessing = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_POST_PROCESSING,
        PostAckPending = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACK_PENDING,
        PostAcked = NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACKED
    }

    internal sealed class InfrastructureTaskResultItem
    {
        public InfrastructureTaskDescription Description { get; internal set; }

        public InfrastructureTaskState State { get; internal set; }

        internal static unsafe Collection<InfrastructureTaskResultItem> FromNativeQueryList(IntPtr nativePtr)
        {
            var castedPtr = (NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_LIST*)nativePtr;

            uint count = castedPtr->Count;
            var arrayPtr = (NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM*)castedPtr->Items;

            var result = new Collection<InfrastructureTaskResultItem>();
            for (uint ix = 0; ix < count; ++ix)
            {
                var item = FromNative(*(arrayPtr + ix));
                result.Add(item);
            }

            return result;
        }

        internal static InfrastructureTaskResultItem FromNative(NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_QUERY_RESULT_ITEM casted)
        {
            var managed = new InfrastructureTaskResultItem();
            managed.Description = InfrastructureTaskDescription.FromNative(casted.Description);
            managed.State = FromNativeTaskState(casted.State);

            return managed;
        }

        private static InfrastructureTaskState FromNativeTaskState(NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE native)
        {
            switch (native)
            {
                case NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_PROCESSING:
                case NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACK_PENDING:
                case NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_PRE_ACKED:
                case NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_POST_PROCESSING:
                case NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACK_PENDING:
                case NativeInfrastructureService.FABRIC_INFRASTRUCTURE_TASK_STATE.FABRIC_INFRASTRUCTURE_TASK_STATE_POST_ACKED:
                    return (InfrastructureTaskState)native;

                default:
                    return InfrastructureTaskState.Invalid;
            }
        }
    }
}