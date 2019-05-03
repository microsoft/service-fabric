// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeOrchestration.Service
{
    internal enum UpgradeFlow
    {
        RollingForward = 0,

        RollingBack = 1
    }

    internal class FaultInjectionConfig
    {
        public FaultInjectionConfig(UpgradeFlow faultFlow, int faultStep)
        {
            this.FaultFlow = faultFlow;
            this.FaultStep = faultStep;
        }

        public UpgradeFlow FaultFlow { get; set; }

        public int FaultStep { get; set; }

        public override bool Equals(object obj)
        {
            if (obj == null || obj.GetType() != typeof(FaultInjectionConfig))
            {
                return false;
            }

            if (obj == this)
            {
                return true;
            }

            FaultInjectionConfig other = obj as FaultInjectionConfig;
            return other.FaultFlow == this.FaultFlow
                && other.FaultStep == this.FaultStep;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public override string ToString()
        {
            return string.Format("{0}:{1}", this.FaultFlow, this.FaultStep);
        }
    }
}