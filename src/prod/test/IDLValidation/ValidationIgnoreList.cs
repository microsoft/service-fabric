// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;

namespace System.Fabric.Setup
{
    // Why this file exists ?
    // Service Fabric NetStandard(NS) SDK on CoreCLR runtime deviated from
    // previous .Net4.5 SDK.
    // In NS SDK with CoreCLR, System.Fabric.dll is picked from application
    // folder instead of picking from runtime. This broke the assumption that
    // internal IDL and managed components will always be shipped together.
    // This caused internal IDL to be treated same as public IDL and required
    // atleast 2 future versions support so applications do not break.
    // IDL Validation test was updated to verify internal IDLs too for breaking
    // changes.
    // Now there are a few exceptions where IDL components do not have a
    // managed counterpart; so the breaking changes in them are acceptable.
    //
    // As a short-term solution, this file is being added for such native
    // components.
    // For long-term solution, this test will be updated to handle this
    // scenario in a more generic way.
    
    internal static class ValidationIgnoreList
    {
        internal static Dictionary<string, int> ValidationIgnoreListInterfaces
          = new Dictionary<string, int> {
              };
        internal static Dictionary<string, int> ValidationIgnoreListStructs
          = new Dictionary<string, int> {
              { "TRANSACTIONAL_REPLICATOR_SETTINGS", 1}
              };
    }
}