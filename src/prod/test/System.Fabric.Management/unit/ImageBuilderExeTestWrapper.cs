// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.ImageBuilderExe;
    using System.Fabric.ImageStore;
    using System.Globalization;
    using System.IO;
    using System.Reflection;

    class ImageBuilderExeTestWrapper
    {
        public static readonly string FabricWorkingDir = "ExeFabricWorkingDir";
        public static readonly string TestDirectory = new FileInfo(Assembly.GetExecutingAssembly().Location).DirectoryName;

        public static readonly int ImageBuilderErrorCode = unchecked((int)System.Fabric.Interop.NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_IMAGEBUILDER_UNEXPECTED_ERROR);

        IImageStore imageStore;
        public ImageBuilderExeTestWrapper(IImageStore imageStore)
        {
            this.imageStore = imageStore;
        }           

        public int GetFabricVersion(string codePath, string configPath, out string codeVersion, out string configVersion)
        {
            int exitCode = -1;

            codeVersion = configVersion = null;

            string tempOutputFile = Path.GetTempFileName();

            string argumentsList = this.GetDefaultArgument();
            argumentsList = AddArgument(argumentsList, StringConstants.Operation, StringConstants.OperationGetFabricVersion);
            argumentsList = AddArgument(argumentsList, StringConstants.CodePath, codePath);
            argumentsList = AddArgument(argumentsList, StringConstants.ConfigPath, configPath);
            argumentsList = AddArgument(argumentsList, StringConstants.Output, tempOutputFile);

            try
            {
                exitCode = ImageBuilderExeTestWrapper.RunImageBuilder(argumentsList);

                if (exitCode == 0)
                {
                    using (StreamReader reader = new StreamReader(tempOutputFile))
                    {
                        string outputString = reader.ReadLine();
                        int delimiterIndex = outputString.IndexOf(":", StringComparison.OrdinalIgnoreCase);

                        if (delimiterIndex != -1)
                        {
                            codeVersion = outputString.Substring(0, delimiterIndex);
                            configVersion = outputString.Substring(delimiterIndex + 1);
                        }
                    }
                }
            }
            finally
            {
                if (File.Exists(tempOutputFile))
                {
                    File.Delete(tempOutputFile);
                }
            }

            return exitCode;
        }

        public int ProvisionFabric(string codePath, string configPath, string infrastructureManifestFilePath)
        {            
            string argumentsList = this.GetDefaultArgument();
            argumentsList = AddArgument(argumentsList, StringConstants.Operation, StringConstants.OperationProvisionFabric);
            argumentsList = AddArgument(argumentsList, StringConstants.CodePath, codePath);
            argumentsList = AddArgument(argumentsList, StringConstants.ConfigPath, configPath);
            argumentsList = AddArgument(argumentsList, StringConstants.InfrastructureManifestFile, infrastructureManifestFilePath);

            return ImageBuilderExeTestWrapper.RunImageBuilder(argumentsList);            
        }

        public int GetApplicationTypeInfo(string buildPath, out string applicationTypeName, out string applicationTypeVersion)
        {
            return GetApplicationTypeInfo(buildPath, null, out applicationTypeName, out applicationTypeVersion);
        }

        public int GetApplicationTypeInfo(string buildPath, string errorDetailsFile, out string applicationTypeName, out string applicationTypeVersion)
        {
            int exitCode = -1;

            applicationTypeName = applicationTypeVersion = null;

            string applicationTypeInfoFile = "ApplicationTypeInfo.txt";

            string argumentsList = this.GetDefaultArgument();
            argumentsList = AddArgument(argumentsList, StringConstants.BuildPath, buildPath);
            argumentsList = AddArgument(argumentsList, StringConstants.Output, applicationTypeInfoFile);

            if (errorDetailsFile == null)
            {
                argumentsList = AddArgument(argumentsList, StringConstants.Operation, StringConstants.OperationBuildApplicationTypeInfo);
            }
            else
            {
                argumentsList = AddArgument(argumentsList, StringConstants.Operation, StringConstants.TestErrorDetails);
                argumentsList = AddArgument(argumentsList, StringConstants.ErrorDetails, errorDetailsFile);
            }

            try
            {
                exitCode = ImageBuilderExeTestWrapper.RunImageBuilder(argumentsList);

                if (exitCode == 0)
                {
                    using (StreamReader reader = new StreamReader(applicationTypeInfoFile))
                    {
                        string outputString = reader.ReadLine();
                        int delimiterIndex = outputString.IndexOf(":", StringComparison.OrdinalIgnoreCase);

                        if (delimiterIndex != -1)
                        {
                            applicationTypeName = outputString.Substring(0, delimiterIndex);
                            applicationTypeVersion = outputString.Substring(delimiterIndex + 1);
                        }
                    }
                }
            }
            finally
            {
                if (File.Exists(applicationTypeInfoFile))
                {
                    File.Delete(applicationTypeInfoFile);
                }
            }

            return exitCode;
        }

        public int GetApplicationType(string buildPath)
        {
            int exitCode = -1;

            string argumentsList = this.GetDefaultArgument();
            argumentsList = AddArgument(argumentsList, StringConstants.Operation, StringConstants.OperationBuildApplicationType);
            argumentsList = AddArgument(argumentsList, StringConstants.BuildPath, buildPath);

            exitCode = ImageBuilderExeTestWrapper.RunImageBuilder(argumentsList);

            return exitCode;
        }

        public int BuildApplication(
            string applicationTypeName,
            string applicationTypeVersion,
            string applicationId,
            string nameUri,
            IDictionary<string, string> applicationParameters,
            string output)
        {
            string argumentsList = this.GetDefaultArgument();
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.Operation, StringConstants.OperationBuildApplication);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppTypeName, applicationTypeName);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppTypeVersion, applicationTypeVersion);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppId, applicationId);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.NameUri, nameUri);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.Output, output);

            string tempFile = null;
            if(applicationParameters.Count != 0)
            {
                tempFile = Path.GetTempFileName();
                WriteApplicationParametersFile(tempFile, applicationParameters);
                argumentsList = AddArgument(argumentsList, StringConstants.AppParam, tempFile);
            }
            var retval = ImageBuilderExeTestWrapper.RunImageBuilder(argumentsList);

            if(File.Exists(tempFile))
            {
                File.Delete(tempFile);
            }
            return retval;
        }

        public int UpgradeApplication(
            string applicationTypeName,
            string currentAppInstanceVersion,
            string targetApplicationTypeVersion,
            string applicationId,
            IDictionary<string, string> applicationParameters,
            string output)
        {
            string argumentsList = this.GetDefaultArgument();
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.Operation, StringConstants.OperationUpgradeApplication);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppTypeName, applicationTypeName);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.CurrentAppInstanceVersion, currentAppInstanceVersion);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppTypeVersion, targetApplicationTypeVersion);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppId, applicationId);
            argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.Output, output);

            foreach (KeyValuePair<string, string> applicationParameter in applicationParameters)
            {
                string formatedParameter = string.Format(CultureInfo.InvariantCulture, "{0};{1}", applicationParameter.Key, applicationParameter.Value);
                argumentsList = ImageBuilderExeTestWrapper.AddArgument(argumentsList, StringConstants.AppParam, formatedParameter);
            }

            return ImageBuilderExeTestWrapper.RunImageBuilder(argumentsList);
        }

        private string GetDefaultArgument()
        {
            string argumentList = string.Empty;
            argumentList = AddArgument(argumentList, StringConstants.SchemaPath, Path.Combine(ImageBuilderExeTestWrapper.TestDirectory, StringConstants.DefaultSchemaPath));
            argumentList = AddArgument(argumentList, StringConstants.StoreRoot, this.imageStore.RootUri);
            argumentList = AddArgument(argumentList, StringConstants.WorkingDir, ImageBuilderExeTestWrapper.FabricWorkingDir);

            return argumentList;
        }

        private static int RunImageBuilder(string argumentsList)
        {
            Process imageBuilderProcess = new Process();
            imageBuilderProcess.StartInfo.FileName = Path.Combine(ImageBuilderExeTestWrapper.TestDirectory, "ImageBuilder.exe");
            imageBuilderProcess.StartInfo.Arguments = argumentsList;
            imageBuilderProcess.StartInfo.CreateNoWindow = true;
            imageBuilderProcess.StartInfo.UseShellExecute = false;
            imageBuilderProcess.StartInfo.RedirectStandardError = true;

            imageBuilderProcess.ErrorDataReceived += new DataReceivedEventHandler(ImageBuilderProcess_ErrorDataReceived);

            imageBuilderProcess.Start();

            imageBuilderProcess.BeginErrorReadLine();

            imageBuilderProcess.WaitForExit();

            return imageBuilderProcess.ExitCode;
        }     

        private static void ImageBuilderProcess_ErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
            Console.WriteLine("Error From ImageBuilder:\n{0}", e.Data);
        }        

        private static string AddArgument(string existingArgumentList, string key, string value)
        {
            return string.Concat(existingArgumentList, string.Format(CultureInfo.InvariantCulture, "{0}:{1} ", key, value));
        }

        private static void WriteApplicationParametersFile(string fileName, IDictionary<string, string> applicationParameters)
        {
            using(StreamWriter writer = new StreamWriter(fileName, false))
            {
                foreach(KeyValuePair<string, string> appParam in applicationParameters)
                {
                    writer.WriteLine(appParam.Key);
                    writer.WriteLine(appParam.Value);
                }
            }
        }
    }
}