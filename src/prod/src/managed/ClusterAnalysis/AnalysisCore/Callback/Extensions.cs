// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback
{
    using System;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    internal static class Extensions
    {
        public static Guid GetUniqueId(this TraceRecord record)
        {
            if (record.ObjectInstanceId != null)
            {
                return record.ObjectInstanceId.Id;
            }

            unchecked
            {
                long hash = 17;
                hash = (hash * 23) + record.TraceId.GetHashCode();
                hash = (hash * 23) + record.TimeStamp.Ticks.GetHashCode();
                hash = (hash * 23) + record.ThreadId.GetHashCode();
                return ConvertToGuid(hash);
            }
        }

        private static Guid ConvertToGuid(long value)
        {
            byte[] bytes = new byte[16];
            BitConverter.GetBytes(value).CopyTo(bytes, 0);
            return new Guid(bytes);
        }
    }
}