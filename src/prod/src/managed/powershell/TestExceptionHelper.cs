// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Management.Automation;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;

    /// <summary>
    /// This class will help comparing exception returned and expected.
    /// </summary>
    public class TestExceptionHelper
    {
        public Type exceptionType
        {
            get;
            private set;
        }

        public string errorMessage
        {
            get;
            private set;
        }

        public Exception exception
        {
            get;
            private set;
        }

        public ErrorCategory errorCategory
        {
            get;
            private set;
        }

        public TestExceptionHelper(Exception exception)
        {
            this.exceptionType = exception.GetType();
            this.errorMessage = exception.Message;
            this.exception = exception;
            this.errorCategory = CommonCmdletBase.GetErrorCategoryForException(exception);
        }

        /// <summary>
        /// Verifies that actual exception is is not null,
        /// same type as original provided,
        /// has same errorMessage as original provided
        /// </summary>
        public void VerifyException(CmdletInvocationException cmdletExp)
        {
            /// Verify type of returned exception is same expected.
            var actualException = this.VerifyExceptionType(cmdletExp);

            /// Verify that returned ErrorCategory is the same as expected.
            Verify.AreEqual(
                this.errorCategory,
                cmdletExp.ErrorRecord.CategoryInfo.Category);

            /// Verify that error message in actualException the same as the original one.
            Verify.AreEqual(this.errorMessage, actualException.Message);
        }

        /// <summary>
        /// Verifies that actual exception is is not null,
        /// same type as original provided,
        /// has same errorMessage as original provided
        /// </summary>
        public Exception VerifyExceptionType(CmdletInvocationException cmdletExp)
        {
            /// Verify type of returned exception is same expected.
            var actualException = cmdletExp.ErrorRecord.Exception;
            Assert.IsNotNull(actualException);
            Assert.IsInstanceOfType(actualException, this.exceptionType);
            return actualException;
        }
    }
}