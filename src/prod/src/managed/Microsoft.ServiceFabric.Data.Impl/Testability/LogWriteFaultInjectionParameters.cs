// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Testability
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Text;

    internal class LogWriteFaultInjectionParameters
    {
        public LogWriteFaultInjectionParameters()
        {
            this.Test_LogMinDelayIntervalMilliseconds = 0;
            this.Test_LogMaxDelayIntervalMilliseconds = 0;
            this.Test_LogDelayRatio = 0;
            this.Test_LogDelayProcessExitRatio = 0;
        }

        public int Test_LogMinDelayIntervalMilliseconds { get; set; }

        public int Test_LogMaxDelayIntervalMilliseconds { get; set; }

        public double Test_LogDelayRatio { get; set; }

        public double Test_LogDelayProcessExitRatio { get; set; }

        public override string ToString()
        {
            var builder = new StringBuilder();
            builder.AppendFormat(Environment.NewLine);
            builder.AppendFormat("Test_LogMinDelayIntervalMilliseconds = {0}" + Environment.NewLine, this.Test_LogMinDelayIntervalMilliseconds);
            builder.AppendFormat("Test_LogMaxDelayIntervalMilliseconds = {0}" + Environment.NewLine, this.Test_LogMaxDelayIntervalMilliseconds);
            builder.AppendFormat("Test_LogDelayRatio = {0}" + Environment.NewLine, this.Test_LogDelayRatio);
            builder.AppendFormat("Test_LogDelayProcessExitRatio = {0}" + Environment.NewLine, this.Test_LogDelayProcessExitRatio);

            return builder.ToString();
        }
    }
}