// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log.Test
{
    using System;
    using System.IO;
    using System.Collections.Generic;
    using System.Threading;
    using System.Diagnostics;
    using System.Threading.Tasks;
    using System.Runtime.InteropServices;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Test;
    using System.Linq;
    using System.Security.AccessControl;
    using System.Fabric.Data.Log;
    using System.Fabric.Data.Log.Interop;
    using System.Runtime.CompilerServices;
    using VS = Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.Logging.Interop;
    using WEX.TestExecution;
    using GuidStruct = System.Fabric.Data.Log.Interop.NativeLog.GuidStruct;


    [VS.TestClass]
    public class LogTest
    {
        const UInt32 UseSparseFile = 1;
        Random rnd = new Random();

        public LogManager.LogCreationFlags GetRandomCreationFlags()
        {
            int r = rnd.Next(1);
            return( (r == 0) ? (LogManager.LogCreationFlags)0 : LogManager.LogCreationFlags.UseSparseFile );
        }

        public static string PickPathOnWhichToRunTest()
        {
            string currentDirectory = Directory.GetCurrentDirectory();
            return currentDirectory;
        }

        public string PickUniqueLogFilename()
        {
            Guid guid = Guid.NewGuid();
            string workPathname;

            workPathname = "\\??\\" + PickPathOnWhichToRunTest() + "\\LogicalLogTestTemp\\" + guid.ToString() + ".testlog";

            return (workPathname);
        }

        [VS.ClassInitialize]
        public static void ClassInitialize(VS.TestContext context)
        {
            string workPath = PickPathOnWhichToRunTest() + "\\LogicalLogTestTemp";

            try
            {
                Console.WriteLine("Creating work directory {0}", workPath);
                Directory.CreateDirectory(workPath);
            }
            catch (Exception)
            {
            }
        }

        [VS.ClassCleanup]
        public static void ClassCleanup()
        {
            // To validate no tasks are left behind with unhandled exceptions
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        //* Local functions
        ILogManager
        MakeManager()
        {
            Task<ILogManager> t = LogManager.OpenAsync(LogManager.LoggerType.Default, CancellationToken.None);
            t.Wait();
            return t.Result;
        }

        IPhysicalLog MakePhyLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId,
            long containerSize,
            uint maximumNumberStreams,
            uint maximumLogicalLogBlockSize,
            LogManager.LogCreationFlags creationFlags = (LogManager.LogCreationFlags)0xffffffff
        ) 
        {
            if (creationFlags == (LogManager.LogCreationFlags)0xffffffff)
            {
                creationFlags = GetRandomCreationFlags();
            }

            Task<IPhysicalLog> t = manager.CreatePhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                containerSize,
                maximumNumberStreams,
                maximumLogicalLogBlockSize,
                creationFlags,
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        IPhysicalLog
        OpenPhyLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId)
        {
            Task<IPhysicalLog> t = manager.OpenPhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                false,
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        ILogicalLog CreateLogicalLog(
            IPhysicalLog phylog,
            Guid logicalLogId,
            string optionalLogStreamAlias,
            string optionalPath,
            FileSecurity optionalSecurityInfo,
            Int64 maximumSize,
            UInt32 maximumBlockSize,
            LogManager.LogCreationFlags creationFlags = (LogManager.LogCreationFlags)0xffffffff
        )
    {
        if (creationFlags == (LogManager.LogCreationFlags)0xffffffff)
        {
            creationFlags = GetRandomCreationFlags();
        }

        Task<ILogicalLog> t = phylog.CreateLogicalLogAsync(
                logicalLogId,
                optionalLogStreamAlias,
                optionalPath,
                optionalSecurityInfo,
                maximumSize,
                maximumBlockSize,
                creationFlags,
                "",
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        ILogicalLog OpenLogicalLog(
            IPhysicalLog phylog,
            Guid logicalLogId)
        {
            Task<ILogicalLog> t = phylog.OpenLogicalLogAsync(
                logicalLogId,
                "",
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        int ReadLogicalLog(
            ILogicalLog logicalLog,
            byte[] buffer,
            int bufferOffset,
            int count)
        {
            Task<int> t = logicalLog.ReadAsync(buffer, bufferOffset, count, 0, CancellationToken.None);

            t.Wait();
            return t.Result;
        }


        [VS.TestMethod]
        [VS.Owner("richhas")]


        public unsafe void BasicLogicalLogTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                //** Main line test
                ILogManager logManager = MakeManager();
                Verify.IsTrue(LogManager.IsLoaded);
                Verify.IsTrue(LogManager.Handles == 1);
                Verify.IsTrue(LogManager.Logs == 0);

                //** Prove Abort works
                Verify.IsTrue(LogManager.IsLoaded);
                logManager.Abort();
                logManager = null;
                GC.Collect();
                GC.WaitForPendingFinalizers();
                //
                // LogManager handle cleanup is an async operation and so we cannot guarantee
                // that it has been run at this point. TFS 6029884 was created as a result
                // of this occurring in this CIT. Waiting for 5 seconds should be sufficient.
                //
                Thread.Sleep(5000);
                Verify.IsFalse(LogManager.IsLoaded);

                //** Prove Finalizer works
                logManager = MakeManager();
                Verify.IsTrue(LogManager.IsLoaded);
                logManager = null;
                GC.Collect();
                GC.WaitForPendingFinalizers();
                //
                // LogManager handle cleanup is an async operation and so we cannot guarantee
                // that it has been run at this point. TFS 6029884 was created as a result
                // of this occurring in this CIT. Waiting for 5 seconds should be sufficient.
                //
                Thread.Sleep(5000);
                Verify.IsFalse(LogManager.IsLoaded);

                logManager = MakeManager();
                Verify.IsTrue(LogManager.IsLoaded);
                Verify.IsTrue(LogManager.Handles == 1);
                Verify.IsTrue(LogManager.Logs == 0);

                ILogManager logManager1 = MakeManager();
                Verify.IsTrue(LogManager.IsLoaded);
                Verify.IsTrue(LogManager.Handles == 2);
                Verify.IsTrue(LogManager.Logs == 0);

                //** Prove physical log can be created
                Guid logId = Guid.NewGuid();

                string logName = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", logName);
                try
                {
                    logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                }
                catch (Exception)
                {
                    // Ok if this fails
                }


                IPhysicalLog phyLog = MakePhyLog(
                    logManager,
                    logName,
                    logId,
                    1024 * 1024 * 1024,
                    0,
                    0);

                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 2);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                //** Prove secondary attempt to create a physical log will fail
                {
                    bool didFail = false;
                    try
                    {
                        IPhysicalLog phyLog1 = MakePhyLog(
                            logManager,
                            logName,
                            logId,
                            1024 * 1024 * 1024,
                            0,
                            0,
                            0);
                    }
                    catch
                    {
                        // TODO: Make sure we get the right execption
                        didFail = true;
                    }

                    VerifyIsTrue(didFail);
                }
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 2);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                //* Prove base close after create works
                VerifyIsTrue(phyLog.IsFunctional);
                phyLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 2);
                VerifyIsTrue(LogManager.Logs == 0);
                phyLog = null;

                //* Prove phylog delete works
                logManager.DeletePhysicalLogAsync(
                    logName,
                    logId,
                    CancellationToken.None).Wait();

                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 2);
                VerifyIsTrue(LogManager.Logs == 0);

                //* Prove we can re-create; open on a different handle; close the original - and all is well
                phyLog = MakePhyLog(
                    logManager,
                    logName,
                    logId,
                    1024 * 1024 * 1024,
                    0,
                    0,
                    0);

                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 2);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                IPhysicalLog phyLog2 = OpenPhyLog(
                    logManager,
                    logName,
                    logId);

                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(phyLog2.IsFunctional);
                VerifyIsTrue(Object.ReferenceEquals(((LogManager.PhysicalLog.Handle)phyLog2).Owner, ((LogManager.PhysicalLog.Handle)phyLog).Owner));
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 0);

                logManager.CloseAsync(CancellationToken.None).Wait();
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);

                logManager1.CloseAsync(CancellationToken.None).Wait();
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 0);
                VerifyIsTrue(LogManager.Logs == 1);

                phyLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog.IsFunctional);
                VerifyIsTrue(phyLog2.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 0);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 0);

                phyLog2.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog.IsFunctional);
                VerifyIsFalse(phyLog2.IsFunctional);
                VerifyIsFalse(LogManager.IsLoaded);

                phyLog = null;
                phyLog2 = null;

                //* Prove Abort, Dispose, and Finalize behavior in IPhysicalLog
                logManager = MakeManager();

                // Abort
                phyLog = OpenPhyLog(logManager, logName, logId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                phyLog.Abort();
                phyLog = null;
                Thread.Sleep(500);      // Allow background Abort-triggered clean up to happen
                GC.Collect();
                GC.WaitForPendingFinalizers();
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 0);

                // Dispose()
                phyLog = OpenPhyLog(logManager, logName, logId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                phyLog.Dispose();
                phyLog = null;
                Thread.Sleep(500);      // Allow background Dispose-triggered clean up to happen
                GC.Collect();
                GC.WaitForPendingFinalizers();
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 0);

                // Finalize
                phyLog = OpenPhyLog(logManager, logName, logId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                phyLog = null;
                GC.Collect();
                GC.WaitForPendingFinalizers();
                Thread.Sleep(500);                      // Allow background Finalizer-triggered clean up to happen
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 0);

                phyLog = OpenPhyLog(logManager, logName, logId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                Guid lLogId = Guid.NewGuid();
                string logicalLogName = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", logicalLogName);

                ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, 128 * 1024, LogManager.LogCreationFlags.UseSparseFile);  
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                lLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(lLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);
                lLog = null;

                // Prove we can re-open a LLog
                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                //* Prove Abort(), Dispose(), and Finalize work for LLog API
                // Abort
                lLog.Abort();
                lLog = null;
                Thread.Sleep(500);      // Allow background Dispose-triggered clean up to happen
                GC.Collect();
                GC.WaitForPendingFinalizers();
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                // Dispose()
                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                lLog.Dispose();
                lLog = null;
                Thread.Sleep(500);      // Allow background Dispose-triggered clean up to happen
                GC.Collect();
                GC.WaitForPendingFinalizers();
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                // Finalize
                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                lLog = null;
                GC.Collect();
                GC.WaitForPendingFinalizers();
                Thread.Sleep(500);      // Allow background Dispose-triggered clean up to happen
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                //* Prove an open LogicalLog is functional with all Physical and Manager handles closed
                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                phyLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                logManager.CloseAsync(CancellationToken.None).Wait();
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 0);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(lLog.IsFunctional);

                lLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog.IsFunctional);
                VerifyIsFalse(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 0);
                VerifyIsTrue(LogManager.Logs == 0);
                lLog = null;

                //* Prove we exclusive LLog open works
                logManager = MakeManager();
                phyLog = OpenPhyLog(logManager, logName, logId);
                phyLog2 = OpenPhyLog(logManager, logName, logId);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 0);

                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.IsFunctional);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);

                {
                    bool didFail = false;

                    try
                    {
                        OpenLogicalLog(phyLog2, lLogId);
                    }
                    catch
                    {
                        // TODO: Make sure we get the right execption
                        didFail = true;
                    }

                    VerifyIsTrue(didFail);
                }
                VerifyIsTrue(lLog.IsFunctional);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 1);

                lLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(lLog.IsFunctional);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 2);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 0);
                lLog = null;


                //* Prove LLog delete works
                phyLog.DeleteLogicalLogAsync(lLogId, CancellationToken.None).Wait();
                {
                    bool didFail = false;

                    try
                    {
                        OpenLogicalLog(phyLog, lLogId);     // Must fail
                    }
                    catch
                    {
                        // TODO: Make sure we get the right execption
                        didFail = true;
                    }

                    VerifyIsTrue(didFail);
                }

                //* Prove close down 
                phyLog.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 0);
                phyLog = null;

                phyLog2.CloseAsync(CancellationToken.None).Wait();
                VerifyIsFalse(phyLog2.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 0);
                VerifyIsFalse(((LogManager.PhysicalLog.Handle)phyLog2).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.PhysicalLogHandles == 0);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog2).Owner.LogicalLogs == 0);
                phyLog2 = null;

                //* Cleanup
                logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                logManager.CloseAsync(CancellationToken.None).Wait();
                logManager1.CloseAsync(CancellationToken.None).Wait();
            },

            "BasicLogicalLogTest");
        }

        [VS.TestMethod]
        [VS.Owner("richhas")]
        public unsafe void BasicLogicalLogIOTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                //** Main line test
                ILogManager logManager = MakeManager();
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 0);

                Guid logId = Guid.NewGuid();

                string logContainerName = PickUniqueLogFilename();
                Console.WriteLine("Using logContainer: {0}", logContainerName);
                try
                {
                    logManager.DeletePhysicalLogAsync(logContainerName, logId, CancellationToken.None).Wait();
                }
                catch (Exception)
                {
                    // Ok if this fails
                }

                IPhysicalLog phyLog = MakePhyLog(
                    logManager,
                    logContainerName,
                    logId,
                    1024 * 1024 * 1024,
                    0,
                    0);

                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                //** ILogicalLog IO Tests
                const int MaxLogBlkSize = 128 * 1024;
                string logStreamFilename = PickUniqueLogFilename();
                Console.WriteLine("Using logStream: {0}", logStreamFilename);

                Guid lLogId = Guid.NewGuid();
                ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logStreamFilename, null, 1024 * 1024 * 100, MaxLogBlkSize);
                VerifyIsTrue(phyLog.IsFunctional);
                VerifyIsTrue(LogManager.IsLoaded);
                VerifyIsTrue(LogManager.Handles == 1);
                VerifyIsTrue(LogManager.Logs == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                VerifyIsTrue(lLog.IsFunctional);
                VerifyIsTrue(lLog.Length == 0);
                VerifyIsTrue(lLog.ReadPosition == 0);
                VerifyIsTrue(lLog.WritePosition == 0);
                VerifyIsTrue(lLog.HeadTruncationPosition == -1);



                //** Prove simple recovery
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.Length == 0);
                VerifyIsTrue(lLog.ReadPosition == 0);
                VerifyIsTrue(lLog.WritePosition == 0);
                VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                const int recordSize = 131;

                Func<int, byte[]>
                BuildBuffer = ((int count) =>
                {
                    byte[] result = new byte[count];

                    for (int ix = 0; ix < result.Length; ix++)
                        result[ix] = (byte)(ix % recordSize);

                    return result;
                });

                Action<byte[], int>
                ValidateBuffer = ((byte[] toCheck, int recOffset) =>
                {
                    for (int ix = 0; ix < toCheck.Length; ix++)
                        toCheck[ix] = (byte)((ix + recOffset) % recordSize);
                });

                Action<ArraySegment<byte>, long>
                ValidateStreamData = ((ArraySegment<byte> ToCheck, long StreamOffset) =>
                {
                    for (int ix = 0; ix < ToCheck.Count; ix++)
                        ToCheck.Array[ix + ToCheck.Offset] = (byte)((ix + StreamOffset) % recordSize);
                });

                //** Prove we can write bytes
                byte[] x = BuildBuffer(recordSize);

                VerifyIsTrue(lLog.WritePosition == 0);
                lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.WritePosition == recordSize);

                lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                //* Prove simple (non-null recovery - only one (asn 1) physical record in the log)
                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.WritePosition == recordSize);

                lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.WritePosition == recordSize * 2);
                lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                //* Prove simple (non-null recovery)
                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.WritePosition == recordSize * 2);

                //* Prove simple read and positioning works
                VerifyIsTrue(lLog.ReadPosition == 0);
                x = new byte[recordSize];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == recordSize);
                ValidateBuffer(x, 0);

                VerifyIsTrue(lLog.ReadPosition == recordSize);

                //* Prove next sequential physical read works
                x = new byte[recordSize];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == recordSize);
                ValidateBuffer(x, 0);

                VerifyIsTrue(lLog.ReadPosition == recordSize * 2);

                //* Positioning and reading partial and across physical records work
                int offset = 1;
                lLog.SeekForRead(offset, IO.SeekOrigin.Begin);
                x = new byte[recordSize];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == recordSize);
                ValidateBuffer(x, offset);

                VerifyIsTrue(lLog.ReadPosition == recordSize + offset);

                // Prove reading a short record works
                x = new byte[recordSize];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == recordSize - 1);
                ValidateBuffer(x, offset);
                for (int ix = 0; ix < recordSize - 1; ix++)
                    VerifyIsTrue(x[ix] == (byte)((ix + offset) % recordSize));

                VerifyIsTrue(lLog.ReadPosition == lLog.WritePosition);

                // Prove reading at EOS works
                x = new byte[recordSize];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == 0);
                VerifyIsTrue(lLog.ReadPosition == lLog.WritePosition);

                // Prove position beyond and just EOS and reading works 
                long currentPos = lLog.SeekForRead(1, IO.SeekOrigin.End);
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == 0);

                long eos = lLog.SeekForRead(0, IO.SeekOrigin.End);
                VerifyIsTrue(eos == lLog.WritePosition);

                currentPos = lLog.SeekForRead(-1, IO.SeekOrigin.End);
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, 1) == 1);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 1), currentPos);

                currentPos = lLog.SeekForRead(-1, IO.SeekOrigin.End);
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, 2) == 1);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 1), currentPos);

                currentPos = lLog.SeekForRead(-2, IO.SeekOrigin.End);
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, 2) == 2);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 2), currentPos);

                //* Prove we can position forward to every location 
                currentPos = eos - 1;

                while (currentPos >= 0)
                {
                    lLog.SeekForRead(currentPos, IO.SeekOrigin.Begin);
                    byte[] rBuff = new byte[eos - currentPos];
                    VerifyIsTrue(ReadLogicalLog(lLog, rBuff, 0, rBuff.Length) == rBuff.Length);

                    currentPos--;
                }

                //* Prove Stream read abstraction works thru a BufferedStream
                Stream readStream = lLog.CreateReadStream(0);
                BufferedStream bStream = new BufferedStream(readStream);

                currentPos = bStream.Seek(1, IO.SeekOrigin.End);
                VerifyIsTrue(bStream.Read(x, 0, x.Length) == 0);

                eos = bStream.Seek(0, IO.SeekOrigin.End);
                VerifyIsTrue(eos == lLog.WritePosition);

                currentPos = bStream.Seek(-1, IO.SeekOrigin.End);
                VerifyIsTrue(bStream.Read(x, 0, 1) == 1);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 1), currentPos);

                currentPos = bStream.Seek(-1, IO.SeekOrigin.End);
                VerifyIsTrue(bStream.Read(x, 0, 2) == 1);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 1), currentPos);

                currentPos = bStream.Seek(-2, IO.SeekOrigin.End);
                VerifyIsTrue(bStream.Read(x, 0, 2) == 2);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 2), currentPos);

                currentPos = eos - 1;

                while (currentPos >= 0)
                {
                    bStream.Seek(currentPos, IO.SeekOrigin.Begin);
                    byte[] rBuff = new byte[eos - currentPos];
                    VerifyIsTrue(bStream.Read(rBuff, 0, rBuff.Length) == rBuff.Length);

                    currentPos--;
                }

                bStream = null;
                readStream = null;

                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                phyLog.DeleteLogicalLogAsync(lLogId, CancellationToken.None).Wait();

                //* Prove some large I/O
                string sss = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", sss);
                lLog = CreateLogicalLog(phyLog, lLogId, null, sss, null, 1024 * 1024 * 100, MaxLogBlkSize);
                x = BuildBuffer(1024 * 1024);

                lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.WritePosition == (1024 * 1024));
                lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                x = new byte[1024 * 1024];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == x.Length);
                ValidateStreamData(new ArraySegment<byte>(x, 0, x.Length), 0);

                //* Prove basic head truncation behavior
                lLog.TruncateHead(128 * 1024);
                VerifyIsTrue(lLog.HeadTruncationPosition == (128 * 1024));

                // Force a record out to make sure the head truncation point is captured
                const string testStr1 = "Test String Record";
                string str = testStr1;
                x = new byte[str.Length * sizeof(char)];
                System.Buffer.BlockCopy(str.ToCharArray(), 0, x, 0, x.Length);
                lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                // Prove head truncation point is recovered
                long currentLength = lLog.WritePosition;
                long currentTrucPoint = lLog.HeadTruncationPosition;

                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.WritePosition == currentLength);
                VerifyIsTrue(lLog.HeadTruncationPosition == currentTrucPoint);

                //* Prove basic tail truncation behavior
                long newEos = lLog.SeekForRead(currentLength - x.Length, IO.SeekOrigin.Begin);
                x = new byte[256];
                int numberRead = ReadLogicalLog(lLog, x, 0, x.Length);
                char[] chars = new char[numberRead / sizeof(char)];
                System.Buffer.BlockCopy(x, 0, chars, 0, numberRead);
                VerifyIsTrue(testStr1 == new string(chars));

                // Chop off the last and part of the 2nd to last records
                newEos = newEos - numberRead - 1;
                lLog.TruncateTail(newEos, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.WritePosition == newEos);

                // Prove we can read valid up to the new eos but not beyond
                lLog.SeekForRead(newEos - 128, IO.SeekOrigin.Begin);
                x = new byte[256];
                numberRead = ReadLogicalLog(lLog, x, 0, x.Length);
                VerifyIsTrue(numberRead == 128);
                ValidateStreamData(new ArraySegment<byte>(x, 0, numberRead), newEos - 128);

                // Prove we recover the correct tail position (WritePosition)
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.WritePosition == newEos);
                VerifyIsTrue(lLog.HeadTruncationPosition == currentTrucPoint);

                // Prove we can truncate all content via tail truncation
                VerifyIsTrue(lLog.Length != 0);
                lLog.TruncateTail(currentTrucPoint+1, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.Length == 0);

                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                lLog = OpenLogicalLog(phyLog, lLogId);
                VerifyIsTrue(lLog.WritePosition == currentTrucPoint + 1);
                VerifyIsTrue(lLog.Length == 0);
                VerifyIsTrue(lLog.HeadTruncationPosition == currentTrucPoint);

                //* Add proof for truncating all but one byte - read it for the right value
                long currentWritePos = lLog.WritePosition;
                x = BuildBuffer(1024 * 1024);

                lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.WritePosition == currentWritePos + (1024 * 1024));
                lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                VerifyIsTrue(lLog.SeekForRead(currentWritePos, IO.SeekOrigin.Begin) == currentWritePos);
                x = new byte[1024 * 1024];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == x.Length);
                ValidateStreamData(new ArraySegment<byte>(x, 0, x.Length), 0);

                lLog.TruncateHead(currentWritePos + (1024 * 513));
                VerifyIsTrue(lLog.HeadTruncationPosition == currentWritePos + (1024 * 513));

                VerifyIsTrue(lLog.SeekForRead(currentWritePos + (1024 * 513) + 1, IO.SeekOrigin.Begin) == currentWritePos + (1024 * 513) + 1);
                x = new byte[lLog.Length];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, x.Length) == x.Length);
                ValidateStreamData(new ArraySegment<byte>(x, 0, x.Length), currentWritePos + (1024 * 513) + 1);

                lLog.TruncateTail(currentWritePos + (1024 * 513) + 2, CancellationToken.None).Wait();
                VerifyIsTrue(lLog.Length == 1);

                VerifyIsTrue(lLog.SeekForRead(currentWritePos + (1024 * 513) + 1, IO.SeekOrigin.Begin) == currentWritePos + (1024 * 513) + 1);
                x[0] = (byte)~x[0];
                VerifyIsTrue(ReadLogicalLog(lLog, x, 0, 1) == 1);
                ValidateStreamData(new ArraySegment<byte>(x, 0, 1), currentWritePos + (1024 * 513) + 1);

                //* Prove close down 
                lLog.CloseAsync(CancellationToken.None).Wait();
                lLog = null;

                phyLog.CloseAsync(CancellationToken.None).Wait();
                phyLog = null;

                //* Cleanup
                logManager.DeletePhysicalLogAsync(logContainerName, logId, CancellationToken.None).Wait();
                logManager.CloseAsync(CancellationToken.None).Wait();
            },


            "BasicLogicalLogIOTest");
        }


        private class TestLogNotification
        {
            public bool Fired = false;

            public async Task SetupLogUsageNotificationAsync(ILogicalLog lLog, uint PercentUsage, CancellationToken CancelToken)
            {
                Fired = false;
                await lLog.WaitCapacityNotificationAsync(PercentUsage, CancelToken);
                Fired = true;
            }
        }

        [VS.TestMethod]
        [VS.Owner("richhas")]
        public unsafe void LogUsageNotificationTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = MakeManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    Guid logId = Guid.NewGuid();
                    string logName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logName);

                    try
                    {
                        logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // ok if this fails
                    }

                    IPhysicalLog phyLog = MakePhyLog(
                        logManager,
                        logName,
                        logId,
                        256 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);



                    //** Prove simple recovery
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    lLog = OpenLogicalLog(phyLog, lLogId);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    const int recordSize = 131;

                    Func<int, byte[]>
                    BuildBuffer = ((int count) =>
                    {
                        byte[] result = new byte[count];

                        for (int ix = 0; ix < result.Length; ix++)
                            result[ix] = (byte)(ix % recordSize);

                        return result;
                    });

                    Action<byte[], int>
                    ValidateBuffer = ((byte[] toCheck, int recOffset) =>
                    {
                        for (int ix = 0; ix < toCheck.Length; ix++)
                            toCheck[ix] = (byte)((ix + recOffset) % recordSize);
                    });

                    Action<ArraySegment<byte>, long>
                    ValidateStreamData = ((ArraySegment<byte> ToCheck, long StreamOffset) =>
                    {
                        for (int ix = 0; ix < ToCheck.Count; ix++)
                            ToCheck.Array[ix + ToCheck.Offset] = (byte)((ix + StreamOffset) % recordSize);
                    });

                    //
                    // Prove a usage notification can be cancelled.
                    //
                    Task taskUsageNotificationCancel;
                    TestLogNotification tnCancel = new TestLogNotification();
                    CancellationTokenSource cancelTokenSourceCancel = new CancellationTokenSource();
                    bool cancelled;

                    taskUsageNotificationCancel = tnCancel.SetupLogUsageNotificationAsync(lLog, 50, cancelTokenSourceCancel.Token);

                    try
                    {
                        cancelTokenSourceCancel.Cancel();
                        taskUsageNotificationCancel.Wait(5000);
                        cancelled = false;
                    }
                    catch(Exception)
                    {
                        cancelled = true;
                    }

                    VerifyIsTrue(cancelled);
                    

                    //
                    // Prove that a usage notification is fired
                    //
                    Task taskUsageNotification50;
                    TestLogNotification tn50 = new TestLogNotification();
                    CancellationTokenSource cancelTokenSource50 = new CancellationTokenSource();
                    byte[] x = BuildBuffer(recordSize);

                    VerifyIsTrue(lLog.WritePosition == 0);
                    lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                    VerifyIsTrue(lLog.WritePosition == recordSize);

                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    taskUsageNotification50 = tn50.SetupLogUsageNotificationAsync(lLog, 50, cancelTokenSource50.Token);

                    long expectedWritePosition = recordSize;

                    do
                    {
                        lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                        expectedWritePosition += recordSize;
                        if (lLog.WritePosition != expectedWritePosition)
                        {
                            VerifyIsTrue(lLog.WritePosition == expectedWritePosition);
                        }

                        if (tn50.Fired)
                        {
                            //
                            // Verify that we are in the range of 50-75% full
                            //
                            long permittableRangeLow = (lLog.Length / 2);
                            long permittableRangeHigh = 3 * (lLog.Length / 4);
                            VerifyIsTrue((expectedWritePosition >= permittableRangeLow) &&
                                              (expectedWritePosition <= (expectedWritePosition + permittableRangeHigh)));
                            break;
                        }
                    } while (true);


                    //* Prove close down 
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "LogUsageNotificationTest");
        }



        [VS.TestMethod]
        [VS.Owner("richhas")]
        public unsafe void LogUsageWorkloadTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = MakeManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    Guid logId = Guid.NewGuid();
                    string logName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logName);

                    IPhysicalLog phyLog = MakePhyLog(
                        logManager,
                        logName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);



                    //** Prove simple recovery
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    lLog = OpenLogicalLog(phyLog, lLogId);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    const int recordSize = 131;

                    Func<int, byte[]>
                    BuildBuffer = ((int count) =>
                    {
                        byte[] result = new byte[count];

                        for (int ix = 0; ix < result.Length; ix++)
                            result[ix] = (byte)(ix % recordSize);

                        return result;
                    });

                    Action<byte[], int>
                    ValidateBuffer = ((byte[] toCheck, int recOffset) =>
                    {
                        for (int ix = 0; ix < toCheck.Length; ix++)
                            toCheck[ix] = (byte)((ix + recOffset) % recordSize);
                    });

                    Action<ArraySegment<byte>, long>
                    ValidateStreamData = ((ArraySegment<byte> ToCheck, long StreamOffset) =>
                    {
                        for (int ix = 0; ix < ToCheck.Count; ix++)
                            ToCheck.Array[ix + ToCheck.Offset] = (byte)((ix + StreamOffset) % recordSize);
                    });



                    //
                    // Prove that a usage notification is fired
                    //
                    Task taskUsageNotification75;
                    TestLogNotification tn75 = new TestLogNotification();
                    CancellationTokenSource cancelTokenSource75 = new CancellationTokenSource();
                    byte[] x = BuildBuffer(recordSize);

                    VerifyIsTrue(lLog.WritePosition == 0);
                    lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                    VerifyIsTrue(lLog.WritePosition == recordSize);

                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    taskUsageNotification75 = tn75.SetupLogUsageNotificationAsync(lLog, 75, cancelTokenSource75.Token);

                    long expectedWritePosition = recordSize;

                    int startTime = System.Environment.TickCount;
                    int endTime = startTime + 60 * 1000;                // 8 seconds
                    while (System.Environment.TickCount < endTime)
                    {
                        lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                        expectedWritePosition += recordSize;
                        if (lLog.WritePosition != expectedWritePosition)
                        {
                            VerifyIsTrue(lLog.WritePosition == expectedWritePosition);
                        }

                        if (tn75.Fired)
                        {
                            //
                            // Truncate back to 50%
                            //
                            long allowedData = lLog.Length / 2;
                            long truncationPosition = expectedWritePosition - allowedData;
                            lLog.TruncateHead(truncationPosition);
                            Console.WriteLine("truncate at {0}", truncationPosition);
                            taskUsageNotification75 = tn75.SetupLogUsageNotificationAsync(lLog, 75, cancelTokenSource75.Token);
                        }
                    }

                    try
                    {
                        cancelTokenSource75.Cancel();
                        taskUsageNotification75.Wait(5000);
                    }
                    catch
                    {
                    }

                    //* Prove close down 
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "LogUsageWorkloadTest");
        }



        [VS.TestMethod]
        [VS.Owner("richhas")]
        public unsafe void BasicInteropTest()
        {
            //** Prove basic IoBufferElement creation
            NativeLog.IKIoBufferElement testElement;

            UInt32 hr = NativeLog.CreateKIoBufferElement(4096, out testElement);
            VerifyAreEqual(hr, (UInt32)0);
            Verify.IsNotNull(testElement);

            void* buffer = null;
            hr = testElement.GetBuffer(out buffer);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyIsTrue(buffer != null);

            for (uint ix = 0; ix < 4096; ix++)
            {
                *(((byte*)buffer) + ix) = (byte)ix;
            }

            //** Prove empty IoBuffer behavior
            NativeLog.IKIoBuffer ioBuffer0;

            hr = NativeLog.CreateEmptyKIoBuffer(out ioBuffer0);
            VerifyAreEqual(hr, (UInt32)0);
            Verify.IsNotNull(ioBuffer0);

            //** Prove IoBuffer contents behaviors
            hr = ioBuffer0.AddIoBufferElement(testElement);
            VerifyAreEqual(hr, (UInt32)0);

            UInt32 numberOfElements;
            hr = ioBuffer0.QueryNumberOfIoBufferElements(out numberOfElements);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(numberOfElements, (UInt32)1);

            UInt32 ioBuffer0Size;
            hr = ioBuffer0.QuerySize(out ioBuffer0Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer0Size, (UInt32)4096);

            UIntPtr token;
            hr = ioBuffer0.First(out token);
            VerifyAreEqual(hr, (UInt32)0);

            UIntPtr nextToken;
            hr = ioBuffer0.Next(token, out nextToken);
            VerifyAreNotEqual(hr, (UInt32)0);

            void* buffer1 = null;
            hr = ioBuffer0.QueryElementInfo(token, out buffer1, out ioBuffer0Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer0Size, (UInt32)4096);
            VerifyIsTrue(buffer1 == buffer);

            for (int ix = 0; ix < 4096; ix++)
            {
                VerifyAreEqual((byte)ix, *(((byte*)buffer1) + ix));
            }

            //** Prove aliased elements point to same buffers
            NativeLog.IKIoBufferElement element1;
            hr = ioBuffer0.GetElement(token, out element1);
            VerifyAreEqual(hr, (UInt32)0);

            buffer1 = null;
            hr = element1.GetBuffer(out buffer1);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyIsTrue(buffer1 == buffer);

            //** Prove adding IoBuffers together and the creation of a simple IoBuffer
            NativeLog.IKIoBuffer ioBuffer1;
            hr = NativeLog.CreateSimpleKIoBuffer(1024 * 1024, out ioBuffer1);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyIsTrue(ioBuffer1 != null);

            UInt32 ioBuffer1Size;
            hr = ioBuffer1.QuerySize(out ioBuffer1Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer1Size, (UInt32)1024 * 1024);

            hr = ioBuffer0.AddIoBuffer(ioBuffer1);
            VerifyAreEqual(hr, (UInt32)0);

            hr = ioBuffer1.QuerySize(out ioBuffer1Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer1Size, (UInt32)0);

            hr = ioBuffer1.QueryNumberOfIoBufferElements(out numberOfElements);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(numberOfElements, (UInt32)0);

            hr = ioBuffer0.QueryNumberOfIoBufferElements(out numberOfElements);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(numberOfElements, (UInt32)2);

            hr = ioBuffer0.QuerySize(out ioBuffer0Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer0Size, (UInt32)4096 + (1024 * 1024));

            hr = ioBuffer0.First(out token);
            VerifyAreEqual(hr, (UInt32)0);

            hr = ioBuffer0.QueryElementInfo(token, out buffer1, out ioBuffer0Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer0Size, (UInt32)4096);

            hr = ioBuffer0.Next(token, out token);
            VerifyAreEqual(hr, (UInt32)0);

            hr = ioBuffer0.QueryElementInfo(token, out buffer1, out ioBuffer0Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer0Size, (UInt32)1024 * 1024);

            hr = ioBuffer0.Next(token, out token);
            VerifyAreNotEqual(hr, (UInt32)0);

            //** Prove resetting the state of an IoBuffer
            hr = ioBuffer0.Clear();
            VerifyAreEqual(hr, (UInt32)0);

            hr = ioBuffer0.QueryNumberOfIoBufferElements(out numberOfElements);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(numberOfElements, (UInt32)0);

            hr = ioBuffer0.QuerySize(out ioBuffer0Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(ioBuffer0Size, (UInt32)0);

            //*** IKArray<GUID> Test
            NativeLog.IKArray guidArray1;
            hr = NativeLog.CreateGuidIKArray(100, out guidArray1);
            VerifyAreEqual(hr, (UInt32)0);

            UInt32 guidArray1Size;
            hr = guidArray1.GetCount(out guidArray1Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(guidArray1Size, (UInt32)0);

            Guid[] guids = new Guid[100];
            for (int ix = 0; ix < 100; ix++)
            {
                guids[ix] = Guid.NewGuid();
                hr = guidArray1.AppendGuid(guids[ix]);
                VerifyAreEqual(hr, (UInt32)0);
            }

            hr = guidArray1.GetCount(out guidArray1Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(guidArray1Size, (UInt32)100);

            void* array1base = null;
            hr = guidArray1.GetArrayBase(out array1base);
            VerifyAreEqual(hr, (UInt32)0);

            int index = 0;
            for (GuidStruct* p = (GuidStruct*)array1base; p < &((GuidStruct*)array1base)[100]; p++)
            {
                Guid g = p->FromBytes();
                VerifyAreEqual(g, guids[index]);

                index++;
            }

            //*** IKArray<RecordMetadata> Test
            NativeLog.IKArray mdArray1;
            hr = NativeLog.CreateLogRecordMetadataIKArray(100, out mdArray1);
            VerifyAreEqual(hr, (UInt32)0);

            UInt32 mdArray1Size;
            hr = mdArray1.GetCount(out mdArray1Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(mdArray1Size, (UInt32)0);

            NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata[] mdItems = new NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata[100];
            for (int ix = 0; ix < 100; ix++)
            {
                NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata item = new NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata(
                    (UInt64)ix,
                    (UInt64)1,
                    NativeLog.KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION.DispositionPersisted,
                    5000,
                    0);

                mdItems[ix] = item;
                hr = mdArray1.AppendRecordMetadata(item);
                VerifyAreEqual(hr, (UInt32)0);
            }

            hr = mdArray1.GetCount(out mdArray1Size);
            VerifyAreEqual(hr, (UInt32)0);
            VerifyAreEqual(mdArray1Size, (UInt32)100);

            hr = mdArray1.GetArrayBase(out array1base);
            VerifyAreEqual(hr, (UInt32)0);

            index = 0;
            for (NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata* p = (NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata*)array1base;
                 p < &((NativeLog.KPHYSICAL_LOG_STREAM_RecordMetadata*)array1base)[100]; p++)
            {
                Verify.AreEqual(*p, mdItems[index]);

                index++;
            }
        }

        [VS.TestMethod]
        [VS.Owner("richhas")]
        [MethodImpl(MethodImplOptions.NoInlining)]
        public unsafe void KIoBufferStreamTest()
        {
            NativeLog.IKIoBufferElement bfr0;
            NativeLog.IKIoBufferElement bfr1;

            byte* ptr0;
            byte* ptr1;
            const UInt32 sizeOfFirstBuffer = 0x2f000;
            const UInt32 sizeOfIoBuffer = 0x73e000;
            const UInt32 sizeOfSecondBuffer = sizeOfIoBuffer - sizeOfFirstBuffer;

            // Test KIoBufferStream fetching buffer ptr after skipping past the last location in an KIoBufferElement
            NativeLog.CreateKIoBufferElement(sizeOfFirstBuffer, out bfr0);
            {
                void* p;
                bfr0.GetBuffer(out p);
                ptr0 = (byte*)p;
            }

            NativeLog.CreateKIoBufferElement(sizeOfSecondBuffer, out bfr1);
            {
                void* p;
                bfr1.GetBuffer(out p);
                ptr1 = (byte*)p;
            }

            NativeLog.IKIoBuffer ioBuffer;
            NativeLog.CreateEmptyKIoBuffer(out ioBuffer);

            // Prove element boundary cases
            byte[] testSpace = new byte[10];
            byte[] tempSpace = new byte[10];

            Action
            ClearTestSpace = () =>
            {
                for (byte ix = 0; ix < testSpace.Length; ix++)
                {
                    testSpace[ix] = ix;
                }
            };

            Action<byte[], string>
            StrToBytes = (byte[] Dest, string Src) =>
            {
                if (!(Dest.Length >= Src.Length))
                {
                    VerifyIsTrue(Dest.Length >= Src.Length);
                }
                fixed (byte* destPtr = &Dest[0]) for (int ix = 0; ix < Src.Length; ix++)
                    {
                        destPtr[ix] = Convert.ToByte(Src[ix]);
                    }
            };

            Action
            ClearBuffers = () =>
            {
                for (int ix = 0; ix < sizeOfFirstBuffer; ix++)
                    ptr0[ix] = 0;

                for (int ix = 0; ix < sizeOfSecondBuffer; ix++)
                    ptr1[ix] = 0;
            };


            fixed (byte* testSpacePtr = &testSpace[0])
            fixed (byte* tempSpacePtr = &tempSpace[0])
            {
                ioBuffer.AddIoBufferElement(bfr0);
                ioBuffer.AddIoBufferElement(bfr1);
                KIoBufferStream stream = new KIoBufferStream(ioBuffer, 0);

                //** Prove we can limit the length of the view over the KIoBuffer
                {
                    uint overallSize;
                    ioBuffer.QuerySize(out overallSize);

                    if (!(stream.GetLength() == overallSize))
                    {
                        VerifyIsTrue((stream.GetLength() == overallSize));
                    }

                    stream.Reuse(ioBuffer, 0, overallSize - 1);

                    if (!(stream.GetLength() == (overallSize - 1)))
                    {
                        VerifyIsTrue((stream.GetLength() == (overallSize - 1)));
                    }

                    stream.Reuse(ioBuffer, 0);
                    VerifyIsTrue((stream.GetLength() == overallSize));
                }


                //** 1st byte of 2nd buffer
                VerifyIsTrue(stream.Skip(sizeOfFirstBuffer));

                byte* edgePosition = stream.GetBufferPointer();

                // Prove memory access
                *edgePosition = (byte)'X';
                VerifyIsTrue(((*edgePosition) == (byte)'X'));
                VerifyIsTrue(stream.GetBufferPointerRange() == sizeOfSecondBuffer);

                // Prove Put
                tempSpacePtr[0] = (byte)'Y';
                VerifyIsTrue(stream.Put(&tempSpacePtr[0], 1));
                VerifyIsTrue((*edgePosition) == (byte)'Y');
                VerifyIsTrue(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));

                // Prove Pull
                VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer));
                VerifyIsTrue(stream.GetBufferPointerRange() == sizeOfSecondBuffer);
                ClearTestSpace();

                VerifyIsTrue(stream.Pull(testSpacePtr, 1));
                VerifyIsTrue(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));
                VerifyIsTrue(testSpace[0] == (byte)'Y');

                // Prove CopyTo
                *tempSpacePtr = (byte)'A';
                VerifyIsTrue(KIoBufferStream.CopyTo(ioBuffer, sizeOfFirstBuffer, 1, tempSpacePtr));
                VerifyIsTrue((*edgePosition) == (byte)'A');

                // Prove CopyFrom
                ClearTestSpace();
                VerifyIsTrue(KIoBufferStream.CopyFrom(ioBuffer, sizeOfFirstBuffer, 1, testSpacePtr));
                VerifyIsTrue(testSpace[0] == (byte)'A');

                //** Last byte of 1st buffer
                VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 1));
                edgePosition = stream.GetBufferPointer();
                *edgePosition = (byte)'Z';
                VerifyIsTrue((*edgePosition) == (byte)'Z');
                VerifyIsTrue(stream.GetBufferPointerRange() == (uint)1);

                // Prove Put
                *tempSpacePtr = (byte)'Q';
                VerifyIsTrue(stream.Put(tempSpacePtr, 1));
                VerifyIsTrue((*edgePosition) == (byte)'Q');
                VerifyIsTrue(stream.GetBufferPointerRange() == sizeOfSecondBuffer);

                // Prove Pull
                VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 1));
                VerifyIsTrue(stream.GetBufferPointerRange() == 1);
                ClearTestSpace();
                VerifyIsTrue(stream.Pull(testSpacePtr, 1));
                VerifyIsTrue(stream.GetBufferPointerRange() == sizeOfSecondBuffer);
                VerifyIsTrue(testSpace[0] == (byte)'Q');

                // Prove CopyTo - partial fast path
                *tempSpacePtr = (byte)'B';
                VerifyIsTrue(KIoBufferStream.CopyTo(ioBuffer, sizeOfFirstBuffer - 1, 1, tempSpacePtr));
                VerifyIsTrue((*edgePosition) == (byte)'B');

                // Prove CopyFrom - partial fast path
                ClearTestSpace();
                VerifyIsTrue(KIoBufferStream.CopyFrom(ioBuffer, sizeOfFirstBuffer - 1, 1, testSpacePtr));
                VerifyIsTrue(testSpace[0] == (byte)'B');

                //** Cross buffer boundary
                VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 1));
                VerifyIsTrue(stream.GetBufferPointerRange() == 1);

                // Prove Put
                StrToBytes(tempSpace, "CD");
                VerifyIsTrue(stream.Put(tempSpacePtr, 2));
                VerifyIsTrue(*(ptr0 + sizeOfFirstBuffer - 1) == (byte)'C');
                VerifyIsTrue(*ptr1 == (byte)'D');
                VerifyIsTrue(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));

                // Prove Pull
                VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 1));
                VerifyIsTrue(stream.GetBufferPointerRange() == 1);
                ClearTestSpace();
                VerifyIsTrue(stream.Pull(testSpacePtr, 2));
                VerifyIsTrue(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));
                VerifyIsTrue(testSpace[0] == (byte)'C');
                VerifyIsTrue(testSpace[1] == (byte)'D');

                // Prove CopyTo
                StrToBytes(tempSpace, "EF");
                VerifyIsTrue(KIoBufferStream.CopyTo(ioBuffer, sizeOfFirstBuffer - 1, 2, tempSpacePtr));
                VerifyIsTrue(*(ptr0 + sizeOfFirstBuffer - 1) == (byte)'E');
                VerifyIsTrue(*ptr1 == (byte)'F');

                // Prove CopyFrom
                ClearTestSpace();
                VerifyIsTrue(KIoBufferStream.CopyFrom(ioBuffer, sizeOfFirstBuffer - 1, 2, testSpacePtr));
                VerifyIsTrue(testSpace[0] == (byte)'E');
                VerifyIsTrue(testSpace[1] == (byte)'F');

                //* Prove over-CopyTo and CopyFrom fails
                StrToBytes(tempSpace, "XX");
                VerifyIsTrue(!KIoBufferStream.CopyTo(ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer - 1, 2, tempSpacePtr));
                VerifyIsTrue(!KIoBufferStream.CopyTo(ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer, 1, tempSpacePtr));
                VerifyIsTrue(!KIoBufferStream.CopyFrom(ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer - 1, 2, testSpacePtr));
                VerifyIsTrue(!KIoBufferStream.CopyFrom(ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer, 1, testSpacePtr));

                //* Prove single buffer optimatzation paths for CopyTo/CopyFrom
                ioBuffer.Clear();
                ioBuffer.AddIoBufferElement(bfr0);
                stream.Reset();
                VerifyIsTrue(stream.Reuse(ioBuffer, 0));

                //* Prove CopyTo - full fast path
                StrToBytes(tempSpace, "ST");
                VerifyIsTrue(KIoBufferStream.CopyTo(ioBuffer, sizeOfFirstBuffer - 2, 2, tempSpacePtr));
                VerifyIsTrue(*(ptr0 + sizeOfFirstBuffer - 2) == (byte)'S');
                VerifyIsTrue(*(ptr0 + sizeOfFirstBuffer - 1) == (byte)'T');

                //* Prove CopyFrom - full fast path
                ClearTestSpace();
                VerifyIsTrue(KIoBufferStream.CopyFrom(ioBuffer, sizeOfFirstBuffer - 2, 2, testSpacePtr));
                VerifyIsTrue(testSpace[0] == (byte)'S');
                VerifyIsTrue(testSpace[1] == (byte)'T');

                //* Prove large typed pull and put
                ioBuffer.Clear();
                ioBuffer.AddIoBufferElement(bfr0);
                ioBuffer.AddIoBufferElement(bfr1);
                stream.Reset();
                VerifyIsTrue(stream.Reuse(ioBuffer, 0));

                VerifyIsTrue(stream.PositionTo(0));
                UInt32 putSize = (stream.GetLength() - 1) / sizeof(UInt16);
                fixed (UInt16* p = new UInt16[putSize])
                {
                    for (int ix = 0; ix < putSize; ix++)
                    {
                        p[ix] = (UInt16)ix;
                    }

                    Boolean b = stream.Put((byte*)p, putSize * sizeof(UInt16));
                    if (!b)
                    {
                        VerifyIsTrue(b);
                    }
                }

                VerifyIsTrue(stream.PositionTo(0));
                fixed (UInt16* p = new UInt16[putSize])
                {
                    VerifyIsTrue(stream.Pull((byte*)p, putSize * sizeof(UInt16)));

                    for (int ix = 0; ix < putSize; ix++)
                    {
                        Boolean b = p[ix] == (UInt16)ix;
                        if (!b)
                        {
                            VerifyIsTrue(b);
                        }
                    }
                }

                //* Prove typed scalars pull and put
                {
                    // UInt64
                    {
                        UInt64 testUInt_0 = 52;
                        UInt64 testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(*((UInt64*)ptr0) == testUInt_0);
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }

                    // And now across a buffer boundary
                    {
                        UInt64 testUInt_0 = UInt64.MaxValue - 1;
                        UInt64 testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 7));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 7));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }
                }

                {
                    // UInt32
                    {
                        UInt32 testUInt_0 = 52;
                        UInt32 testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(*((UInt32*)ptr0) == testUInt_0);
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }

                    // And now across a buffer boundary
                    {
                        UInt32 testUInt_0 = UInt32.MaxValue - 1;
                        UInt32 testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 3));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 3));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }
                }

                {
                    // UInt16
                    {
                        UInt16 testUInt_0 = 52;
                        UInt16 testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(*((UInt16*)ptr0) == testUInt_0);
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }

                    // And now across a buffer boundary
                    {
                        UInt16 testUInt_0 = UInt16.MaxValue - 1;
                        UInt16 testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 1));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 1));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }
                }

                {
                    // byte
                    {
                        byte testUInt_0 = 52;
                        byte testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(*((byte*)ptr0) == testUInt_0);
                        VerifyIsTrue(stream.PositionTo(0));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }

                    // And now across a buffer boundary
                    {
                        byte testUInt_0 = byte.MaxValue - 1;
                        byte testUInt_1 = 0;

                        ClearBuffers();
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 0));
                        VerifyIsTrue(stream.Put(testUInt_0));
                        VerifyIsTrue(stream.PositionTo(sizeOfFirstBuffer - 0));
                        VerifyIsTrue(stream.Pull(out testUInt_1));
                        VerifyIsTrue(testUInt_0 == testUInt_1);
                    }
                }

                //* Gather some basic perf numbers
                // Time interop call overhead
                {
                    const ulong reps = 100000000;
                    ulong todo = reps;

                    DateTime start = DateTime.Now;
                    while (todo-- > 0)
                    {
                        NativeLog.CopyMemory(null, null, 0);
                    }
                    DateTime stop = DateTime.Now;

                    TimeSpan runTime = stop - start;

                    if (runTime.Seconds != 0)
                    {
                        Debug.WriteLine(
                            String.Format(
                                "{0} calls made in {1} secs; {2} calls / sec; {3} secs / call",
                                reps,
                                runTime.Seconds,
                                reps / (uint)runTime.Seconds,
                                1 / (double)(reps / (uint)runTime.Seconds)));
                    }
                }

                fixed (byte* p = new byte[putSize])
                {
                    ulong limit = 100 * (1024 * 1024 * (ulong)1024);      // 100 GB
                    ulong leftToDo = limit;

                    DateTime start = DateTime.Now;
                    while (leftToDo > 0)
                    {
                        UInt64 todo = Math.Min(leftToDo, putSize);

                        Boolean b = stream.PositionTo(0);
                        if (!b)
                        {
                            VerifyIsTrue(b);
                        }

                        b = stream.Put(p, (UInt32)todo);
                        if (!b)
                        {
                            VerifyIsTrue(b);
                        }
                        leftToDo -= todo;
                    }

                    DateTime stop = DateTime.Now;
                    TimeSpan runTime = stop - start;

                    if (runTime.Seconds != 0)
                    {
                        Debug.WriteLine(
                            String.Format(
                                "{0} GB Transfered in {1} secs; {2} bytes / sec; in {3} blocks of {4} bytes each",
                                limit,
                                runTime.Seconds,
                                limit / (uint)runTime.Seconds,
                                limit / putSize,
                                putSize));
                    }
                }
            }
        }

        [VS.TestMethod]
        [VS.Owner("richhas")]
        public unsafe void BasicLogInteropTest()
        {
            //*** IKPhysicalLogManager Test
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                PhysicalLogManager logMgr;

                //
                // Try to open driver first and if it fails then try inproc
                //
                try
                {
                    logMgr = new PhysicalLogManager(LogManager.LoggerType.Driver);
                    logMgr.OpenAsync(CancellationToken.None).Wait();
                }
                catch (Exception)
                {
                    //
                    // Don't fail if driver isn't loaded
                    //
                    logMgr = null;
                }

                if (logMgr == null)
                {
                    //
                    // Now try inproc
                    //
                    logMgr = new PhysicalLogManager(LogManager.LoggerType.Inproc);
                    logMgr.OpenAsync(CancellationToken.None).Wait();
                }

                //** Prove we can determine the test vol id
                Guid diskId;
                {
                    Task<Guid> t = logMgr.GetVolumeIdFromPathAsync("\\??\\c:", CancellationToken.None);
                    t.Wait();
                    diskId = t.Result;
                }

                Guid logId = Guid.NewGuid();
                string logName = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", logName);

                try
                {
                    Task t = logMgr.DeleteLogContainerAsync(
                    logName,
                    logId,
                    CancellationToken.None);

                    t.Wait();
                }
                catch (Exception)
                {
                    // ok if this fails
                }

                //** Prove we can create a physical log
                IPhysicalLogContainer physContainer;
                {
                    Task<IPhysicalLogContainer> t = logMgr.CreateContainerAsync(
                        logName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0,
                        GetRandomCreationFlags(),
                        CancellationToken.None);

                    t.Wait();
                    physContainer = t.Result;
                }

                //** Prove we can close physical container
                physContainer.CloseAsync(CancellationToken.None).Wait();
                physContainer = null;

                //** Prove we can re-open the closed container
                {
                    Task<IPhysicalLogContainer> t = logMgr.OpenLogContainerAsync(
                        logName,
                        logId,
                        CancellationToken.None);

                    t.Wait();
                    physContainer = t.Result;
                }
                physContainer.CloseAsync(CancellationToken.None).Wait();

                //** Prove we can delete the log
                {
                    Task t = logMgr.DeleteLogContainerAsync(
                        logName,
                        logId,
                        CancellationToken.None);

                    t.Wait();
                }

                //** Prove we can re-create after delete
                {
                    Task<IPhysicalLogContainer> t = logMgr.CreateContainerAsync(
                        logName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0,
                        GetRandomCreationFlags(),
                        CancellationToken.None);

                    t.Wait();
                    physContainer = t.Result;
                }

                //** Prove we can create a new stream
                Guid logicalLogId = Guid.NewGuid();
                string logicalLogName = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", logicalLogName);

                IPhysicalLogStream logicalLog;
                {
                    Task<IPhysicalLogStream> t = physContainer.CreatePhysicalLogStreamAsync(
                        logicalLogId,
                        Guid.Empty,
                        null,
                        logicalLogName,
                        null,
                        10 * 1024 * 1024,
                        64 * 1024,
                        GetRandomCreationFlags(),
                        CancellationToken.None);

                    t.Wait();
                    logicalLog = t.Result;
                }

                logicalLog.CloseAsync(CancellationToken.None).Wait();

                //** Prove we can re-open the logical log
                {
                    Task<IPhysicalLogStream> t = physContainer.OpenPhysicalLogStreamAsync(
                        logicalLogId,
                        CancellationToken.None);

                    t.Wait();
                    logicalLog = t.Result;
                }

                //** Prove we can assign an alias; resolve it; and remove it
                physContainer.AssignAliasAsync(logicalLogId, "TestAlias1", CancellationToken.None).Wait();
                {
                    Task<Guid> t = physContainer.ResolveAliasAsync("TestAlias1", CancellationToken.None);
                    t.Wait();

                    VerifyAreEqual(t.Result, logicalLogId);

                    physContainer.RemoveAliasAsync("TestAlias1", CancellationToken.None).Wait();
                }

                //** Prove resolve fails
                try
                {
                    Task<Guid> t = physContainer.ResolveAliasAsync("TestAlias1", CancellationToken.None);
                    t.Wait();
                }
                catch (AggregateException aex)
                {
                    Debug.WriteLine(aex.ToString());
                    Exception ex = aex.GetBaseException();
                    Debug.WriteLine(ex.ToString());
                    COMException cex = ex.InnerException as COMException;
                    if ((cex == null) || (unchecked((int)0x80070490) != cex.ErrorCode))
                    {
                        // Not a ELEMENT NOT FOUND - as expected
                        throw aex;
                    }
                }

                logicalLog.CloseAsync(CancellationToken.None).Wait();

                //** Prove we can delete the stream
                {
                    Task t = physContainer.DeletePhysicalLogStreamAsync(
                        logicalLogId,
                        CancellationToken.None);

                    t.Wait();
                }

                physContainer.CloseAsync(CancellationToken.None).Wait();

                //** Clean up the container
                {
                    Task t = logMgr.DeleteLogContainerAsync(
                        logName,
                        logId,
                        CancellationToken.None);

                    t.Wait();
                }

                //** Prove we can close the PhysicalLogManager
                logMgr.CloseAsync(CancellationToken.None).Wait();
            },

            "BasicLogInteropTest");
        }

        [VS.TestMethod]
        [VS.Owner("richhas")]
        public unsafe void BasicLogStreamTest()
        {
            //*** IKPhysicalLogStream Test
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                PhysicalLogManager logMgr;

                //
                // Try to open driver first and if it fails then try inproc
                //
                try
                {
                    logMgr = new PhysicalLogManager(LogManager.LoggerType.Driver);
                    logMgr.OpenAsync(CancellationToken.None).Wait();
                }
                catch (Exception)
                {
                    //
                    // Don't fail if driver isn't loaded
                    //
                    logMgr = null;
                }

                if (logMgr == null)
                {
                    //
                    // Now try inproc
                    //
                    logMgr = new PhysicalLogManager(LogManager.LoggerType.Inproc);
                    logMgr.OpenAsync(CancellationToken.None).Wait();
                }

                //** Prove we can determine the test vol id
                Guid diskId;
                {
                    Task<Guid> t = logMgr.GetVolumeIdFromPathAsync("\\??\\c:", CancellationToken.None);
                    t.Wait();
                    diskId = t.Result;
                }

                Guid logId = Guid.NewGuid();
                string logName = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", logName);

                try
                {
                    Task t = logMgr.DeleteLogContainerAsync(
                    logName,
                    logId,
                    CancellationToken.None);

                    t.Wait();
                }
                catch (Exception)
                {
                    // ok if this fails
                }

                //** Prove we can create a physical log
                IPhysicalLogContainer physContainer;
                {
                    Task<IPhysicalLogContainer> t = logMgr.CreateContainerAsync(
                        logName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0,
                        GetRandomCreationFlags(),
                        CancellationToken.None);

                    t.Wait();
                    physContainer = t.Result;
                }

                //** Prove we can create a new stream
                Guid logicalLogId = Guid.NewGuid();
                string logicalLogName = PickUniqueLogFilename();
                Console.WriteLine("Using file: {0}", logicalLogName);

                IPhysicalLogStream logicalLog;
                {
                    Task<IPhysicalLogStream> t = physContainer.CreatePhysicalLogStreamAsync(
                        logicalLogId,
                        Guid.Empty,
                        null,
                        logicalLogName,
                        null,
                        10 * 1024 * 1024,
                        64 * 1024,
                        GetRandomCreationFlags(),
                        CancellationToken.None);

                    t.Wait();
                    logicalLog = t.Result;
                }

                //* Prove IsFunctional
                VerifyAreEqual(logicalLog.IsFunctional(), true);

                //* Prove QueryLogStreamId
                VerifyAreEqual(logicalLog.QueryLogStreamId(), logicalLogId);

                //* Prove QueryReservedMetadataSize
                {
                    uint size = logicalLog.QueryReservedMetadataSize();
                }

                //* Provide we can write a record
                const UInt32 sizeOfPageBuffers = 16 * 1024;
                const UInt32 sizeOfMdBuffers = 4 * 1024;
                NativeLog.IKIoBuffer pageBuffers;
                NativeLog.IKIoBuffer mdBuffer;

                UInt32 hr = NativeLog.CreateSimpleKIoBuffer(sizeOfPageBuffers, out pageBuffers);
                VerifyAreEqual(hr, (UInt32)0);

                {
                    // Fill with known data
                    KIoBufferStream stream = new KIoBufferStream(pageBuffers);

                    stream.Iterate(
                        (byte* Segment, ref UInt32 SegmentSize) =>
                        {
                            VerifyIsTrue((SegmentSize % sizeof(UInt32)) == 0);
                            UInt32* values = (UInt32*)Segment;

                            for (UInt32 ix = 0; ix < (SegmentSize / sizeof(UInt32)); ix++)
                            {
                                values[ix] = ix;
                            }

                            return 0;
                        },
                        sizeOfPageBuffers);
                }

                hr = NativeLog.CreateSimpleKIoBuffer(sizeOfMdBuffers, out mdBuffer);
                VerifyAreEqual(hr, (UInt32)0);

                logicalLog.WriteAsync(1, 0, 0, mdBuffer, pageBuffers, CancellationToken.None).Wait();

                pageBuffers = null;
                {
                    Task<PhysicalLogStreamAsnRangeInfo> t = logicalLog.QueryRecordRangeAsync(CancellationToken.None);
                    t.Wait();

                    VerifyAreEqual(t.Result.HighestAsn, (ulong)1);
                    VerifyAreEqual(t.Result.LowestAsn, (ulong)1);
                    VerifyAreEqual(t.Result.LogTruncationAsn, (ulong)0);
                }

                {
                    Task<PhysicalLogStreamReadResults> t = logicalLog.ReadAsync(1, CancellationToken.None);
                    t.Wait();

                    PhysicalLogStreamReadResults results = t.Result;
                    VerifyAreEqual(results.Version, (ulong)0);
                    VerifyAreEqual(results.ResultingMetadataSize, (ulong)0);
                    mdBuffer = results.ResultingMetadata;
                    pageBuffers = results.ResultingIoPageAlignedBuffer;

                    uint size;
                    pageBuffers.QuerySize(out size);
                    VerifyAreEqual(size, (uint)sizeOfPageBuffers);

                    // Prove data read back correctly
                    KIoBufferStream stream = new KIoBufferStream(pageBuffers);

                    stream.Iterate(
                        (byte* Segment, ref UInt32 SegmentSize) =>
                        {
                            VerifyIsTrue((SegmentSize % sizeof(UInt32)) == 0);
                            UInt32* values = (UInt32*)Segment;

                            for (UInt32 ix = 0; ix < (SegmentSize / sizeof(UInt32)); ix++)
                            {
                                VerifyIsTrue((values[ix] == ix));
                            }

                            return 0;
                        },
                        sizeOfPageBuffers);
                }

                //** Prove truncation API works
                {
                    logicalLog.Truncate(1, 1);
                    Thread.Sleep(1000);

                    Task<PhysicalLogStreamAsnRangeInfo> t = logicalLog.QueryRecordRangeAsync(CancellationToken.None);
                    t.Wait();

                    VerifyAreEqual(t.Result.HighestAsn, (ulong)1);
                    VerifyAreEqual(t.Result.LowestAsn, (ulong)1);
                    VerifyAreEqual(t.Result.LogTruncationAsn, (ulong)1);
                }

                //** Clean up the container
                logicalLog.CloseAsync(CancellationToken.None).Wait();
                physContainer.CloseAsync(CancellationToken.None).Wait();
                {
                    Task t = logMgr.DeleteLogContainerAsync(
                        logName,
                        logId,
                        CancellationToken.None);

                    t.Wait();
                }

                //** Prove we can close the PhysicalLogManager
                logMgr.CloseAsync(CancellationToken.None).Wait();
            },

            "BasicLogInteropTest");
        }


        // **
        //* Local functions
        ILogManager CreateLogManager()
        {
            Task<ILogManager> t = LogManager.OpenAsync(LogManager.LoggerType.Default, CancellationToken.None);
            t.Wait();
            return t.Result;
        }

        IPhysicalLog CreatePhysicalLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId,
            long containerSize,
            uint maximumNumberStreams,
            uint maximumLogicalLogBlockSize)
        {
            Task<IPhysicalLog> t = manager.CreatePhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                containerSize,
                maximumNumberStreams,
                maximumLogicalLogBlockSize,
                GetRandomCreationFlags(),
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        IPhysicalLog OpenPhysicalLog(
            ILogManager manager,
            string pathToCommonContainer,
            Guid physicalLogId)
        {
            Task<IPhysicalLog> t = manager.OpenPhysicalLogAsync(
                pathToCommonContainer,
                physicalLogId,
                false,
                CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        int ReadLogicalLog(
            ILogicalLog logicalLog,
            byte[] buffer,
            int readOffset,
            int bufferOffset,
            int count)
        {
            if (readOffset != -1)
            {
                logicalLog.SeekForRead(readOffset, IO.SeekOrigin.Begin);
            }

            Task<int> t = logicalLog.ReadAsync(buffer, bufferOffset, count, 0, CancellationToken.None);

            t.Wait();
            return t.Result;
        }

        void AppendLogicalLog(
            ILogicalLog logicalLog,
            byte[] buffer)
        {
            Task t = logicalLog.AppendAsync(buffer, 0, buffer.Length, CancellationToken.None);

            t.Wait();
        }

        // Truncate the oldest information
        void TruncateHead(
            ILogicalLog logicalLog,
            long streamOffset)
        {
            logicalLog.TruncateHead(streamOffset);
        }

        // Truncate the most recent information
        void TruncateTail(
            ILogicalLog logicalLog,
            long streamOffset)
        {
            Task t = logicalLog.TruncateTail(streamOffset, CancellationToken.None);

            t.Wait();
        }

        byte[] BuildBuffer(int count)
        {
            byte[] result = new byte[count];

            for (int ix = 0; ix < result.Length; ix++)
                result[ix] = (byte)(ix % count);

            return result;
        }

        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void WriteAtTruncationPointsTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = CreateLogManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    string physLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", physLogName);

                    Guid logId = Guid.NewGuid();

                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }

                    
                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    //** Prove simple recovery
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    lLog = OpenLogicalLog(phyLog, lLogId);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);


                    //
                    // Test 1: Write all 1's from 0 - 999, truncate tail (newest data) at 500, write 2's at 500 - 599
                    // and read back verifying that 0 - 499 are 1's and 500-599 are 2's. Read back at 499, 500 and 501
                    // and verify that those are correct.
                    //
                    byte[] buffer1 = new byte[1000];
                    for (ulong i = 0; i < 1000; i++)
                    {
                        buffer1[i] = 1;
                    }

                    lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    VerifyIsTrue(lLog.WritePosition == 1000);

                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    TruncateTail(lLog, 500);
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                    VerifyIsTrue(lLog.WritePosition == 500);

                    byte[] buffer2 = new byte[100];
                    for (ulong i = 0; i < 100; i++)
                    {
                        buffer2[i] = 2;
                    }
                    lLog.AppendAsync(buffer2, 0, buffer2.Length, CancellationToken.None).Wait();
                    VerifyIsTrue(lLog.WritePosition == 600);

                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    byte[] bufferRead = new byte[600];
                    ReadLogicalLog(lLog, bufferRead, -1, 0, 600);

                    for (ulong i = 0; i < 500; i++)
                    {
                        VerifyIsTrue(bufferRead[i] == 1);
                    }

                    for (ulong i = 500; i < 600; i++)
                    {
                        VerifyIsTrue(bufferRead[i] == 2);
                    }

                    byte[] bufferRead499 = new byte[1];
                    ReadLogicalLog(lLog, bufferRead499, 499, 0, 1);
                    VerifyIsTrue(bufferRead499[0] == 1);

                    byte[] bufferRead500 = new byte[1];
                    ReadLogicalLog(lLog, bufferRead500, 500, 0, 1);
                    VerifyIsTrue(bufferRead500[0] == 2);

                    byte[] bufferRead501 = new byte[1];
                    ReadLogicalLog(lLog, bufferRead501, 501, 0, 1);
                    VerifyIsTrue(bufferRead501[0] == 2);

                    
                    //
                    // Test 2: Truncate head (oldest data) at 500 and try to truncate tail (newest data) to 490 and expect failure.
                    // Truncate tail back to 500 and then write and then read back
                    //
                    TruncateHead(lLog, 499);

                    Boolean hitException = false;

                    try
                    {
                        TruncateTail(lLog, 490);
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        hitException = true;
                    }
                    VerifyIsTrue(hitException);
                    
                    TruncateTail(lLog, 500);
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    byte[] buffer3 = new byte[50];
                    for (ulong i = 0; i < 50; i++)
                    {
                        buffer3[i] = 3;
                    }
                    lLog.AppendAsync(buffer3, 0, buffer3.Length, CancellationToken.None).Wait();
                    VerifyIsTrue(lLog.WritePosition == 550);

                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    byte[] bufferRead3 = new byte[50];

                    ReadLogicalLog(lLog, bufferRead3, 500, 0, 50);

                    for (ulong i = 0; i < 50; i++)
                    {
                        VerifyIsTrue(bufferRead3[i] == 3);
                    }

                    //
                    // Test 3: truncate tail all the way back to the head
                    //
                    long headTruncationPosition = lLog.HeadTruncationPosition;
                    TruncateTail(lLog, headTruncationPosition+1);

                    //* Prove close down 
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "WriteAtTruncationPointsTest");
        }

        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void RemoveFalseProgressTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = CreateLogManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    string physLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", physLogName);

                    Guid logId = Guid.NewGuid();

                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }


                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    //
                    // Test 1: Write 100000 bytes and then close the log. Reopen and then truncate
                    //         tail at 80000. Next write and flush 1000 more. Verify no failure on last 
                    //         write.
                    //
                    byte[] buffer1 = new byte[1000];
                    for (ulong i = 0; i < 1000; i++)
                    {
                        buffer1[i] = 1;
                    }

                    for (int i = 0; i < 100; i++)
                    {
                        lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                        VerifyIsTrue(lLog.WritePosition == (i + 1) * 1000);
                    }

                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    //* Prove close down 
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;


                    lLog = OpenLogicalLog(phyLog, lLogId);

                    TruncateTail(lLog, 80000);

                    lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    //* Prove close down 
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "RemoveFalseProgressTest");
        }


        internal void BuildDataBuffer(ref byte[] buffer, long positionInStream, int entropy = 0)
        {
            //
            // Build a data buffer that represents the position in the log. Buffers built
            // are exactly reproducible as they are used for writing and reading
            //

            int len = buffer.Length;

            for (int i = 0; i < len; i++)
            {
                long pos = positionInStream + i;
                long value = (pos * pos) + pos + entropy;
                buffer[i] = (byte)(value % 255);
            }
        }

        internal bool ValidateDataBuffer(ref byte[] buffer, int length, int offsetInBuffer, long positionInStream, int entropy = 0)
        {
            //
            // Build a data buffer that represents the position in the log. Buffers built
            // are exactly reproducible as they are used for writing and reading
            //

            for (int i = 0; i < length; i++)
            {
                long position = positionInStream + i;
                long value = (position * position) + position + entropy;

                byte b = (byte)(value % 255);

                if (buffer[i + offsetInBuffer] != b)
                {
                    Console.WriteLine("Data error {0} expecting {1} at stream offset {2}", buffer[i + offsetInBuffer], b, positionInStream + i);
                    Debugger.Break();
                }
            }

            return (true);
        }


        public void VerifyIsTrue(bool condition, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (!condition)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.IsTrue(condition, s);
            }
        }

        public void VerifyIsFalse(bool condition, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (condition)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.IsFalse(condition, s);
            }
        }

        public void VerifyAreEqual(ulong a, ulong b, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (a != b)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.AreEqual(a, b, s);
            }
        }

        public void VerifyAreEqual(bool a, bool b, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (a != b)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.AreEqual(a, b, s);
            }
        }

        public void VerifyAreEqual(UInt32 a, UInt32 b, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (a != b)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.AreEqual(a, b, s);
            }
        }

        public void VerifyAreEqual(Guid a, Guid b, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (a != b)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.AreEqual(a, b, s);
            }
        }


        public void VerifyAreNotEqual(UInt32 a, UInt32 b, [CallerFilePath] string filePath = "", [CallerLineNumber] int lineNumber = 0)
        {
            if (a == b)
            {
                string s = string.Format("File: {0}, Line: {1}", filePath, lineNumber);
                Verify.AreNotEqual(a, b, s);
            }
        }


        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void ReadAheadCacheTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = CreateLogManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    string physLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", physLogName);

                    Guid logId = Guid.NewGuid();

                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }


                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 1024 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 512, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);


                    //
                    // Write a nice pattern in the log which would be easy to validate the data
                    //
                    int dataSize = 16 * 1024 * 1024;     // 16MB
                    int chunkSize = 1024;
                    int prefetchSize = 1024 * 1024;
                    int loops = dataSize / chunkSize;

                    byte[] bufferBig = new byte[2*prefetchSize];
                    byte[] buffer1 = new byte[chunkSize];
                    for (int i = 0; i < loops; i++)
                    {
                        long pos = i * chunkSize;
                        BuildDataBuffer(ref buffer1, pos);
                        lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    }
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    VerifyIsTrue(lLog.Length == dataSize);

                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    lLog = OpenLogicalLog(phyLog, lLogId);
                    VerifyIsTrue(lLog.Length == dataSize);
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;


                    //
                    // Test 1: Sequentially read through file and verify that data is correct
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        for (int i = 0; i < loops; i++)
                        {
                            bool b;
                            int read = stream.Read(buffer1, 0, chunkSize);

                            long pos = i * chunkSize;
                            b = ValidateDataBuffer(ref buffer1, read, 0, pos);
                            VerifyIsTrue(b);
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 2: Read at the end of the file to trigger a prefetch read beyond the end of the file is where the 
                    //         last prefetch isn't a full record.
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        long pos = dataSize - (prefetchSize + (prefetchSize / 2));
                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(buffer1, 0, chunkSize);

                        bool b = ValidateDataBuffer(ref buffer1, read, 0, pos);
                        VerifyIsTrue(b);

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 3: Read at the end of the file to trigger a prefetch read that is exactly the end of the file.
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        long pos = dataSize - 2 * prefetchSize;
                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(buffer1, 0, chunkSize);

                        bool b = ValidateDataBuffer(ref buffer1, chunkSize, 0, pos);
                        VerifyIsTrue(b);

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 4: Read at end of file where read isn't a full record and prefetch read is out of bounds.
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        long pos = dataSize - (prefetchSize / 2);
                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(buffer1, 0, chunkSize);

                        bool b = ValidateDataBuffer(ref buffer1, read, 0, pos);
                        VerifyIsTrue(b);

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 5: Read beyond end of file.
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        long pos = dataSize - 1;
                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(buffer1, 0, chunkSize);

                        bool b = ValidateDataBuffer(ref buffer1, read, 0, pos);
                        VerifyIsTrue(b);

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }
                    
                    //
                    // Test 6: Random access reads of less than a prefetch record size and verify that data is correct
                    //
                    {
                        Random rnd = new Random();
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        for (int i = 0; i < 1024; i++)
                        {
                            int len = rnd.Next(prefetchSize - 1);
                            byte[] buffer = new byte[len];

                            //
                            // Pick a random place but do not go beyond end of file
                            //
                            long pos = rnd.Next(len, dataSize) - len;
                            stream.Seek(pos, SeekOrigin.Begin);

                            bool b;
                            int read = stream.Read(buffer, 0, len);

                            VerifyIsTrue(len == read);
                            b = ValidateDataBuffer(ref buffer, read, 0, pos);
                            VerifyIsTrue(b);
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        int count = 56;
                        int[] pos1 = new int[count];
                        int[] len1 = new int[count];
                        pos1[0] = 6643136;
                        len1[0] = 284158;
                        pos1[1] = 8749250;
                        len1[1] = 719631;
                        pos1[2] = 7968798;
                        len1[2] = 217852;
                        pos1[3] = 5589085;
                        len1[3] = 37828;
                        pos1[4] = 8951191;
                        len1[4] = 658471;
                        pos1[5] = 36603;
                        len1[5] = 779626;
                        pos1[6] = 13515405;
                        len1[6] = 753509;
                        pos1[7] = 2026546;
                        len1[7] = 127354;
                        pos1[8] = 12065124;
                        len1[8] = 485889;
                        pos1[9] = 2922527;
                        len1[9] = 748745;
                        pos1[10] = 12362362;
                        len1[10] = 324286;
                        pos1[11] = 15079374;
                        len1[11] = 411635;
                        pos1[12] = 308588;
                        len1[12] = 515382;
                        pos1[13] = 7394979;
                        len1[13] = 535056;
                        pos1[14] = 9878541;
                        len1[14] = 445104;
                        pos1[15] = 6615835;
                        len1[15] = 924314;
                        pos1[16] = 11835666;
                        len1[16] = 259902;
                        pos1[17] = 8862263;
                        len1[17] = 949164;
                        pos1[18] = 5386414;
                        len1[18] = 517708;
                        pos1[19] = 11259477;
                        len1[19] = 675057;
                        pos1[20] = 14395727;
                        len1[20] = 973734;
                        pos1[21] = 9391625;
                        len1[21] = 1021568;
                        pos1[22] = 298673;
                        len1[22] = 858879;
                        pos1[23] = 11966559;
                        len1[23] = 170338;
                        pos1[24] = 5228510;
                        len1[24] = 331768;
                        pos1[25] = 12273354;
                        len1[25] = 711678;
                        pos1[26] = 13006046;
                        len1[26] = 865901;
                        pos1[27] = 8447558;
                        len1[27] = 519960;
                        pos1[28] = 12824484;
                        len1[28] = 10713;
                        pos1[29] = 3156718;
                        len1[29] = 55952;
                        pos1[30] = 9152582;
                        len1[30] = 1018119;
                        pos1[31] = 365742;
                        len1[31] = 953577;
                        pos1[32] = 5241993;
                        len1[32] = 706563;
                        pos1[33] = 32648;
                        len1[33] = 791071;
                        pos1[34] = 8868714;
                        len1[34] = 983844;
                        pos1[35] = 2129328;
                        len1[35] = 658494;
                        pos1[36] = 246918;
                        len1[36] = 101502;
                        pos1[37] = 6577103;
                        len1[37] = 266030;
                        pos1[38] = 12713793;
                        len1[38] = 814882;
                        pos1[39] = 7880614;
                        len1[39] = 107285;
                        pos1[40] = 12539585;
                        len1[40] = 898134;
                        pos1[41] = 1772169;
                        len1[41] = 145642;
                        pos1[42] = 1857848;
                        len1[42] = 971131;
                        pos1[43] = 6954824;
                        len1[43] = 620271;
                        pos1[44] = 6379690;
                        len1[44] = 231407;
                        pos1[45] = 11807777;
                        len1[45] = 576393;
                        pos1[46] = 7501047;
                        len1[46] = 291417;
                        pos1[47] = 5570240;
                        len1[47] = 763674;
                        pos1[48] = 15955814;
                        len1[48] = 1591;
                        pos1[49] = 7792962;
                        len1[49] = 967045;
                        pos1[50] = 2646688;
                        len1[50] = 277177;
                        pos1[51] = 12045117;
                        len1[51] = 820311;
                        pos1[52] = 8775100;
                        len1[52] = 723454;
                        pos1[53] = 13038687;
                        len1[53] = 699561;
                        pos1[54] = 1578657;
                        len1[54] = 591083;
                        pos1[55] = 3419389;
                        len1[55] = 778550;

                        for (int i = 0; i < len1.Length; i++)
                        {
                            int len = len1[i];
                            byte[] buffer = new byte[len];

                            //
                            // Pick a random place but do not go beyond end of file
                            //
                            long pos = pos1[i];
                            stream.Seek(pos, SeekOrigin.Begin);

                            bool b;
                            int read = stream.Read(buffer, 0, len);

                            VerifyIsTrue(len == read);
                            b = ValidateDataBuffer(ref buffer, read, 0, pos);
                            VerifyIsTrue(b);
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    {
                        Random rnd = new Random();
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        for (int i = 0; i < 4; i++)
                        {
                            int len = 996198;
                            byte[] buffer = new byte[len];

                            //
                            // Pick a random place but do not go beyond end of file
                            //
                            long pos = 15117795; 
                            stream.Seek(pos, SeekOrigin.Begin);

                            bool b;
                            int read = stream.Read(buffer, 0, len);

                            VerifyIsTrue(len == read);
                            b = ValidateDataBuffer(ref buffer, read, 0, pos);
                            VerifyIsTrue(b);
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 7: Random access reads of more than a prefetch record size and verify that data is correct
                    //
                    {
                        Random rnd = new Random();
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        for (int i = 0; i < 1024; i++)
                        {
                            int len = rnd.Next(3 * prefetchSize);
                            byte[] buffer = new byte[len];

                            //
                            // Pick a random place but do not go beyond end of file
                            //
                            long pos = rnd.Next(len, dataSize) - len;
                            stream.Seek(pos, SeekOrigin.Begin);

                            bool b;
                            int read = stream.Read(buffer, 0, len);

                            VerifyIsTrue(len == read);
                            b = ValidateDataBuffer(ref buffer, read, 0, pos);
                            VerifyIsTrue(b);
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 8: Read section of file and then truncate head in the middle of the read buffer. Ensure reading truncated 
                    //         space fails and untruncated succeeds
                    //
                    //
                    // Test 9: Read section of file and then truncate head in the middle of the prefetch read buffer. Ensure reading truncated 
                    //         space fails and untruncated succeeds
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        //
                        // Read a bunch of stuff
                        //
                        long pos = 512;
                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(bufferBig, 0, chunkSize);
                        bool b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);

                        //
                        // truncate head in the middle of the prefetch buffer
                        //
                        lLog.TruncateHead(prefetchSize + 4096);

                        //
                        // reads should not work
                        //
                        read = stream.Read(bufferBig, 0, chunkSize);
                        VerifyIsTrue(read == 0);

                        pos = 4096;
                        stream.Seek(pos, SeekOrigin.Begin);
                        read = stream.Read(bufferBig, 0, chunkSize);
                        VerifyIsTrue(read == 0);

                        pos = prefetchSize + 256;
                        stream.Seek(pos, SeekOrigin.Begin);
                        read = stream.Read(bufferBig, 0, chunkSize);
                        VerifyIsTrue(read == 0);

                        //
                        // read in untruncated space
                        //
                        pos = prefetchSize + 4096 + 512;
                        stream.Seek(pos, SeekOrigin.Begin);
                        read = stream.Read(bufferBig, 0, chunkSize);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 10: Read section of file and then truncate tail in the middle of the read buffer. Ensure reading truncated 
                    //          space fails and untruncated succeeds
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        //
                        // Test 10a: Fill read buffer and truncate tail ahead of it. Verify that only data before truncate tail
                        //           is read and is correct
                        //
                        long pos = dataSize - (16384 + 4096);
                        long tailTruncatePos = pos + (8192 + 4096);

                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(bufferBig, 0, 4096);
                        bool b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        pos += read;
                        VerifyIsTrue(b);

                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        read = stream.Read(bufferBig, 0, 16384);
                        VerifyIsTrue(read == 8192);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - prefetchSize;
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }

                        //
                        // Test 10b: Fill read buffer and truncate before current read pointer. Verify no data read
                        //
                        pos = dataSize - (16384 + 4096);
                        stream.Seek(pos, SeekOrigin.Begin);
                        read = stream.Read(bufferBig, 0, 4096);
                        VerifyIsTrue(read == 4096);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        pos += read;
                        VerifyIsTrue(b);

                        tailTruncatePos = pos - 128;
                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        read = stream.Read(bufferBig, 0, chunkSize);
                        VerifyIsTrue(read == 0);

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - prefetchSize;
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }


                        //
                        // Test 10c: Fill read buffer and overwrite it with different data. Continue reading and verify that
                        //           new data is read
                        //
                        pos = dataSize - (16384 + 4096);
                        stream.Seek(pos, SeekOrigin.Begin);
                        read = stream.Read(bufferBig, 0, 4096);
                        VerifyIsTrue(read == 4096);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);
                        pos += read;

                        tailTruncatePos = pos - 128;
                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        long fillSize = dataSize - tailTruncatePos;
                        byte[] fillBuffer = new byte[fillSize];
                        BuildDataBuffer(ref fillBuffer, tailTruncatePos, 5);
                        lLog.AppendAsync(fillBuffer, 0, (int)fillSize, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                        read = stream.Read(bufferBig, 0, 256);
                        VerifyIsTrue(read == 256);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos, 5);
                        VerifyIsTrue(b);
                        pos += read;

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - prefetchSize;
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 11: Read section of file and then truncate tail in the middle of the prefetch read buffer. Ensure reading truncated 
                    //          space fails and untruncated succeeds
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        //
                        // Test 11a: Fill read buffer and truncate tail ahead of it. Verify that only data before truncate tail
                        //           is read and is correct
                        //
                        long pos = dataSize - (prefetchSize + 16384 + 4096);
                        long tailTruncatePos = pos + (prefetchSize + 8192 + 4096);

                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(bufferBig, 0, 4096);
                        bool b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        pos += read;
                        VerifyIsTrue(b);

                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        read = stream.Read(bufferBig, 0, 16384 + prefetchSize);
                        VerifyIsTrue(read == 8192 + prefetchSize);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - prefetchSize;
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }


                        //
                        // Test 11c: Fill read buffer and overwrite it with different data. Continue reading and verify that
                        //           new data is read
                        //
                        pos = dataSize - (prefetchSize + 16384 + 4096);
                        stream.Seek(pos, SeekOrigin.Begin);
                        read = stream.Read(bufferBig, 0, 4096);
                        VerifyIsTrue(read == 4096);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);
                        pos += read;

                        tailTruncatePos = (pos + prefetchSize) - 128;
                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        long fillSize = dataSize - tailTruncatePos;
                        byte[] fillBuffer = new byte[fillSize];
                        BuildDataBuffer(ref fillBuffer, tailTruncatePos, 5);
                        lLog.AppendAsync(fillBuffer, 0, (int)fillSize, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                        long read1Len = tailTruncatePos - pos;
                        read = stream.Read(bufferBig, 0, (int)read1Len);
                        VerifyIsTrue(read == (int)read1Len);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        VerifyIsTrue(b);
                        pos += read;

                        long read2Len = dataSize - tailTruncatePos;
                        read = stream.Read(bufferBig, 0, (int)read2Len);
                        VerifyIsTrue(read == (int)read2Len);
                        b = ValidateDataBuffer(ref bufferBig, read, 0, pos, 5);
                        VerifyIsTrue(b);
                        pos += read;

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - prefetchSize;
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 12: Write at the end of the file within the read buffer and verify that read succeeds with correct data
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        //
                        // Truncate tail to make space at the end of the file and then read at the end of the file to fill
                        // read buffer. Write at end of file and then continue to read the new data.
                        //
                        long pos = dataSize - (16384 + 4096);
                        long tailTruncatePos = pos + 4096;
                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(bufferBig, 0, 2048);
                        VerifyIsTrue(read == 2048);
                        bool b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        pos += read;
                        VerifyIsTrue(b);

                        long fillSize = dataSize - tailTruncatePos;
                        byte[] fillBuffer = new byte[fillSize];
                        BuildDataBuffer(ref fillBuffer, tailTruncatePos, 7);
                        lLog.AppendAsync(fillBuffer, 0, (int)fillSize, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                        read = stream.Read(bufferBig, 0, 4096);
                        VerifyIsTrue(read == 4096);
                        b = ValidateDataBuffer(ref bufferBig, 2048, 0, pos);
                        VerifyIsTrue(b);
                        b = ValidateDataBuffer(ref bufferBig, read - 2048, 2048, pos + 2048, 7);
                        VerifyIsTrue(b);
                        pos += read;

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - prefetchSize;
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    //
                    // Test 13: Write at the end of the file within the prefetch buffer and verify that read succeeds with correct data
                    //
                    {
                        Stream stream;
                        lLog = OpenLogicalLog(phyLog, lLogId);

                        stream = lLog.CreateReadStream(prefetchSize);

                        //
                        // Truncate tail to make space at the end of the file and then read at the end of the file to fill
                        // read buffer. Write at end of file and then continue to read the new data.
                        //
                        long pos = dataSize - (16384 + 4096 + prefetchSize);
                        long tailTruncatePos = pos + 4096 + prefetchSize;
                        lLog.TruncateTail(tailTruncatePos, CancellationToken.None).Wait();

                        stream.Seek(pos, SeekOrigin.Begin);
                        int read = stream.Read(bufferBig, 0, 2048);
                        VerifyIsTrue(read == 2048);
                        bool b = ValidateDataBuffer(ref bufferBig, read, 0, pos);
                        pos += read;
                        VerifyIsTrue(b);

                        long fillSize = dataSize - tailTruncatePos;
                        byte[] fillBuffer = new byte[fillSize];
                        BuildDataBuffer(ref fillBuffer, tailTruncatePos, 9);
                        lLog.AppendAsync(fillBuffer, 0, (int)fillSize, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                        int readExpected = (16384 + 2048 + prefetchSize);
                        read = stream.Read(bufferBig, 0, readExpected);
                        VerifyIsTrue(read == readExpected);
                        int firstRead = prefetchSize + 2048;
                        b = ValidateDataBuffer(ref bufferBig, firstRead, 0, pos);
                        VerifyIsTrue(b);
                        b = ValidateDataBuffer(ref bufferBig, readExpected - firstRead, firstRead, pos + firstRead, 9);
                        VerifyIsTrue(b);
                        pos += read;

                        //
                        // Overwrite end of file to bring back to original state
                        //
                        {
                            long fixPos = dataSize - (2 * prefetchSize);
                            lLog.TruncateTail(fixPos, CancellationToken.None).Wait();
                            byte[] fixBuffer = new byte[2 * prefetchSize];
                            BuildDataBuffer(ref fixBuffer, fixPos);
                            lLog.AppendAsync(fixBuffer, 0, 2 * prefetchSize, CancellationToken.None).Wait();
                            lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                        }

                        lLog.CloseAsync(CancellationToken.None).Wait();
                        lLog = null;
                    }

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    Debugger.Break();
                    throw;
                }
            },

            "ReadAheadCacheTest");
        }



        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void TruncateInDataBufferTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = CreateLogManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    string physLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", physLogName);

                    Guid logId = Guid.NewGuid();

                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }


                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    //
                    // Test 1: Write a whole bunch of stuff to the log and then flush to be sure the logical log
                    //         write buffer is empty. Next write less than a write buffer's worth to fill the write buffer
                    //         with some data but do not flush. TruncateTail at an offset that is in the write buffer and
                    //         verify success.
                    //
                    byte[] buffer1 = new byte[1000];
                    for (ulong i = 0; i < 1000; i++)
                    {
                        buffer1[i] = 1;
                    }

                    for (int i = 0; i < 100; i++)
                    {
                        lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                        VerifyIsTrue(lLog.WritePosition == (i + 1) * 1000);
                    }
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    long truncateTailPosition = lLog.WritePosition - 50;

                    lLog.TruncateTail(truncateTailPosition, CancellationToken.None).Wait();

                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    //
                    // prove truncate tail succeeded
                    //
                    lLog = OpenLogicalLog(phyLog, lLogId);

                    VerifyIsTrue(lLog.WritePosition == truncateTailPosition);

                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;


                    //
                    // Test 2: Write a whole bunch of stuff to the log and then flush to be sure the logical log
                    //         write buffer is empty. Next write less than a write buffer's worth to fill the write buffer
                    //         with some data but do not flush. TruncateHead at an offset that is in the write buffer and
                    //         verify correct exception. Next flush and TruncateHead and verify success.
                    //
                    lLog = OpenLogicalLog(phyLog, lLogId);

                    for (int i = 0; i < 100; i++)
                    {
                        lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    }
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    long truncateHeadPosition = lLog.WritePosition - 50;

                    lLog.FlushAsync(CancellationToken.None).Wait();
                    lLog.TruncateHead(truncateHeadPosition);

                    //
                    // Write after the truncate head to help force the truncate to happen
                    //
                    lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    Thread.Sleep(60 * 1000);

                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //
                    // prove truncate head succeeded
                    //
                    PhysicalLogManager phyLogMgr;

                    //
                    // Try to open driver first and if it fails then try inproc
                    //
                    try
                    {
                        phyLogMgr = new PhysicalLogManager(LogManager.LoggerType.Driver);
                        phyLogMgr.OpenAsync(CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        //
                        // Don't fail if driver isn't loaded
                        //
                        phyLogMgr = null;
                    }

                    if (phyLogMgr == null)
                    {
                        //
                        // Now try inproc
                        //
                        phyLogMgr = new PhysicalLogManager(LogManager.LoggerType.Inproc);
                        phyLogMgr.OpenAsync(CancellationToken.None).Wait();
                    }

                    IPhysicalLogContainer phyLogContainer;
                    Task<IPhysicalLogContainer> tphyLogContainer;
                    tphyLogContainer = phyLogMgr.OpenLogContainerAsync(physLogName, logId, CancellationToken.None);
                    phyLogContainer = tphyLogContainer.Result;

                    IPhysicalLogStream phyLogStream;
                    Task<IPhysicalLogStream> tphyLogStream;
                    tphyLogStream = phyLogContainer.OpenPhysicalLogStreamAsync(lLogId, CancellationToken.None);
                    phyLogStream = tphyLogStream.Result;

                    PhysicalLogStreamAsnRangeInfo phyAsnRange;
                    Task<PhysicalLogStreamAsnRangeInfo> tphyAsnRange;
                    
                    tphyAsnRange = phyLogStream.QueryRecordRangeAsync(CancellationToken.None);
                    phyAsnRange = tphyAsnRange.Result;

                    VerifyIsTrue(phyAsnRange.LogTruncationAsn != 0);

                    phyLogStream.CloseAsync(CancellationToken.None).Wait();
                    phyLogStream = null;
                    phyLogContainer.CloseAsync(CancellationToken.None).Wait();
                    phyLogContainer = null;
                    phyLogMgr.CloseAsync(CancellationToken.None).Wait();
                    phyLogMgr = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                    logManager = null;
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "TruncateInDataBufferTest");
        }



