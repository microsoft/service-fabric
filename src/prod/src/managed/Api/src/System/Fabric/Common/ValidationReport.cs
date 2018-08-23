// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Chaos.Common;

    internal sealed class ValidationReport
    {
        public static readonly ValidationReport Default = new ValidationReport(false, string.Empty);

        private readonly bool validationFailed;

        private readonly string failureReason;

        public ValidationReport(bool failed, string reason)
        {
            ThrowIf.IsTrue(failed && string.IsNullOrEmpty(reason), "If validation fails, you must provide a reason for the failure.");

            this.validationFailed = failed;
            this.failureReason = reason;
        }

        public bool ValidationFailed
        {
            get
            {
                return this.validationFailed;
            }
        }

        public string FailureReason
        {
            get
            {
                return this.failureReason;
            }
        }
    }
}