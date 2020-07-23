// ------------------------------------------------------------
//  Copyright (c) Microsoft Corporation.  All rights reserved.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder.SingleInstance
{
    using System.Collections.Generic;
    using System.IO;

    internal class AutoScalingMetric
    {
        public AutoScalingMetric()
        {
            this.kind = null;
            this.name = null;
        }

        public string kind;
        public string name;
    }

    internal class ScalingTrigger
    {
        public ScalingTrigger()
        {
            this.kind = null;
            this.metric = null;
            this.lowerLoadThreshold = null;
            this.upperLoadThreshold = null;
            this.scaleIntervalInSeconds = null;
        }

        public string kind;
        public AutoScalingMetric metric;
        public string lowerLoadThreshold;
        public string upperLoadThreshold;
        public string scaleIntervalInSeconds;
    }

    internal class ScalingMechanism
    {
        public ScalingMechanism()
        {
            this.kind = null;
            this.minCount = null;
            this.maxCount = null;
            this.scaleIncrement = null;
        }

        public string kind;
        public string minCount;
        public string maxCount;
        public string scaleIncrement;
    }

    internal class ScalingPolicy
    {
        public ScalingPolicy()
        {
            this.name = null;
            this.trigger = null;
            this.mechanism = null;
        }
        
        public string name;
        public ScalingTrigger trigger;
        public ScalingMechanism mechanism;
    };
}