//
        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void SequentialAndRandomStreamTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    //** Main line test
                    ILogManager logManager = CreateLogManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    string physLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", physLogName);

                    Guid logId = Guid.NewGuid();

                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }


                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    Guid lLogId = Guid.NewGuid();
                    string logicalLogName = PickUniqueLogFilename();
                    Console.WriteLine("Using file: {0}", logicalLogName);

                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogName, null, 1024 * 1024 * 100, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);


                    //
                    // Write a lot of stuff to the log in short records and then read it back sequentially
                    // using a random access stream and a sequential access stream. Expect the sequential
                    // access stream to run much faster
                    //


                    byte[] buffer1 = new byte[1000];
                    byte[] buffer2 = new byte[10000];
                    for (ulong i = 0; i < 1000; i++)
                    {
                        buffer1[i] = 1;
                    }

                    for (int i = 0; i < 10000; i++)
                    {
                        lLog.AppendAsync(buffer1, 0, buffer1.Length, CancellationToken.None).Wait();
                        lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();
                    }

                    Console.WriteLine("Data written....");

                    Stream rsStream;
                    Stopwatch rTime = new Stopwatch();
                    Stopwatch sTime = new Stopwatch();
    

                    //
                    // Verify random faster for random access
                    //
                    Random r = new Random();
                    rsStream = lLog.CreateReadStream(1024 * 1024);

                    sTime.Restart();
                    for (int i = 0; i < 1000; i++)
                    {
                        long offset = r.Next(999 * 1000);
                        rsStream.Seek(offset, SeekOrigin.Begin);
                        rsStream.Read(buffer2, 0, 10000);
                    }
                    sTime.Stop();

                    lLog.SetSequentialAccessReadSize(rsStream, 0);
                    rsStream.Seek(0, SeekOrigin.Begin);
                    rTime.Restart();
                    for (int i = 0; i < 1000; i++)
                    {
                        long offset = r.Next(999 * 1000);
                        rsStream.Seek(offset, SeekOrigin.Begin);
                        rsStream.Read(buffer2, 0, 10000);
                    }
                    rTime.Stop();

                    Console.WriteLine("Random:");
                    Console.WriteLine("    Sequential Time: {0}", sTime.ElapsedMilliseconds);
                    Console.WriteLine("    Random Time: {0}", rTime.ElapsedMilliseconds);

                    VerifyIsTrue(rTime.ElapsedMilliseconds < sTime.ElapsedMilliseconds);



                    lLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;


                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                    logManager = null;
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "SequentialAndRandomStreamTest");
        }
