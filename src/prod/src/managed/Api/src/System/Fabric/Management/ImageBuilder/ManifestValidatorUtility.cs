// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Management.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Reflection;

    static class ManifestValidatorUtility
    {
        private static readonly string TraceType = "ManifestValidatorUtility";
        private static HashSet<string> fabricAssemblies;

 	    internal static string[] FabricAssemblies
        {
            get
            {
                return fabricAssemblies.ToArray<string>();
            }
        }

        static ManifestValidatorUtility()
        {
            InitializeFabricAssemblies();
        }

        private static void InitializeFabricAssemblies()
        {
            fabricAssemblies = new HashSet<string>();

            // Skip dlls and their debug version from the remove list (wastorage.dll and cpprest140_2_9.dll)
            var assembliesToSkipRemoval = new HashSet<string>(new string[] { "wastorage.dll", "cpprest140_2_9.dll", "wastoraged.dll", "cpprest140d_2_9.dll" });

            HashSet<string> featureNames = new HashSet<string>(new string[] { "WinFabCommonBinaries", "WinFabRuntime" });

            foreach (var entry in ArtifactsSpecificationDescription.Entries)
            {                
                if (!featureNames.Contains(entry.Feature, StringComparer.OrdinalIgnoreCase)) { continue; }

                if (!entry.FileName.EndsWith("dll", StringComparison.OrdinalIgnoreCase)) { continue; }
                
                if (assembliesToSkipRemoval.Contains(entry.FileName, StringComparer.OrdinalIgnoreCase)) { continue; }

                if (assembliesToSkipRemoval.Contains(entry.FileName, StringComparer.OrdinalIgnoreCase)) { continue; }

                // We need to consider only native dlls
                if (entry.SigningCertificateType == ArtifactsSpecificationDescription.SigningCertificateType.Native)
                {
                    fabricAssemblies.Add(entry.FileName);
                }
            }

            ReleaseAssert.AssertIf(fabricAssemblies.Count() == 0, "FabricAssemblies count read from ArtifactsSpecification.csv cannot be 0.");
        }

        public static T ConvertProperty<T>(string value, string propertyName, string fileName)
        {
            try
            {
                return ImageBuilderUtility.ConvertString<T>(value);
            }
            catch (Exception exception)
            {
                ImageBuilder.TraceSource.WriteError(
                    TraceType,
                    "Value '{0}' cannot be assigned to {1}.",
                    value,
                    propertyName);

                throw new FabricImageBuilderValidationException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        StringResources.ImageBuilderError_InvalidValue,   
                        propertyName,
                        value),
                    fileName,
                    exception);
            }
        }

        public static bool IsValidName(string name, bool isComposeDeployment = false)
        {
            if (string.IsNullOrEmpty(name))
            {
                return false;
            }

            //When deployment type is not composed type but the name starts with "Compose_"
            if (!isComposeDeployment && name.StartsWith(StringConstants.ComposeDeploymentTypePrefix))
            {
                return false;
            }

            if (name.Contains(StringConstants.DoubleDot))
            {
                return false;
            }

            if (!ImageBuilderUtility.IsValidFileName(name))
            {
                return false;
            }

            return true;
        }

        public static bool IsValidVersion(string version)
        {
            if (!IsValidName(version) || char.IsWhiteSpace(version, 0) || char.IsWhiteSpace(version, version.Length -1))
            {
                return false;
            }
            else
            {
                return true;
            }
        }

        public static void RemoveFabricAssemblies(string localApplicationPackagePath)
        {
            var filePaths = FabricDirectory.GetFiles(localApplicationPackagePath, "*.dll", true, SearchOption.AllDirectories);
            foreach (var filePath in filePaths)
            {
                var fileName = Path.GetFileName(filePath);
                if (fabricAssemblies.Contains(fileName, StringComparer.OrdinalIgnoreCase))
                {
                    FabricFile.Delete(filePath, deleteReadonly: true);
                    ImageBuilder.TraceSource.WriteWarning(TraceType, "File {0} has been removed from the ApplicationPackage since fabric assemblies are not allowed.", filePath);
                }
            }
        }

        public static long? ValidateAsLong(string value, string propertyName, IDictionary<string, string> parameters, string fileName)
        {
            if (!IsValidParameter(value, propertyName, parameters, fileName))
            {
                return ConvertProperty<long>(value, propertyName, fileName);
            }
            else
            {
                return null;
            }
        }

        public static double? ValidateAsDouble(string value, string propertyName, IDictionary<string, string> parameters, string fileName)
        {
            if (!IsValidParameter(value, propertyName, parameters, fileName))
            {
                return ConvertProperty<double>(value, propertyName, fileName);
            }
            else
            {
                return null;
            }
        }

        public static double? ValidateAsPositiveZeroIncluded(decimal value, string propertyName, string fileName)
        {
            double convertedValue = (double) value;
            if (convertedValue < 0)
            {
                ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                    TraceType,
                    fileName,
                    StringResources.ImageBuilderError_ValueMustNotBeNegative,
                    convertedValue,
                    propertyName);
            }

            return convertedValue;
        }

        public static int? ValidateAsPositiveIntZeroIncluded(string value, string propertyName, IDictionary<string, string> parameters, string fileName)
        {
            int convertedValue;
            if (!IsValidParameter(value, propertyName, parameters, fileName))
            {
                convertedValue = ConvertProperty<int>(value, propertyName, fileName);
                if (convertedValue < 0)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_ValueMustNotBeNegative,
                        convertedValue,
                        propertyName);
                }
            }
            else
            {
                return null;
            }
            return convertedValue;
        }

        public static int? ValidateAsPositiveInteger(string value, string propertyName, IDictionary<string, string> parameters, string fileName)
        {
            int convertedValue;
            if (!IsValidParameter(value, propertyName, parameters, fileName))
            {
                convertedValue = ConvertProperty<int>(value, propertyName, fileName);
                if (convertedValue < 1)
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_ValueMustBePositiveInt,
                        convertedValue,
                        propertyName);
                }
            }
            else
            {
                return null;
            }
            return convertedValue;
        }

        public static TimeSpan? ValidateAsTimeSpan(string value, string propertyName, IDictionary<string, string> parameters, string fileName)
        {
            if (!IsValidParameter(value, propertyName, parameters, fileName))
            {
                return ConvertProperty<TimeSpan>(value, propertyName, fileName);
            }
            else
            {
                return null;
            }
        }

        public static bool IsValidParameter(string value, string propertyName, IDictionary<string, string> parameters, string fileName)
        {
            if (ImageBuilderUtility.IsParameter(value))
            {
                string parameterName = value.Substring(1, value.Length - 2);
                if (!parameters.ContainsKey(parameterName))
                {
                    ImageBuilderUtility.TraceAndThrowValidationErrorWithFileName(
                        TraceType,
                        fileName,
                        StringResources.ImageBuilderError_UndefinedParameter,
                        propertyName,
                        parameterName);
                }

                return true;
            }
            else
            {
                return false;
            }
        }
    }
}