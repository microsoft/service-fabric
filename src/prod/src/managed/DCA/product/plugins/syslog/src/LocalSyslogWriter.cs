// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Runtime.InteropServices;

    internal sealed class LocalSysLogger : IDisposable
    {
        private const string MessageFormat = "%s";

        /// <summary>
        /// Please note that the identify string supplied during open call is retained internally by the Syslog routine. Therefore,
        /// we need to make sure that the memory that identity points to doesn't get freed. So, what we are doing is that when we receive the identity,
        /// we would copy the string to unmanaged memory and then store the pointer to it's address in the below pointer. This would be release during dispose.
        /// </summary>
        private IntPtr connectionIdentityPointer = IntPtr.Zero;

        private string connectionIdentity;

        private SyslogFacility connectionFacility;

        // To keep track if object already disposed.
        private bool isDisposed = false;

        public LocalSysLogger(string identity, SyslogFacility facility)
        {
            if (string.IsNullOrWhiteSpace(identity))
            {
                throw new ArgumentNullException("identify", "identity can't be null/empty");
            }

            this.connectionIdentity = identity;
            this.connectionIdentityPointer = Marshal.StringToHGlobalAnsi(this.connectionIdentity);
            this.connectionFacility = facility;

            // open up the syslog connection with the specified identity and facility. Option 1 indicate that Include the PID of the logging process.
            openlog(this.connectionIdentityPointer, 1, this.connectionFacility);
        }

        public void LogMessage(string message, SyslogSeverity severity)
        {
            this.ThrowIfDisposed();

            syslog(this.GetLogPriority(severity), MessageFormat, message);
        }

        private int GetLogPriority(SyslogSeverity messageSeverity)
        {
            return (int)messageSeverity + ((int)this.connectionFacility * 8);
        }

        #region syslog interops

        /// <summary>
        /// opens or reopens a connection to Syslog in preparation for submitting messages.
        /// </summary>
        /// <remarks>
        /// Details available here - https://www.gnu.org/software/libc/manual/html_node/openlog.html#openlog
        /// </remarks>
        /// <param name="ident">This is the arbitrary identification string which will be used as prefix to each message.</param>
        /// <param name="option">This is a bit string</param>
        /// <param name="facility">It defines the facility code for this connection</param>
        [DllImport("libc")]
        private static extern void openlog(IntPtr ident, int option, SyslogFacility facility);

        /// <summary>
        /// Submits a message to the Syslog facility.
        /// </summary>
        /// <remarks>
        /// Details are available here - https://www.gnu.org/software/libc/manual/html_node/syslog_003b-vsyslog.html#syslog_003b-vsyslog
        /// Also, please note that the syslog method is very similar to printf i.e. it takes var arguments and a format. To make it work in c#
        /// we need to provide args explicityly. For our interop, there is only one Arg and the Format needs to be always "%s".
        /// </remarks>
        /// <param name="priority">priority</param>
        /// <param name="format">format. In our case, it is always "%s"</param>
        /// <param name="message">Actual message payload</param>
        [DllImport("libc", CharSet = CharSet.Ansi, CallingConvention = CallingConvention.Cdecl)]
        private static extern void syslog(int priority, string format, string message);

        /// <summary>
        /// This closes the current Syslog connection, if there is one.
        /// </summary>
        /// <remarks>
        /// Details available here - https://www.gnu.org/software/libc/manual/html_node/closelog.html#closelog
        /// </remarks>
        [DllImport("libc")]
        private static extern void closelog();

        #endregion syslog interops

        #region IDisposable Support

        internal void ThrowIfDisposed()
        {
            if (this.isDisposed)
            {
                throw new ObjectDisposedException("LocalSyslogWriter");
            }
        }

        private void Dispose(bool disposing)
        {
            if (!isDisposed)
            {
                if (disposing)
                {
                    // dispose managed state (managed objects).
                }

                try
                {
                    // close the syslog connection.
                    closelog();
                }
                finally
                {
                    // Release the unmanaged memory block.
                    if (this.connectionIdentityPointer != IntPtr.Zero)
                    {
                        Marshal.FreeHGlobal(this.connectionIdentityPointer);
                        this.connectionIdentityPointer = IntPtr.Zero;
                    }
                }

                isDisposed = true;
            }
        }

        ~LocalSysLogger()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        // Dispose the sys writer.
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion IDisposable Support
    }
}