//


        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void WorkloadTimingTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    string workPath = PickPathOnWhichToRunTest();

                    // Test config section:
                    //
                    //      Configure the shared (physical) log to be on a dedicated disk
                    //      Configute the dedicated (logical) log to be on a disk other than the dedicated
                    //      Pick a reasonable record size
                    //
                    string physLogName = "\\??\\" + workPath + "\\LogicalLogTestTemp\\WorkloadTimingTestP.log";
                    string logicalLogFilename = "\\??\\" + workPath + "\\LogicalLogTestTemp\\WorkloadTimingTestL.log";
                    Console.WriteLine("Using physLogName: {0}", physLogName);
                    Console.WriteLine("Using logicalLogFilename: {0}", logicalLogFilename);

                    int recordSize = 512;
                    int loops = 1024 * 1024;
                    //
                    // End Test config section
                    
                    //** Main line test
                    ILogManager logManager = CreateLogManager();
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 0);

                    Guid logId = Guid.NewGuid();

                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }

                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 0);

                    //** ILogicalLog IO Tests

                    const int MaxLogBlkSize = 128 * 1024;

                    //
                    // Configure the dedicated (logical) log to be on a disk different from the shared log
                    //
                    Guid lLogId = Guid.NewGuid();
                    ILogicalLog lLog = CreateLogicalLog(phyLog, lLogId, null, logicalLogFilename, null, 1024 * 1024 * 512, MaxLogBlkSize);
                    VerifyIsTrue(phyLog.IsFunctional);
                    VerifyIsTrue(LogManager.IsLoaded);
                    VerifyIsTrue(LogManager.Handles == 1);
                    VerifyIsTrue(LogManager.Logs == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.IsOpen);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.PhysicalLogHandles == 1);
                    VerifyIsTrue(((LogManager.PhysicalLog.Handle)phyLog).Owner.LogicalLogs == 1);
                    VerifyIsTrue(lLog.IsFunctional);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    //** Prove simple recovery
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    lLog = OpenLogicalLog(phyLog, lLogId);
                    VerifyIsTrue(lLog.Length == 0);
                    VerifyIsTrue(lLog.ReadPosition == 0);
                    VerifyIsTrue(lLog.WritePosition == 0);
                    VerifyIsTrue(lLog.HeadTruncationPosition == -1);

                    //
                    // Perform a fixed workload and time it
                    //
                    int expectedWritePosition = 0;
                    int numberTruncations = 0;
                    byte[] x = BuildBuffer(recordSize);

                    Task taskUsageNotification50;
                    TestLogNotification tn50 = new TestLogNotification();
                    CancellationTokenSource cancelTokenSource50 = new CancellationTokenSource();
                    taskUsageNotification50 = tn50.SetupLogUsageNotificationAsync(lLog, 50, cancelTokenSource50.Token);


                    int startTime = System.Environment.TickCount;
              
                    for (int i = 0; i < loops; i++)
                    {
                        lLog.AppendAsync(x, 0, x.Length, CancellationToken.None).Wait();
                        expectedWritePosition += recordSize;

                        if (tn50.Fired)
                        {
                            //
                            // Truncate back to 25%
                            //
                            numberTruncations++;
                            long allowedData = lLog.Length / 4;
                            long truncationPosition = expectedWritePosition - allowedData;
                            lLog.TruncateHead(truncationPosition);
                            taskUsageNotification50 = tn50.SetupLogUsageNotificationAsync(lLog, 50, cancelTokenSource50.Token);
                        }
                    }
                    lLog.FlushWithMarkerAsync(CancellationToken.None).Wait();

                    int endTime = System.Environment.TickCount;

                    Console.WriteLine("Start time: {0}, End time {1}, Runtime in seconds: {2}, number truncations {3}",
                        startTime, endTime, (endTime - startTime)/1000, numberTruncations
                        );

                    try
                    {
                        cancelTokenSource50.Cancel();
                        taskUsageNotification50.Wait(5000);
                    }
                    catch
                    {
                    }


                    //* Prove close down 
                    lLog.CloseAsync(CancellationToken.None).Wait();
                    lLog = null;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "WorkloadTimingTest");
        }

        private async Task ReadWriteCloseRaceWorker(
            ILogicalLog lLog,
            int maxSize,
            int taskId,
            CancellationToken CancelToken
            )
        {
            Random rnd = new Random();

            try
            {
                long bytes = 0;

                bool canceled = false;

                while (!canceled)
                {
                    int recordSize = rnd.Next(80, maxSize);
                    byte[] recordBuffer = new byte[recordSize];
                    await lLog.AppendAsync(recordBuffer, 0, recordBuffer.Length, CancellationToken.None);
                    bytes += recordBuffer.Length;
                    await lLog.FlushWithMarkerAsync(CancellationToken.None);

                    await lLog.ReadAsync(recordBuffer, 0, recordBuffer.Length, 0, CancellationToken.None);

                    if (CancelToken.IsCancellationRequested)
                    {
                        canceled = true;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Task {0} Exception {1}", taskId, ex.ToString());
            }

            Console.WriteLine("Exit Task {0}", taskId);

            return;
        }


        [VS.TestMethod]
        [VS.Owner("AlanWar")]
        public unsafe void ReadWriteCloseRaceTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                bool failOnException = true;
                try
                {
                    string workPath = PickPathOnWhichToRunTest();

                    // Test config section:
                    //
                    //      Configure the shared (physical) log to be on a dedicated disk
                    //      Configute the dedicated (logical) log to be on a disk other than the dedicated
                    //      Pick a reasonable record size
                    //
                    Guid logId = Guid.NewGuid();
                    string physLogName = "\\??\\" + workPath + "\\LogicalLogTestTemp\\" + logId.ToString() + ".log";
                    Console.WriteLine("Using physLogName: {0}", physLogName);

                    //
                    // End Test config section

                    //** Main line test
                    ILogManager logManager = CreateLogManager();


                    try
                    {
                        logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    }
                    catch (Exception)
                    {
                        // If this fails then it is not a problem
                    }

                    IPhysicalLog phyLog = CreatePhysicalLog(
                        logManager,
                        physLogName,
                        logId,
                        1024 * 1024 * 1024,
                        0,
                        0);

                    const int _NumberTasks = 16;
                    const int MaxLogBlkSize = 128 * 1024;
                    CancellationTokenSource[] cancelToken = new CancellationTokenSource[_NumberTasks];
                    Task[] tasks = new Task[_NumberTasks];
                    ILogicalLog[] lLogs = new ILogicalLog[_NumberTasks];

                    for (int i = 0; i < _NumberTasks; i++)
                    {
                        Guid lLogId = Guid.NewGuid();

                        Console.WriteLine("Log {0} is {1}", i, lLogId.ToString());

                        string logicalLogFilename = "\\??\\" + workPath + "\\LogicalLogTestTemp\\" + lLogId.ToString() + ".log";
                        Console.WriteLine("Using logicalLogFilename: {0}", logicalLogFilename);

                        lLogs[i] = CreateLogicalLog(phyLog, lLogId, null, logicalLogFilename, null, 1024 * 1024 * 512, MaxLogBlkSize);
                   
                        lLogs[i].CloseAsync(CancellationToken.None).Wait();
                        lLogs[i] = null;
                        lLogs[i] = OpenLogicalLog(phyLog, lLogId);

                        cancelToken[i] = new CancellationTokenSource();
                    }

                    for (int i = 0; i < _NumberTasks; i++)
                    {
                        int ii = i;
                        tasks[i] = Task.Factory.StartNew(() =>
                        {
                            ILogicalLog logicalLog = lLogs[ii];
                            CancellationToken ct = cancelToken[ii].Token;
                            ReadWriteCloseRaceWorker(logicalLog, MaxLogBlkSize, ii, ct).Wait();
                        });
                    }

                    Thread.Sleep(15 * 1000);

                    Console.WriteLine("Closing logs");

                    failOnException = false;
                    Task[] closeTasks = new Task[_NumberTasks/2];
                    for (int i = 0; i < _NumberTasks/2; i++)
                    {
                        Console.WriteLine("Async Closing log {0}", i);
                        closeTasks[i] = lLogs[i].CloseAsync(CancellationToken.None);
                    }

                    Task.WaitAll(closeTasks);
                    Console.WriteLine("Finished Async Closed logs");

                    for (int i = _NumberTasks/2; i < _NumberTasks; i++)
                    {
                        Console.WriteLine("Sync Closing log {0}", i);
                        lLogs[i].CloseAsync(CancellationToken.None).Wait();
                    }
                    Console.WriteLine("Finished Sync Closed logs");


                    for (int i = 0; i < _NumberTasks; i++)
                    {
                        System.Fabric.Data.Log.LogManager.LogicalLog lLog = lLogs[i] as System.Fabric.Data.Log.LogManager.LogicalLog;
                        VerifyIsTrue(lLog._OutstandingOperations == 0);
                        lLogs[i] = null;
                    }

                    Thread.Sleep(250);

                    for (int i = 0; i < _NumberTasks; i++)
                    {
                        Console.WriteLine("Cancel Token {0}", i);
                        cancelToken[i].Cancel();
                    }

                    Console.WriteLine("Wait For Tasks");
                    Task.WaitAll(tasks);

                    Console.WriteLine("Tasks completed");

                    failOnException = true;

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(physLogName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();

                    try
                    {
                        File.Delete(physLogName.Substring(4));
                    }
                    catch (Exception)
                    {
                        // ok if this fails
                    }
                }
                catch (Exception ex)
                {
                    if (failOnException)
                    {
                        Console.WriteLine(ex.ToString());
                        throw;
                    }
                }
            },

            "ReadWriteCloseRaceTest");
        }


