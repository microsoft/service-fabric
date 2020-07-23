// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/* 
   Use this sample to write tests for System.Fabric.Management.Test project.
*/

namespace System.Fabric.Management.Test
{
    //Define all headers
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Sample Template for System.Fabric.Management.Test project
    /// </summary>
    [TestClass]
    public class ClassName
    {
        //Any constants should be defined here

        /// <summary>
        /// Test setup information.
        /// </summary>
        [TestInitialize]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestSetup()
        {
            
        }
		
        /// <summary>
        /// Test summaries go here.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCase_1()
        {
			
        }

        /// <summary>
        /// Test summaries go here.
        /// </summary>
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestCase_2()
        {
            
        }
		
        /// <summary>
        /// Test clean up information.
        /// </summary>
        [TestCleanup]
        [TestProperty("ThreadingModel", "MTA")]
        public void Cleanup()
        {
			
        }
		
        /// <summary>
        /// Any helper method that needs to be called by Test Methods.
        /// </summary>
        public void HelperMethod_1()
        {
		
        }
		
        /// <summary>
        /// Any helper method that needs to be called by Test Methods.
        /// </summary>
        public void HelperMethod_2()
        {
	
        }
    }
}