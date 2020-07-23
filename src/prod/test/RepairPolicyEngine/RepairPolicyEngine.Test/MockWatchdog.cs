// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Linq;
using System.Threading;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Fabric;
using System.Fabric.Health;
using System.Fabric.Test;

namespace RepairPolicyEngine.Test
{
    class MockWatchdog
    {
        private static readonly TimeSpan HealthUpdateDelayTime = TimeSpan.FromSeconds(0.5);
        private const string Source = "RepairPolicyEngine.Test";

        public string NodeName { get; private set; }
        private readonly FabricClient.HealthClient _healthManager;
        public FabricClient.HealthClient HealthManager
        {
            get { return _healthManager; }
        }

        public MockWatchdog(FabricClient.HealthClient healthManager, string nodeName)
        {
            _healthManager = healthManager;
            NodeName = nodeName;
        }

        public void ReportOk(string property)
        {
            ReportHealth(property, HealthState.Ok);
        }

        public void ReportError(string property, string description = Source)
        {
            ReportHealth(property, HealthState.Error, description);
        }

        private void ReportHealth(string property, HealthState state, string description = Source)
        {
            LogHelper.Log("Report health for node={0}; source={1};property={2};state={3};description={4}", NodeName, Source, property, state.ToString(), description);
            HealthInformation healthInformation = new HealthInformation(Source, property, state);
            healthInformation.Description = description;
            HealthReport healthReport = new NodeHealthReport(NodeName, healthInformation);
            try
            {
                HealthManager.ReportHealth(healthReport);
            }
            catch (Exception exception)
            {
                Assert.Fail("Report health throws exception {0}", exception.Message);
            }

            bool healthEventUpdated = false;
            while (!healthEventUpdated)
            {
                Thread.Sleep(HealthUpdateDelayTime);
                NodeHealth nodeHealth = HealthManager.GetNodeHealthAsync(NodeName).Result;
                var eventQuery = from healthEvent in nodeHealth.HealthEvents
                                 where healthEvent.HealthInformation.SourceId == Source &&
                                       healthEvent.HealthInformation.Property == property &&
                                       healthEvent.HealthInformation.HealthState == state
                                 select healthEvent;
                healthEventUpdated = eventQuery.Any();
            }
        }

        public void ClearError()
        {
            NodeHealth nodeHealth = HealthManager.GetNodeHealthAsync(NodeName).Result;
            var eventQuery = from healthEvent in nodeHealth.HealthEvents
                             where healthEvent.HealthInformation.SourceId == Source &&
                                   healthEvent.HealthInformation.HealthState == HealthState.Error
                             select healthEvent;
            foreach (var healthEvent in eventQuery)
            {
                ReportOk(healthEvent.HealthInformation.Property);
            }
        }
    }
}