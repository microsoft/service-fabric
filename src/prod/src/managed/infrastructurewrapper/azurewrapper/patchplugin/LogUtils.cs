// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.IO;

namespace Microsoft.Xdb.PatchPlugin
{
	/// <summary>
	/// Log utils class
	/// </summary>
	public static class LogUtils
	{
		// Write to log file
		public static void WriteTrace(DateTime timestamp, string message)
		{
			if (!Directory.Exists(Environment.ExpandEnvironmentVariables(Common.TextLogDirectory)))
			{
				Directory.CreateDirectory(Environment.ExpandEnvironmentVariables(Common.TextLogDirectory));
			}

			using (FileStream fs = new FileStream(Path.Combine(Environment.ExpandEnvironmentVariables(Common.TextLogDirectory), Common.TextLogFileName), FileMode.Append, FileAccess.Write, FileShare.Read))
			{
				using (StreamWriter tw = new StreamWriter(fs))
				{
					tw.WriteLine("{0}:{1}", timestamp, message);
				}
			}
		}

		// Flush the output
		public static void Flush()
		{
			if (!Directory.Exists(Environment.ExpandEnvironmentVariables(Common.TextLogDirectory)))
			{
				Directory.CreateDirectory(Environment.ExpandEnvironmentVariables(Common.TextLogDirectory));
			}

			using (FileStream fs = new FileStream(Path.Combine(Environment.ExpandEnvironmentVariables(Common.TextLogDirectory), Common.TextLogFileName), FileMode.Append, FileAccess.Write, FileShare.Read))
			{
				using (StreamWriter tw = new StreamWriter(fs))
				{
					tw.Flush();
					tw.Dispose();
				}
			}
		}
	}
}