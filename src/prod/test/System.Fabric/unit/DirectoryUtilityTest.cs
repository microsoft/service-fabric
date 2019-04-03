// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.IO;
    using System.Fabric.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Diagnostics;

    [TestClass]
    public class DirectoryUtilityTest
    {
        private readonly string BadPath = @"BadDrive:\BadPath";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void DirectoryUtility_CreateNegative()
        {
            try
            {
                LogHelper.Log("DirectoryUtility.Create {0}", BadPath);
                DirectoryUtility.Create(BadPath);
                Assert.Fail("should never reach here");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void DirectoryUtility_CopyNegative()
        {
            try
            {
                var src = Environment.ExpandEnvironmentVariables(@"%_NTTREE%\FabricDrop");
                LogHelper.Log("DirectoryUtility.Copy from {0} to {1}", src, BadPath);
                DirectoryUtility.Copy(src, BadPath, true);
                Assert.Fail("should never reach here");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("biqian")]
        public void DirectoryUtility_DeleteNegative()
        {
            try
            {
                LogHelper.Log("DirectoryUtility.Delete {0}", BadPath);
                DirectoryUtility.Delete(BadPath, true, false);
                Assert.Fail("should never reach here");
            }
            catch (Exception e)
            {
                LogHelper.Log("caught exception {0}", e);
                Assert.IsTrue(e is IOException);
            }
        }

    }
}