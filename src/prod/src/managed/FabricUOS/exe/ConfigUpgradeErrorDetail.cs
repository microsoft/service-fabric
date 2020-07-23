// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    internal class ConfigUpgradeErrorDetail
    {
        public string ErrorMessage
        {
            get;
            set;
        }

        public int ErrorCode
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format("{0} : {1}", this.ErrorCode, this.ErrorMessage);
        }
    }
}
