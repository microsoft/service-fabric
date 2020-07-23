// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventsStore.EventsModels.Application
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.HM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Testability;

    public static class ApplicationEventAdapter
    {
        public static ApplicationEvent Convert(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord != null");
            if (traceRecord.GetType() == typeof(ApplicationCreatedTraceRecord))
            {
                return new ApplicationCreatedEvent((ApplicationCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationDeletedTraceRecord))
            {
                return new ApplicationDeletedEvent((ApplicationDeletedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationUpgradeStartTraceRecord))
            {
                return new ApplicationUpgradeStartEvent((ApplicationUpgradeStartTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationUpgradeCompleteTraceRecord))
            {
                return new ApplicationUpgradeCompleteEvent((ApplicationUpgradeCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationUpgradeDomainCompleteTraceRecord))
            {
                return new ApplicationUpgradeDomainCompleteEvent((ApplicationUpgradeDomainCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationUpgradeRollbackStartTraceRecord))
            {
                return new ApplicationUpgradeRollbackStartEvent((ApplicationUpgradeRollbackStartTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationUpgradeRollbackCompleteTraceRecord))
            {
                return new ApplicationUpgradeRollbackCompleteEvent((ApplicationUpgradeRollbackCompleteTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationHealthReportCreatedTraceRecord))
            {
                return new ApplicationHealthReportCreatedEvent((ApplicationHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ApplicationHealthReportExpiredTraceRecord))
            {
                return new ApplicationHealthReportExpiredEvent((ApplicationHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(DeployedApplicationHealthReportCreatedTraceRecord))
            {
                return new DeployedApplicationHealthReportCreatedEvent((DeployedApplicationHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(DeployedApplicationHealthReportExpiredTraceRecord))
            {
                return new DeployedApplicationHealthReportExpiredEvent((DeployedApplicationHealthReportExpiredTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ContainerExitedTraceRecord))
            {
                return new ContainerDeactivatedEvent((ContainerExitedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ProcessExitedTraceRecord))
            {
                return new ProcessDeactivatedEvent((ProcessExitedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(ChaosRestartCodePackageFaultScheduledTraceRecord))
            {
                return new ChaosRestartCodePackageFaultScheduledEvent((ChaosRestartCodePackageFaultScheduledTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(DeployedServiceHealthReportCreatedTraceRecord))
            {
                return new DeployedServiceHealthReportCreatedEvent((DeployedServiceHealthReportCreatedTraceRecord)traceRecord);
            }

            if (traceRecord.GetType() == typeof(DeployedServiceHealthReportExpiredTraceRecord))
            {
                return new DeployedServiceHealthReportExpiredEvent((DeployedServiceHealthReportExpiredTraceRecord)traceRecord);
            }

            throw new NotSupportedException(string.Format(CultureInfo.InvariantCulture, "App Event of Type: {0} Not supported", traceRecord.GetType()));
        }
    }
}
