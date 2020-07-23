// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ServiceFabricUpdateService.Test
{
    using System;
    using System.Collections.Generic;
    using System.Collections.Specialized;
    using System.IO;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.ServiceFabricUpdateService;

    [TestClass]
    public class GoalStateTest
    {
        private const string DefaultGoalStateFileUrl = "https://go.microsoft.com/fwlink/?LinkID=824848&clcid=0x409";

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TryCreateTest()
        {
            GoalState result;
            Assert.IsTrue(GoalState.TryCreate(new Uri(GoalStateTest.DefaultGoalStateFileUrl, UriKind.Absolute), new TraceLogger(new MockUpTraceEventProvider()), out result));
            Assert.IsTrue(result.Packages.Any());

            Assert.IsFalse(GoalState.TryCreate(new Uri("http://6532A867-8DC0-4A2F-8870-95FB567664A9.com", UriKind.Absolute), new TraceLogger(new MockUpTraceEventProvider()), out result));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void SerializeTest()
        {
            GoalState result;
            Assert.IsTrue(GoalState.TryCreate(new Uri(TestUtility.TestDirectory + "\\goalstate.json", UriKind.Absolute), new TraceLogger(new MockUpTraceEventProvider()), out result));
            Assert.IsTrue(result.Packages.Any());

            string newJsonPath = TestUtility.TestDirectory + "\\goalstate222.json";
            string content = result.Serialize();
            File.WriteAllText(newJsonPath, content);
            Assert.IsTrue(GoalState.TryCreate(new Uri(newJsonPath, UriKind.Absolute), new TraceLogger(new MockUpTraceEventProvider()), out result));
            Assert.IsTrue(result.Packages.Any());
        }
    }
}