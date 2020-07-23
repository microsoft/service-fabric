// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace System.Fabric.Interop
{
    using System.Globalization;
    using System.Text;

    internal class InteropApi
    {
        public static readonly InteropApi Default = new InteropApi();

        public InteropApi()
        {
            this.CopyExceptionDetailsToThreadErrorMessage = false;
            this.ShouldIncludeStackTraceInThreadErrorMessage = true;
        }

        public bool CopyExceptionDetailsToThreadErrorMessage { get; set; }

        public bool ShouldIncludeStackTraceInThreadErrorMessage { get; set; }

        // Copies the exception message and stack trace to ErrorThreadMessage
        public void HandleException(Exception ex)
        {
            if (this.CopyExceptionDetailsToThreadErrorMessage)
            {
                using(var pin = new PinCollection())
                {
                    NativeCommon.FabricSetLastErrorMessage(pin.AddObject(CreateThreadErrorMessage(ex, this.ShouldIncludeStackTraceInThreadErrorMessage)));
                }
            }
        }

        private static string GetExceptionInfoString(System.Type type, int HResult)
        {
            return string.Format(CultureInfo.InvariantCulture, "{0} ({1})", type, HResult);
        }

        private static string GetErrorStackTrace(Exception ex)
        {
            if (ex == null)
            {
                return String.Empty;
            }

            string stackTrace = GetErrorStackTrace(ex.InnerException);
            StringBuilder sb = new StringBuilder();
            if (stackTrace != String.Empty)
            {
                sb.Append(stackTrace);
                sb.AppendLine();
                sb.Append("--- End of inner exception stack trace ---");
                sb.AppendLine();
            }

            sb.Append(ex.StackTrace);

            return sb.ToString();
        }

        private static string CreateThreadErrorMessage(Exception ex, bool includeStackTrace = true)
        {
            const int MaxCharactersOfMessageToCopy = 512;

            // there are limits such as 4k for the health report size and 4k for the interop
            // use a smaller limit here
            const int MaxLength = 4000; 

            ExceptionStringBuilder sb = new ExceptionStringBuilder(MaxLength);

            var exception = ex;
            while (exception != null)
            {
                if (exception.Message != null)
                {
                    sb.Append(GetExceptionInfoString(exception.GetType(), exception.HResult), MaxCharactersOfMessageToCopy);
                    sb.AppendLine();
                    sb.Append(exception.Message, MaxCharactersOfMessageToCopy);
                    sb.AppendLine();
                }
                exception = exception.InnerException;
            }

            if (includeStackTrace)
            {
                string st = GetErrorStackTrace(ex);
                sb.Append(st);
            }

            return sb.ToString();
        }

        private class ExceptionStringBuilder
        {
            private readonly int maxLength;
            private readonly StringBuilder sb = new StringBuilder();

            public ExceptionStringBuilder(int maxLength)
            {
                this.maxLength = maxLength;
            }

            public void Append(string s)
            {
                this.sb.Append(s);
            }

            public void AppendLine()
            {
                this.sb.AppendLine();
            }

            public void Append(string s, int maxCharacters)
            {
                if (sb.Length > maxCharacters)
                {
                    //already exceeded limit
                    return;
                }

                if ((sb.Length + s.Length) > maxCharacters)
                {
                    this.sb.Append(s, 0, (maxCharacters - sb.Length));
                }
                else
                {
                    this.sb.Append(s);
                }
            }

            public override string ToString()
            {
                if (this.sb.Length > this.maxLength)
                {
                    return sb.ToString(0, this.maxLength);
                }
                else
                {
                    return sb.ToString();
                }
            }
        }
    }
}