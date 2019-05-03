// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Fabric.Test;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;
using System.IO;
using System.Xml;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Fabric;
using System.Fabric.Health;
using System.Fabric.Query;
using System.Fabric.Repair;

namespace RepairPolicyEngine.Test
{
    [TestClass]
    public class PolicyEngineTest
    {
        private const string DefaultSettingsPath = @"%_NTTREE%\RepairPolicyEngineApplication\RepairPolicyEnginePackage\RepairPolicyEngineService.Config\Settings.xml";
        private const string DefaultApplicationTypeName = "RepairPolicyEngineApplication";

        private static readonly TimeSpan _repairManagerLatency = TimeSpan.FromSeconds(30); // Latency for repair request to be received by Repair Manager
        private static TimeSpan _actionSchedulingInterval;
        private static TimeSpan _probationToFailing;
        private static TimeSpan _minimumHealthyDuration;
        
        private static FabricClient _fabricClient;

        private static List<MockRepairExecutor> Executors { get; set; }

        private static IEnumerator<Node> NodeEnumerator;

        [ClassInitialize]
        public static void ClassInitialize(TestContext context)
        {
            LogHelper.Log("ClassInitialize()");

            InitializeFabricClient(true);
            Assert.IsNotNull(_fabricClient, "Fabric Client must be valid");

            XDocument xmlSettings = LoadSettingsFile();
            LoadCommonSettings(xmlSettings);
            LoadExecutors(xmlSettings);

            // Cleanup everything before start the test suite
            TestCleanup();
        }

        [TestCleanup]
        public static void TestCleanup()
        {
            LogHelper.Log("TestCleanup()");

            // Clear health store
            var nodeList = _fabricClient.QueryManager.GetNodeListAsync().Result;
            foreach (var node in nodeList)
            {
                MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
                watchdog.ClearError();
            }

            // Cancel existing tasks
            MockRepairExecutor.CancelAll(Executors, _fabricClient.RepairManager);
        }
        
        [TestMethod]
        public void Onebox_Successful_ScheduleSingleRepair()
        {
            const string property = "Onebox_Successful_ScheduleSingleRepair";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0} for executor {1}", property, executor.Name);

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");
        }

