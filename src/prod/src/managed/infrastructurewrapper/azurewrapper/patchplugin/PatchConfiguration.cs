// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Xml;
using System.Xml.XPath;

namespace Microsoft.Xdb.PatchPlugin
{
	public class PatchConfiguration
	{
		public class PatchRegistry
		{
			public string RegistryKeyName { get; set; }
			public string RegistryValueName { get; set; }
			public string ExpectedValue { get; set; }
		}

		public class Patch
		{
			public string Name { get; set; }
			public string CommandLine { get; set; }
			public string LogFolderPath { get; set; }
			public IEnumerable<PatchRegistry> RegistrySettings { get; set; }
		}

		private const string OverallTimeoutValueXPath = "//Patches/OverallTimeout/@value";
		private const string PatchXPath = "//Patches/Patch";
		private const string PatchRegistryXPath = "RegistryTable/Registry";
		private const string CommandLineXPath = "Command/@value";
		private const string LogFolderPathXPath = "LogFolder/@path";
		private const string RegistryKeyNameAttribute = "@keyName";
		private const string RegistryValueNameAttribute = "@valueName";
		private const string RegistryExpectedValueAttribute = "@expectedValue";
		
		private int _overallTimeoutSeconds = 120;
		private readonly Dictionary<string, Patch> _patchMap = new Dictionary<string, Patch>(StringComparer.OrdinalIgnoreCase);

		public int OverallTimeoutSeconds { get { return _overallTimeoutSeconds; } }
		public IDictionary<string, Patch> PatchMap { get { return _patchMap; } }

		public void Load(string configFilePath)
		{
			if (string.IsNullOrEmpty(configFilePath))
			{
				LogUtils.WriteTrace(DateTime.UtcNow, "configFilePath cannot be null or empty");
				throw new ArgumentException("Patch configuration file path cannot be null or empty.", "configFilePath");
			}

			if (!File.Exists(configFilePath))
			{
				LogUtils.WriteTrace(DateTime.UtcNow, string.Format("Cannot find patch configuration file: {0}", configFilePath));
				throw new InvalidOperationException("Cannot find patch configuration file: " + configFilePath);
			}

			LoadConfiguration(configFilePath);
		}

		private static XPathNavigator GetNavigator(string configFilePath)
		{
			XmlDocument xmlDocument = new XmlDocument();
			xmlDocument.Load(configFilePath);
			return xmlDocument.CreateNavigator();
		}

