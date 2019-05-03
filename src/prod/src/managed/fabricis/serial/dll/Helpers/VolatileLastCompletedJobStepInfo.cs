// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;
    using Globalization;

    /// <summary>
    /// Class that maintains state about the last CompleteJobStep notification and the action that was send to 
    /// MR (and finally to Fabric Controller - FC).
    /// Note: 
    /// The last notification detail is held in memory and is lost when IS fails over to another node/VM.
    /// This in-memory implementation is mainly used as a quick workaround to avoid operator's (person) 
    /// intervention whenever the last notification is lost (which usually result in delays in livesite mitigation). 
    /// So, this workaround helps as long as IS doesn't failover to a new node/VM.
    /// </summary>    
    /// <remarks>
    /// TODO - create an interface, a factory (and a factory interface) and pass the factory interface in the constructor to the 
    /// <see cref="WindowsAzureInfrastructureCoordinator"/>. This allows controlling the coordinator's workflow better.
    /// </remarks>
    internal sealed class VolatileLastCompletedJobStepInfo
    {
        private static readonly TraceType TraceType = new TraceType(typeof(VolatileLastCompletedJobStepInfo).Name);

        private readonly object locker = new object();
        
        public VolatileLastCompletedJobStepInfo(TimeSpan updateThreshold)
        {
            this.UpdateThreshold = updateThreshold;
        }

        public string JobId { get; private set; }

        public int JobStepTargetUD { get; private set; }

        public NotificationAction Action { get; private set; }

        public string Detail { get; private set; }

        public DateTimeOffset UpdateTime { get; private set; }

        public TimeSpan UpdateThreshold { get; private set; }

        /// <summary>
        /// Updates the last notification details with the latest info.
        /// This update is useful for checking if a re-ack is needed via the <see cref="QualifiesForReAck"/> method 
        /// when SignalReady (or SignalError) is called repeatedly.
        /// Please ensure that this method is invoked only after a SignalReady or a SignalError for a notification type of
        /// CompleteJobStep.
        /// </summary>
        public void Update(
            string newJobId,
            int newUD,
            NotificationType newNotificationType,
            NotificationAction newAction,
            string newDetail = null)
        {
            newJobId.ThrowIfNullOrWhiteSpace("newJobId");

            lock (locker)
            {
                var prevJobId = this.JobId;
                var prevUD = this.JobStepTargetUD;
                var prevAction = this.Action;
                var prevDetail = this.Detail;

                Trace.WriteInfo(
                    TraceType,
                    "Updating VolatileLastCompletedNotification. {0}, {1}",
                    ToTraceString("Current", this.JobId, this.JobStepTargetUD, this.Action, this.Detail),
                    ToTraceString("New", newJobId, newUD, newAction, newDetail));

                this.JobId = newJobId;
                this.JobStepTargetUD = newUD;
                this.Action = newAction;
                this.Detail = newDetail;

                this.UpdateTime = DateTimeOffset.UtcNow;

                Trace.WriteInfo(
                    TraceType,
                    "Updated VolatileLastCompletedNotification at {0:o}. {1}, {2}",
                    this.UpdateTime,
                    ToTraceString("Previous", prevJobId, prevUD, prevAction, prevDetail),
                    ToTraceString("Current", this.JobId, this.JobStepTargetUD, this.Action, this.Detail));
            }
        }

        /// <summary>
        /// Determines if a notfication qualifies for an re-ack (i.e. another SignalReady or SignalError being sent to FC). 
        /// This assessment could be ignored and <see cref="Update"/> could still be invoked successfully.
        /// What scenarios does this happen?
        /// 1. Enough time hasnt elapsed to reack. (Post-UD notifications take a while to disappear on FC side)
        /// 2. This is really an "unmatched post". A new post shows up out of nowhere.
        /// 3. Somehow the job id/UDs dont match.
        /// 4. Failover where we not longer have the record of the last notification. 
        /// 5. The ack is just not being accepted (node is in HI).
        /// </summary>
        public bool QualifiesForReAck(IManagementNotificationContext other)
        {
            if (!other.Matches(this.JobId, this.JobStepTargetUD) || other.NotificationType != NotificationType.CompleteJobStep)
            {
                Trace.WriteInfo(
                    TraceType,
                    "Notification does not qualify for a CompleteJobStep re-ack since there is no existing match. JobId/UD/type: {0}/{1}/{2}",                    
                    other.ActiveJobId, 
                    other.ActiveJobStepTargetUD,
                    other.NotificationType);

                return false;
            }

            if (IsRecentlyUpdated())
            {
                Trace.WriteInfo(
                    TraceType,
                    "Notification does not qualify for CompleteJobStep re-ack since an update happened recently. Last update time: {0}, JobId/UD: {1}/{2}",
                    this.UpdateTime,
                    other.ActiveJobId, 
                    other.ActiveJobStepTargetUD);

                return false;
            }

            Trace.WriteInfo(
                TraceType,
                "Notification qualifies for CompleteJobStep re-ack. JobId/UD: {0}, {1}",
                this.UpdateTime,
                other.ActiveJobId, other.ActiveJobStepTargetUD);

            return true;
        }

        private bool IsRecentlyUpdated()
        {
            bool recentlyUpdated = (DateTimeOffset.UtcNow - this.UpdateTime < UpdateThreshold);
            return recentlyUpdated;
        }

        private static string ToTraceString(string prefix, string jobId, int ud, NotificationAction action, string detail)
        {
            string text = string.Format(
                CultureInfo.InvariantCulture, "{0} JobId/UD/Action/Detail: {1}/{2}/{3}/{4}", prefix, jobId, ud, action, detail);
            return text;
        }
    }
}