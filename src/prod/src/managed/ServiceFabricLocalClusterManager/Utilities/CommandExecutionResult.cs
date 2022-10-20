// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LocalClusterManager.Utilities
{
    using System;

    internal class CommandExecutionResult
    {
        #region C'tor

        public CommandExecutionResult()
        {
            this.ExitCode = -1;
            this.CommandText = String.Empty;
            this.outputBuilder = new ThreadSafeStringBuilder();
        }

        #endregion

        #region Public Methods

        public void AppendOutput(String str)
        {
            this.outputBuilder.Append(str);
        }

        public void AppendOutputLine(String str)
        {
            this.outputBuilder.AppendLine(str);
        }

        public void AppendOutputLine()
        {
            this.outputBuilder.AppendLine();
        }

        #endregion

        #region Public Properties

        public String CommandText { get; set; }

        public int ExitCode { get; set; }

        public String Output
        {
            get { return this.outputBuilder.ToString(); }
        }

        #endregion

        #region Private Properties

        private ThreadSafeStringBuilder outputBuilder;

        #endregion
    }
}