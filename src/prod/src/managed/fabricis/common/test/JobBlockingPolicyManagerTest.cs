// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System;
    using System.Collections.Generic;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading.Tasks;

    [TestClass]
    public class JobBlockingPolicyManagerTest
    {
        private static readonly TraceType TraceType = new TraceType(typeof(JobBlockingPolicyManagerTest).Name);

        /// <summary>
        /// We need this to set custom values of max attempts and delta back off for the unit tests to finish quickly.
        /// </summary>
        private readonly IRetryPolicyFactory retryPolicyFactory = new LinearRetryPolicyFactory(
            TraceType, 
            500, 
            3,
            AzureHelper.IsRetriableException);

        private int funcToInvoke;

        [TestInitialize]
        public void Setup()
        {
            funcToInvoke = 0;
        }

        private async Task<IJobBlockingPolicyManager> CreateAsync(IPropertyManagerWrapper propertyManager)
        {
            var versionedPropertyStore = await VersionedPropertyStore.CreateAsync(
                Guid.NewGuid(),
                TraceType,
                new Uri(Constants.StoreName),
                propertyManager,
                retryPolicyFactory).ConfigureAwait(false);
            IJobBlockingPolicyManager jobPolicyManager =
                await JobBlockingPolicyManager.CreateAsync(
                    TraceType, versionedPropertyStore).ConfigureAwait(false);

            return jobPolicyManager;
        }

        [TestMethod]
        public async Task InitializeTest1Async()
        {
            var propertyManager = new MockPropertyManagerWrapper
            {
                SubmitPropertyBatchAsyncFunc = WorkflowForInitializeTest1
            };

            IJobBlockingPolicyManager jobPolicyManager = await CreateAsync(propertyManager).ConfigureAwait(false);

            Assert.IsNotNull(jobPolicyManager);
            Assert.AreEqual(
                jobPolicyManager.Policy,
                JobBlockingPolicy.BlockAllJobs,
                "Verifying if new policy has been successfully read after Initialize");
        }

        [TestMethod]
        public async Task InitializeTest2Async()
        {
            var propertyManager = new MockPropertyManagerWrapper
            {
                SubmitPropertyBatchAsyncFunc = WorkflowForInitializeTest2
            };

            try
            {
                await CreateAsync(propertyManager).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                Assert.IsTrue(
                    e is FabricException,
                    "Throwing FabricException as expected in Initialize");
            }
        }

        /// <summary>
        /// Testing a version mismatch when updating policy. But getting the existing policy from the store after that should succeed.
        /// </summary>
        [TestMethod]
        public async Task UpdatePolicyAsyncTest1Async()
        {
            var propertyManager = new MockPropertyManagerWrapper
            {
                SubmitPropertyBatchAsyncFunc = WorkflowForUpdatePolicyAsyncTest1
            };

            // mock a result where we return ok on initialize, then an error for the first operation
            // and return okay when an internal Read is called

            IJobBlockingPolicyManager jobPolicyManager = await CreateAsync(propertyManager).ConfigureAwait(false);

            try
            {
                await jobPolicyManager.UpdatePolicyAsync(JobBlockingPolicy.BlockNone).ConfigureAwait(false);
                Assert.Fail("Expected to throw an exception");
            }
            catch (FabricException ex)
            {
                Assert.AreEqual(
                   ex.ErrorCode,
                    FabricErrorCode.WriteConflict,
                    "Throwing FabricException as expected because of version mismatch");
            }

            Assert.AreEqual(
                jobPolicyManager.Policy,
                JobBlockingPolicy.BlockAllJobs,
                "Update policy failed as expected, but reading policy from store succeeded");
        }

        /// <summary>
        /// Testing happy path while updating policy.
        /// </summary>
        [TestMethod]
        public async Task UpdatePolicyAsyncTest3Async()
        {
            var propertyManager = new MockPropertyManagerWrapper(); 
            propertyManager.SubmitPropertyBatchAsyncFunc = WorkflowForUpdatePolicyAsyncTest3;

            IJobBlockingPolicyManager jobPolicyManager = await CreateAsync(propertyManager).ConfigureAwait(false);

            Assert.AreEqual(
               jobPolicyManager.Policy,
                JobBlockingPolicy.BlockNone,
                "Verifying if starting state of policy is as expected");

            await jobPolicyManager.UpdatePolicyAsync(JobBlockingPolicy.BlockAllJobs).ConfigureAwait(false);

            Assert.AreEqual(
                jobPolicyManager.Policy, 
                JobBlockingPolicy.BlockAllJobs,
                "Verifying if new policy has been successfully applied");
        }

        /// <summary>
        /// Tests a migration scenario where existing IS (serial IS) has a valid job blocking policy
        /// and is has to be moved to a per-tenant job blocking policy.
        /// </summary>
        [TestMethod]
        public async Task MigrationToNamespacePrefixTestAsync()
        {
            var mockVersionedPropertyStore = new MockVersionedPropertyStore();            

            IJobBlockingPolicyManager jobPolicyManager = await JobBlockingPolicyManager.CreateAsync(
                TraceType, mockVersionedPropertyStore).ConfigureAwait(false);

            Assert.AreEqual(jobPolicyManager.Policy, JobBlockingPolicy.BlockNone);

            await jobPolicyManager.UpdatePolicyAsync(JobBlockingPolicy.BlockAllJobs).ConfigureAwait(false);
            Assert.AreEqual(jobPolicyManager.Policy, JobBlockingPolicy.BlockAllJobs);

            // IS is migrated to parallel mode with per-tenant jobblocking policy option
            var mockTenantSpecificVersionedPropertyStore = new MockVersionedPropertyStore();
            IJobBlockingPolicyManager jobPolicyManager2 = await JobBlockingPolicyManager.CreateAsync(
                TraceType, mockTenantSpecificVersionedPropertyStore, mockVersionedPropertyStore).ConfigureAwait(false);

            // the namespace properties should have the same policy as the existing old properties
            Assert.AreEqual(jobPolicyManager2.Policy, JobBlockingPolicy.BlockAllJobs);

            var activityId = Guid.NewGuid();
            var vkv = await mockVersionedPropertyStore
                    .GetValueAsync(activityId, JobBlockingPolicyManager.PolicyPropertyName, JobBlockingPolicyManager.PolicyVersionName)
                    .ConfigureAwait(false);
            Assert.IsNotNull(vkv);

            var vkv2 = await mockTenantSpecificVersionedPropertyStore
                    .GetValueAsync(activityId, JobBlockingPolicyManager.PolicyPropertyName, JobBlockingPolicyManager.PolicyVersionName)
                    .ConfigureAwait(false);
            Assert.IsNotNull(vkv2);

            Assert.AreEqual(vkv.Value, vkv2.Value);
        }

        private MockPropertyBatchResultWrapper WorkflowForInitializeTest1(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper
            {
                FailedOperationIndex = -1,
                FailedOperationException = new FabricException(FabricErrorCode.Unknown)
            };

            result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)1 });
            result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = JobBlockingPolicy.BlockAllJobs.ToString() });

            TraceType.ConsoleWriteLine("Setting FailedOperationIndex = -1 to simulate success from SubmitPropertyBatchAsync inside Initialize");
            
            return result;
        }

        private MockPropertyBatchResultWrapper WorkflowForInitializeTest2(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper
            {
                FailedOperationIndex = 0,
                FailedOperationException = new FabricException(FabricErrorCode.Unknown)
            };

            TraceType.ConsoleWriteLine("Setting FailedOperationIndex = 0 to simulate error from SubmitPropertyBatchAsync inside Initialize");

            return result;
        }

        private MockPropertyBatchResultWrapper WorkflowForUpdatePolicyAsyncTest1(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper();

            if (funcToInvoke == 0)
            {
                Trace.ConsoleWriteLine(TraceType, "Setting FailedOperationIndex = -1 to simulate success from ReadProperties() inside UpdatePolicyAsync");
                result.FailedOperationIndex = -1; // operation succeeds
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)1 });
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = JobBlockingPolicy.BlockAllJobs.ToString() });

                funcToInvoke++;
            }
            else if (funcToInvoke == 1)
            {
                Trace.ConsoleWriteLine(TraceType, "Setting FailedOperationIndex = 0 to simulate version mismatch from SubmitPropertyBatchAsync inside UpdatePolicyAsync");
                result.FailedOperationIndex = 0; // operation fails
                result.FailedOperationException = new FabricException(FabricErrorCode.Unknown);

                funcToInvoke++; // the next time SubmitFunc is called, it should succeed
            }
            else
            {
                Trace.ConsoleWriteLine(TraceType, "Setting FailedOperationIndex = -1 to simulate success from ReadProperties() inside UpdatePolicyAsync");
                result.FailedOperationIndex = -1; // operation succeeds
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)1 });
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = JobBlockingPolicy.BlockAllJobs.ToString() });                
            }

            return result;
        }

        private MockPropertyBatchResultWrapper WorkflowForUpdatePolicyAsyncTest3(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var result = new MockPropertyBatchResultWrapper();

            if (funcToInvoke == 0)
            {
                Trace.ConsoleWriteLine(
                    TraceType,
                    "Setting FailedOperationIndex = -1 to simulate success from ReadProperties() inside UpdatePolicyAsync");
                result.FailedOperationIndex = -1; // operation succeeds
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)1 });
                result.NamedProperties.Add(new MockNamedPropertyWrapper
                {
                    Value = JobBlockingPolicy.BlockNone.ToString()
                });

                funcToInvoke++;
            }
            else
            {
                Trace.ConsoleWriteLine(TraceType, "Setting FailedOperationIndex = -1 (i.e. success) to simulate success inside UpdatePolicyAsync");

                // there are 6 operations that are batched up inside UpdatePropertiesAsync
                // If we succeed, we get the values of indices 2 and 4 to update the cache.
                // Simulating this behavior
                result.NamedProperties.Add(new MockNamedPropertyWrapper());
                result.NamedProperties.Add(new MockNamedPropertyWrapper());
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = (Int64)2 });
                result.NamedProperties.Add(new MockNamedPropertyWrapper());
                result.NamedProperties.Add(new MockNamedPropertyWrapper { Value = JobBlockingPolicy.BlockAllJobs.ToString() });
                result.NamedProperties.Add(new MockNamedPropertyWrapper());                
            }

            return result;
        }
    }
}