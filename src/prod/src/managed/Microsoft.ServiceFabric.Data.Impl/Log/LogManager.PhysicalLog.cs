// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Data.Log.Interop;
    using System.Security.AccessControl;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// LogManager.PhysicalLog implementation
    /// </summary>
    public static partial class LogManager
    {
        internal class PhysicalLog
        {
            // Instance state
            private readonly AutoResetEventAsync _PhysicalLogLock;
            // NOTE: Over all OM locking model: If multiple locks are needed (e.g. Manager and P.Log), all locks
            //       are obtained top down from the Manager (same order). This is why, for example, LogicalLog 
            //       instances call into the Manager to report their closure - so that the Manager log can be taken 
            //       and then the closure event forwarded to the PhysicalLog instance for it's lock acquire and 
            //       processing of the event.

            private long _NextPhysicalLogHandle;
            private readonly IPhysicalLogContainer _Container;
            private readonly Guid _Id;
            private readonly Dictionary<Guid, LogicalLogInfo> _LogicalLogs;
            private readonly Dictionary<long, WeakReference<Handle>> _PhysicalLogHandles;

            // Properties
            public Guid Id
            {
                get { return this._Id; }
            }

            public bool IsOpen { get; set; }

            public int LogicalLogs
            {
                get { return this._LogicalLogs.Count; }
            }

            public int PhysicalLogHandles
            {
                get { return this._PhysicalLogHandles.Count; }
            }


            public
                PhysicalLog(Guid id, IPhysicalLogContainer underlyingContainer)
            {
                this._PhysicalLogLock = new AutoResetEventAsync();
                this._PhysicalLogHandles = new Dictionary<long, WeakReference<Handle>>();
                this._LogicalLogs = new Dictionary<Guid, LogicalLogInfo>();
                this._Container = underlyingContainer;
                this._NextPhysicalLogHandle = 0;
                this._Id = id;
                this.IsOpen = false;
            }

            /// <summary>
            /// Handle factory
            /// </summary>
            /// <returns>IPhysicalLog</returns>
            public async Task<IPhysicalLog>
                OnCreateHandle(CancellationToken cancellationToken)
            {
                Handle result = null;

                await this._PhysicalLogLock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
                try
                {
                    result = new Handle(this, this._NextPhysicalLogHandle++);
                    this._PhysicalLogHandles.Add(result.Id, new WeakReference<Handle>(result));
                    result.IsOpen = true;
                }
                finally
                {
                    this._PhysicalLogLock.Set();
                }

                return result;
            }

            /// <summary>
            /// Handle closed event
            /// </summary>
            /// <returns>true if last handle on the P.Log has been closed and no L.Logs are making reference</returns>
            public async Task<bool>
                OnCloseHandle(Handle toClose, CancellationToken cancellationToken)
            {
                await this._PhysicalLogLock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
                try
                {
                    ReleaseAssert.AssertIfNot(this._PhysicalLogHandles.Remove(toClose.Id), "handle missing from _HandlesPL");
                    if ((this._PhysicalLogHandles.Count == 0) && (this._LogicalLogs.Count == 0))
                    {             
                        try
                        {
                            await this._Container.CloseAsync(cancellationToken).ConfigureAwait(false);
                        } 
                        catch (Exception ex)
                        {
                            //
                            // An exception is ok and shouldn't cause a failure
                            //
                            AppTrace.TraceSource.WriteInfo(
                                    "LogicalLog",
                                    "PhysicalLogClose: Exception {0}",
                                    ex.ToString());
                        }

                        this.IsOpen = false;
                        return true; // Indicate last handle was closed
                    }

                    return false;
                }
                finally
                {
                    this._PhysicalLogLock.Set();
                }
            }

            /// <summary>
            /// LogicalLog closed event
            /// </summary>
            /// <returns>true if last handle on the P.Log has been closed and no L.Logs are making reference</returns>
            public async Task<bool>
                OnCloseLogicalLog(LogicalLog toClose, CancellationToken cancellationToken)
            {
                await this._PhysicalLogLock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
                try
                {
                    LogicalLogInfo llInfo;

                    if (this._LogicalLogs.TryGetValue(toClose.Id, out llInfo) == false)
                    {
                        ReleaseAssert.Failfast("toClose missing from _LogicalLogs");
                    }

                    await llInfo.UnderlyingStream.CloseAsync(cancellationToken).ConfigureAwait(false);

                    ReleaseAssert.AssertIfNot(this._LogicalLogs.Remove(toClose.Id), "handle missing from _LogicalLogs");
                    if ((this._PhysicalLogHandles.Count == 0) && (this._LogicalLogs.Count == 0))
                    {
                        await this._Container.CloseAsync(cancellationToken).ConfigureAwait(false);
                        this.IsOpen = false;
                        return true; // Indicate p.log was closed
                    }

                    return false;
                }
                finally
                {
                    this._PhysicalLogLock.Set();
                }
            }

            /// <summary>
            /// LogicalLog creation requested - public API implementation - create and catalog it
            /// </summary>
            /// <returns>ILogicalLog</returns>
            private async Task<ILogicalLog>
                OnCreateLogicalLogAsync(
                Handle owningHandle,
                Guid logicalLogId,
                string optionalLogStreamAlias,
                string path,
                FileSecurity optionalSecurityInfo,
                Int64 maximumSize,
                UInt32 maximumBlockSize,
                LogManager.LogCreationFlags creationFlags,
                string traceType,
                CancellationToken cancellationToken)
            {
                Requires.Argument("path", path).NotNull();

                await this._PhysicalLogLock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
                try
                {
                    LogicalLog logicalLog = null;
                    LogicalLogInfo llInfo;

                    if ((this._LogicalLogs.TryGetValue(logicalLogId, out llInfo) == false) ||
                        (llInfo.LogicalLog.TryGetTarget(out logicalLog) == false))
                    {
                        // LLog has not been opened or created or has gone away - attempt to create it

                        var underlyingStream = await this._Container.CreatePhysicalLogStreamAsync(
                            logicalLogId,
                            KLogicalLogInformation.GetLogicalLogStreamType(),
                            optionalLogStreamAlias,
                            path,
                            optionalSecurityInfo,
                            maximumSize,
                            maximumBlockSize,
                            creationFlags,
                            cancellationToken).ConfigureAwait(false);

                        Exception caughtException = null;
                        try
                        {
                            var newLogicalLog = new LogicalLog(this, owningHandle.Id, logicalLogId, underlyingStream, traceType);

                            if (logicalLog == null)
                            {
                                this._LogicalLogs.Add(logicalLogId, new LogicalLogInfo(newLogicalLog, underlyingStream));
                            }
                            else
                            {
                                // The LLog was indexed but had gone away - just update the index with the new LLog
                                llInfo.LogicalLog.SetTarget(newLogicalLog);
                            }

                            newLogicalLog.IsOpen = true;

                            await newLogicalLog.OnCreated(cancellationToken).ConfigureAwait(false);
                            return newLogicalLog;
                        }
                        catch (Exception ex)
                        {
                            // Cause clean up failed create
                            caughtException = ex;
                        }

                        System.Diagnostics.Debug.Assert(caughtException != null);

                        // clean up failed create and forward the causing exception
                        if (logicalLog != null)
                        {
                            this._LogicalLogs.Remove(logicalLogId);
                        }

                        // TODO: Consider adding trace output when secondary exceptions occur
                        try
                        {
                            await underlyingStream.CloseAsync(cancellationToken).ConfigureAwait(false);
                        }
                        catch
                        {
                        }

                        try
                        {
                            await this._Container.DeletePhysicalLogStreamAsync(logicalLogId, cancellationToken).ConfigureAwait(false);
                        }
                        catch
                        {
                        }

                        throw caughtException;
                    }
                    else
                    {
                        // Already open - fault - only exclusive opens supported
                        throw new UnauthorizedAccessException();
                    }
                }
                finally
                {
                    this._PhysicalLogLock.Set();
                }
            }

            /// <summary>
            /// LogicalLog open requested - public API implementation - open and catalog it
            /// </summary>
            /// <returns>ILogicalLog</returns>
            private async Task<ILogicalLog>
                OnOpenLogicalLogAsync(
                Handle owningHandle,
                Guid logicalLogId,
                string traceType,
                CancellationToken cancellationToken)
            {
                await this._PhysicalLogLock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
                try
                {
                    LogicalLog logicalLog = null;
                    LogicalLogInfo llInfo;

                    if ((this._LogicalLogs.TryGetValue(logicalLogId, out llInfo) == false) ||
                        (llInfo.LogicalLog.TryGetTarget(out logicalLog) == false))
                    {
                        // LLog has not been opened or created - attempt to open it
                        IPhysicalLogStream underlyingStream = null;
                        var retries = 0;
                        const int retryLimit = 3;

                        do
                        {
                            try
                            {
                                underlyingStream = await this._Container.OpenPhysicalLogStreamAsync(
                                    logicalLogId,
                                    cancellationToken).ConfigureAwait(false);
                                retries = retryLimit;
                            }
                            catch (System.IO.FileLoadException ex)
                            {
                                retries++;
                                if ((ex.HResult != unchecked((int) 0x80070020)) ||
                                    (retries == retryLimit))
                                {
                                    throw;
                                }
                            }
                        } while (retries < retryLimit);

                        Exception caughtException = null;
                        LogicalLog newLogicalLog = null;
                        try
                        {
                            newLogicalLog = new LogicalLog(this, owningHandle.Id, logicalLogId, underlyingStream, traceType);

                            this._LogicalLogs.Add(logicalLogId, new LogicalLogInfo(newLogicalLog, underlyingStream));

                            newLogicalLog.IsOpen = true;

                            await newLogicalLog.OnRecoverAsync(cancellationToken).ConfigureAwait(false);
                        }
                        catch (Exception ex)
                        {
                            // Cause clean up failed create
                            caughtException = ex;
                        }

                        if (caughtException != null)
                        {
                            // clean up failed create and forward the causing exception
                            if (newLogicalLog != null)
                            {
                                newLogicalLog.IsOpen = false;
                                this._LogicalLogs.Remove(logicalLogId);
                            }

                            // TODO: Consider adding trace output when secondary exceptions occur
                            try
                            {
                                await underlyingStream.CloseAsync(cancellationToken).ConfigureAwait(false);
                            }
                            catch
                            {
                            }
                            throw caughtException;
                        }

                        return newLogicalLog;
                    }
                    else
                    {
                        // Already open - fault - only exclusive opens supported
                        throw new UnauthorizedAccessException();
                    }
                }
                finally
                {
                    this._PhysicalLogLock.Set();
                }
            }

            private async Task OnReplaceAliasLogsAsync(
                string sourceLogAliasName,
                string logAliasName,
                string backupLogAliasName,
                CancellationToken cancellationToken)
            {
                var logAliasGuid = await this._Container.ResolveAliasAsync(logAliasName, cancellationToken).ConfigureAwait(false);
                var sourceLogAliasGuid = await this._Container.ResolveAliasAsync(sourceLogAliasName, cancellationToken).ConfigureAwait(false);

                AppTrace.TraceSource.WriteInfo(
                    "LogicalLog",
                    "OnReplaceAliasLogAsync: Enter {0} {1}, {2} {3}, {4}",
                    sourceLogAliasName,
                    sourceLogAliasGuid,
                    logAliasName,
                    logAliasGuid,
                    backupLogAliasName);

                try
                {
                    var idOfOldBackupLog = await this._Container.ResolveAliasAsync(backupLogAliasName, cancellationToken).ConfigureAwait(false);
                    if (idOfOldBackupLog != logAliasGuid)
                    {
                        try
                        {
                            // Delete underlying logical log
                            await this._Container.DeletePhysicalLogStreamAsync(idOfOldBackupLog, cancellationToken).ConfigureAwait(false);
                        }
                        catch ( /*FabricElementNotFoundException*/ Exception ex)
                        {
                            AppTrace.TraceSource.WriteInfo(
                                "LogicalLog",
                                "OnReplaceAliasLogAsync: DeleteBackupLog Exception {0} {1}, {2} {3}, {4}" + Environment.NewLine + "{5}",
                                sourceLogAliasName,
                                sourceLogAliasGuid,
                                logAliasName,
                                logAliasGuid,
                                backupLogAliasName,
                                ex.ToString());
                        }
                    }
                }
                catch ( /*FabricElementNotFoundException*/ Exception ex)
                {
                    AppTrace.TraceSource.WriteInfo(
                        "LogicalLog",
                        "OnReplaceAliasLogAsync: GetBackupLogId Exception {0} {1}, {2} {3}, {4}" + Environment.NewLine + "{5}",
                        sourceLogAliasName,
                        sourceLogAliasGuid,
                        logAliasName,
                        logAliasGuid,
                        backupLogAliasName,
                        ex.ToString());
                }

                await this._Container.AssignAliasAsync(logAliasGuid, backupLogAliasName, cancellationToken).ConfigureAwait(false); // Step #1
                await this._Container.AssignAliasAsync(sourceLogAliasGuid, logAliasName, cancellationToken).ConfigureAwait(false); // Step #2
            }

            private async Task<Guid> OnRecoverAliasLogsAsync(
                string copyLogAliasName,
                string logAliasName,
                string backupLogAliasName,
                CancellationToken cancellationToken)
            {
                try
                {
                    return await this._Container.ResolveAliasAsync(logAliasName, cancellationToken).ConfigureAwait(false);
                }
                catch ( /*FabricElementNotFoundException*/ Exception ex)
                {
                    var e = ex.ToString();
                }

                // desired alias missing - could be due to result of failure in OnReplaceAliasLogsAsync during 
                // steps 1 and 2 - rollback
                var idOfBackupLogAlias = await this._Container.ResolveAliasAsync(backupLogAliasName, cancellationToken).ConfigureAwait(false);
                await this._Container.AssignAliasAsync(idOfBackupLogAlias, logAliasName, cancellationToken).ConfigureAwait(false);
                return idOfBackupLogAlias;
            }

            // Desc for tracking opened LogicalLogs
            private struct LogicalLogInfo
            {
                public WeakReference<LogicalLog> LogicalLog;
                public IPhysicalLogStream UnderlyingStream;

                public LogicalLogInfo(LogicalLog logicalLog, IPhysicalLogStream underlyingStream)
                {
                    this.LogicalLog = new WeakReference<LogicalLog>(logicalLog);
                    this.UnderlyingStream = underlyingStream;
                }
            }


            //** Public API: PhysicalLog implementation of IPhysicalLog
            public class Handle : IPhysicalLog
            {
                private int _IsOpen;
                private readonly PhysicalLog _Owner;
                private readonly Guid _OwnerId;
                private readonly long _HandleId;

                public long Id
                {
                    get { return this._HandleId; }
                }

                public Guid OwnerId
                {
                    get { return this._OwnerId; }
                }

                public bool IsOpen
                {
                    get { return this._IsOpen != 0; }
                    set { this._IsOpen = (value ? 1 : 0); }
                }

                public PhysicalLog Owner
                {
                    get { return this._Owner; }
                }


                public
                    Handle(PhysicalLog owner, long handleId)
                {
                    this._Owner = owner;
                    this._OwnerId = owner.Id;
                    this._HandleId = handleId;
                    this.IsOpen = false;
                }

                ~Handle()
                {
                    ((IPhysicalLog) this).Abort();
                }

                void
                    IDisposable.Dispose()
                {
                    ((IPhysicalLog) this).Abort();
                }

                //* IPhysicalLog implementation - See Interfaces.cs
                Task
                    IPhysicalLog.CloseAsync(CancellationToken cancellationToken)
                {
                    GC.SuppressFinalize(this);

                    if (this.IsOpen)
                    {
                        // Make sure IsOpen = false
                        if (Interlocked.CompareExchange(ref this._IsOpen, 0, 1) == 1)
                        {
                            // First close on and open physical log
                            return OnPhysicalLogHandleClose(this, cancellationToken);
                        }
                    }

                    // Was not open
                    var result = new Task(() => { }, cancellationToken);
                    result.RunSynchronously();
                    return result;
                }

                void
                    IPhysicalLog.Abort()
                {
                    // Start the close process but do not hold continuation until done

                    ((IPhysicalLog) this).CloseAsync(CancellationToken.None);
                }

                bool
                    IPhysicalLog.IsFunctional
                {
                    get
                    {
                        if (!this.IsOpen)
                            return false;

                        return this._Owner._Container.IsFunctional();
                    }
                }

                Task<ILogicalLog>
                    IPhysicalLog.CreateLogicalLogAsync(
                    Guid logicalLogId,
                    string optionalLogStreamAlias,
                    string Path,
                    FileSecurity optionalSecurityInfo,
                    Int64 maximumSize,
                    UInt32 maximumBlockSize,
                    LogManager.LogCreationFlags creationFlags,
                    string traceType,
                    CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner.OnCreateLogicalLogAsync(
                        this,
                        logicalLogId,
                        optionalLogStreamAlias,
                        Path,
                        optionalSecurityInfo,
                        maximumSize,
                        maximumBlockSize,
                        creationFlags,
                        traceType,
                        cancellationToken);
                }

                Task<ILogicalLog>
                    IPhysicalLog.OpenLogicalLogAsync(Guid logicalLogId, string traceType, CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    AppTrace.TraceSource.WriteInfo(
                        "LogicalLog@" + traceType,
                        "OpenLogicalLog: Enter LogId {0}",
                        logicalLogId);

                    return this._Owner.OnOpenLogicalLogAsync(this, logicalLogId, traceType, cancellationToken);
                }

                Task
                    IPhysicalLog.DeleteLogicalLogAsync(Guid logicalLogId, CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner._Container.DeletePhysicalLogStreamAsync(logicalLogId, cancellationToken);
                }

                Task
                    IPhysicalLog.AssignAliasAsync(Guid logicalLogId, string alias, CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner._Container.AssignAliasAsync(logicalLogId, alias, cancellationToken);
                }

                Task<Guid>
                    IPhysicalLog.ResolveAliasAsync(string alias, CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner._Container.ResolveAliasAsync(alias, cancellationToken);
                }

                Task
                    IPhysicalLog.RemoveAliasAsync(string alias, CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner._Container.RemoveAliasAsync(alias, cancellationToken);
                }

                Task IPhysicalLog.ReplaceAliasLogsAsync(
                    string sourceLogAliasName,
                    string logAliasName,
                    string backupLogAliasName,
                    CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner.OnReplaceAliasLogsAsync(
                        sourceLogAliasName,
                        logAliasName,
                        backupLogAliasName,
                        cancellationToken);
                }

                Task<Guid> IPhysicalLog.RecoverAliasLogsAsync(
                    string sourceLogAliasName,
                    string logAliasName,
                    string backupLogAliasName,
                    CancellationToken cancellationToken)
                {
                    if (!this.IsOpen)
                        throw new FabricObjectClosedException();

                    return this._Owner.OnRecoverAliasLogsAsync(
                        sourceLogAliasName,
                        logAliasName,
                        backupLogAliasName,
                        cancellationToken);
                }
            }
        }
    }
}