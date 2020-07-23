// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;
    using System.Runtime.CompilerServices;

    /// <summary>
    /// Wraps the TraceWriter in a class instance so that it can be injected as a dependency via constructor.
    /// </summary>
    internal class TraceWriterWrapper
    {
        internal virtual void Exception(Exception ex, [CallerMemberName]string callerName = "Unknown")
        {
            var exceptionTrace = this.GetExceptionTrace(callerName, ex);
            var innerException = ex.InnerException;

            while (innerException != null)
            {
                exceptionTrace += this.GetInnerExceptionTrace(innerException);
                innerException = innerException.InnerException;
            }

            this.WriteError(exceptionTrace);
        }

        internal virtual void WriteInfo(string format, params object[] args)
        {
            MonitoringServiceEventSource.Current.Message(format, args);
        }

        internal virtual void WriteError(string format, params object[] args)
        {
            MonitoringServiceEventSource.Current.Error(format, args);
        }

        private string GetExceptionTrace(string callerName, Exception ex)
        {
            var exceptionTrace = this.GetExceptionTraceWithStack(ex);
            return string.Format("Exception encountered in {0}. Exception info: {1}.", callerName, exceptionTrace);
        }

        private string GetInnerExceptionTrace(Exception ex)
        {
            return string.Format("InnerException: {0}", this.GetExceptionTraceWithStack(ex));
        }

        private string GetExceptionTraceWithStack(Exception ex)
        {
            return string.Format("{0}. StackTrace: {1}", ex.Message, ex.StackTrace);
        }
    }
}