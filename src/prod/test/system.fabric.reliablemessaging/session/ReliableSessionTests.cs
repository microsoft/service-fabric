// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ReliableSession.Test
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Test;
    using System.Threading.Tasks;
    using System.Fabric.Interop;
    using System.Linq;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Reliable Session tests
    /// </summary>
    [VS.TestClass]
    public partial class ReliableSessionTest
    {
        private static readonly Random RandGen = new Random(unchecked((int)DateTime.Now.Ticks));

        /// <summary>
        /// Initialize test
        /// </summary>
        /// <param name="context"></param>
        [VS.ClassInitialize]
        public static void ClassInitialize(VS.TestContext context)
        {
        }

        /// <summary>
        /// Test for processing multiple parallel sessions with random payloads and session abort behavior 
        /// </summary>
        [VS.TestMethod]
        public void RandomTests_ParallelSessionsProfile()
        {
            try
            {

                Utility.WrapNativeSyncInvokeInMTA(() =>
                {
                    try
                    {
                        // Number of parallel sessions tests
                        const int sessionCount = 100;

                        // Instantiate multiple test sessions.
                        SessionTest[] sessionTests = new SessionTest[sessionCount];

                        // Number of service instances
                        const int instanceCount = 5;

                        // Instantiate the required service instances.
                        InstanceRecord[] instances = new InstanceRecord[instanceCount];

                        // Max number of partition per instance
                        const int maxPartitionsPerInstance = 3;

                        // If Partition type is int64 range, than use this to set the low value range
                        const int int64PartitionKeyLowValMultiplier = 10000;

                        // Initialize Service Instances and randomly allocate partitions for each.
                        InitializeInstances(instances, true, maxPartitionsPerInstance);

                        // Initialize Partitions and respective reliable sessions
                        foreach (InstanceRecord instance in instances)
                        {
                            InitializePartitions(instance, true, new PartitionKeyType(), int64PartitionKeyLowValMultiplier);

                            foreach (PartitionRecord partition in instance.Partitions)
                            {
                                this.InitializeReliableSessions(instance.InstanceName, partition);
                            }
                        }

                        // Min Segment Size to set the boundaries for random segment size 
                        const int minSegmentSize = 100;

                        // Max Segment Size to set the boundaries for random segment size 
                        const int maxSegmentSize = 499;

                        // Min Message count to set the boundaries for random messages per session

                        const int minMessageCountPerSession = 500;

                        // Min Message count to set the boundaries for random messages per session
                        const int maxMessageCountPerSession = 2500;

                        // Wait for send Segment Wait In MilliSeconds Per Message
                        const int waitPerMessage = 1000; // in milliseconds 

                        // Dictionary of unique pairs of inbound and outbound sessions
                        Dictionary<string, string> checkForUniqueness = new Dictionary<string, string>();

                        // Initialize and Setup the session tests by randomly selecting message sizes and abort sessions.
                        for (int i = 0; i < sessionCount; i++)
                        {
                            // Pick a random Source Partition and Target Partition and setup the session test between them
                            PartitionRecord sourcePartitionRecord = GetRandomPartitionRecord(instances);
                            PartitionRecord targetPartitionRecord = GetRandomPartitionRecord(instances);
                            if (!String.Equals(sourcePartitionRecord.PartitionKey, targetPartitionRecord.PartitionKey))
                            {

                                int segmentSize = RandGen.Next(minSegmentSize, maxSegmentSize);
                                int messageCount = RandGen.Next(minMessageCountPerSession, maxMessageCountPerSession);

                                SessionTest sessionTest = new SessionTest(segmentSize, messageCount, waitPerMessage, i);
                                SetupSessionTest(sessionTest, sourcePartitionRecord, targetPartitionRecord);

                                // Check if it's an unique test session combination before adding it to the test sessions.
                                if (
                                    checkForUniqueness.ContainsKey(sourcePartitionRecord.PartitionKey +
                                                                   targetPartitionRecord.PartitionKey))
                                {
                                    sessionTest.SkipTest = true;
                                }
                                else
                                {
                                    checkForUniqueness.Add(
                                        sourcePartitionRecord.PartitionKey + targetPartitionRecord.PartitionKey, "");
                                    LogHelper.Log("SessionId: {0}; Unique Partitions: s: {1} t: {2}", sessionTest.SessionId, sourcePartitionRecord.PartitionKey, targetPartitionRecord.PartitionKey);
                                }

                                sessionTests[i] = sessionTest;

                            }
                            else
                            {
                                i--;
                            }
                        }

                        // Randomly start the tests in parallel
                        const int maxTestDelay = sessionCount*10;

                        // Cache all run test() in an action delegate array with random delay starts
                        Action[] testActions = sessionTests.Select(
                            element =>
                                (element.SkipTest)
                                    ? null
                                    : new Action(() => element.RunTest(RandGen.Next()%maxTestDelay)))
                            .ToArray();

                       // Create and start tasks for all non skipped tests
                        Task[] tempTestTasksToRun = testActions
                            .Where(element => element != null)
                            .Select(element => Task.Factory.StartNew(element)).ToArray();

                        LogHelper.Log("Total number of Tests started = {0}", tempTestTasksToRun.Count());

                        Task.WaitAll(tempTestTasksToRun);

                        LogHelper.Log("Total number of Tests {0} ran to completion.", tempTestTasksToRun.Count());
                    }
                    catch (Exception ex)
                    {
                        LogHelper.Log("Caught Exception: {0}", ex);

                        throw;
                    }

                }, "MultipleParallelSessions");
            }
            catch (Exception e)
            {
                VS.Assert.IsTrue(false,
                      "Unexpected exception {0} during RandomTests_ParallelSessionsProfile()", e.ToString());
            }
        }

        /// <summary>
        /// Clean up test
        /// </summary>
        [VS.ClassCleanup]
        public static void ClassCleanup()
        {
            // To validate no tasks are left behind with unhandled exceptions
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

    }
}