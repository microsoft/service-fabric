// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Fabric.Test;
    using System.Linq;
    using System.Fabric.ReliableMessaging.Session;
    using System.Threading;
    using System.Threading.Tasks;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;

    public partial class ReliableSessionTest
    {
        /// <summary>
        /// Create service instances.
        /// </summary>
        /// <param name="instances">Create and initialize the service instances</param>
        /// <param name="isRandomNumberOfPartitions">True if random number of Partitions (less than or equal to max number of partitions) per instance, false initialize max number of partitions</param>
        /// <param name="numOfPartitionsPerInstance">Number of partitions per instance</param>
        static void InitializeInstances(InstanceRecord[] instances, bool isRandomNumberOfPartitions, int numOfPartitionsPerInstance)
        {
            for (int instanceIndex = 0; instanceIndex < instances.Count(); instanceIndex++)
            {
                Uri instanceName = new Uri("fabric:/test/svcInstance/" + instanceIndex);

                // Initialize partitions
                int partitionCount = numOfPartitionsPerInstance;
                if (isRandomNumberOfPartitions)
                {
                    partitionCount = RandGen.Next(1, numOfPartitionsPerInstance);
                }

                instances[instanceIndex] = new InstanceRecord(instanceName, partitionCount);
            }
        }

        /// <summary>
        /// Create and Initialize partitions for the given service instance .
        /// </summary>
        /// <param name="instance">Initialize partitions for this service instance</param>
        /// <param name="isRandomPartitionType">true - if the partition type is selected randomly - String or Int64</param>
        /// <param name="type">Set the partition type to this parameter, if isRandomPartitionType is false. 
        /// Singleton type is not allowed as we will set this type if instance partitions count = 1</param>
        /// <param name="int64PartitionKeyLowValMultiplier"></param>
        static void InitializePartitions(InstanceRecord instance, bool isRandomPartitionType, PartitionKeyType type,int int64PartitionKeyLowValMultiplier)
        {
            // Singletone is not a valid type
            VS.Assert.IsTrue(type != PartitionKeyType.Singleton, "unexpected partition key type {0} in StartSessionManagersForServiceInstance", type);
            
            // Ensure we have a valid partitions count > 0
            Uri instanceName = instance.InstanceName;
            int partitionCount = instance.CountOfPartitions;
            VS.Assert.IsTrue(partitionCount > 0, "unexpected partitionCount {0} in StartSessionManagersForServiceInstance", partitionCount);
            
            // singleton case
            if (partitionCount == 1)
            {
                PartitionRecord newPartition = new PartitionRecord(
                                                                instanceName, 
                                                                PartitionKeyType.Singleton, 
                                                                null, 
                                                                new IntegerPartitionKeyRange());
                instance.Partitions[0] = newPartition;
                return;
            }
             
            bool stringPartitions = false;
            // randomly allocate string or int64 partition type.
            if (isRandomPartitionType)
            {
                stringPartitions = (RandGen.Next()%2) == 0; // decide if we have string or int64 partitions
            }
            else
            {
                if (type == PartitionKeyType.StringKey)
                {
                    stringPartitions = true;
                }
            }

            for (int i = 0; i < partitionCount; i++)
            {
                PartitionRecord newPartition;

                if (stringPartitions)
                {
                    // string partition key
                    string partitionKey = i.ToString(CultureInfo.InvariantCulture);
                    newPartition = new PartitionRecord(
                        instanceName, 
                        PartitionKeyType.StringKey, 
                        partitionKey, 
                        new IntegerPartitionKeyRange());

                    instance.Partitions[i] = newPartition;
                    LogHelper.Log("Created; Instance: {0} StringPartition: {1}", instanceName, partitionKey);
                }
                // numerical partition key -- single number range
                IntegerPartitionKeyRange partitionInt64Key = new IntegerPartitionKeyRange
                {
                    IntegerKeyLow = i*int64PartitionKeyLowValMultiplier,
                    IntegerKeyHigh = i*int64PartitionKeyLowValMultiplier + (RandGen.Next() % (int64PartitionKeyLowValMultiplier - 1))
                };
                        
                newPartition = new PartitionRecord(
                    instanceName, 
                    PartitionKeyType.Int64Key, 
                    null, 
                    partitionInt64Key);

                instance.Partitions[i] = newPartition;
                LogHelper.Log("Created; Instance: {0} Int64Partition: {1}-{2}", instanceName, partitionInt64Key.IntegerKeyLow, partitionInt64Key.IntegerKeyHigh);
           }
        }

        /// <summary>
        /// Each partition is assigned a new session manager and
        /// registered for the inbound session callback.
        /// </summary>
        /// <param name="instanceName"></param>
        /// <param name="partition"></param>
        void InitializeReliableSessions(Uri instanceName, PartitionRecord partition )
        {
            IReliableSessionManager sessionManager;
            switch (partition.PartitionType)
            {
                case PartitionKeyType.Singleton:
                    sessionManager = new ReliableSessionManager(Guid.NewGuid(), 0, instanceName);
                    break;
                case PartitionKeyType.StringKey:
                    sessionManager = new ReliableSessionManager(Guid.NewGuid(), 0, instanceName, partition.StringPartitionPartitionKey);
                    break;
                case PartitionKeyType.Int64Key:
                    sessionManager = new ReliableSessionManager(Guid.NewGuid(), 0, instanceName, partition.Int64PartitionPartitionKey);
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
           
            // Open session, register callbacks
            sessionManager.OpenAsync(new SessionAbortedCallBack(), new CancellationToken()).Wait();
            Task<string> endpointResultTask = sessionManager.RegisterInboundSessionCallbackAsync(
                                                                                new InboundSessionCallback(partition), 
                                                                                new CancellationToken());
            partition.SessionManager = sessionManager;
            endpointResultTask.Wait();
            partition.Endpoint = endpointResultTask.Result;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="suffix"></param>
        /// <returns></returns>
        public static string GetEndpoint(string suffix)
        {
            Uri dummySvcInstance = new Uri("fabric:/test/dummy/svcInstance/" + suffix);
            ReliableSessionManager dummySessionManager = new ReliableSessionManager(Guid.NewGuid(), 0, dummySvcInstance);

            LogHelper.Log("open dummy reliable session manager to register to discover endpoint");
            dummySessionManager.OpenAsync(new SessionAbortedCallBack(), new CancellationToken()).Wait();

            // not a real test -- don't run this!
            // SessionTest dummyTest = new SessionTest(0, 0, 0, 0);
            PartitionRecord dummyRecord = new PartitionRecord(dummySvcInstance, PartitionKeyType.Singleton, null, new IntegerPartitionKeyRange());

            Task<string> dummyEndpointResultTask = dummySessionManager.RegisterInboundSessionCallbackAsync(
                                                                                new InboundSessionCallback(dummyRecord), new CancellationToken());

            dummyEndpointResultTask.Wait();

            return dummyEndpointResultTask.Result;
        }

        public static PartitionRecord GetRandomPartitionRecord(InstanceRecord[] instances)
        {
            InstanceRecord randomInstanceRecord = instances[RandGen.Next(instances.Count())];
            return randomInstanceRecord.Partitions[RandGen.Next(randomInstanceRecord.CountOfPartitions)];
        }
        
        /// <summary>
        /// Setup a session from source partition to target partition.
        /// </summary>
        /// <param name="sessionTest"></param>
        /// <param name="sourcePartition"></param>
        /// <param name="targetPartition"></param>
        private void SetupSessionTest(SessionTest sessionTest, PartitionRecord sourcePartition, PartitionRecord targetPartition)
        {
            VS.Assert.IsFalse(sourcePartition == targetPartition,
                "sourcePartition {0} equals targetPartition {1} in SetupSessionTests", sourcePartition, targetPartition);

            IReliableSessionManager sourceSessionManager = sourcePartition.SessionManager;
            Uri targetSvcInstance = targetPartition.InstanceName;
            string targetEndpoint = targetPartition.Endpoint;

            Task<IReliableMessagingSession> newOutboundSessionTask = null;
            bool testSkipped = false;

            try
            {
                // Setup the outbound session from source to target partition
                switch (targetPartition.PartitionType)
                {
                    case PartitionKeyType.Singleton:
                        newOutboundSessionTask = sourceSessionManager.CreateOutboundSessionAsync(targetSvcInstance,
                            targetEndpoint, new CancellationToken());
                        break;
                    case PartitionKeyType.StringKey:
                        string targetStringKey = targetPartition.StringPartitionPartitionKey;
                        newOutboundSessionTask = sourceSessionManager.CreateOutboundSessionAsync(targetSvcInstance,
                            targetStringKey, targetEndpoint, new CancellationToken());
                        break;
                    case PartitionKeyType.Int64Key:
                        IntegerPartitionKeyRange targetInt64Key = targetPartition.Int64PartitionPartitionKey;
                        long rangeSize = targetInt64Key.IntegerKeyHigh - targetInt64Key.IntegerKeyLow + 1;
                        long targetPartitionNumber = targetInt64Key.IntegerKeyLow + (RandGen.Next()%rangeSize);
                        newOutboundSessionTask = sourceSessionManager.CreateOutboundSessionAsync(targetSvcInstance,
                            targetPartitionNumber, targetEndpoint, new CancellationToken());
                        break;
                }

                VS.Assert.IsNotNull(newOutboundSessionTask, "Null newOutboundSessionTask in SetupSessionTests");

                newOutboundSessionTask.Wait();
                sessionTest.OutboundSession = newOutboundSessionTask.Result;
            }
            catch (System.AggregateException e)
            {
                Exception inner = e.InnerException.InnerException;
                VS.Assert.AreEqual(inner.GetType(), typeof (System.Runtime.InteropServices.COMException),
                    "Unexpected AggregateException {0} from CreateOutboundSessionAsync", inner.GetType().ToString());

                // Due to random nature of source and target selection, it might be possible we create a duplicate selection.
                // in this case, we skip the test.
                System.Runtime.InteropServices.COMException realException =
                    (System.Runtime.InteropServices.COMException) inner;
                uint hresult = (uint) realException.ErrorCode;
                uint expectedError = (uint) NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_RELIABLE_SESSION_ALREADY_EXISTS;
                VS.Assert.AreEqual(expectedError, hresult,
                    "Unexpected error HRESULT code {0} from CreateOutboundSessionAsync", hresult);

                testSkipped = true;
                LogHelper.Log("Test# skipped due to {0} exception ", hresult);
            }

            if (testSkipped)
            {
                sessionTest.SkipTest = true;
            }
            else
            {
                // initialize snapshot test and open the session for message communication.
                sessionTest.OutboundSession.OpenAsync(new CancellationToken()).Wait();
                sessionTest.InboundSession = targetPartition.InboundSessions[sourcePartition.PartitionKey];

                sessionTest.SnapshotOutbound = sessionTest.OutboundSession.GetDataSnapshot();
                sessionTest.SnapshotInbound = sessionTest.InboundSession.GetDataSnapshot();

                VS.Assert.IsFalse(sessionTest.InboundSession == sessionTest.OutboundSession,
               "sourcePartition equals targetPartition  in SetupSessionTests");
            }
        }
    }
}