// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using Microsoft.Win32;

namespace Microsoft.Xdb.PatchPlugin
{
	public class PatchUtil
	{
		private const string PatchConfigurationFileName = "PatchConfiguration.xml";
		private bool _rebootRequired;

		/// <summary>
		/// Read only property to indicate that we have installed at least one patch and require reboot
		/// </summary>
		public bool RebootRequired { get { return _rebootRequired; } }

		/// <summary>
		/// Install patches
		/// </summary>
		public void InstallPatches()
		{
		    string patchConfigFilePath = Path.Combine(Path.GetPathRoot(Assembly.GetExecutingAssembly().Location), "plugins", "patchplugin", PatchConfigurationFileName);

			PatchConfiguration patchConfiguration = new PatchConfiguration();
			patchConfiguration.Load(patchConfigFilePath);

			if (patchConfiguration.PatchMap == null || patchConfiguration.PatchMap.Count == 0)
			{
				LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: no patches to install.");
				return;
			}

			// First check if all patches have been installed
			IList<PatchConfiguration.Patch> uninstalledPatches = GetUninstalledPatches(patchConfiguration);
			if (uninstalledPatches.Count == 0)
			{
				LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: all patches have been installed.");
				return;
			}

		    try
			{
				InstallPatches(uninstalledPatches, patchConfiguration.OverallTimeoutSeconds * 1000);

				// Wait timeout and return
				DateTime startTime = DateTime.UtcNow;
				while (DateTime.UtcNow < startTime + TimeSpan.FromSeconds(patchConfiguration.OverallTimeoutSeconds))
				{
					uninstalledPatches = GetUninstalledPatches(patchConfiguration);
					if (uninstalledPatches.Count == 0)
					{
						LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: all patches have been installed. Exit and reboot without waiting until timeout.");
						return;
					}

					LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: sleep 1 minutes before checking patch installation again.");
					Thread.Sleep(TimeSpan.FromMinutes(1));
				}
			}
			catch (Exception e)
			{
				// Log the exception
				LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: Patch installation encountered an Exception: " + e);

				if (!_rebootRequired)
				{
					// We haven't start install patches, so throw and the process will be restarted (without reboot)
					throw;
				}

				// Eat the exception and the VM is going to be rebooted.
				LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: At least one patch is installed, waiting to be rebooted.");
			}
		}

		/// <summary>
		/// Get uninstalled patches list
		/// </summary>
		/// <param name="patchConfiguration"></param>
		/// <returns></returns>
		private static IList<PatchConfiguration.Patch> GetUninstalledPatches(PatchConfiguration patchConfiguration)
		{
			List<PatchConfiguration.Patch> uninstalledPatches = new List<PatchConfiguration.Patch>();

			foreach (PatchConfiguration.Patch patch in patchConfiguration.PatchMap.Values)
			{
				LogUtils.WriteTrace(DateTime.UtcNow,
									string.Format("PatchUtil: check registry setting for patch '{0}'", patch.Name));

				if (patch.RegistrySettings == null)
				{
					continue;
				}

				foreach (PatchConfiguration.PatchRegistry reg in patch.RegistrySettings)
				{
					LogUtils.WriteTrace(DateTime.UtcNow, string.Format(@"PatchUtil: read registry value '{0}\{1}' for patch '{2}'",
												  reg.RegistryKeyName, reg.RegistryValueName, patch.Name));

					object value = Registry.GetValue(reg.RegistryKeyName, reg.RegistryValueName, null);

					if (value == null)
					{
						LogUtils.WriteTrace(DateTime.UtcNow,
							string.Format(
								@"PatchUtil: registry value '{0}\{1}' does not exist or has null value. Need to install patch '{2}'",
								reg.RegistryKeyName, reg.RegistryValueName, patch.Name));

						// PatchConfiguration doesn't allow the same patch be defined multiple times. No need to check it here again.
						uninstalledPatches.Add(patch);

						// Go to next patch
						break;
					}

					// Compare registry value. We only support REG_DWORD and REG_SZ for now
					Type valueType = value.GetType();

					if (valueType == typeof(string))
					{
						string stringValue = value as string;

						LogUtils.WriteTrace(DateTime.UtcNow,
											string.Format(
												@"PatchUtil: registry value '{0}\{1}' has string value: '{2}'",
												reg.RegistryKeyName, reg.RegistryValueName, stringValue));

						// If reg.ExpectedValue is null, we only validate there is a reg value there.
						if (reg.ExpectedValue == null)
						{
							LogUtils.WriteTrace(DateTime.UtcNow, @"PatchUtil: skip validation because the ExpectedValue is null");
						}
						else if (!string.Equals(stringValue, reg.ExpectedValue, StringComparison.OrdinalIgnoreCase))
						{
							LogUtils.WriteTrace(DateTime.UtcNow, string.Format(
								@"PatchUtil: registry value '{0}\{1}' does not match the expected value: '{2}'. Need to install patch '{3}'",
								reg.RegistryKeyName,
								reg.RegistryValueName,
								reg.ExpectedValue,
								patch.Name));

							// Add to the list
							uninstalledPatches.Add(patch);

							// Go to next patch
							break;
						}
					}
					else if (valueType == typeof(int))
					{
						int intValue = (int)value;

						LogUtils.WriteTrace(DateTime.UtcNow, string.Format(@"PatchUtil: registry value '{0}\{1}' has integer value: '{2}'",
													  reg.RegistryKeyName, reg.RegistryValueName, intValue));

						int expectedValue;
						if (reg.ExpectedValue == null)
						{
							LogUtils.WriteTrace(DateTime.UtcNow, @"PatchUtil: skip validation because the ExpectedValue is null");
						}
						else if ((!int.TryParse(reg.ExpectedValue, out expectedValue)) || (expectedValue != intValue))
						{
							LogUtils.WriteTrace(DateTime.UtcNow, string.Format(
								@"PatchUtil: registry value '{0}\{1}' does not match the expected value: '{2}'. Need to install patch '{3}'",
								reg.RegistryKeyName,
								reg.RegistryValueName,
								reg.ExpectedValue,
								patch.Name));

							// Add to the list
							uninstalledPatches.Add(patch);

							// Go to next patch
							break;
						}
					}
					else
					{
						throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "PatchUtil: Registry value type '{0}' is not supported.", valueType));
					}
				}
			}

