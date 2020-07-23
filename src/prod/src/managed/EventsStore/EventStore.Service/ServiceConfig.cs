// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
{
    using System;

    internal class ServiceConfig
    {
        public const string EventStoreServiceSectionName = "EventStoreService";
        public const string PlacementConstraintsParamName = "PlacementConstraints";
        public const string TargetReplicaSetSizeParamName = "TargetReplicaSetSize";
        public const string MinReplicaSetSizeParamName = "MinReplicaSetSize";
    }
}
