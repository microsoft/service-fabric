// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using RD.Fabric.PolicyAgent;

namespace System.Fabric.InfrastructureService.Parallel.ManualControl
{
    /// <summary>
    /// A tool for manually controlling the Azure PolicyAgent.
    /// </summary>
    public static class Program
    {
        private const string ToolName = "FabricInfrastructureManualControl";
        private static readonly TimeSpan MaximumAutoApproveRunDuration = TimeSpan.FromHours(8);
        private static readonly TimeSpan JobPollingInterval = TimeSpan.FromSeconds(5);

        private static readonly Dictionary<string, Func<Task>> commandHandlerMap = new Dictionary<string, Func<Task>>(StringComparer.OrdinalIgnoreCase)
        {
            { "ApproveAllJobs", ApproveAllJobsAsync },
        };

        public static int Main(string[] args)
        {
            Trace.ConsoleOutputEnabled = false;

            Func<Task> commandHandler;
            if ((args.Length < 1) ||
                !commandHandlerMap.TryGetValue(args[0], out commandHandler))
            {
                Console.WriteLine("Usage: {0} ApproveAllJobs", ToolName);
                return 1;
            }

            commandHandler().GetAwaiter().GetResult();

            return 0;
        }

        private sealed class NullConfigSection : IConfigSection
        {
            public string Name
            {
                get { return "FabricInfrastructureManualControl"; }
            }

            public bool SectionExists
            {
                get { return true; }
            }

            public T ReadConfigValue<T>(string keyName, T defaultValue = default(T))
            {
                return defaultValue;
            }
        }

        private static async Task ApproveAllJobsAsync()
        {
            Console.WriteLine(
                "{0:O}: Starting to approve all Azure jobs (max run time = {1})",
                DateTime.UtcNow,
                MaximumAutoApproveRunDuration);

            Stopwatch watch = Stopwatch.StartNew();

            try
            {
                // Limit maximum run time to avoid inadvertently leaving this running and auto-approving jobs forever
                while (watch.Elapsed < MaximumAutoApproveRunDuration)
                {
                    PolicyAgentClient policyAgentClient = null;

                    try
                    {
                        policyAgentClient = CreatePolicyAgentClient();
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine("ERROR: {0}", e.Message);
                    }

                    if (policyAgentClient == null)
                    {
                        Console.WriteLine("ERROR: Azure infrastructure coordination channel not enabled on this VM. Check tenant configuration or try running this tool on another VM.");
                        return;
                    }

                    try
                    {
                        await ApproveAllJobsOnceAsync(policyAgentClient);
                    }
                    catch (Exception e)
                    {
                        Console.WriteLine("ERROR: {0}", e.Message);
                    }

                    await Task.Delay(JobPollingInterval);
                }

                Console.WriteLine(
                    "{0:O}: Exiting because max run time has elapsed ({1})",
                    DateTime.UtcNow,
                    MaximumAutoApproveRunDuration);
            }
            catch (Exception e)
            {
                Console.WriteLine("ERROR: {0}", e.Message);
            }
        }

        private static PolicyAgentClient CreatePolicyAgentClient()
        {
            CoordinatorEnvironment env = new CoordinatorEnvironment(
                ToolName,
                new NullConfigSection(),
                string.Empty,
                null);

            var traceType = new TraceType(ToolName);
            var activityLogger = new ActivityLoggerFactory().Create(traceType);

            var policyAgentServiceWrapper = new PolicyAgentServiceWrapper(env, activityLogger, new Uri("http://169.254.169.254:80/mrzerosdk"));
            var policyAgentClient = new PolicyAgentClient(env, policyAgentServiceWrapper, activityLogger);
            return policyAgentClient;
        }

        private static async Task ApproveAllJobsOnceAsync(PolicyAgentClient policyAgentClient)
        {
            IPolicyAgentDocumentForTenant doc = null;
            try
            {
                doc = await policyAgentClient.GetDocumentAsync(Guid.Empty, CancellationToken.None);
            }
            catch (Exception e)
            {
                // PA client can return huge multi-line error messages. Most of that info is not necessary.
                string msgFirstLine = new StringReader(e.Message ?? string.Empty).ReadLine();

                throw new InvalidOperationException(
                    $"Failed to get job information. If you continue to receive this error, check tenant configuration or try running this tool on another VM. {msgFirstLine}",
                    e);
            }

            if (doc == null)
            {
                // Parallel job mode is not enabled
                throw new InvalidOperationException("Failed to get job information. Check tenant configuration.");
            }

            Dictionary<Guid, JobStepResponseEnum> jobResponses = new Dictionary<Guid, JobStepResponseEnum>();

            // Log all incomplete jobs, and record the ID of any that are waiting for tenant approval.
            foreach (var job in doc.Jobs.Where(j => j.JobStatus != JobStatusEnum.Completed))
            {
                bool approve = IsJobWaitingForApproval(job);

                if (approve)
                {
                    jobResponses.Add(job.Id, JobStepResponseEnum.Acknowledged);
                }

                Console.WriteLine(
                    "{0:O}: Job {1} ({2}/{3}/UD{4}/{5}/{6}): {7}",
                    DateTime.UtcNow,
                    job.Id,
                    job.GetImpactAction(),
                    job.JobStatus,
                    job.JobStep == null ? "-" : string.Join(",", job.JobStep.CurrentlyImpactedRoleInstances.Select(r => r.UpdateDomain).Distinct().OrderBy(ud => ud)),
                    job.JobStep == null ? "-" : job.JobStep.ImpactStep.ToString(),
                    job.JobStep == null ? "-" : job.JobStep.AcknowledgementStatus.ToString(),
                    approve ? "Sending acknowledgement" : "Acknowledgement not needed in this state");
            }

            if (jobResponses.Count > 0)
            {
                // Send all job responses (if any) as a single batch response
                await policyAgentClient.SendJobResponseAsync(
                    Guid.Empty,
                    doc.JobDocumentIncarnation,
                    jobResponses,
                    $"Manually approved by {ToolName}",
                    CancellationToken.None);

                Console.WriteLine(
                    "{0:O}: Approved job count = {1}, incarnation = {2}",
                    DateTime.UtcNow,
                    jobResponses.Count,
                    doc.JobDocumentIncarnation);
            }
            else
            {
                Console.WriteLine(
                    "{0:O}: No jobs to approve",
                    DateTime.UtcNow);
            }
        }

        private static bool IsJobWaitingForApproval(ITenantJob job)
        {
            return
                (job.JobStatus == JobStatusEnum.Executing) &&
                (job.JobStep != null) &&
                (job.JobStep.AcknowledgementStatus == AcknowledgementStatusEnum.WaitingForAcknowledgement);
        }
    }
}