// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.Log
{
    using Microsoft.ServiceFabric.Data;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Data.Log.Interop;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// LogManager implementation
    /// 
    /// The implementation OM for the LogManager class is:
    ///     LogManager which wraps PhysicalLogManager (native proxy) instance and contains:
    ///         - LogManager.Handle instances which implement ILogManager
    ///         - LogManager.PhysicalLog instances which wraps IPhysicalLogContainer (native proxy) and contains:
    ///             - LogManager.PhysicalLog.Handle instances which implement IPhysicalLog
    ///             - LogManager.LogicalLog instances which wraps IPhysicalLogStream (native proxy) and implements ILogicalLog
    /// </summary>
    public static partial class LogManager
    {
        private const string TraceType = "LogManager";

        /// <summary>
        /// 
        /// </summary>
        public enum LogCreationFlags : uint
        {
            /// <summary>
            /// 
            /// </summary>
            UseSparseFile = 1
        };

        private static readonly AutoResetEventAsync _Lock;
        // NOTE: Over all OM locking model: If multiple locks are needed (e.g. Manager and P.Log), all locks
        //       are obtained top down from the Manager (same order). This is why, for example, LogicalLog 
        //       instances call into the Manager to report their closure - so that the Manager log can be taken 
        //       and then the closure event forwarded to the PhysicalLog instance for it's lock acquire and 
        //       processing of the event.

        private static readonly Dictionary<long, WeakReference<Handle>> _Handles;
        private static readonly Dictionary<Guid, PhysicalLog> _Logs;

        private static long _NextHandleId;
        private static PhysicalLogManager _AppDomainPhysLogManager;

        //* Test support
        internal static bool IsLoaded
        {
            get { return _AppDomainPhysLogManager != null; }
        }

        internal static int Handles
        {
            get { return _Handles.Count; }
        }

        internal static int Logs
        {
            get { return _Logs.Count; }
        }

        private static LogManager.LoggerType _LoggerType = LogManager.LoggerType.Unknown;

        /// <summary>
        /// AppDomain singleton ctor
        /// </summary>
        static
            LogManager()
        {
            _Handles = new Dictionary<long, WeakReference<Handle>>();
            _Lock = new AutoResetEventAsync();
            _Logs = new Dictionary<Guid, PhysicalLog>();
            _NextHandleId = 0;
        }

        /// <summary>
        /// Public API implementation for gaining basic access the log API
        /// </summary>
        /// <returns>ILogManager</returns>
        private static async Task<ILogManager>
            InternalCreateAsync(LogManager.LoggerType logManagerType, CancellationToken cancellationToken)
        {
            Handle result = null;
            PhysicalLogManager newManager = null;
            Exception caughtException = null;

            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                if (_AppDomainPhysLogManager == null)
                {
                    if (logManagerType == LogManager.LoggerType.Default)
                    {                        
                        //
                        // Try to open driver first and if it fails then try inproc
                        //
                        try
                        {
                            newManager = new PhysicalLogManager(LogManager.LoggerType.Driver);
                            await newManager.OpenAsync(cancellationToken).ConfigureAwait(false);
                        }
                        catch (System.IO.FileNotFoundException)
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType, "InternalCreateAsync: FileNotFound returned when trying to open Driver. Proceeding with an Inproc log.");

                            //
                            // Don't fail if driver isn't loaded
                            //
                            newManager = null;
                        }

                        if (newManager == null)
                        {
                            //
                            // Now try inproc
                            //
                            newManager = new PhysicalLogManager(LogManager.LoggerType.Inproc);
                            await newManager.OpenAsync(cancellationToken).ConfigureAwait(false);
                        }
                    }
                    else
                    {
                        //
                        // Only try to open a specific log manager type
                        //
                        newManager = new PhysicalLogManager(logManagerType);
                        await newManager.OpenAsync(cancellationToken).ConfigureAwait(false);
                    }

                    _LoggerType = newManager.GetLoggerType();
                    _AppDomainPhysLogManager = newManager; // No error during Open - 
                }

                _NextHandleId++;
                result = new Handle(_NextHandleId);
                _Handles.Add(_NextHandleId, new WeakReference<Handle>(result, false));
                result.IsOpen = true;
                newManager = null;
            }
            catch (Exception ex)
            {
                caughtException = ex; // for later re-throw

                if (result != null)
                {
                    // Undo handle creation
                    result.IsOpen = false;
                    _Handles.Remove(result.Id);
                }

                if (newManager != null)
                {
                    // Reverse creation of AppDomain singleton
                    _AppDomainPhysLogManager = null;
                    _LoggerType = LogManager.LoggerType.Unknown;
                }
            }
            finally
            {
                _Lock.Set();
            }

            if (caughtException != null)
            {
                if (newManager != null)
                {
                    // Finish reversal of creation of AppDomain singleton
                    await newManager.CloseAsync(cancellationToken).ConfigureAwait(false);
                }

                throw caughtException;
            }

            return result;
        }


        /// <summary>
        /// GetLoggerType() ILogManager instances report here
        /// </summary>
        private static LogManager.LoggerType
            OnGetLoggerType()
        {
            return _LoggerType;
        }


        /// <summary>
        /// Closing ILogManager instances report here
        /// </summary>
        private static async Task
            OnCloseHandle(Handle toClose, CancellationToken cancellationToken)
        {
            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                WeakReference<Handle> wr;
                if (_Handles.TryGetValue(toClose.Id, out wr) == false)
                {
                    ReleaseAssert.Failfast("toClose missing from _Handles");
                }

                _Handles.Remove(toClose.Id);
                if ((_Handles.Count == 0) && (_Logs.Count == 0))
                {
                    // No references to this PhysicalLogManager - reverse allocation
                    var m = _AppDomainPhysLogManager;
                    _AppDomainPhysLogManager = null;

                    try
                    {
                        await m.CloseAsync(cancellationToken).ConfigureAwait(false);
                    }
                    catch (Exception ex)
                    {
                        AppTrace.TraceSource.WriteInfo(TraceType, "OnCloseHandle: Exception {0}", ex);
                    }
                }
            }
            finally
            {
                _Lock.Set();
            }
        }

        /// <summary>
        /// Create Physical Log public API implementation
        /// </summary>
        /// <returns>IPhysicalLog</returns>
        private static async Task<IPhysicalLog>
            OnCreatePhysicalLogAsync(
            string pathToCommonContainer,
            Guid physicalLogId,
            long containerSize,
            uint maximumNumberStreams,
            uint maximumLogicalLogBlockSize,
            LogManager.LogCreationFlags CreationFlags,
            CancellationToken cancellationToken)
        {
            Requires.Argument("pathToCommonContainer", pathToCommonContainer).NotNull();

            IPhysicalLog result = null;

            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                PhysicalLog subjectLog;

                if (_Logs.TryGetValue(physicalLogId, out subjectLog) == false)
                {
                    // P.Log has not been opened or created - attempt to create it
                    var underlyingContainer = await _AppDomainPhysLogManager.CreateContainerAsync(
                        pathToCommonContainer,
                        physicalLogId,
                        containerSize,
                        maximumNumberStreams,
                        maximumLogicalLogBlockSize,
                        CreationFlags,
                        cancellationToken).ConfigureAwait(false);

                    Exception caughtException = null;
                    try
                    {
                        // Make and "open" PhysicalLog representing the underlying IPhysicalLogContainer
                        subjectLog = new PhysicalLog(physicalLogId, underlyingContainer);
                        _Logs.Add(physicalLogId, subjectLog);
                        subjectLog.IsOpen = true;
                        result = await subjectLog.OnCreateHandle(cancellationToken).ConfigureAwait(false);
                    }
                    catch (Exception ex)
                    {
                        // Cause clean up failed create
                        caughtException = ex;
                    }

                    if (caughtException != null)
                    {
                        // clean up failed - create and forward the causing exception
                        if (subjectLog != null)
                        {
                            subjectLog.IsOpen = false;
                            _Logs.Remove(physicalLogId);
                        }

                        try
                        {
                            await underlyingContainer.CloseAsync(cancellationToken).ConfigureAwait(false);
                        }
                        catch (Exception ex)
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType, "OnCreatePhysicalLogAsync - CloseAsync: Exception {0}", ex);
                        }

                        try
                        {
                            await underlyingContainer.DeletePhysicalLogStreamAsync(physicalLogId, cancellationToken).ConfigureAwait(false);
                        }
                        catch (Exception ex)
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType, "OnCreatePhysicalLogAsync - DeletePhysicalLogStreamAsync: Exception {0}", ex);
                        }

                        throw caughtException;
                    }
                }
                else
                {
                    // Have an existing physical log with this id already - fault
                    throw new UnauthorizedAccessException(SR.Error_PhysicalLog_Exists);
                }
            }
            finally
            {
                _Lock.Set();
            }

            return result;
        }

        /// <summary>
        /// Open existing Physical Log public API implementation
        /// </summary>
        /// <returns>IPhysicalLog</returns>
        private static async Task<IPhysicalLog>
            OnOpenPhysicalLogAsync(
            string pathToCommonContainer,
            Guid physicalLogId,
            bool isStagingLog,
            CancellationToken cancellationToken)
        {
            Requires.Argument("pathToCommonContainer", pathToCommonContainer).NotNull();

            IPhysicalLog result = null;

            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                PhysicalLog subjectLog;

                if (_Logs.TryGetValue(physicalLogId, out subjectLog) == false)
                {
                    // P.Log has not been opened yet- attempt to open its underpinnings
                    var underlyingContainer = await _AppDomainPhysLogManager.OpenLogContainerAsync(
                        pathToCommonContainer,
                        isStagingLog ? Guid.Empty : physicalLogId,
                        cancellationToken).ConfigureAwait(false);

                    Exception caughtException = null;
                    try
                    {
                        subjectLog = new PhysicalLog(physicalLogId, underlyingContainer);
                        _Logs.Add(physicalLogId, subjectLog);
                        subjectLog.IsOpen = true;
                        result = await subjectLog.OnCreateHandle(cancellationToken).ConfigureAwait(false);
                    }
                    catch (Exception ex)
                    {
                        // Cause clean up failed create
                        caughtException = ex;
                    }

                    if (caughtException != null)
                    {
                        // clean up failed create and forward the causing exception
                        if (subjectLog != null)
                        {
                            subjectLog.IsOpen = false;
                            _Logs.Remove(physicalLogId);
                        }

                        try
                        {
                            await underlyingContainer.CloseAsync(cancellationToken).ConfigureAwait(false);
                        }
                        catch (Exception ex)
                        {
                            AppTrace.TraceSource.WriteInfo(TraceType, "OnOpenPhysicalLogAsync - CloseAsync: Exception {0}", ex);
                        }

                        throw caughtException;
                    }
                }
                else
                {
                    // Already open - just alias with another handle
                    result = await subjectLog.OnCreateHandle(cancellationToken).ConfigureAwait(false);
                }
            }
            finally
            {
                _Lock.Set();
            }

            return result;
        }

        /// <summary>
        /// Delete existing Physical Log public API implementation
        /// </summary>
        private static async Task
            OnDeletePhysicalLogAsync(
            string pathToCommonContainer,
            Guid physicalLogId,
            CancellationToken cancellationToken)
        {
            Requires.Argument("pathToCommonContainer", pathToCommonContainer).NotNull();

            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                // Just issue underlying delete - if there are open handle/streams on the container
                // they will start failing and clean up normally

                await _AppDomainPhysLogManager.DeleteLogContainerAsync(
                    pathToCommonContainer,
                    physicalLogId,
                    cancellationToken).ConfigureAwait(false);
            }
            finally
            {
                _Lock.Set();
            }
        }

        /// <summary>
        /// Physical log handle is closing
        /// </summary>
        private static async Task
            OnPhysicalLogHandleClose(PhysicalLog.Handle ToClose, CancellationToken cancellationToken)
        {
            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                PhysicalLog handleOwner = null;

                ReleaseAssert.AssertIfNot(_Logs.TryGetValue(ToClose.OwnerId, out handleOwner), "_Logs index is inconsistent");

                if (await handleOwner.OnCloseHandle(ToClose, cancellationToken).ConfigureAwait(false))
                {
                    // last reference on the PhysicalLog removed - it is closed, remove from this _Logs index
                    _Logs.Remove(ToClose.OwnerId);
                    if ((_Handles.Count == 0) && (_Logs.Count == 0))
                    {
                        var m = _AppDomainPhysLogManager;
                        _AppDomainPhysLogManager = null;
                        try
                        {
                            await m.CloseAsync(cancellationToken).ConfigureAwait(false);
                        } 
                        catch (Exception ex)
                        {
                            //
                            // Close can throw STATUS_OBJECT_NO_LONGER_EXISTS if the log has been deleted while
                            // the opened.  This is benign and so just ignore it.
                            //
                            // TODO asnegi: RDBug #10278280 Adding 0xc0190021 as it is expected HResult for exception. However, it seems that it is not being
                            // translated to 0x80071A97 or generated properly in dotnet_coreclr_linux.
                            if ((ex.InnerException == null)
                                || ((ex.InnerException.HResult != unchecked((int) 0x80071A97)) && (ex.InnerException.HResult != unchecked((int)0xc0190021)))
                                || (ex.InnerException.InnerException != null && ex.InnerException.InnerException.HResult != unchecked((int)0xc0190021)))
                            {
                                // Only ERROR_OBJECT_NO_LONGER_EXISTS is an expected result code. Any other error should be passed back up
                                throw;
                            }                            
                        }
                    }
                }
            }
            finally
            {
                _Lock.Set();
            }
        }

        /// <summary>
        /// LogicalLog instance being closed
        /// </summary>
        private static async Task
            OnLogicalLogClose(LogicalLog ToClose, CancellationToken cancellationToken)
        {
            await _Lock.WaitUntilSetAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                PhysicalLog handleOwner = null;

                ReleaseAssert.AssertIfNot(_Logs.TryGetValue(ToClose.OwnerId, out handleOwner), "_Logs index is inconsistent");

                if (await handleOwner.OnCloseLogicalLog(ToClose, cancellationToken).ConfigureAwait(false))
                {
                    // last handle or logical log closed on the PhysicalLog, removed from this index
                    _Logs.Remove(ToClose.OwnerId);
                    if ((_Handles.Count == 0) && (_Logs.Count == 0))
                    {
                        var m = _AppDomainPhysLogManager;
                        _AppDomainPhysLogManager = null;
                        await m.CloseAsync(cancellationToken).ConfigureAwait(false);
                    }
                }
            }
            finally
            {
                _Lock.Set();
            }
        }


        //** LogManager implementation of ILogManager -- Public API surface implementation - See Interfaces.cs
        private class Handle : ILogManager
        {
            private readonly long _HandleId;
            private int _IsOpen;

            public long Id
            {
                get { return this._HandleId; }
            }

            public bool IsOpen
            {
                get { return this._IsOpen != 0; }
                set { this._IsOpen = (value ? 1 : 0); }
            }

            public Handle(long id)
            {
                this.IsOpen = false;
                this._HandleId = id;
            }

            ~Handle()
            {
                ((ILogManager) this).Abort();
            }

            void IDisposable.Dispose()
            {
                ((ILogManager) this).Abort();
            }

            //* ILogManager implementation
            async Task
                ILogManager.CloseAsync(CancellationToken cancellationToken)
            {
                GC.SuppressFinalize(this);

                if (this.IsOpen)
                {
                    if (Interlocked.CompareExchange(ref this._IsOpen, 0, 1) == 1)
                    {
                        await OnCloseHandle(this, cancellationToken).ConfigureAwait(false);
                    }
                }
            }

            void
                ILogManager.Abort()
            {
                // NOTE: Does not hold the continuation for completion of Close
                ((ILogManager) this).CloseAsync(CancellationToken.None);
            }

            Task<IPhysicalLog>
                ILogManager.CreatePhysicalLogAsync(
                string pathToCommonContainer,
                Guid physicalLogId,
                long containerSize,
                uint maximumNumberStreams,
                uint maximumLogicalLogBlockSize,
                LogManager.LogCreationFlags CreationFlags,
                CancellationToken cancellationToken)
            {
                if (!this.IsOpen)
                    throw new FabricObjectClosedException();

                return OnCreatePhysicalLogAsync(
                    pathToCommonContainer,
                    physicalLogId,
                    containerSize,
                    maximumNumberStreams,
                    maximumLogicalLogBlockSize,
                    CreationFlags,
                    cancellationToken);
            }

            Task<IPhysicalLog>
                ILogManager.OpenPhysicalLogAsync(string pathToCommonPhysicalLog, Guid physicalLogId, bool isStagingLog, CancellationToken cancellationToken)
            {
                if (!this.IsOpen)
                    throw new FabricObjectClosedException();

                return OnOpenPhysicalLogAsync(pathToCommonPhysicalLog, physicalLogId, isStagingLog, cancellationToken);
            }

            Task
                ILogManager.DeletePhysicalLogAsync(string pathToCommonPhysicalLog, Guid physicalLogId, CancellationToken cancellationToken)
            {
                if (!this.IsOpen)
                    throw new FabricObjectClosedException();

                return OnDeletePhysicalLogAsync(pathToCommonPhysicalLog, physicalLogId, cancellationToken);
            }

             LogManager.LoggerType
             ILogManager.GetLoggerType()
            {
                if (!this.IsOpen)
                    throw new FabricObjectClosedException();

                return OnGetLoggerType();
            }
        }
    }
}