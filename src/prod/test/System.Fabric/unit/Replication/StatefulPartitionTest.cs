// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Fabric.Health;

    [TestClass]
    public class StatefulPartitionTest
    {
        private static readonly ServiceInitializationParameters DefaultInitializationParameters = new StatefulServiceInitializationParameters
        {
            ServiceTypeName = "foo",
            ServiceName = new Uri("http://foo"),
        };

        private const long DefaultReplicaId = 123;

        private static PinCollection pinCollection;

        private static readonly NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION DefaultPartitionInfo;

        static StatefulPartitionTest()
        {
            StatefulPartitionTest.pinCollection = new PinCollection();
            StatefulPartitionTest.pinCollection.AddBlittable(
                new NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION
                {
                    Id = Guid.NewGuid()
                }
            );

            StatefulPartitionTest.DefaultPartitionInfo = new NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION
            {
                Kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_SINGLETON,
                Value = StatefulPartitionTest.pinCollection.AddrOfPinnedObject()
            };
        }

        private static StatefulServiceReplicaBroker CreateDefaultBroker(ReplicatorSettings settings = null)
        {
            var serviceStub = new StatefulServiceStub();
            ((StatefulServiceStub)serviceStub).ReplicatorSettingsInternal = settings;

            return new StatefulServiceReplicaBroker(serviceStub, StatefulPartitionTest.DefaultInitializationParameters, StatefulPartitionTest.DefaultReplicaId);
        }

        #region Creation

        private static Tuple<StatefulNativePartitionStubForStatefulPartitionCreation, StatefulServicePartition, FabricReplicator> ReplicatorCreationHelper(ReplicatorSettings settings = null)
        {
            StatefulServiceReplicaBroker broker = StatefulPartitionTest.CreateDefaultBroker(settings);
            var nativePartition = new StatefulNativePartitionStubForStatefulPartitionCreation
            {
                NativePartitionInfo = StatefulPartitionTest.DefaultPartitionInfo,
                ReplicatorOut = new ReplicatorStubBase(),
                StateReplicatorOut = new StateReplicatorStubBase()
            };

            StatefulServicePartition partition = new StatefulServicePartition(nativePartition, ServicePartitionInformation.FromNative(nativePartition.GetPartitionInfo()));

            var fabricReplicator = partition.CreateReplicator((IStateProvider)broker.Service, settings);

            return Tuple.Create(nativePartition, partition, fabricReplicator);
        }


        private void ReplicatorSettingsTestHelper(
            ReplicatorSettings valueToPass,
            NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS flagsExpected,
            Action<NativeTypes.FABRIC_REPLICATOR_SETTINGS> validator)
        {
            var rv = StatefulPartitionTest.ReplicatorCreationHelper(valueToPass);
            NativeTypes.FABRIC_REPLICATOR_SETTINGS nativeObj = rv.Item1.ReplicatorSettings.Value;

            Assert.AreEqual<uint>((uint)flagsExpected, rv.Item1.ReplicatorSettings.Value.Flags);
            validator(nativeObj);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_ReplicatorSettings_NullResultsInNoObject()
        {
            var rv = StatefulPartitionTest.ReplicatorCreationHelper(null);
            Assert.IsFalse(rv.Item1.ReplicatorSettings.HasValue);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_ReplicatorSettings_RetryIntervalIsParsed()
        {
            const uint retryInterval = 231;
            this.ReplicatorSettingsTestHelper(
                new ReplicatorSettings { RetryInterval = TimeSpan.FromMilliseconds(retryInterval) },
                NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_RETRY_INTERVAL,
                (obj) => Assert.AreEqual<uint>(retryInterval, obj.RetryIntervalMilliseconds));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_ReplicatorSettings_AckIntervalIsParsed()
        {
            const uint ackInterval = 231;
            this.ReplicatorSettingsTestHelper(
                new ReplicatorSettings { BatchAcknowledgementInterval = TimeSpan.FromMilliseconds(ackInterval) },
                NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL,
                (obj) => Assert.AreEqual<uint>(ackInterval, obj.BatchAcknowledgementIntervalMilliseconds));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_ReplicatorSettings_Address()
        {
            const string addr = "moo";
            this.ReplicatorSettingsTestHelper(
                new ReplicatorSettings { ReplicatorAddress = addr },
                NativeTypes.FABRIC_REPLICATOR_SETTINGS_FLAGS.REPLICATOR_ADDRESS,
                (obj) => Assert.AreEqual(addr, NativeTypes.FromNativeString(obj.ReplicatorAddress)));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_Creation_ServicePartitionInfoIsCreated()
        {
            var rv = StatefulPartitionTest.ReplicatorCreationHelper();

            var expected = ServicePartitionInformation.FromNative(StatefulPartitionTest.DefaultPartitionInfo);
            var actual = rv.Item2.PartitionInfo;

            Assert.AreEqual<Guid>(expected.Id, actual.Id);
            Assert.AreEqual(expected.GetType(), actual.GetType());
            Assert.AreEqual<Guid>(expected.Id, (Guid)rv.Item2.PartitionInfo.Id);
        }

        #endregion

        class Instance<TNativePartition, TNativeReplicator, TNativeStateReplicator>
            where TNativePartition : NativeRuntime.IFabricStatefulServicePartition, new()
            where TNativeReplicator : NativeRuntime.IFabricReplicator, new()
            where TNativeStateReplicator : NativeRuntime.IFabricStateReplicator, NativeRuntime.IOperationDataFactory, new()
        {
            public TNativePartition NativePartition { get; private set; }
            public TNativeReplicator NativeReplicator { get; private set; }
            public TNativeStateReplicator NativeStateReplicator { get; private set; }
            public StatefulServiceReplicaBroker Broker { get; private set; }

            public StatefulServicePartition Partition { get; private set; }

            public Instance()
            {
                this.NativePartition = new TNativePartition();
                this.NativeReplicator = new TNativeReplicator();
                this.NativeStateReplicator = new TNativeStateReplicator();
                this.Broker = StatefulPartitionTest.CreateDefaultBroker();
                this.Partition = new StatefulServicePartition(
                    this.NativePartition,
                    ServicePartitionInformation.FromNative(StatefulPartitionTest.DefaultPartitionInfo));
            }

            public void CreateReplicator()
            {
                var nativePartition = this.NativePartition as StatefulNativePartitionStubForStatefulPartitionCreation;
                nativePartition.ReplicatorOut = this.NativeReplicator;
                nativePartition.StateReplicatorOut = this.NativeStateReplicator;
                this.Partition.CreateReplicator((IStateProvider)this.Broker.Service, null);
            }
        }

        #region Partition Access Status

        private void PartitionAccessStatusTestHelper(
            Func<IStatefulServicePartition, PartitionAccessStatus> getter,
            Action<StatefulNativePartitionSupportingStatus, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS> setter,
            PartitionAccessStatus expectedStatus,
            NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS statusToSet)
        {
            var inst = new Instance<StatefulNativePartitionSupportingStatus, ReplicatorStubBase, StateReplicatorStubBase>();

            // set the status on the stub
            setter(inst.NativePartition, statusToSet);

            // get the value reported by the partition
            var actualStatus = getter(inst.Partition);

            // compare
            Assert.AreEqual<PartitionAccessStatus>(expectedStatus, actualStatus);
        }

        private static readonly List<Tuple<PartitionAccessStatus, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS>> partitionAccessStatusMappings = new List<Tuple<PartitionAccessStatus, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS>>
            {
                Tuple.Create(PartitionAccessStatus.Granted, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED),
                Tuple.Create(PartitionAccessStatus.Invalid, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID),
                Tuple.Create(PartitionAccessStatus.NotPrimary, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY),
                Tuple.Create(PartitionAccessStatus.NoWriteQuorum, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM),
                Tuple.Create(PartitionAccessStatus.ReconfigurationPending, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS.FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING),
            };

        private void PartitionAccessStatusTestHelper(
            Func<IStatefulServicePartition, PartitionAccessStatus> getter,
            Action<StatefulNativePartitionSupportingStatus, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS> setter)
        {
            // assert to make sure that changes to the native runtime enum are reflected in the partition access status
            Assert.AreEqual(partitionAccessStatusMappings.Count, Enum.GetNames(typeof(NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS)).Length);

            foreach (var item in StatefulPartitionTest.partitionAccessStatusMappings)
            {
                this.PartitionAccessStatusTestHelper(getter, setter, item.Item1, item.Item2);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_ReadStatusIsReturnedCorrectly()
        {
            Func<IStatefulServicePartition, PartitionAccessStatus> getter = (p) => p.ReadStatus;
            Action<StatefulNativePartitionSupportingStatus, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS> setter = (p, s) => p.ReadStatusOut = s;

            this.PartitionAccessStatusTestHelper(getter, setter);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void StatefulPartition_WriteStatusIsReturnedCorrectly()
        {
            Func<IStatefulServicePartition, PartitionAccessStatus> getter = (p) => p.WriteStatus;
            Action<StatefulNativePartitionSupportingStatus, NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS> setter = (p, s) => p.WriteStatusOut = s;

            this.PartitionAccessStatusTestHelper(getter, setter);
        }

        #endregion

        class StatefulPartitionStubReturningReplicationOperations : IStatefulServicePartition
        {
            public class OperationStub : IOperation
            {

                public byte[] DataBuffer { get; set; }

                #region IOperation Members

                public void Acknowledge()
                {
                }

                public OperationData Data
                {
                    get { return new OperationData(this.DataBuffer); }
                }

                public OperationType OperationType
                {
                    get;
                    set;
                }

                public long SequenceNumber
                {
                    get;
                    set;
                }

                public long AtomicGroupId
                {
                    get;
                    set;
                }

                #endregion
            }

            public List<OperationStub> CopyOperations { get; private set; }
            public List<OperationStub> ReplicationOperations { get; private set; }

            private int copyOperationIndex = 0;
            private int replicationOperationIndex = 0;

            public StatefulPartitionStubReturningReplicationOperations()
            {
                this.CopyOperations = new List<OperationStub>();
                this.ReplicationOperations = new List<OperationStub>();
            }

            #region IStatefulPartition Members

            public Task BuildReplicaAsync(ReplicaInformation replica, object asyncState)
            {
                throw new NotImplementedException();
            }

            public long GetCatchUpCapability()
            {
                throw new NotImplementedException();
            }

            public long GetCurrentProgress()
            {
                throw new NotImplementedException();
            }

            public void ReportReplicaHealth(HealthInformation healthInfo)
            {
                throw new NotImplementedException();
            }

            public void ReportReplicaHealth(HealthInformation healthInfo, HealthReportSendOptions sendOptions)
            {
                throw new NotImplementedException();
            }

            public void ReportPartitionHealth(HealthInformation healthInfo)
            {
                throw new NotImplementedException();
            }

            public void ReportPartitionHealth(HealthInformation healthInfo, HealthReportSendOptions sendOptions)
            {
                throw new NotImplementedException();
            }

            public Task<IOperation> FetchCopyOperationAsync()
            {
                Assert.IsTrue(this.copyOperationIndex < this.CopyOperations.Count);
                var taskProducer = new TaskCompletionSource<IOperation>();

                ThreadPool.QueueUserWorkItem((o) => taskProducer.SetResult(this.CopyOperations[this.copyOperationIndex++]));

                return taskProducer.Task;
            }

            public Task<IOperation> FetchReplicationOperationAsync()
            {
                Assert.IsTrue(this.replicationOperationIndex < this.ReplicationOperations.Count);

                var taskProducer = new TaskCompletionSource<IOperation>();

                ThreadPool.QueueUserWorkItem((o) => taskProducer.SetResult(this.ReplicationOperations[this.replicationOperationIndex++]));

                return taskProducer.Task;
            }

            public ServicePartitionInformation PartitionInfo
            {
                get { throw new NotImplementedException(); }
            }

            public PartitionAccessStatus ReadStatus
            {
                get { throw new NotImplementedException(); }
            }

            public void RemoveReplica(long replicaId)
            {
                throw new NotImplementedException();
            }

            public Task ReplicateAsync(IEnumerable<ArraySegment<byte>> data, out long sequenceNumber)
            {
                throw new NotImplementedException();
            }

            public void ReportLoad(IEnumerable<LoadMetric> metrics)
            {
                throw new NotImplementedException();
            }

            public void ReportMoveCost(MoveCost moveCost)
            {
                throw new NotImplementedException();
            }

            public void UpdateEpoch(Epoch epoch)
            {
                throw new NotImplementedException();
            }

            public void UpdateCatchUpReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration, ReplicaSetConfiguration previousConfiguration)
            {
                throw new NotImplementedException();
            }

            public Task WaitForCatchUpQuorumAsync(ReplicaSetQuorumMode mode, object asyncState)
            {
                throw new NotImplementedException();
            }

            public void UpdateCurrentReplicaSetConfiguration(ReplicaSetConfiguration currentConfiguration)
            {
                throw new NotImplementedException();
            }

            public PartitionAccessStatus WriteStatus
            {
                get { throw new NotImplementedException(); }
            }

            #endregion

            #region IPartition Members

            public T GetPartitionKey<T>() where T : class
            {
                throw new NotImplementedException();
            }

            public ServicePartitionKind ServicePartitionKind
            {
                get { throw new NotImplementedException(); }
            }

            public void ReportFault(FaultType faultType)
            {
                throw new NotImplementedException();
            }

            #endregion

            public FabricReplicator CreateReplicator(IStateProvider stateProvider, ReplicatorSettings replicatorSettings)
            {
                throw new NotImplementedException();
            }
        }

        // #endregion

        class StateReplicatorStubReturningStream : StateReplicatorStubBase
        {
            public OperationStreamStubBase CopyStreamOut { get; set; }
            public OperationStreamStubBase ReplicationStreamOut { get; set; }

            public override NativeRuntime.IFabricOperationStream GetCopyStream()
            {
                return this.CopyStreamOut;
            }

            public override NativeRuntime.IFabricOperationStream GetReplicationStream()
            {
                return this.ReplicationStreamOut;
            }
        }

        class OperationStreamStubBase : NativeRuntime.IFabricOperationStream
        {
            public class OperationStub : NativeRuntime.IFabricOperation
            {
                public byte[] DataOut { get; set; }
                public NativeTypes.FABRIC_OPERATION_METADATA MetadataOut;

                #region IOperation Members

                public void Acknowledge()
                {
                }

                public IntPtr GetData(out uint length)
                {
                    length = 1;
                    GCHandle handle = GCHandle.Alloc(this.DataOut, GCHandleType.Pinned);

                    var operationDataBuffer = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER();

                    operationDataBuffer.BufferSize = (uint)this.DataOut.Length;
                    operationDataBuffer.Buffer = handle.AddrOfPinnedObject();

                    GCHandle operationHandle = GCHandle.Alloc(operationDataBuffer, GCHandleType.Pinned);

                    return operationHandle.AddrOfPinnedObject();
                }

                public IntPtr get_Metadata()
                {
                    return this.GetMetadata();
                }

                private unsafe IntPtr GetMetadata()
                {
                    fixed (NativeTypes.FABRIC_OPERATION_METADATA* p = &this.MetadataOut)
                    {
                        return (IntPtr)(void*)p;
                    }
                }

                #endregion
            }

            // private int index = 0;
            public List<OperationStub> Operations { get; private set; }

            public OperationStreamStubBase()
            {
                this.Operations = new List<OperationStub>();
            }

            #region IOperationStream Members

            public NativeCommon.IFabricAsyncOperationContext BeginGetOperation(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                throw new NotImplementedException();

                //var asyncAdapter = new AsyncCallInAdapter(callback);
                //var asyncResult = new AsyncResult<NativeRuntime.IFabricOperation>(asyncAdapter.InnerCallback, asyncAdapter.InnerState);
                //asyncAdapter.InnerResult = asyncResult;

                //ThreadPool.QueueUserWorkItem((o) => asyncResult.Complete(this.Operations[this.index++]));
                //return asyncAdapter;                
            }

            public NativeRuntime.IFabricOperation EndGetOperation(NativeCommon.IFabricAsyncOperationContext context)
            {
                throw new NotImplementedException();
                //var asyncResult = (AsyncResult<NativeRuntime.IFabricOperation>)((AsyncCallInAdapter)context).InnerResult;
                //AsyncResult<NativeRuntime.IFabricOperation>.End(asyncResult);
                //return asyncResult.Result;
            }

            #endregion
        }

        class StatefulNativePartitionSupportingStatus : Stubs.StatefulPartitionStubBase
        {
            public NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS ReadStatusOut { get; set; }
            public NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS WriteStatusOut { get; set; }

            public override NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus()
            {
                return this.ReadStatusOut;
            }

            public override NativeTypes.FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus()
            {
                return this.WriteStatusOut;
            }
        }

        class ReplicatorSupportingProgressAndCatchupCapability : ReplicatorStubBase
        {
            public long CatchupCapabilitySequenceNumberOut { get; set; }
            public long CurrentProgressSequenceNumberOut { get; set; }

            public override void GetCatchUpCapability(out long fromSequenceNumber)
            {
                fromSequenceNumber = this.CatchupCapabilitySequenceNumberOut;
            }

            public override void GetCurrentProgress(out long lastSequenceNumber)
            {
                lastSequenceNumber = this.CurrentProgressSequenceNumberOut;
            }
        }

        class StateReplicatorStubBase : Stubs.StateReplicatorStubBase, NativeRuntime.IOperationDataFactory
        {
            #region IOperationDataFactory Members

            public virtual NativeRuntime.IFabricOperationData CreateOperationData(IntPtr segmentSizes, UInt32 segmentSizesCount)
            {
                throw new NotImplementedException();
            }

            #endregion
        }

        class ReplicatorStubBase : NativeRuntime.IFabricReplicator
        {

            #region IReplicator Members

            public virtual NativeCommon.IFabricAsyncOperationContext BeginChangeRole(IntPtr epoch, NativeTypes.FABRIC_REPLICA_ROLE role, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                throw new NotImplementedException();
            }

            public virtual NativeCommon.IFabricAsyncOperationContext BeginClose(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                throw new NotImplementedException();
            }

            public virtual NativeCommon.IFabricAsyncOperationContext BeginOpen(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                throw new NotImplementedException();
            }

            public virtual void EndChangeRole(NativeCommon.IFabricAsyncOperationContext context)
            {
                throw new NotImplementedException();
            }

            public virtual void EndClose(NativeCommon.IFabricAsyncOperationContext context)
            {
                throw new NotImplementedException();
            }

            public virtual NativeCommon.IFabricStringResult EndOpen(NativeCommon.IFabricAsyncOperationContext context)
            {
                throw new NotImplementedException();
            }

            public virtual void Abort()
            {
                throw new NotImplementedException();
            }

            public virtual void GetCatchUpCapability(out long fromSequenceNumber)
            {
                throw new NotImplementedException();
            }

            public virtual void GetCurrentProgress(out long lastSequenceNumber)
            {
                throw new NotImplementedException();
            }

            #endregion


            public NativeCommon.IFabricAsyncOperationContext BeginUpdateEpoch(IntPtr epoch, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                throw new NotImplementedException();
            }

            public void EndUpdateEpoch(NativeCommon.IFabricAsyncOperationContext context)
            {
                throw new NotImplementedException();
            }
        }

        class StatefulNativePartitionStubSupportingReporting : Stubs.StatefulPartitionStubBase
        {
            public StatefulNativePartitionStubSupportingReporting()
            {
                this.FaultType = new List<NativeTypes.FABRIC_FAULT_TYPE>();
            }

            public IList<NativeTypes.FABRIC_FAULT_TYPE> FaultType { get; private set; }

            public override void ReportFault(NativeTypes.FABRIC_FAULT_TYPE faultType)
            {
                this.FaultType.Add(faultType);
            }
        }

        class StatefulNativePartitionStubForStatefulPartitionCreation : Stubs.StatefulPartitionStubBase
        {
            public NativeTypes.FABRIC_REPLICATOR_SETTINGS? ReplicatorSettings { get; set; }
            public NativeRuntime.IFabricStateReplicator StateReplicatorOut { get; set; }
            public NativeRuntime.IFabricReplicator ReplicatorOut { get; set; }
            public NativeRuntime.IFabricStateProvider StateProvider { get; set; }

            public NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION NativePartitionInfo
            {
                get;
                set;
            }

            public override IntPtr GetPartitionInfo()
            {
                GCHandle handle = GCHandle.Alloc(this.NativePartitionInfo, GCHandleType.Pinned);
                return handle.AddrOfPinnedObject();
            }

            public override NativeRuntime.IFabricStateReplicator CreateReplicator(NativeRuntime.IFabricStateProvider stateProvider, IntPtr fabricReplicatorSettings, out NativeRuntime.IFabricReplicator replicator)
            {
                this.StateProvider = stateProvider;

                unsafe
                {
                    if (fabricReplicatorSettings == IntPtr.Zero)
                    {
                        this.ReplicatorSettings = null;
                    }
                    else
                    {
                        this.ReplicatorSettings = *(NativeTypes.FABRIC_REPLICATOR_SETTINGS*)fabricReplicatorSettings.ToPointer();
                    }
                }

                replicator = this.ReplicatorOut;
                return this.StateReplicatorOut;
            }
        }

        class StatefulServiceStub : Stubs.StatefulServiceReplicaStubBase
        {
            public override ReplicatorSettings ReplicatorSettings
            {
                get
                {
                    return this.ReplicatorSettingsInternal;
                }
            }

            public ReplicatorSettings ReplicatorSettingsInternal = null;
        }
    }
}