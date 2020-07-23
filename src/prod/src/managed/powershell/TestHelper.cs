// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    /// class TestHelper would help in testing cmdlet implementation.
    /// It will have default constants, methods which can be used for any Test class.
    /// method to validate results/exception
    /// method to init MockClusterConnection
    public class TestHelper
    {
        public string cmdletScript
        {
            get;
            internal set;
        }

        public MockClusterConnection mockClusterConnection
        {
            get;
            internal set;
        }

        public TestExceptionHelper testExceptionHelper
        {
            get;
            internal set;
        }

        /// Constructor
        public TestHelper()
        {
            this.SetDefaultValues();
        }

        public void SetException(Exception expected)
        {
            this.testExceptionHelper = new TestExceptionHelper(expected);
        }

        public virtual bool InvokeCmdletAndVerify(MockClusterConnection givenMockClusterConnection, string givenScript)
        {
            try
            {
                TestUtility.ExecuteScript(givenMockClusterConnection, givenScript);
            }
            catch (CmdletInvocationException actual)
            {
                if (this.testExceptionHelper == null)
                {
                    /// If the exception is not set from MockClusterConnection, passing it upstream to be handled later.
                    throw actual;
                }
                else
                {
                    this.testExceptionHelper.VerifyException(actual);
                }
            }

            return true;
        }

        public virtual bool InvokeCmdletAndVerify()
        {
            return this.InvokeCmdletAndVerify(this.mockClusterConnection, this.cmdletScript);
        }

        public virtual void SetDefaultValues()
        {
            this.cmdletScript = "";
            this.mockClusterConnection = new MockClusterConnection();
            this.testExceptionHelper = null;
        }
    }
}