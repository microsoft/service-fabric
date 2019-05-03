// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Moq;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Dca;
    using System.IO;
    using Tools.EtlReader;

    internal class TestUtility
    {
        internal const string ManifestFolderName = "Manifests";
        internal const string TestEtlFilePatterns = "TESTfabric*.etl";

        internal static readonly MockRepository MockRepository = new MockRepository(MockBehavior.Strict);

        internal static void CopyAndOverwrite(string source, string dest)
        {
            if (Directory.Exists(dest))
            {
                Directory.Delete(dest, true);
            }

            Directory.CreateDirectory(dest);
            foreach (var file in Directory.GetFiles(source))
            {
                var destFile = Path.Combine(dest, Path.GetFileName(file));
                File.Copy(file, destFile);
            }
        }

        internal static string GetActualOutputFolderPath(string logDirectory)
        {
            return Path.Combine(logDirectory, "output");
        }

        internal static string InitializeTestFolder(string testCategory, string testName, string executingAssemblyPath, string dcaTestArtifactPath, string testFilePatterns = TestEtlFilePatterns)
        {
            var testProgramDirectory = Path.Combine(executingAssemblyPath, testCategory, testName);
            try
            {
                Directory.Delete(testProgramDirectory, true);
            }
            catch (DirectoryNotFoundException)
            {
            }

            Directory.CreateDirectory(testProgramDirectory);

            Utility.DcaProgramDirectory = testProgramDirectory;
            Utility.LogDirectory = Path.Combine(testProgramDirectory, "Traces");
            Directory.CreateDirectory(Utility.LogDirectory);

            foreach (var manifestFile in Directory.GetFiles(Path.Combine(dcaTestArtifactPath, ManifestFolderName)))
            {
                File.Copy(manifestFile, Path.Combine(testProgramDirectory, Path.GetFileName(manifestFile)));
            }

            foreach (var etlFile in Directory.GetFiles(Path.Combine(dcaTestArtifactPath, "Data"), testFilePatterns))
            {
                File.Copy(etlFile, Path.Combine(Utility.LogDirectory, Path.GetFileName(etlFile)));
            }

            Utility.InitializeWorkDirectory();
            return Utility.LogDirectory;
        }

        internal static void AssertFinished(string logDirectory, string expectedOutputDataFolderPath, string outputAdditionalSuffix = "")
        {
            foreach (var etlFile in Directory.GetFiles(logDirectory, TestEtlFilePatterns))
            {
                var expectedFilePrefix = Path.GetFileNameWithoutExtension(etlFile);
                var actualFilePrefix = expectedFilePrefix + outputAdditionalSuffix;
                TestUtility.AssertDtrPartsEqual(expectedFilePrefix, expectedOutputDataFolderPath, actualFilePrefix, Path.Combine(logDirectory, "output"));
            }
        }

        internal static Mock<ITraceFileEventReader> CreateForwardingEventReader(string filename)
        {
            var mockActiveTraceReader = MockRepository.Create<ITraceFileEventReader>();
            mockActiveTraceReader
                .Setup(reader => reader.ReadEvents(It.IsAny<DateTime>(), It.IsAny<DateTime>()))
                .Callback((DateTime startTime, DateTime endTime) =>
                {
                    var realTraceReader = new TraceFileEventReader(filename);
                    var eventIdx = 0;
                    realTraceReader.EventRead += (sender, arg) =>
                    {
                        // Only give x at a time. This should really be % of total to produce x passes.
                        if (eventIdx < 400)
                        {
                            mockActiveTraceReader.Raise(
                                reader => reader.EventRead += null,
                                arg);
                        }

                        eventIdx++;
                    };
                    realTraceReader.ReadEvents(startTime, endTime);
                });
            mockActiveTraceReader
                .Setup(reader => reader.ReadTraceSessionMetadata())
                .Returns(new TraceSessionMetadata("test", 0, DateTime.UtcNow, DateTime.UtcNow));
            mockActiveTraceReader
                .Setup(reader => reader.Dispose());
            return mockActiveTraceReader;
        }

        internal static Mock<ITraceFileEventReader> CreateEventsLostReader(string filename)
        {
            var mockEventsLostReader = MockRepository.Create<ITraceFileEventReader>();
            mockEventsLostReader
                    .Setup(reader => reader.ReadTraceSessionMetadata())
                    .Returns(() =>
                    {
                        using (var realTraceReader = new TraceFileEventReader(filename))
                        {
                            realTraceReader.TestEventsLost = 10;
                            return realTraceReader.ReadTraceSessionMetadata();
                        }
                    });
            mockEventsLostReader
                .Setup(reader => reader.Dispose());
            return mockEventsLostReader;
        }

        internal static void AssertDtrPartsEqual(
           string expectedFileNameChunk,
           string expectedDtrFolder,
           string actualFileNameChunk,
           string actualDtrFolder)
        {
            var expectedLines = GetMergedDtrLines(expectedFileNameChunk, expectedDtrFolder);
            var actualLines = GetMergedDtrLines(actualFileNameChunk, actualDtrFolder);

            // We do this comparison first to help with failures so we know first line of difference.
            for (var i = 0; i < expectedLines.Count && i < actualLines.Count; i++)
            {
                Assert.AreEqual(
                    expectedLines[i],
                    actualLines[i],
                    string.Format("Decoded events should be equal at index {0}.", i));
            }

            // Workaround to decrease test failures until investigation of failures
            // TODO - Investigate fix and return to Assert.Equal (RDBug 11404901)
            // Assert.AreEqual(
            //    expectedLines.Count,
            //    actualLines.Count,
            //    string.Format("Total number of events should be equal for {0} and {1}.", expectedFileNameChunk, actualFileNameChunk));
            Assert.IsFalse(
               Math.Abs(expectedLines.Count - actualLines.Count) > 1,
               string.Format("<Expected: <{2}>. Actual <{3}>. Total number of events should be equal for {0} and {1} or have a difference of only 1 event.", expectedFileNameChunk, actualFileNameChunk, expectedLines.Count, actualLines.Count));
        }

        internal static List<string> GetMergedDtrLines(
            string commonFileNameChunk,
            string folder)
        {
            var lines = new List<string>();
            foreach (var file in Directory.GetFiles(folder, string.Format("*{0}*", commonFileNameChunk)))
            {
                lines.AddRange(File.ReadAllLines(file));
            }

            return lines;
        }
    }
}