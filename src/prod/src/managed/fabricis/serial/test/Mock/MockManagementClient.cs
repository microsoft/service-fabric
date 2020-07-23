// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using Common;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;

    /// <summary>
    /// A test class for mocking the Azure management client.
    /// </summary>
    internal class MockManagementClient : IManagementClient
    {
        private static readonly TraceType TraceType = new TraceType("TestManagementClient");

        public Func<IManagementNotificationContext> GetCurrentNotificationFunc { get; set; }
        public Func<IList<RoleInstance>> GetRoleInstancesFunc { get; set; }
        public Action<string, InfrastructureServiceMaintenanceAction, string, string> RequestMaintenanceAction { get; set; }
        public Func<IList<IManagementJobWrapper>> GetAllJobsFunc { get; set; }

        public IManagementNotificationContext GetCurrentNotification()
        {
            return GetCurrentNotificationFunc == null ? DefaultGetCurrentNotification() : GetCurrentNotificationFunc();
        }

        public IList<RoleInstance> GetRoleInstances()
        {
            return GetRoleInstancesFunc == null ? DefaultGetRoleInstances() : GetRoleInstancesFunc();
        }

        public void RequestMaintenance(string roleInstanceName, InfrastructureServiceMaintenanceAction action, string reason, string contextId)
        {            
            if (RequestMaintenanceAction == null)
                DefaultRequestMaintenanceAction(roleInstanceName, action, reason, contextId);
            else
                RequestMaintenanceAction(roleInstanceName, action, reason, contextId);
        }

        public IList<IManagementJobWrapper> GetAllJobs()
        {
            return GetAllJobsFunc == null ? DefaultGetAllJobs() : GetAllJobsFunc();
        }

        public ulong? GetJobListIncarnation()
        {
            return null;
        }

        public ulong? GetServiceHealthIncarnation()
        {
            return null;
        }

        private static IManagementNotificationContext DefaultGetCurrentNotification()
        {
            return new MockManagementNotificationContext();
        }

        private static IList<RoleInstance> DefaultGetRoleInstances()
        {
            const string RoleInstancePrefix = "TestWindowsFabricRole_IN_";

            var list = new List<RoleInstance>();
            for (int i = 0; i < 5; i++)
            {
                list.Add(new RoleInstance(RoleInstancePrefix + i, ManagementRoleInstanceStatus.ReadyRole.ToRoleInstanceState(), DateTime.UtcNow));
            }

            return list;
        }

        private static void DefaultRequestMaintenanceAction(string roleInstanceName,
            InfrastructureServiceMaintenanceAction action, string reason, string contextId)
        {
        }

        private static IList<IManagementJobWrapper> DefaultGetAllJobs()
        {
            return new List<IManagementJobWrapper>();
        }
    }
}