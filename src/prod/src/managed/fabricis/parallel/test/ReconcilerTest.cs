// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;
    using InfrastructureService.Test;
    using IO;
    using Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Reflection;
    using Repair;
    using Runtime.Serialization.Formatters;
    using Threading.Tasks;

    [TestClass]
    public class ReconcilerTest
    {
        private const string RootTestFilesPath = "TestFiles";

        // using i/o for input/output to keep file name small
        // suffixing .json so that IDEs can view it like json (versus plain text due to unknown extension)
        private const string PolicyAgentTestInputFileExtension = ".i.pa.json";
        private const string RepairTasksTestInputFileExtension = ".i.rt.json";
        private const string TestOutputFileExtension = ".o.json";

        private static JsonSerializerSettings internalSerializationSettings = GetInternalSerializationSettings();

        private readonly MockConfigStore configStore = new MockConfigStore();
        private readonly ConfigSection configSection;
        private readonly IActivityLogger activityLogger = new ActivityLoggerFactory().Create(Constants.TraceType);
        private readonly IPolicyAgentClient policyAgentClient;
        private readonly IRepairManager repairManager = new MockRepairManager();

        public ReconcilerTest()
        {
            configSection = new ConfigSection(Constants.TraceType, configStore, "IS/Reconciler");

            var env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                configSection,
                string.Empty,
                new MockInfrastructureAgentWrapper());

            policyAgentClient = new PolicyAgentClient(env, new MockPolicyAgentService(), activityLogger);
        }

        [TestMethod]
        public async Task BasicTest1Async()
        {
            var activityId = Guid.NewGuid();
            var job = TenantJobHelper.CreateNewTenantJob();
            var jobs = new List<ITenantJob> { job };

            var doc = TenantJobHelper.CreateNewPolicyAgentDocumentForTenant(jobs, 1);

            var coordinatorContext = new CoordinatorContext {Doc = doc};
            ActionHelper.CreateMappedWorkItems(jobs, new List<IRepairTask>(), coordinatorContext);

            await CreateReconciler().ReconcileAsync(activityId, coordinatorContext).ConfigureAwait(false);

            Assert.AreEqual(coordinatorContext.MappedTenantJobs[job.Id].Actions.Count, 1);
            var action = coordinatorContext.MappedTenantJobs[job.Id].Actions[0];
            Assert.IsTrue(action is CreateInPreparingAction, "Verifying if new repair task is being created");
        }

        /// <summary>
        /// Serialization/deserialization of mock policy agent doc
        /// </summary>
        [TestMethod]
        public void PolicyAgentDocSerDeTest()
        {
            var job = TenantJobHelper.CreateNewTenantJob();
            var jobs = new List<ITenantJob> { job };
            var doc = TenantJobHelper.CreateNewPolicyAgentDocumentForTenant(jobs, 1);

            // Note: do not use ToJson() since it uses public settings for serialization which means
            // that the object cannot be deserialized.
            string json = JsonConvert.SerializeObject(doc, internalSerializationSettings);
            var doc2 = json.FromJson<MockPolicyAgentDocumentForTenant>();

            Assert.AreEqual(doc.Incarnation, doc2.Incarnation);
            Assert.AreEqual(doc.JobDocumentIncarnation, doc2.JobDocumentIncarnation);
            Assert.AreEqual(doc.RoleInstanceHealthInfoIncarnation, doc2.RoleInstanceHealthInfoIncarnation);
            Assert.AreEqual(doc.RoleInstanceHealthInfoTimestamp, doc2.RoleInstanceHealthInfoTimestamp);
            Assert.AreEqual(doc.Jobs.Count, doc2.Jobs.Count);
            Assert.AreEqual(doc.RoleInstanceHealthInfos.Count, doc2.RoleInstanceHealthInfos.Count);

            for (int i = 0; i < doc.Jobs.Count; i++)
            {
                Assert.AreEqual(doc.Jobs[i].Id, doc2.Jobs[i].Id);
                Assert.AreEqual(doc.Jobs[i].ContextStringGivenByTenant, doc2.Jobs[i].ContextStringGivenByTenant);
                Assert.AreEqual(doc.Jobs[i].JobStatus, doc2.Jobs[i].JobStatus);
                Assert.IsTrue(doc.Jobs[i].RoleInstancesToBeImpacted.SequenceEqual(doc2.Jobs[i].RoleInstancesToBeImpacted));

            }

            string json3 = JsonConvert.SerializeObject(doc2, internalSerializationSettings);
            Assert.AreEqual(json, json3);

        }

        /// <summary>
        /// Serialization/deserialization of mock repair tasks
        /// </summary>
        [TestMethod]
        public void RepairTaskSerDeTest()
        {
            var repairTask = new MockRepairTask("taskA", "System.Azure.Reboot");

            string json = JsonConvert.SerializeObject(repairTask, internalSerializationSettings);
            var repairTask2 = json.FromJson<MockRepairTask>();

            string json3 = JsonConvert.SerializeObject(repairTask2, internalSerializationSettings);
            Assert.AreEqual(json, json3);
        }

        [TestMethod]
        public async Task ProcessTestFilesAsync()
        {
            var exePath = Path.GetDirectoryName(typeof(ReconcilerTest).GetTypeInfo().Assembly.Location);
            var dir = Path.Combine(exePath, RootTestFilesPath);

            var expectedOutputFiles = Directory.GetFiles(dir, "*" + TestOutputFileExtension, SearchOption.AllDirectories);

            var processedFiles = new List<string>();
            var failedFiles = new List<string>();

            foreach (var expectedOutputFile in expectedOutputFiles)
            {
                var file = Path.GetFileName(expectedOutputFile);

                Constants.TraceType.WriteInfo("Processing {0}", file);

                try
                {
                    var fileDir = Path.GetDirectoryName(expectedOutputFile);
                    var fileName = Path.GetFileNameWithoutExtension(expectedOutputFile);
                    fileName = fileName.Substring(0, fileName.Length - 2); // the .o remains after getting file name without extension

                    var inputPolicyAgentFile = Path.Combine(fileDir, fileName + PolicyAgentTestInputFileExtension);
                    var inputRepairTasksFile = Path.Combine(fileDir, fileName + RepairTasksTestInputFileExtension);

                    IPolicyAgentDocumentForTenant doc = JsonTestFileLoader.LoadPolicyAgentDoc(inputPolicyAgentFile);
                    IList<IRepairTask> repairTasks = JsonTestFileLoader.LoadRepairTasks(inputRepairTasksFile);
                    IList<JsonExpectedActions> expectedActions = JsonTestFileLoader.LoadExpectedActions(expectedOutputFile);

                    await ValidateActionsAsync(doc, repairTasks, expectedActions).ConfigureAwait(false);

                    processedFiles.Add(file);
                }
                catch (Exception ex)
                {
                    Constants.TraceType.WriteError("Error while processing {0}. Exception: {1}", expectedOutputFile, ex);
                    failedFiles.Add(file);
                }
            }

            Constants.TraceType.WriteInfo(
                "Processed files: {0}",
                processedFiles.Count > 0
                    ? string.Join(", ", processedFiles)
                    : "None");
            Constants.TraceType.WriteInfo(
                "Failed files: {0}", 
                failedFiles.Count > 0 
                    ? string.Join(", ", failedFiles)
                    : "None");

            Assert.IsTrue(expectedOutputFiles.Length > 0);
            Assert.AreEqual(failedFiles.Count, 0);
        }

        private static JsonSerializerSettings GetInternalSerializationSettings()
        {
            if (internalSerializationSettings != null)
            {
                return internalSerializationSettings;
            }

            internalSerializationSettings = new JsonSerializerSettings
            {
                Formatting = Formatting.Indented,
                NullValueHandling = NullValueHandling.Ignore,
                ReferenceLoopHandling = ReferenceLoopHandling.Ignore,
                TypeNameHandling = TypeNameHandling.Auto,
                TypeNameAssemblyFormat = FormatterAssemblyStyle.Simple,
                ConstructorHandling = ConstructorHandling.Default,
                DateFormatHandling = DateFormatHandling.IsoDateFormat,
                DateFormatString = "o",
            };

            // for seeing enums as strings not as integer values
            internalSerializationSettings.Converters.Add(new StringEnumConverter());

            return internalSerializationSettings;
        }

        private Reconciler CreateReconciler()
        {
            var env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                configSection,
                string.Empty,
                new MockInfrastructureAgentWrapper());

            return new Reconciler(env, policyAgentClient, repairManager, new RepairActionProvider(configSection));
        }

        private async Task ValidateActionsAsync(
            IPolicyAgentDocumentForTenant doc,
            IList<IRepairTask> repairTasks,
            IList<JsonExpectedActions> idsAndExpectedActions)
        {
            Guid activityId = Guid.NewGuid();

            var reconciler = CreateReconciler();

            var coordinatorContext = new CoordinatorContext {Doc = doc};
            ActionHelper.CreateMappedWorkItems(doc.Jobs, repairTasks, coordinatorContext);

            await reconciler.ReconcileAsync(activityId, coordinatorContext).ConfigureAwait(false);

            var IdActionMap = GetReconciledActions(coordinatorContext);

            Assert.IsTrue(IdActionMap.Keys.SequenceEqual(idsAndExpectedActions.Select(e => e.Id)));

            foreach (var idAndExpectedAction in idsAndExpectedActions)
            {
                var actualAction = IdActionMap[idAndExpectedAction.Id];
                Assert.IsTrue(actualAction.SequenceEqual(idAndExpectedAction.Actions));
            }
        }

        private static IDictionary<string, IList<string>> GetReconciledActions(CoordinatorContext coordinatorContext)
        {
            var IdActionMap = new Dictionary<string, IList<string>>(StringComparer.OrdinalIgnoreCase);

            foreach (var mappedTenantJob in coordinatorContext.MappedTenantJobs.Values)
            {
                // don't add Ids if there are no actions associated with it
                if (mappedTenantJob.Actions.Count > 0)
                {
                    IdActionMap.Add(mappedTenantJob.Id.ToString(), mappedTenantJob.Actions.Select(e => e.Name).ToList());
                }
            }

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                // assume repair task ids don't conflict with tenant job ids.
                if (mappedRepairTask.Actions.Count > 0)
                {
                    IdActionMap.Add(mappedRepairTask.Id, mappedRepairTask.Actions.Select(e => e.Name).ToList());
                }
            }

            return IdActionMap;
        }
    }
}