// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    static class LttProducerConstants
    {
        internal const string EnabledParamName = "IsEnabled";
        internal const string LttReadIntervalParamName = "LttReadIntervalInMinutes";
        internal const string LttTimerPrefix = "_LttTimer";
        internal const int DefaultLttReadIntervalMinutes = 10;
        internal const int minRecommendedLttReadInterval = 5;
        internal const string LttSubDirectoryUnderLogDirectory = "lttng-traces";
        internal const string LttTraceSessionFolderNamePrefix = "fabric_traces_";
        internal const string DtrSubDirectoryUnderLogDirectory = "Traces";

        internal const string ServiceFabricLttTypeParamName = "ServiceFabricLttType";
        internal const string QueryLtt = "QueryLtt";
        internal const string DefaultLtt = "DefaultLtt";
    }
}