		private void LoadConfiguration(string configFilePath)
		{
			XPathNavigator navigator = GetNavigator(configFilePath);
			string overallTimeoutString = navigator.SelectSingleNode(OverallTimeoutValueXPath).Value;
			if (string.IsNullOrEmpty(overallTimeoutString) || (!int.TryParse(overallTimeoutString, out _overallTimeoutSeconds)))
			{
				LogUtils.WriteTrace(DateTime.UtcNow, "OverallTimeout value is not defined or is not a valid integer value. OverallTimeout = " +
					(string.IsNullOrEmpty(overallTimeoutString) ? "null" : overallTimeoutString));

				throw new InvalidOperationException(
					"OverallTimeout value is not defined or is not a valid integer value. OverallTimeout = " +
					(string.IsNullOrEmpty(overallTimeoutString) ? "null" : overallTimeoutString));
			}

			if (_overallTimeoutSeconds < 0)
			{
				LogUtils.WriteTrace(DateTime.UtcNow, "OverallTimeout value cannot be less than 0. OverallTimeout = " +
					overallTimeoutString);

				throw new InvalidOperationException(
					"OverallTimeout value cannot be less than 0. OverallTimeout = " +
					overallTimeoutString);
			}

			// Loop through all patches
			XPathNodeIterator nodeIterator = navigator.Select(PatchXPath);
			while (nodeIterator.MoveNext())
			{
				Patch patch = new Patch();

				// Read name
				if (nodeIterator.Current.SelectSingleNode("@name") == null)
				{
					LogUtils.WriteTrace(DateTime.UtcNow, "name attribute needed for Patch element");

					throw new InvalidOperationException("name attribute needed for Patch element");
				}

				patch.Name = nodeIterator.Current.SelectSingleNode("@name").Value.Trim();
				if (string.IsNullOrEmpty(patch.Name))
				{
					LogUtils.WriteTrace(DateTime.UtcNow, "name attribute can't be empty for Patch element");

					throw new InvalidOperationException("name attribute can't be empty for Patch element");
				}

				if (_patchMap.ContainsKey(patch.Name))
				{
					LogUtils.WriteTrace(DateTime.UtcNow, string.Format(CultureInfo.InvariantCulture, "Patch '{0}' is defined in multiple places.", patch.Name));

					throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Patch '{0}' is defined in multiple places.", patch.Name));
				}

				// Read OS versions the patch applies to and check whether it is applicable to current OS version               
				if (nodeIterator.Current.SelectSingleNode("@appliesToOsVersion") != null)
				{
					string osVersionAppliesTo = string.Empty;

					try
					{
						bool invalidVersionFormat = false;
						osVersionAppliesTo = nodeIterator.Current.SelectSingleNode("@appliesToOsVersion").Value.Trim();

						Version currentOsVersion = Environment.OSVersion.Version;

						char[] versionDelimiter = new char[] { '.' };
						string[] versionComponents = osVersionAppliesTo.Split(versionDelimiter);
						string[] currentOsVersionComponents = currentOsVersion.ToString().Split(versionDelimiter);

						// OS version the patch applies to should have four components: major.minor.build.revision                            
						if (versionComponents.Length != 4)
						{
							invalidVersionFormat = true;
						}
						else
						{
							// Each version component should be an integer or a wildcard character '*'
							foreach (string t in versionComponents)
							{
								int versionComponent;
								if (t.Equals("*", StringComparison.OrdinalIgnoreCase) || int.TryParse(t, out versionComponent)) continue;
								invalidVersionFormat = true;
								break;
							}
						}

						if (invalidVersionFormat)
						{
							LogUtils.WriteTrace(DateTime.UtcNow, string.Format(
								@"PathUtil: Invalid version format for OS version patch {0} applies to : {1}; the patch will be installed",
								patch.Name,
								osVersionAppliesTo));
						}
						else
						{
							bool skipPatch = false;
							for (int i = 0; i < Math.Min(versionComponents.Length, currentOsVersionComponents.Length); i++)
							{
								if (!versionComponents[i].Equals("*", StringComparison.OrdinalIgnoreCase) &&
									!versionComponents[i].Equals(currentOsVersionComponents[i], StringComparison.OrdinalIgnoreCase))
								{
									skipPatch = true;
								}
							}

							// If the patch is not applicable to current OS version, skip it
							if (skipPatch)
							{
								LogUtils.WriteTrace(DateTime.UtcNow, string.Format(
									@"PatchUtil: Patch {0} will be skipped since it is only applicable to OS version {1} while current OS version is {2}",
									patch.Name,
									osVersionAppliesTo,
									currentOsVersion));

								continue;
							}
						}
					}
					catch (Exception e)
					{
						LogUtils.WriteTrace(DateTime.UtcNow, string.Format(
							@"PathUtil: Exception occurred when checking whether patch {0} (applicable to OS version {1}) is applicable to current OS version, skip version checking and continue installing : {2}",
							patch.Name,
							osVersionAppliesTo,
							e));
					}
				}

				// Read command line.
				if (nodeIterator.Current.SelectSingleNode(CommandLineXPath) == null)
				{
					LogUtils.WriteTrace(DateTime.UtcNow, string.Format(CultureInfo.InvariantCulture, "Command value attribute needed for Patch '{0}'.", patch.Name));

					throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Command value attribute needed for Patch '{0}'.", patch.Name));
				}

				patch.CommandLine = nodeIterator.Current.SelectSingleNode(CommandLineXPath).Value.Trim();

				if (string.IsNullOrEmpty(patch.CommandLine))
				{
					LogUtils.WriteTrace(DateTime.UtcNow, string.Format(CultureInfo.InvariantCulture, "Command value can't be empty for Patch '{0}'.", patch.Name));

					throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Command value can't be empty for Patch '{0}'.", patch.Name));
				}

				// Read log path (optional element).
				patch.LogFolderPath = null;
				if (nodeIterator.Current.SelectSingleNode(LogFolderPathXPath) != null)
				{
					string logFolderPath = nodeIterator.Current.SelectSingleNode(LogFolderPathXPath).Value.Trim();

					if (!string.IsNullOrEmpty(logFolderPath))
					{
						patch.LogFolderPath = logFolderPath;
					}
				}

				// Read registry settings
				List<PatchRegistry> registrySettings = new List<PatchRegistry>();
				XPathNodeIterator iterator = nodeIterator.Current.Select(PatchRegistryXPath);
				while (iterator.MoveNext())
				{
					string keyName = ReadPatchRegistryAttribute(iterator, patch.Name, RegistryKeyNameAttribute, true);
					string valueName = ReadPatchRegistryAttribute(iterator, patch.Name, RegistryValueNameAttribute, true);
					string expectedValue = ReadPatchRegistryAttribute(iterator, patch.Name, RegistryExpectedValueAttribute, false);

					PatchRegistry registrySetting = new PatchRegistry
						{
							RegistryKeyName = keyName,
							RegistryValueName = valueName,
							ExpectedValue = expectedValue,
						};
					registrySettings.Add(registrySetting);
				}

				// We need at least one registry setting to confirm the patch is installed.
				if (!registrySettings.Any())
				{
					LogUtils.WriteTrace(DateTime.UtcNow, string.Format(CultureInfo.InvariantCulture, "There is no registry setting found for Patch '{0}'", patch.Name));

					throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "There is no registry setting found for Patch '{0}'", patch.Name));
				}

				patch.RegistrySettings = registrySettings;
				_patchMap.Add(patch.Name, patch);
			}
		}

		private static string ReadPatchRegistryAttribute(
			XPathNodeIterator iterator,
			string patchName,
			string attributeName,
			bool throwIfError)
		{
			// Read registry attribute
			string attributeValue = null;
			XPathNavigator tempIterator = iterator.Current.SelectSingleNode(attributeName);
			if (tempIterator != null)
			{
				attributeValue = tempIterator.Value.Trim();
			}

			if (throwIfError && string.IsNullOrEmpty(attributeValue))
			{
				LogUtils.WriteTrace(DateTime.UtcNow, string.Format(
					CultureInfo.InvariantCulture,
					"{0} attribute not found or is empty in element Registry of Patch name {1}",
					attributeName,
					patchName));

				throw new InvalidOperationException(
					string.Format(
					CultureInfo.InvariantCulture,
					"{0} attribute not found or is empty in element Registry of Patch name {1}",
					attributeName,
					patchName));
			}

			return attributeValue;
		}
	}
}