			return uninstalledPatches;
		}

		[SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Our code runs in full trust environment")]
		private void InstallPatches(IList<PatchConfiguration.Patch> patchesToInstall, int timeoutMs)
		{
			string patchInstallFolder = GetPatchInstallationFolder();
			LogUtils.WriteTrace(DateTime.UtcNow,
								string.Format("PatchUtil: set patch installation folder: '{0}'", patchInstallFolder));

			foreach (PatchConfiguration.Patch patch in patchesToInstall)
			{
				StringBuilder outString = new StringBuilder();
				StringBuilder errString = new StringBuilder();

				LogUtils.WriteTrace(DateTime.UtcNow, string.Format("PatchUtil: install patch '{0}'", patch.Name));
				string command = patch.CommandLine.TrimStart();
				LogUtils.WriteTrace(DateTime.UtcNow, string.Format("PatchUtil: run commandline '{0}'", command));

				// Prepare the log folder
				if (!string.IsNullOrEmpty(patch.LogFolderPath))
				{
					string expandedPath = Environment.ExpandEnvironmentVariables(patch.LogFolderPath);
					LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: patch log folder is at: " + expandedPath);

					if (!Directory.Exists(expandedPath))
					{
						Directory.CreateDirectory(expandedPath);
						LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: patch log folder is created.");
					}
					else
					{
						LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: patch log folder exist.");
					}
				}

				// Start to run the command to install patch. Set _rebootRequired to be true
				_rebootRequired = true;

				int exitCode = ScriptingHelper.Execute(
					command,
					new StringWriter(outString, CultureInfo.InvariantCulture),
					new StringWriter(errString, CultureInfo.InvariantCulture),
					timeoutMs,
					patchInstallFolder);

				LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: run commandline exit with code: " + exitCode);
				if (!string.IsNullOrEmpty(outString.ToString()))
				{
					LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: run commandline output: " + outString);
				}

				if (!string.IsNullOrEmpty(errString.ToString()))
				{
					LogUtils.WriteTrace(DateTime.UtcNow, "PatchUtil: run commandline error: " + errString);
				}

				// Do not check the exit code because we don't know what is expected
			}
		}

		/// <summary>
		/// Get patch installation folder root that contains PatchConfiguration.xml
		/// </summary>
		/// <returns></returns>
		private static string GetPatchInstallationFolder()
		{
            string rootDirectory = Path.Combine(Path.GetPathRoot(Assembly.GetExecutingAssembly().Location), "plugins", "patchplugin");

			return rootDirectory;
		}
	}
}