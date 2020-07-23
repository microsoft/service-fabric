// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace LocalClusterManager.Utilities
{
    using System;
    using System.Text;

    internal class ThreadSafeStringBuilder
    {
        #region C'tor

        public ThreadSafeStringBuilder()
        {
            this.strBuilder = new StringBuilder();
        }

        #endregion

        #region Public Methods

        public void Append(String str)
        {
            lock (this.strBuilder)
            {
                this.strBuilder.Append(str);
            }
        }

        public void AppendLine(String str)
        {
            lock (this.strBuilder)
            {
                this.strBuilder.AppendLine(str);
            }
        }

        public void AppendLine()
        {
            lock (this.strBuilder)
            {
                this.strBuilder.AppendLine();
            }
        }

        public override String ToString()
        {
            lock (this.strBuilder)
            {
                return this.strBuilder.ToString();
            }
        }

        #endregion

        #region Private Properties

        private StringBuilder strBuilder;

        #endregion
    }
}