        [TestMethod]
        public void Onebox_Successful_SingleFastTrackRepair()
        {
            const string test = "Onebox_Successful_SingleFastTrackRepair";
            const string property = "RequestRepair";
            // Use reimage executor
            MockRepairExecutor executor = Executors.Skip(1).First();
            Console.WriteLine("Starting test {0} for executor {1}", test, executor.Name);
            string description = executor.Name;

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property, description);

            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");
        }


        [TestMethod]
        public void Onebox_Successful_SelfRepairAfterTaskIsCreated()
        {
            const string property = "Onebox_Successful_SelfRepairAfterTaskIsCreated";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0}", property);

            Node node = GetHealthyNode(_fabricClient);
            // Inject error
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");

            ClearHealthError(watchdog, property, executor);

            // Make sure the task is cancelled
            Thread.Sleep(_repairManagerLatency);
            RepairTaskList repairTaskList = _fabricClient.RepairManager.GetRepairTaskListAsync(createdTask.TaskId, RepairTaskStateFilter.All, null).Result;
            Assert.IsTrue((repairTaskList.Any() && repairTaskList.Count == 1), "There must be a unique repair task with task id = {0}", createdTask.TaskId);

            RepairTask cancelledTask = repairTaskList.Single();
            Assert.IsTrue(cancelledTask.Flags == RepairTaskFlags.CancelRequested && cancelledTask.ResultStatus == RepairTaskResult.Cancelled && cancelledTask.State == RepairTaskState.Completed,
                "PE should cancel the task and set state to Completed. Current flags = {0}, Current result = {1}", cancelledTask.Flags, cancelledTask.ResultStatus);
        }

        [TestMethod]
        public void Onebox_Successful_SelfRepairAfterTaskIsApproved()
        {
            const string property = "Onebox_Successful_SelfRepairAfterTaskIsCreated";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0}", property);

            Node node = GetHealthyNode(_fabricClient);
            // Inject error
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");

            executor.Approve(ref createdTask);

            ClearHealthError(watchdog, property, executor);

            // Make sure the task is cancelled
            Thread.Sleep(_repairManagerLatency);
            RepairTaskList repairTaskList = _fabricClient.RepairManager.GetRepairTaskListAsync(createdTask.TaskId, RepairTaskStateFilter.All, null).Result;
            Assert.IsTrue((repairTaskList.Any() && repairTaskList.Count == 1), "There must be a unique repair task with task id = {0}", createdTask.TaskId);

            RepairTask cancelledTask = repairTaskList.First();
            Assert.IsTrue(cancelledTask.Flags == RepairTaskFlags.CancelRequested, "PE should request cancellation of the task");
        }

        [TestMethod]
        public void Onebox_Successful_SelfRepairNoCreatedTask()
        {
            const string property = "Onebox_Successful_SelfRepairNoCreatedTask";
            LogHelper.Log("Starting test {0}", property);


            Node node = GetHealthyNode(_fabricClient);
            // Inject error
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            ClearHealthError(watchdog, property, Executors.First());

            var nodeHealth = _fabricClient.HealthManager.GetNodeHealthAsync(node.NodeName).Result;
            Assert.IsTrue(nodeHealth.AggregatedHealthState == HealthState.Ok, "Node health must be Ok after health error event is cleared");

            Parallel.ForEach(Executors, executor =>
                {
                    if (!executor.IsEnabled) return;
                    RepairTask createdTask = PollingForCreatedTask(executor, node);
                    Assert.IsNull(createdTask, "There must be no scheduled task in case of self-repair");
                });
        }

        [TestMethod]
        public void Onebox_Successful_CorrectRepairOrderWithOKInTheMiddle()
        {
            const string property = "Onebox_Successful_CorrectRepairOrderWithOKInTheMiddle";
            Onebox_Successful_CorrectRepairOrder(property, true);
        }

        [TestMethod]
        public void Onebox_Successful_CorrectRepairOrderWithoutOK()
        {
            const string property = "Onebox_Successful_CorrectRepairOrderWithoutOK";
            Onebox_Successful_CorrectRepairOrder(property, false);
        }

        [TestMethod]
        public void Onebox_Successful_CorrectRepairStateTransition()
        {
            const string property = "Onebox_Successful_CorrectRepairStateTransition";
            LogHelper.Log("Starting test {0}", property);

            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            MockRepairExecutor executor = Executors.First();

            // Inject error
            watchdog.ReportError(property);

            // Wait for PE to schedule the task
            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");

            executor.Process(ref createdTask);
        }

        // Note this test does not test the outcome of the deletion as it is an internal state 
        // change of the PE and we cannot externally observe it but it makes sure it does not 
        // crash or otherwise go into a bad state due to this. 
        [TestMethod]
        public void Onebox_Successful_DeleteRepairInTheMiddle()
        {
            const string property = "Onebox_Successful_DeleteRepairInTheMiddle";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0} for executor {1}", property, executor.Name);

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");
            // cancel the task
            long result = _fabricClient.RepairManager.CancelRepairTaskAsync(createdTask.TaskId, 0, true).Result;

            // Delete the repair task
            MockRepairExecutor.DeleteTask(createdTask, _fabricClient.RepairManager);

            ClearHealthError(watchdog, property, executor);
            
            // Report Error
            watchdog.ReportError(property);

            createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created after deletion.");
            
            // Complete the repair task
            executor.Complete(createdTask);
        }

        [TestMethod]
        public void Onebox_Successful_ReplicationAfterRepairCreated()
        {
            const string property = "Onebox_Successful_ReplicationAfterRepairCreated";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0} for executor {1}", property, executor.Name);

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            // Wait for PE to schedule the task
            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");

            KillPrimaryReplica();

            RepairTask newTask = PollingForCreatedTask(executor, node);
            Assert.IsTrue(newTask.TaskId.Equals(createdTask.TaskId, StringComparison.OrdinalIgnoreCase),
                          "The repair task should be the same created before");
        }

        [TestMethod]
        public void Onebox_Successful_ReplicationAfterRepairCompleted()
        {
            const string property = "Onebox_Successful_ReplicationAfterRepairCancelled";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0} for executor {1}", property, executor.Name);

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            var nodeHealth = _fabricClient.HealthManager.GetNodeHealthAsync(node.NodeName).Result;
            var oldRepairHistory = nodeHealth.HealthEvents.Single(e => e.HealthInformation.Property.Equals("RepairPolicyEngineService::RepairHistory",
                                                                                                        StringComparison.OrdinalIgnoreCase));

            // Wait for PE to schedule the task
            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");
            DateTime scheduledAt = DateTime.Now;
            executor.Complete(createdTask);

            KillPrimaryReplica();

            LogHelper.Log("Waiting for the RepairHistory in health store to have the latest repair");
            // Make sure the RepairHistory has the entry within 5 minutes
            string newRepairDescription = null;
            TimeSpan timeout = TimeSpan.FromMinutes(5);
            while (timeout >= TimeSpan.Zero)
            {
                nodeHealth = _fabricClient.HealthManager.GetNodeHealthAsync(node.NodeName).Result;
                var repairHistory = nodeHealth.HealthEvents.Single(e => e.HealthInformation.Property.Equals("RepairPolicyEngineService::RepairHistory",
                                                                                                            StringComparison.OrdinalIgnoreCase));
                if (!repairHistory.HealthInformation.Description.Equals(oldRepairHistory.HealthInformation.Description, StringComparison.OrdinalIgnoreCase))
                {
                    string[] descriptions = repairHistory.HealthInformation.Description.Split(';');
                    newRepairDescription = descriptions[descriptions.Count()-2]; // Last element is empty
                    break;
                }
                TimeSpan sleepTime = TimeSpan.FromSeconds(10);
                Thread.Sleep(sleepTime);
                timeout -= sleepTime;
            }
            Assert.IsFalse(String.IsNullOrEmpty(newRepairDescription), "There must be a new repair description");

            string repairType = newRepairDescription.Split('=').First();
            Assert.IsTrue(repairType.Equals(executor.Name, StringComparison.OrdinalIgnoreCase), "Repair type is the same as executor name");
            DateTime repairScheduledAt = DateTime.Parse(newRepairDescription.Split('=').Last());
            Assert.IsTrue(repairScheduledAt >= scheduledAt, "There should be a new entry in repair history");
        }

        [TestMethod]
        public void Onebox_Successful_ReplicationAfterRepairCancelled()
        {
            const string property = "Onebox_Successful_ReplicationAfterRepairCancelled";

            MockRepairExecutor executor = Executors.First();
            LogHelper.Log("Starting test {0} for executor {1}", property, executor.Name);

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            // Wait for PE to schedule the task
            RepairTask createdTask = PollingForCreatedTask(executor, node);
            Assert.IsNotNull(createdTask, "There must be a repair task created.");
            Assert.IsNotNull(createdTask.CreatedTimestamp != null, "CreatedTimestamp must not be null");
            DateTime firstCreated = createdTask.CreatedTimestamp.Value;

            watchdog.ReportOk(property);

            // Make sure the task is cancelled
            Thread.Sleep(_minimumHealthyDuration + _actionSchedulingInterval + _repairManagerLatency);
            RepairTask cancelledTask = _fabricClient.RepairManager.GetRepairTaskListAsync(createdTask.TaskId, RepairTaskStateFilter.All, null).Result.Single();
            Assert.IsTrue(cancelledTask.State == RepairTaskState.Completed && cancelledTask.ResultStatus == RepairTaskResult.Cancelled, 
                "PE should request to cancel the task. Current State = {0}. Result Status = {1}", cancelledTask.State, cancelledTask.ResultStatus);

            KillPrimaryReplica();

            Thread.Sleep(_minimumHealthyDuration + _actionSchedulingInterval + _repairManagerLatency);
            var repairList = _fabricClient.RepairManager.GetRepairTaskListAsync(createdTask.TaskId, RepairTaskStateFilter.Completed, null).Result;

            var query = from repairTask in repairList
                        where repairTask.Target is NodeRepairTargetDescription &&
                              ((NodeRepairTargetDescription)repairTask.Target).Nodes.First() == node.NodeName &&
                              repairTask.State == RepairTaskState.Completed &&
                              repairTask.ResultStatus == RepairTaskResult.Cancelled &&
                              repairTask.CreatedTimestamp >= firstCreated
                        select repairTask;

            Assert.IsTrue(query.Count() == 1, "There must be no duplicated cancel request");
        }

        #region Private methods

        private void Onebox_Successful_CorrectRepairOrder(string property, bool reportOK)
        {
            LogHelper.Log("Starting test {0}", property);

            // Inject error
            Node node = GetHealthyNode(_fabricClient);
            DateTime lastHealthyAt = DateTime.Now;
            MockWatchdog watchdog = new MockWatchdog(_fabricClient.HealthManager, node.NodeName);
            watchdog.ReportError(property);

            int currentIndex = -1;
            MockRepairExecutor executor = null;
            // timeout is 2 times the longest policy plusplus a little extra time to account for software delays due to onebox environments
            DateTime timeout = DateTime.Now + Executors.Last().ActionTime + Executors.Last().ActionTime + TimeSpan.FromMinutes(3);
            while (DateTime.Now < timeout)
            {
                // Wait for PE to schedule the task
                RepairTask createdTask = PollingForAnyCreatedTask(node);
                Assert.IsNotNull(createdTask, "There should be a new repair task");
                if (executor == null || executor.IsExecutable(createdTask) == false)
                {
                    ++currentIndex;
                    while (Executors[currentIndex].IsEnabled == false)
                    {
                        LogHelper.Log("Skipping disabled repair executor {0}", Executors[currentIndex].Name);
                        ++currentIndex;
                        if (currentIndex >= Executors.Count())
                        {
                            Assert.Fail("No subsequent enabled executor");
                        }
                    }
                }
                executor = Executors[currentIndex];

                Assert.IsTrue(executor.IsEnabled, "Current executor '{0}' should be enabled", executor.Name);
                
                Assert.IsTrue(executor.IsExecutable(createdTask), "Task should be executable by current repair executor {0}", executor.Name);

                Assert.IsTrue(DateTime.Now - lastHealthyAt > executor.ActionTime,
                              "Processing time should be bigger than executor's action time ({0} seconds)",
                              executor.ActionTime.TotalSeconds);

                executor.Complete(createdTask);
                
                
                if (reportOK)
                {
                    // Report OK in the middle to make sure that the escalation happens even in the flip 
                    // flopping case where after a repair a machine stays healthy for a shortwhile and then reports error again. 
                    Thread.Sleep(_actionSchedulingInterval);
                    watchdog.ReportOk(property);
                    // wait for the next action to happen
                    Thread.Sleep(_actionSchedulingInterval);
		    // give additional time to deal with any delays
                    Thread.Sleep(TimeSpan.FromMilliseconds(5));
                    // Flop it back to error
                    watchdog.ReportError(property);
		    Thread.Sleep(TimeSpan.FromMilliseconds(5));
                }
            }
            // By going thru the loop we should have reached the last executor
            Assert.IsTrue(executor.ActionTime == Executors.Last().ActionTime);
        }

        private void KillPrimaryReplica()
        {
            // Kill the primary
            Application application =
                _fabricClient.QueryManager.GetApplicationListAsync()
                             .Result.Single(a => a.ApplicationTypeName == DefaultApplicationTypeName);
            Service service =
                _fabricClient.QueryManager.GetServiceListAsync(application.ApplicationName).Result.Single();
            Partition partition =
                _fabricClient.QueryManager.GetPartitionListAsync(service.ServiceName).Result.Single();
            StatefulServiceReplica primaryReplica =
                _fabricClient.QueryManager.GetReplicaListAsync(partition.PartitionInformation.Id)
                             .Result.Select(replica => replica as StatefulServiceReplica)
                             .Single(statefulServiceReplica => statefulServiceReplica.ReplicaRole == ReplicaRole.Primary);
            LogHelper.Log("Killing the primary replica at node {0}", primaryReplica.NodeName);

            ReplicaSelector replicaSelector = ReplicaSelector.ReplicaIdOf(PartitionSelector.PartitionIdOf(service.ServiceName, partition.PartitionInformation.Id), primaryReplica.Id);

            _fabricClient.FaultManager.RemoveReplicaAsync(replicaSelector, CompletionMode.DoNotVerify, false);
        }

        private void ClearHealthError(MockWatchdog watchdog, string property, MockRepairExecutor executor)
        {
            watchdog.ReportOk(property);
            // Make sure that the internal PE state is also back to Healthy
            Thread.Sleep(_minimumHealthyDuration + _actionSchedulingInterval);
            Thread.Sleep(executor.ProbationToHealthyPostRepair);
        }

        private static RepairTask PollingForAnyCreatedTask(Node node)
        {
            LogHelper.Log("Polling for any created task for node '{0}'", node.NodeName);
            TimeSpan timeout = Executors.Max(executor => executor.ActionTime) + _probationToFailing + _actionSchedulingInterval + _repairManagerLatency;

            // Give a little extra time to account for software delays due to onebox environments
            timeout = timeout.Add(TimeSpan.FromMinutes(2));
            DateTime startTime = DateTime.Now;
            while (DateTime.Now - startTime <= timeout)
            {
                RepairTask task = MockRepairExecutor.GetAnyCreatedTask(node.NodeName, _fabricClient.RepairManager);
                if (task != null) return task;
                Thread.Sleep(_actionSchedulingInterval);
            }

            return null;
        }

        private static RepairTask PollingForCreatedTask(MockRepairExecutor executor, Node node)
        {
            LogHelper.Log("Polling for created task for executor '{0}' and node '{1}'", executor.Name, node.NodeName);
            TimeSpan timeout = executor.ActionTime + _probationToFailing + _actionSchedulingInterval + _repairManagerLatency;
            // Give a little extra time to account for software delays due to onebox environments
            timeout = timeout.Add(TimeSpan.FromMinutes(2));
            DateTime startTime = DateTime.Now;
            while (DateTime.Now - startTime <= timeout)
            {
                RepairTask task = executor.GetCreatedTask(node.NodeName);
                if (task != null) return task;
                Thread.Sleep(_actionSchedulingInterval);
            }

            return null;
        }

        private Node GetHealthyNode(FabricClient fabricClient)
        {
            while (true)
            {
                bool hasNext = NodeEnumerator.MoveNext();
                if (!hasNext)
                {
                    // Reset to to the beginning
                    NodeEnumerator.Reset();
                    continue;
                }

                var node = NodeEnumerator.Current;
                if (node.HealthState == HealthState.Ok)
                {
                    LogHelper.Log("Select healthy node {0}", node.NodeName);
                    return node;
                }
            }
        }

        private static void InitializeFabricClient(bool isOneBox)
        {
            if (!isOneBox)
            {
                // Only for Onebox now
                return;
            }

            _fabricClient = new FabricClient();

            NodeList nodeList = _fabricClient.QueryManager.GetNodeListAsync().Result;
            var query = from n in nodeList
                        where n.HealthState == HealthState.Ok
                        select n;
            Assert.IsTrue(query.Any(), "There should be at least 1 healthy node");
            NodeEnumerator = nodeList.GetEnumerator();
        }

        private static XDocument LoadSettingsFile()
        {
            string settingsPath = Environment.ExpandEnvironmentVariables(DefaultSettingsPath);
            try
            {
                XDocument xDocument = XDocument.Load(settingsPath);
                return xDocument;
            }
            catch (FileNotFoundException)
            {
                Assert.Fail("Settings path is not valid {0}", settingsPath);
            }
            catch (XmlException)
            {
                Assert.Fail("XML file is not well-formed, please try again. Path = {0}", settingsPath);
            }
            return null;
        }

        private static void LoadExecutors(XDocument xmlSettings)
        {
            Executors = new List<MockRepairExecutor>();
            var allRepairAction = from element in xmlSettings.Descendants()
                                  where element.Name.LocalName == "Section" && element.Attribute("Name").Value == "NodeRepairActionList"
                                  select element;
            foreach (var repairActionSection in allRepairAction.First().Descendants())
            {
                string repairActionSectionName = repairActionSection.Attribute("Value").Value;

                string name = repairActionSection.Attribute("Name").Value;
                //TimeSpan ProbationToFailingPostRepair = GetParameterTimeSpan(xmlSettings, repairActionSectionName, "ProbationToFailingWaitDurationPostRepairInSeconds");
                TimeSpan ProbationToHealthyPostRepair = GetParameterTimeSpan(xmlSettings, repairActionSectionName, "ProbationToHealthyWaitDurationPostRepairInSeconds");
                TimeSpan policyActionTime = GetParameterTimeSpan(xmlSettings, repairActionSectionName, "PolicyActionTimeInSeconds");
                bool isEnabled = Convert.ToBoolean(GetParameter(xmlSettings, repairActionSectionName, "IsEnabled"));

                MockRepairExecutor executor = new MockRepairExecutor(name, policyActionTime, isEnabled, ProbationToHealthyPostRepair, _fabricClient.RepairManager);
                Executors.Add(executor);
            }

            Executors.Sort((executor1, executor2) => executor1.ActionTime.CompareTo(executor2.ActionTime));
        }

        private static void LoadCommonSettings(XDocument xmlSettings)
        {   
            _actionSchedulingInterval = GetParameterTimeSpan(xmlSettings, "RepairPolicyEngine", "ActionSchedulingIntervalInSeconds");
            _probationToFailing = GetParameterTimeSpan(xmlSettings, "NodeRepairPolicy", "ProbationToFailingWaitDurationInSeconds");
            //TimeSpan probationToHealthy = GetParameterTimeSpan(xmlSettings, "NodeRepairPolicy", "ProbationToHealthyWaitDurationInSeconds");
            _minimumHealthyDuration = GetParameterTimeSpan(xmlSettings, "NodeRepairPolicy", "MinimumHealthyDurationInSeconds");
        }

        private static TimeSpan GetParameterTimeSpan(XDocument document, string sectionName, string parameterName)
        {
            return TimeSpan.FromSeconds(Convert.ToInt32(GetParameter(document, sectionName, parameterName)));
        }

        private static string GetParameter(XDocument document, string sectionName, string parameterName)
        {
            var sectionQuery = from element in document.Descendants()
                               where element.Name.LocalName == "Section" && element.Attribute("Name").Value == sectionName
                               select element;
            var sections = sectionQuery.ToList();
            Assert.IsTrue(sections.Any() && sections.Count()==1, "There should be a single section with name {0}", sectionName);

            var parameterQuery = from element in sections.First().Descendants()
                        where element.Name.LocalName == "Parameter" && element.Attribute("Name").Value == parameterName
                        select element.Attribute("Value").Value;
            var parameters = parameterQuery.ToList();
            Assert.IsTrue(parameters.Any() && parameters.Count() == 1, "There should be a single parameter with name {0} under section {1}", parameterName, sectionName);
            return parameters.First();
        }

#endregion
    
    }
}