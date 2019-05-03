// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    internal class ActivityLoggerFactory : IActivityLoggerFactory
    {
        public IActivityLogger Create(TraceType traceType)
        {
            return new TraceActivitySubject(traceType.Validate("traceType"));
        }
    }
}