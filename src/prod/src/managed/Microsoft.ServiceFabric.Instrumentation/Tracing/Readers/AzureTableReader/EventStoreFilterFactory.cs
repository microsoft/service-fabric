// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CM;

    public static class EventStoreFilterFactory
    {
        #region RecordLevelFilter

        public static TraceRecordFilter CreateApplicationRecordFilter(Uri applicationName)
        {
            return new TraceRecordFilter(typeof(ApplicationCreatedTraceRecord), CreateApplicatioIdProperyFilter(applicationName));
        }

        #endregion RecordLevelFilter

        #region PropertyLevelFilter

        public static PropertyFilter CreatePartitionIdPropertyFilter(Guid value)
        {
            return new PropertyFilter(WellKnownFields.PartitionId, value.ToString());
        }

        public static PropertyFilter CreateReplicaIdPropertyFilter(long replicaId)
        {
            return new PropertyFilter(WellKnownFields.ReplicaId, replicaId.ToString());
        }

        public static PropertyFilter CreateApplicatioIdProperyFilter(Uri applicationName)
        {
            return new PropertyFilter(WellKnownFields.ApplicationId, applicationName.ToString());
        }

        public static PropertyFilter CreateServiceIdPropertyFilter(Uri serviceName)
        {
            return new PropertyFilter(WellKnownFields.ServiceId, serviceName.ToString());
        }

        public static PropertyFilter CreateNodeNamePropertyFilter(string nodeName)
        {
            return new PropertyFilter(WellKnownFields.NodeName, nodeName);
        }

        #endregion PropertyLevelFilter
    }
}