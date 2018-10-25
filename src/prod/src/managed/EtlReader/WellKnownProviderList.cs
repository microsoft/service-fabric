// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.Linq;

    public class WellKnownProviderList
    {
        // Array of well-known GUIDs. It includes Windows Fabric product and test GUIDs.
        // EtlReader has special behavior when processing events for these GUIDs.
        internal static readonly Guid[] WellKnownGuids = new Guid[]
        {
            new Guid("{cbd93bc2-71e5-4566-b3a7-595d8eeca6e8}"), // System.Fabric
            new Guid("{3f68b79e-a1cf-4b10-8cfd-3dfe322f07cb}"), // Lease
            new Guid("{E658F859-2416-4AEF-9DFC-4D303897A37A}"), // KTL
            new Guid("{05A7BE1B-AC8A-4920-B78E-6B371E1CCED2}"), // test GUID
            new Guid("{fc9e797f-436c-44c9-859e-502bbd654403}"), // test GUID
            new Guid("{6c955abf-b2ad-4236-a033-00f9c978e0e7}"), // test GUID
            new Guid("{C083AB76-A7F3-4CA7-9E64-CA7E5BA8807A}"), // DCA AzureTableUploader CIT
        };

        // Array of well-known GUIDs. It includes Windows Fabric programming model GUIDs
        // EtlReader has special behavior when processing events for these GUIDs.
        // Add new eventsource guids which you want to be processed/formatted via EtlReader
        internal static readonly Guid[] WellKnownDynamicProviderGuids = new Guid[]
        {
            new Guid("{e2f2656b-985e-5c5b-5ba3-bbe8a851e1d7}"),  // for Actors.
            new Guid("{27b7a543-7280-5c2a-b053-f2f798e2cbb7}"),  // for Services.
            new Guid("{c4766d6f-5414-5d26-48de-873499ff0976}"),  // for ServiceFabricHttpClientEventSource (SF-ClientLib)
            new Guid("{24afa313-0d3b-4c7c-b485-1047fd964b60}"),  // for SF Patch Orchestration Application CoordinatorService
            new Guid("{e39b723c-590c-4090-abb0-11e3e6616346}"),  // for SF Patch Orchestration Application NodeAgentService
            new Guid("{fc0028ff-bfdc-499f-80dc-ed922c52c5e9}"),  // for SF Patch Orchestration Application NodeAgentNTService
            new Guid("{05dc046c-60e9-4ef7-965e-91660adffa68}"),  // for SF Patch Orchestration Application NodeAgentSFUtility
            new Guid("{ecec9d7a-5003-53fa-270b-5c9f9cc66271}"),  // for ServiceFabric.ReliableCollection
            new Guid("{fa8ea6ea-9bf3-4630-b3ef-2b01ebb2a69b}"),  // for Microsoft.ServiceFabric.Internal
            new Guid("{74CF0846-E6A3-4a3e-A10F-80FD527DA5FD}"),  // for Azure Files Volume Driver
        };

        public static bool IsWellKnownProvider(Guid guid)
        {
            return WellKnownGuids.Contains(guid) || IsDynamicProvider(guid);
        }

        public static bool IsDynamicProvider(Guid guid)
        {
            return WellKnownDynamicProviderGuids.Contains(guid);
        }
    }
}