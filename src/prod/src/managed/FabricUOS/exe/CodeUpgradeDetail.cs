// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    internal class CodeUpgradeDetail
    {
        public string CodeVersion
        {
            get;
            set;
        }

        public bool IsUserInitiated
        {
            get;
            set;
        }
    }
}
