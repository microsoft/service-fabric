// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Replicator
{
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    internal class StatefulServicePartitionEx : StatefulServicePartition, IStatefulServicePartitionEx
    {
        private readonly NativeRuntime.IFabricStatefulServicePartitionEx nativePartition;
        
        // internal constructor - used by test code
        internal StatefulServicePartitionEx(
            NativeRuntime.IFabricStatefulServicePartitionEx nativeStatefulPartition,
            ServicePartitionInformation partitionInfo)
            : base(nativeStatefulPartition, partitionInfo)
        {
            Requires.Argument("nativeStatefulPartition", nativeStatefulPartition).NotNull();
            
            this.nativePartition = nativeStatefulPartition;
        }

        public bool IsPersisted
        {
            get
            {
                return Utility.WrapNativeSyncInvoke<bool>(
                    () => NativeTypes.FromBOOLEAN(this.nativePartition.IsPersisted()),
                    "StatefulServicePartitionEx.IsPersisted");
            }
        }

        public FabricReplicatorEx CreateReplicatorEx(IAtomicGroupStateProviderEx stateProvider, ReplicatorSettings replicatorSettings, ReplicatorLogSettings logSettings)
        {
            return Utility.WrapNativeSyncInvoke(
                () => this.CreateFabricReplicatorHelperEx(stateProvider, replicatorSettings), 
                "StatefulServicePartitionEx.CreateFabricReplicator");
        }

        private FabricReplicatorEx CreateFabricReplicatorHelperEx(IAtomicGroupStateProviderEx stateProvider, ReplicatorSettings replicatorSettings)
        {
            Requires.Argument<IAtomicGroupStateProviderEx>("stateProvider", stateProvider).NotNull();

            // Initialize replicator
            if (replicatorSettings == null)
            {
                AppTrace.TraceSource.WriteNoise("StatefulServicePartitionEx.CreateFabricReplicatorEx", "Using default replicator settings");
            }
            else
            {
                AppTrace.TraceSource.WriteNoise("StatefulServicePartitionEx.CreateFabricReplicatorEx", "{0}", replicatorSettings);
            }

            NativeRuntime.IFabricReplicator nativeReplicator;
            NativeRuntime.IFabricAtomicGroupStateReplicatorEx nativeStateReplicator;

            AppTrace.TraceSource.WriteNoise("StatefulServicePartitionEx.CreateFabricReplicatorEx", "Creating state provider borker");
            AtomicGroupStateProviderExBroker stateProviderBroker = new AtomicGroupStateProviderExBroker(stateProvider);

            using (var pin = new PinCollection())
            {
                unsafe
                {
                    IntPtr replicatorSettingsNative = IntPtr.Zero;
                    IntPtr logSettingsNative = IntPtr.Zero;

                    if (replicatorSettings != null)
                    {
                        replicatorSettingsNative = replicatorSettings.ToNative(pin);
                    }

                    nativeStateReplicator = this.nativePartition.CreateReplicatorEx(
                        stateProviderBroker, 
                        replicatorSettingsNative, 
                        logSettingsNative, 
                        out nativeReplicator);
                }
            }

            NativeRuntime.IOperationDataFactory nativeOperationDataFactory = (NativeRuntime.IOperationDataFactory)nativeReplicator;
            if (nativeStateReplicator == null || nativeReplicator == null || nativeOperationDataFactory == null)
            {
                AppTrace.TraceSource.WriteError("StatefulServicePartitionEx.Create", "Native replicators could not be initialized.");
                throw new InvalidOperationException(StringResources.Error_StatefulServicePartition_Native_Replicator_Init_Failed);
            }

            stateProviderBroker.OperationDataFactory = new OperationDataFactoryWrapper(nativeOperationDataFactory);

            return new FabricReplicatorEx(nativeReplicator, nativeStateReplicator, nativeOperationDataFactory);
        }
    }
}