// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System.Text;

    /// <summary>
    /// Helper class to log stuff with a timestamp so that we know when a particular trace was emitted
    /// </summary>
    public class LogHelper : IDisposable
    {
        static int indent = 0;

        public LogHelper(string name)
        {
            LogHelper.Log("Operation: " + name);
            LogHelper.indent++;
        }

        public static void Log(string format, params object[] args)
        {
            string message = string.Format(format, args);

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < LogHelper.indent; i++)
            {
                sb.Append("  ");
            }

            DateTime time = DateTime.Now;
            Console.WriteLine(string.Format("{0}({1,2:00.##}:{2,2:00.##}:{3,2:00.##}.{4,3:000.##}): {5}", sb.ToString(), time.Hour, time.Minute, time.Second, time.Millisecond, message));
        }

        #region IDisposable Members

        public void Dispose()
        {
            LogHelper.indent--;
            if (LogHelper.indent < 0)
            {
                LogHelper.indent = 0;
            }
        }

        #endregion
    }
}