// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    internal class MockupManifestVersionGenerator : IManifestVersionGenerator
    {
        public MockupManifestVersionGenerator()
        {
        
        }

        public string GetNextClusterManifestVersion()
        {
            return "2.0";
        }
    }
}