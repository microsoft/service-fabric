// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Autopilot
{
    using System.Diagnostics;
    using System.Text;

    internal class ProcessStandardOutputHandler
    {
        private readonly StringBuilder standardOutput = new StringBuilder();
        private readonly StringBuilder standardError = new StringBuilder();

        public string StandardOutput { get { return this.standardOutput.ToString(); } }

        public string StandardError { get { return this.standardError.ToString(); } }

        public void StandardOutputHandler(Object sender, DataReceivedEventArgs e)
        {
            if (e!= null && !string.IsNullOrEmpty(e.Data))
            {
                if (this.standardOutput.Length > 0)
                {
                    this.standardOutput.Append(Environment.NewLine);
                }

                this.standardOutput.Append(e.Data);
            }
        }

        public void StandardErrorHandler(Object sender, DataReceivedEventArgs e)
        {
            if (e!=null && !string.IsNullOrEmpty(e.Data))
            {
                if (this.standardError.Length > 0)
                {
                    this.standardError.Append(Environment.NewLine);
                }

                this.standardError.Append(e.Data);
            }
        }
    }
}