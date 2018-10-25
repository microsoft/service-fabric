// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;
    using System.ComponentModel;
    using System.Fabric.Strings;
    using System.Threading;

    public sealed class RealTimeEventReader : IDisposable
    {
        [ThreadStatic]
        private static bool hasProcessEventLock;

        private TraceSession traceSession;
        private ReaderWriterLockSlim rwLock;
        private Thread processingThread;
        private bool canceled;
        private bool disposed;
        private Exception processingException;

        public RealTimeEventReader(string name, Guid providerId, TimeSpan flushInterval)
            : this(name, new ProviderInfo(providerId), flushInterval)
        {
        }

        public RealTimeEventReader(string name, ProviderInfo providerInfo, TimeSpan flushInterval)
        {
            if (name == null)
            {
                throw new ArgumentNullException("name");
            }

            if (name.Length == 0)
            {
                throw new ArgumentException(StringResources.ETLReaderError_EmptyParameter, "name");
            }

            int max = EventTraceProperties.MaxLoggerNameLength - 1;
            if (name.Length > max)
            {
                throw new ArgumentOutOfRangeException("name", String.Format(System.Globalization.CultureInfo.CurrentCulture,
                                                                            StringResources.ETLReaderError_ParameterExceedsMax_Formatted,
                                                                            max)
                                                    );
            }

            if (providerInfo == null)
            {
                throw new ArgumentNullException("providerInfo");
            }

            if (flushInterval < TimeSpan.Zero)
            {
                throw new ArgumentOutOfRangeException("flushInterval");
            }

            this.rwLock = new ReaderWriterLockSlim();
            this.traceSession = new TraceSession(name, providerInfo, flushInterval);
        }

        public event EventHandler<EventRecordEventArgs> EventRead;

        public uint EventsLost
        {
            get;
            private set;
        }

        public void Run()
        {
            this.StartInner(true);
        }

        public void Start()
        {
            this.StartInner(false);
        }

        public void Stop()
        {
            this.ThrowIfDisposed();

            // Do not allow call to go through if we are in the event handler,
            // as we need to acquire the write lock to dispose the session.
            if (RealTimeEventReader.hasProcessEventLock)
            {
                throw new InvalidOperationException(StringResources.ETLReaderError_StopCalledFromEventreadHandler);
            }

            this.DisposeTraceSession();

            // Do not Join here if we are running on the processing thread
            if ((this.processingThread != null) && (this.processingThread != Thread.CurrentThread))
            {
                // The thread should stop shortly after the trace session is closed
                this.processingThread.Join();
                this.processingThread = null;
            }

            if (this.processingException != null)
            {
                Exception e = this.processingException;
                this.processingException = null;
                throw new InvalidOperationException(StringResources.ETLReaderError_ErrorProcessingTraces, e);
            }
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void DisposeTraceSession()
        {
            if (this.traceSession != null)
            {
                this.rwLock.EnterWriteLock();
                try
                {
                    this.traceSession.Dispose();
                    this.traceSession = null;
                }
                finally
                {
                    this.rwLock.ExitWriteLock();
                }
            }
        }

        private void StartInner(bool runOnCurrentThread)
        {
            if (this.traceSession == null)
            {
                throw new InvalidOperationException(StringResources.ETLReaderError_SessionAlreadyStarted);
            }

            this.ThrowIfDisposed();

            if (this.EventRead == null)
            {
                throw new InvalidOperationException(StringResources.ETLReaderError_NoSubscribersForEventRead);
            }

            if (this.processingThread != null)
            {
                throw new InvalidOperationException(StringResources.ETLReaderError_ReaderAlreadyStarted);
            }

            this.canceled = false;

            this.traceSession.EventRead += this.OnEventRead;
            this.traceSession.Open();

            if (runOnCurrentThread)
            {
                this.Process();
            }
            else
            {
                // Start on a new thread since this is a synchronous call
                this.processingThread = new Thread(this.Process);
                this.processingThread.Start();
            }
        }

        private void Process()
        {
            try
            {
                this.traceSession.Process();
            }
            catch (Win32Exception e)
            {
                this.processingException = e;
            }
        }

        private void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (!this.disposed)
                {
                    try
                    {
                        this.Stop();
                    }
                    catch (InvalidOperationException)
                    {
                        // Do not allow exceptions to bubble up from Dispose
                    }

                    this.rwLock.Dispose();
                }
            }

            this.disposed = true;
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("RealTimeEventReader");
            }
        }

        private void OnEventRead(object sender, EventRecordEventArgs e)
        {
            if (!this.canceled)
            {
                this.rwLock.EnterReadLock();
                RealTimeEventReader.hasProcessEventLock = true;
                try
                {
                    if (this.traceSession != null)
                    {
                        this.EventsLost = this.traceSession.EventsLost;

                        // Guaranteed non-null due to check in Start()
                        this.EventRead(this, e);
                    }
                }
                finally
                {
                    RealTimeEventReader.hasProcessEventLock = false;
                    this.rwLock.ExitReadLock();
                }

                this.StopIfCanceled(e.Cancel);
            }
        }

        private void StopIfCanceled(bool shouldCancel)
        {
            if (shouldCancel)
            {
                this.canceled = true;
                try
                {
                    this.Stop();
                }
                finally
                {
                    this.processingThread = null;
                }
            }
        }
    }
}