#if INCLUDE_PERF_WORKLOADS
        private ManualResetEvent StartPerfWorkloads = new ManualResetEvent(false);
        private long TotalByteCount = 0;

        private async Task LogPerformanceWorkloadWorker(
            ILogicalLog lLog,
            byte[] recordBuffer,
            TestLogNotification tn75,
            CancellationToken tn75CancelToken,
            CancellationToken CancelToken
            )
        {
            try
            {
                Task taskUsageNotification75;
                long bytes = 0;
                taskUsageNotification75 = tn75.SetupLogUsageNotificationAsync(lLog, 75, tn75CancelToken);

                bool canceled = false;
                this.StartPerfWorkloads.WaitOne();

                while (!canceled)
                {
                    await lLog.AppendAsync(recordBuffer, 0, recordBuffer.Length, CancellationToken.None);
                    await lLog.FlushWithMarkerAsync(CancellationToken.None);
                    bytes += recordBuffer.Length;

                    if (tn75.Fired)
                    {
                        //
                        // Truncate back to 10%
                        //
                        long allowedData = lLog.Length / 10;
                        long truncationPosition = bytes - allowedData;
                        lLog.TruncateHead(truncationPosition);
                        taskUsageNotification75 = tn75.SetupLogUsageNotificationAsync(lLog, 75, tn75CancelToken);
                    }

                    if (CancelToken.IsCancellationRequested)
                    {
                        canceled = true;
                    }
                }
                Interlocked.Add(ref this.TotalByteCount, bytes);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }

            Console.WriteLine("Tasks completed");

            return;
        }

        private void StartWorkloadThreads(
            int timeToRunInSeconds,
            ILogicalLog[] lLog,
            byte[] recordBuffer,
            TestLogNotification[] tn75,
            CancellationTokenSource[] tn75CancelToken,
            CancellationTokenSource[] CancelToken
            )
        {
            int numberTasks = tn75.Length;
            Task[] tasks = new Task[numberTasks];

            for (int i = 0; i < numberTasks; i++)
            {
                int ii = i;
                tasks[i] = Task.Factory.StartNew(() =>
                    {
                        ILogicalLog logicalLog = lLog[ii];
                        TestLogNotification tn75a = tn75[ii];
                        CancellationToken tn75ct = tn75CancelToken[ii].Token;
                        CancellationToken ct = CancelToken[ii].Token;
                        LogPerformanceWorkloadWorker(logicalLog, recordBuffer, tn75a, tn75ct, ct).Wait();
                    });
            }

            long StartTicks = DateTime.Now.Ticks;
            this.StartPerfWorkloads.Set();
            Thread.CurrentThread.Join(TimeSpan.FromSeconds(timeToRunInSeconds));

            for (int i = 0; i < numberTasks; i++)
            {
                try
                {
                    tn75CancelToken[i].Cancel();
                }
                catch
                {
                }
                CancelToken[i].Cancel();
            }

            Task.WaitAll(tasks);
            
            long byteCount = TotalByteCount;
            long EndTicks = DateTime.Now.Ticks;

            long totalTimeInSeconds = (EndTicks - StartTicks) / 10000000;
            long bytesPerSecond = TotalByteCount / totalTimeInSeconds;

            Console.WriteLine("Number Tasks: {0}", numberTasks);
            Console.WriteLine("Seconds running {0}", totalTimeInSeconds );
            Console.WriteLine("RecordSize {0}", recordBuffer.Length);
            Console.WriteLine("Total bytes {0}", TotalByteCount);
            Console.WriteLine("Bytes per second: {0}\nMB per second: {1}", bytesPerSecond, (bytesPerSecond / (1024 * 1024)));
        }

        [VS.TestMethod]
        [VS.Owner("alanwar")]
        public void LogPerformanceWorkloadTest()
        {
            Utility.WrapNativeSyncInvokeInMTA(() =>
            {
                try
                {
                    string workPath = PickPathOnWhichToRunTest();

                    // Test config section:
                    //
                    //      Configure the shared (physical) log to be on a dedicated disk
                    //      Configute the dedicated (logical) log to be on a disk other than the dedicated
                    //      Pick a reasonable record size
                    //
                    string physLogName = "\\??\\" + workPath + "\\PerfWorkloadTestP.log";
                    Console.WriteLine("Using physLogName: {0}", physLogName);

                    int timeToRunInSeconds = 5 * 60;
                    int MagicRecordSize = 1032192 - 1088;  // This is 4 complete records
                    int numberTasks = 4;
                    long logSize = 8 * 1024L * 1024L * 1024L;
                    long llogSize = 2 * 1024L * 1024L * 1024L;

                    // END: Test config section
                    
                    ILogManager logManager = MakeManager();

                    try
                    {
                        File.Delete(physLogName.Substring(4));
                    }
                    catch (Exception)
                    {
                        // ok if this fails
                    }

                    Guid logId = Guid.NewGuid();
                    string logName = physLogName;
                    IPhysicalLog phyLog = MakePhyLog(
                        logManager,
                        logName,
                        logId,
                        logSize,
                        0,
                        0);

                    //
                    // Determine the largest record size. We write full records to maximmize our I/O rate.
                    // Max record size is determined by the Maximum block size plus the metadata space (4K) minus the 
                    // overhead of the headers.
                    //
                    const int MaxLogBlkSize = 1024 * 1024;
                    const int recordSize = 131;

                    //
                    // Get a buffer
                    //
                    byte[] result = new byte[MagicRecordSize];

                    for (int ix = 0; ix < result.Length; ix++)
                    {
                        result[ix] = (byte)(ix % recordSize);
                    }

                    CancellationTokenSource[] cancelToken = new CancellationTokenSource[numberTasks];
                    Task[] tasks = new Task[numberTasks];
                    ILogicalLog[] lLogs = new ILogicalLog[numberTasks];
                    TestLogNotification[] tn75 = new TestLogNotification[numberTasks];
                    CancellationTokenSource[] cancelTokenSource75 = new CancellationTokenSource[numberTasks];

                    this.StartPerfWorkloads.Reset();

                    for (int i = 0; i < numberTasks; i++)
                    {

                        Guid lLogId = Guid.NewGuid();
                        string logicalLogName = "\\??\\" + workPath + "\\Perf" + lLogId.ToString() + ".log";
                        Console.WriteLine("Using logicalLogName: {0}", logicalLogName);
                        lLogs[i] = CreateLogicalLog(phyLog, 
                                                            lLogId, 
                                                            null, 
                                                            logicalLogName, 
                                                            null, 
                                                            llogSize, 
                                                            MaxLogBlkSize);

                        lLogs[i].CloseAsync(CancellationToken.None).Wait();
                        lLogs[i] = null;
                        lLogs[i] = OpenLogicalLog(phyLog, lLogId);

                        //
                        // Truncate at 75%
                        //

                        cancelToken[i] = new CancellationTokenSource();
                        CancellationToken token = cancelToken[i].Token;

                        tn75[i] = new TestLogNotification();
                        cancelTokenSource75[i] = new CancellationTokenSource();
                    }

                    StartWorkloadThreads(timeToRunInSeconds, lLogs, result, tn75, cancelTokenSource75, cancelToken);

                    for (int i = 0; i < numberTasks; i++)
                    {
                        lLogs[i].CloseAsync(CancellationToken.None).Wait();
                        lLogs[i] = null;
                    }

                    phyLog.CloseAsync(CancellationToken.None).Wait();
                    phyLog = null;

                    //* Cleanup
                    logManager.DeletePhysicalLogAsync(logName, logId, CancellationToken.None).Wait();
                    logManager.CloseAsync(CancellationToken.None).Wait();

                    try
                    {
                        File.Delete(physLogName.Substring(4));
                    }
                    catch (Exception)
                    {
                        // ok if this fails
                    }

                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.ToString());
                    throw;
                }
            },

            "LogPerformanceWorkloadTest");
        }
#endif


        // **




    }
}