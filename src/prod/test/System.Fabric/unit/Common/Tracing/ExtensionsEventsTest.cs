// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Common.Tracing.Test
{
    using System.Diagnostics.Tracing;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Text;
    using System.Text.RegularExpressions;

    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class ExtensionsEventsTest
    {
        private const string TraceFolder = "System.Fabric.Test";
        private const EventTask TestTask = FabricEvents.Tasks.ManagedGeneral;
        private const string TestType = "TestType";
        private const string TestId = "TestId";
        private const string TestFormatMessage = "Test {0}";
        private const string TestParam = "message";
        private const string TestMessage = "Test message";
        private readonly Exception testException = new Exception(TestParam);

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteError()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteError(TestType, TestFormatMessage, TestParam),
                    BuildPredicate("Error", string.Format("\\.{0},{1}", TestType, TestMessage)),
                    "WriteError"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteErrorWithId()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteErrorWithId(TestType, TestId, TestFormatMessage, TestParam),
                    BuildPredicate("Error", string.Format("\\.{0}@{1},{2}", TestType, TestId, TestMessage)),
                    "WriteErrorWithId"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteExceptionAsError()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteExceptionAsError(TestType, testException),
                    BuildPredicate("Error", string.Format("\\.{0},(\r)*\t{1}", TestType, testException)),
                    "WriteExceptionAsError"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteExceptionAsErrorWithId()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteExceptionAsErrorWithId(TestType, TestId, testException, TestFormatMessage, TestParam),
                    BuildPredicate("Error", string.Format("\\.{0}@{1},{2}(\r)*\t{3}", TestType, TestId, TestMessage, testException)),
                    "WriteExceptionAsErrorWithId"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteExceptionAsWarning()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteExceptionAsWarning(TestType, testException),
                    BuildPredicate("Warning", string.Format("\\.{0},(\r)*\t{1}", TestType, testException)),
                    "WriteExceptionAsWarning"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteExceptionAsWarningWithId()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteExceptionAsWarningWithId(TestType, TestId, testException, TestFormatMessage, TestParam),
                    BuildPredicate("Warning", string.Format("\\.{0}@{1},{2}(\r)*\t{3}", TestType, TestId, TestMessage, testException)),
                    "WriteExceptionAsWarningWithId"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteInfo()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteInfo(TestType), 
                    BuildPredicate("Info", string.Format("\\.{0},", TestType)), 
                    "WriteInfo"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteInfoArgs()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteInfo(TestType, TestFormatMessage, TestParam),
                    BuildPredicate("Info", string.Format("\\.{0},{1}", TestType, TestMessage)), 
                    "WriteInfoArgs"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteInfoWithId()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteInfoWithId(TestType, TestId, TestFormatMessage, TestParam), 
                    BuildPredicate("Info", string.Format("\\.{0}@{1},{2}", TestType, TestId, TestMessage)), 
                    "WriteInfoWithId"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteNoise()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteNoise(TestType), 
                    BuildPredicate("Verbose", string.Format("\\.{0},", TestType)),
                    "WriteNoise"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteNoiseWithArgs()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteNoise(TestType, TestFormatMessage, TestParam), 
                    BuildPredicate("Verbose", string.Format("\\.{0},{1}", TestType, TestMessage)), 
                    "WriteNoiseWithArgs"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteNoiseWithId()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteNoiseWithId(TestType, TestId, TestFormatMessage, TestParam),
                    BuildPredicate("Verbose", string.Format("\\.{0}@{1},{2}", TestType, TestId, TestMessage)),
                    "WriteNoiseWithId"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteWarning()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteWarning(TestType, TestFormatMessage, TestParam), 
                    BuildPredicate("Warning", string.Format("\\.{0},{1}", TestType, TestMessage)), 
                    "WriteWarning"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestWriteWarningWithId()
        {
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => t.WriteWarningWithId(TestType, TestId, TestFormatMessage, TestParam),
                    BuildPredicate("Warning", string.Format("\\.{0}@{1},{2}", TestType, TestId, TestMessage)),
                    "WriteWarningWithId"));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public void TestStructuredTrace()
        {
            var predicate = new StringBuilder("^.*?,Info,\\d+,FabricDCA\\.TestStructuredTrace\\d+?,Test message");
            const string EndOfTrace = "(\r\n)*$";
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace0(),
                    predicate + EndOfTrace,
                    "StructuredTrace0"));

            var arg1 = int.MaxValue;
            predicate.Append(" 2147483647");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace1(arg1),
                    predicate + EndOfTrace,
                    "StructuredTrace1"));

            var arg2 = "test";
            predicate.Append(";test");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace2(arg1, arg2),
                    predicate + EndOfTrace,
                    "StructuredTrace2"));

            var arg3 = DateTime.Parse("4:05AM Jan 3 2016");
            predicate.Append(";01/03/2016 04:05:00");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace3(arg1, arg2, arg3),
                    predicate + EndOfTrace,
                    "StructuredTrace3"));

            var arg4 = Guid.Parse("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
            predicate.Append(";aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace4(arg1, arg2, arg3, arg4),
                    predicate + EndOfTrace,
                    "StructuredTrace4"));

            var arg5 = ulong.MaxValue;
            predicate.Append(";18446744073709551615");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace5(arg1, arg2, arg3, arg4, arg5),
                    predicate + EndOfTrace,
                    "StructuredTrace5"));

            var arg6 = byte.MaxValue;
            predicate.Append(";255");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace6(arg1, arg2, arg3, arg4, arg5, arg6),
                    predicate + EndOfTrace,
                    "StructuredTrace6"));

            var arg7 = true;
            predicate.Append(";True");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace7(arg1, arg2, arg3, arg4, arg5, arg6, arg7),
                    predicate + EndOfTrace,
                    "StructuredTrace7"));

            var arg8 = short.MaxValue;
            predicate.Append(";32767");
            EvaluateWriteMethodTest(
                new WriteMethodTestCase(
                    t => FabricEvents.Events.TestStructuredTrace8(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8),
                    predicate + EndOfTrace,
                    "StructuredTrace8"));
        }

        private static string BuildPredicate(string level, string message)
        {
            return string.Format("^.*?,{0},\\d+,ManagedGeneral{1}(\r\n)*$", level, message);
        }

        private static void EvaluateWriteMethodTest(WriteMethodTestCase testCase)
        {
            var traceSource = new FabricEvents.ExtensionsEvents(TestTask);

            TraceConfig.SetDefaultLevel(TraceSinkType.TextFile, EventLevel.Verbose);
            Directory.CreateDirectory(TraceFolder);
            var traceFile = Path.Combine(TraceFolder, testCase.Description + ".trace");
            TraceTextFileSink.SetPath(string.Empty);
            TraceTextFileSink.SetPath(traceFile);

            testCase.TraceAction(traceSource);
            TraceTextFileSink.SetPath(string.Empty);
            var output = File.ReadAllText(traceFile);
            Assert.IsTrue(!string.IsNullOrWhiteSpace(testCase.Predicate), "Predicate must be set.");
            Assert.IsTrue(
                Regex.IsMatch(output, testCase.Predicate), 
                string.Format("Actual text \"{0}\" should match predicate {1}", output, testCase.Predicate));
        }

        private class WriteMethodTestCase
        {
            private readonly Action<FabricEvents.ExtensionsEvents> traceAction;
            private readonly string predicate;
            private readonly string description;

            public WriteMethodTestCase(
                Action<FabricEvents.ExtensionsEvents> traceAction, 
                string predicate, 
                string description)
            {
                this.traceAction = traceAction;
                this.predicate = predicate;
                this.description = description;
            }

            public Action<FabricEvents.ExtensionsEvents> TraceAction
            {
                get { return this.traceAction; }
            }

            public string Predicate
            {
                get { return this.predicate; }
            }

            public string Description
            {
                get { return description; }
            }
        }
    }
}