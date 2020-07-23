// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace StandaloneLogCollector.Test
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Serialization;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.ServiceFabric.StandaloneLogCollector;

    [TestClass]
    public class TaskManagerTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ConstructorTest()
        {
            string logPath = Utility.GenerateProgramFilePath(TestUtility.TestDirectory, ".log1");
            using (TraceLogger traceLogger = new TraceLogger(logPath))
            {
                TaskManager result = new TaskManager(traceLogger);

                Assert.IsNotNull(result.TraceLogger);
                Assert.AreSame(traceLogger, result.TraceLogger);
                Assert.IsFalse(result.Tasks.Any());
            }

            File.Delete(logPath);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void AddTaskTest()
        {
            string logPath = Utility.GenerateProgramFilePath(TestUtility.TestDirectory, ".log2");
            using (TraceLogger traceLogger = new TraceLogger(logPath))
            {
                TaskManager result = new TaskManager(traceLogger);
                Assert.IsFalse(result.Tasks.Any());

                for (int i = 0; i < 3; i++)
                {
                    result.AddTask(new Task(delegate { }));
                }

                Assert.AreEqual(3, result.Tasks.Count);
            }

            File.Delete(logPath);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WaitForAllTasksTest_HappyPath()
        {
            string logPath = Utility.GenerateProgramFilePath(TestUtility.TestDirectory, ".log3");
            using (TraceLogger traceLogger = new TraceLogger(logPath))
            {
                TaskManager result = new TaskManager(traceLogger);

                Assert.IsFalse(result.Tasks.Any());
                Assert.IsTrue(result.WaitForAllTasks());

                for (int i = 0; i < 3; i++)
                {
                    result.AddTask(Task.Factory.StartNew(() => { Thread.Sleep(3000); }));
                }

                Assert.AreEqual(3, result.Tasks.Count);
                Assert.IsTrue(result.WaitForAllTasks());
            }

            File.Delete(logPath);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WaitForAllTasksTest_Error()
        {
            string logPath = Utility.GenerateProgramFilePath(TestUtility.TestDirectory, ".log3");
            using (TraceLogger traceLogger = new TraceLogger(logPath))
            {
                TaskManager result = new TaskManager(traceLogger);

                Assert.IsFalse(result.Tasks.Any());

                for (int i = 0; i < 3; i++)
                {

                    Task t = Task.Factory.StartNew(
                        (object p) =>
                            {
                                Console.WriteLine(p);
                                if (p.Equals(0))
                                {
                                    throw new ArgumentOutOfRangeException();
                                }

                                Thread.Sleep(2000);
                            },
                            i);
                    result.AddTask(t);
                    if (i == 0)
                    {
                        Thread.Sleep(5000);
                    }
                }

                Assert.AreEqual(3, result.Tasks.Count);
                Assert.IsFalse(result.WaitForAllTasks());
            }

            File.Delete(logPath);
        }
    }
}