// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{    
    using Globalization;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;
    using Common;

    /// <summary>
    /// A concrete implementation of <see cref="IManagementJobWrapper"/> for wrapping
    /// jobs from Azure.
    /// </summary>
    internal class WindowsAzureManagementJobWrapper : IManagementJobWrapper
    {
        private static readonly TraceType TraceType = new TraceType("WindowsAzureManagementJobWrapper");

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsAzureManagementJobWrapper"/> class.
        /// </summary>
        /// <param name="job">The job.</param>
        /// <exception cref="System.ArgumentNullException">job</exception>
        public WindowsAzureManagementJobWrapper(ManagementJob job)
        {
            if (job == null)
                throw new ArgumentNullException("job");

            Job = job;
            Id = job.Id;            
            Status = job.Status;
            Type = job.GetJobType();
            DetailedStatus = job.DetailedStatus;
            ContextId = job.ContextId;
        }

        /// <summary>
        /// Gets the underlying Azure job that the current class wraps.
        /// </summary>
        public ManagementJob Job { get; private set; }

        /// <summary>
        /// Gets the job Id.
        /// </summary>
        public string Id { get; private set; }

        /// <summary>
        /// Gets the status of the job.
        /// </summary>
        public ManagementJobStatus Status { get; private set; }

        /// <summary>
        /// Gets the type of the job as an enumeration.
        /// </summary>
        public JobType Type { get; private set; }

        /// <summary>
        /// Detailed status about the job.
        /// </summary>
        public ManagementJobDetailedStatus DetailedStatus { get; private set; }

        /// <summary>
        /// Context Id of the job.
        /// </summary>
        public string ContextId { get; private set; }
        /// <summary>
        /// Request resumption of a suspended RootHE (HE = Hosting Environment) job 
        /// This method only works for jobs of type <see cref="Microsoft.WindowsAzure.ServiceRuntime.Management.PlatformUpdateJob"/>.
        /// Also, this feature is applicable for privileged tenants that own the node. Only such tenants can resume a suspended RootHE job.
        /// </summary>
        /// <seealso cref="RequestSuspend"/>
        public void RequestResume()
        {
            const string ActionName = "RequestResume";
            PlatformUpdateJob job = GetPlatformUpdateJob(ActionName);
            InvokePlatformUpdateAction(job.RequestResume, ActionName);
        }

        /// <summary>
        /// Request suspension of a RootHE (HE = Hosting Environment) job 
        /// from the MR SDK. If the request succeeds, the related job will change state to be Suspended. 
        /// This suspension will be held for a predefined time unless a resumption request  
        /// comes from the tenant. After this time passes, the related RootHE job is alerted. 
        /// By this capability, tenants would be able to postpone a RootHE job in order 
        /// to save time for tenant update, etc. before infrastructure update.
        /// This method only works for jobs of type <see cref="Microsoft.WindowsAzure.ServiceRuntime.Management.PlatformUpdateJob"/>.
        /// Also, this feature is applicable for privileged tenants that own the node. Only such tenants can suspend a RootHE.
        /// </summary>
        /// <seealso cref="RequestResume"/>
        public void RequestSuspend()
        {
            const string ActionName = "RequestSuspend";
            PlatformUpdateJob job = GetPlatformUpdateJob(ActionName);
            InvokePlatformUpdateAction(job.RequestSuspend, ActionName);

        }

        public override string ToString()
        {
            string text = string.Format(
                CultureInfo.InvariantCulture,
                "Job Id: {0}, status: {1}, type: {2}, detailed status: {3}, context Id: {4}",
                Id, Status, Type, DetailedStatus, ContextId);

            return text;
        }

        private PlatformUpdateJob GetPlatformUpdateJob(string actionName)
        {
            var platformUpdateJob = Job as PlatformUpdateJob;

            if (platformUpdateJob == null)
            {
                string message = string.Format(
                    CultureInfo.InvariantCulture, 
                    "Invalid command arguments. Job Id '{0}' is not of type PlatformUpdateJob. {1} cannot be invoked on this job type.", 
                    Id, 
                    actionName);

                Trace.WriteWarning(TraceType, message);
                throw new InvalidOperationException(message);
            }

            return platformUpdateJob;
        }

        private void InvokePlatformUpdateAction(Action action, string actionName)
        {
            try
            {
                action();
            }
            catch (Exception e)
            {
                Trace.WriteWarning(TraceType, "{0} on job Id: {1} threw exception: {2}", actionName, Id, e);
                throw;
            }
        }
    }
}