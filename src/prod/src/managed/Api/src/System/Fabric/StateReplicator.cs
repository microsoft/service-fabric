// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    internal class StateReplicator : IStateReplicator2
    {
        private readonly OperationDataFactoryWrapper operationDataFactory;
        private readonly NativeRuntime.IFabricStateReplicator2 nativeStateReplicator;
        private readonly NativeRuntime.IFabricInternalManagedReplicator replicatorUsingOperationEx1; 
       
        public StateReplicator(NativeRuntime.IFabricStateReplicator nativeStateReplicator, NativeRuntime.IOperationDataFactory nativeOperationDataFactory)
        {
            Requires.Argument("nativeStateReplicator", nativeStateReplicator).NotNull();
            Requires.Argument("nativeOperationDataFactory", nativeOperationDataFactory).NotNull();
            
            // This is either the V1 replicator or the service groups atomic group replicator 
            this.nativeStateReplicator = nativeStateReplicator as NativeRuntime.IFabricStateReplicator2;
            this.replicatorUsingOperationEx1 = nativeStateReplicator as NativeRuntime.IFabricInternalManagedReplicator;

            this.operationDataFactory = new OperationDataFactoryWrapper(nativeOperationDataFactory);
        }

        protected OperationDataFactoryWrapper OperationDataFactory
        {
            get
            {
                return this.operationDataFactory;
            }
        }

        public IOperationStream GetCopyStream()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetOperationStream(false), "StateReplicator.CopyStream");            
        }

        public IOperationStream GetReplicationStream()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetOperationStream(true), "StateReplicator.ReplicationStream");            
        }

        public Task<long> ReplicateAsync(OperationData operationData, CancellationToken cancellationToken, out long sequenceNumber)
        {
            Requires.Argument("operationData", operationData).NotNull();
            long sequenceNumberOut = 0;
            var returnValue = Utility.WrapNativeAsyncInvoke<long>(
                (callback) =>
                {
                    NativeCommon.IFabricAsyncOperationContext context;
                    sequenceNumberOut = this.ReplicateBeginWrapper(operationData, callback, out context);
                    return context;
                },
                this.ReplicateEndWrapper,
                InteropExceptionTracePolicy.None,
                cancellationToken,
                "StateReplicator.ReplicateAsync");

            // assign out variable
            sequenceNumber = sequenceNumberOut;
            return returnValue;
        }

        public void UpdateReplicatorSettings(ReplicatorSettings settings)
        {
            Utility.WrapNativeSyncInvoke(() => this.UpdateReplicatorSettingsHelper(settings), "FabricReplicator.UpdateReplicatorSettings");
        }

        public ReplicatorSettings GetReplicatorSettings()
        {
            return Utility.WrapNativeSyncInvoke(() => this.GetReplicatorSettingsHelper(), "FabricReplicator.GetReplicatorSettingsHelper");
        }

        private void UpdateReplicatorSettingsHelper(ReplicatorSettings replicatorSettings)
        {
            // Initialize replicator
            if (replicatorSettings == null)
            {
                AppTrace.TraceSource.WriteNoise("FabricReplicator.UpdateReplicatorSettings", "Using default replicator settings");
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("FabricReplicator.UpdateReplicatorSettings", "Replicator Settings - address {0}, listenAddress {1}, publishAddress {2}, retyrInterval {3}, ackInterval {4}, credentials provided {5}", replicatorSettings.ReplicatorAddress, replicatorSettings.ReplicatorListenAddress, replicatorSettings.ReplicatorPublishAddress, replicatorSettings.RetryInterval, replicatorSettings.BatchAcknowledgementInterval, replicatorSettings.SecurityCredentials != null);
            }

            using (var pin = new PinCollection())
            {
                unsafe
                {
                    if (replicatorSettings == null)
                    {
                        this.nativeStateReplicator.UpdateReplicatorSettings(IntPtr.Zero);
                    }
                    else
                    {
                        IntPtr nativeReplicatorSettings = replicatorSettings.ToNative(pin);
                        this.nativeStateReplicator.UpdateReplicatorSettings(nativeReplicatorSettings);
                    }
                }
            }
        }

        private ReplicatorSettings GetReplicatorSettingsHelper()
        {
            AppTrace.TraceSource.WriteNoise("FabricReplicator.GetReplicatorSettings", "Getting Replicator Settings from native replicator");
            using (var pin = new PinCollection())
            {
                NativeRuntime.IFabricReplicatorSettingsResult replicatorSettingsResult = 
                    nativeStateReplicator.GetReplicatorSettings();

                return ReplicatorSettings.CreateFromNative(replicatorSettingsResult);
            }
        }

        private OperationStream GetOperationStream(bool getReplicationStream)
        {
            if (getReplicationStream)
            {
                return new OperationStream(this.nativeStateReplicator.GetReplicationStream());
            }
            else
            {
                return new OperationStream(this.nativeStateReplicator.GetCopyStream());
            }
        }

        private long ReplicateBeginWrapper(OperationData operationData, NativeCommon.IFabricAsyncOperationCallback callback, out NativeCommon.IFabricAsyncOperationContext context)
        {
            long sequenceNumber = 0;
            
            using (var pin = new PinCollection())
            {
                var nativeOperationBuffer = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER_EX1[operationData.Count];

                var buffersAddress = pin.AddBlittable(nativeOperationBuffer);

                for (int i = 0; i < operationData.Count; i++)
                {
                    nativeOperationBuffer[i].Offset = (uint) operationData[i].Offset;
                    nativeOperationBuffer[i].Count = (uint) operationData[i].Count;
                    nativeOperationBuffer[i].Buffer = pin.AddBlittable(operationData[i].Array);
                }

                context = this.replicatorUsingOperationEx1.BeginReplicate2(
                    operationData.Count,
                    buffersAddress,
                    callback,
                    out sequenceNumber);
            }
        
            return sequenceNumber;
        }

        private long ReplicateEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
        {
            return this.nativeStateReplicator.EndReplicate(context);
        }
    }
}