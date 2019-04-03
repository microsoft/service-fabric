// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ImageStoreClient
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.ImageStore;
    using System.Fabric.Management;
    using System.Fabric.Management.ImageStore;
    using System.Fabric.Management.WindowsFabricValidator;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Text;

    /// <summary>
    /// This is a wrapper class over the ImageStore to provide an executable to access the image store.
    /// </summary>
    public class ImageStoreClient
    {
        internal static FabricEvents.ExtensionsEvents TraceSource;

        private static readonly string TraceType = "ImageStoreClientExe";

        private static readonly int SuccessExitCode = 0;
        private static readonly int GenericImageStoreErrorExitCode = 1;
        private static readonly int FileNotFoundExitCode = 2;

        /// <summary>
        /// The enumeration of possible commands.
        /// </summary>
        internal enum Commands
        {
            /// <summary>
            /// Command to delete a particular tag in the store.
            /// </summary>
            Delete, // This is required for cleanup in tests.

            /// <summary>
            /// Command to upload the tag to the image store.
            /// </summary>
            Upload, // This is required for tests and also for deploying the build layout.

            /// <summary>
            /// Command to download the tags from the image store.
            /// </summary>
            Download,

            /// <summary>
            /// Command to check if the tag exists in the image store.
            /// </summary>
            Exists
        }

        static ImageStoreClient()
        {
            // Try reading config
            TraceConfig.InitializeFromConfigStore();

            // Create the trace source based on the config
            ImageStoreClient.TraceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.ImageStoreClient);
#if !DotNetCoreClr
            ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;
#endif
        }

        /// <summary>
        /// Main function.
        /// ImageStoreClient is a program used to access the image store. Its usage is as follows:
        ///        ImageStoreClient <command> -c <connectionString> [-f <fileName>] [-x <xstoreTag>] [-l <localTag>] [-g <flag>]
        ///          <command>: Delete (just takes the <xStoreTag>), Download, Upload
        ///          <connectionString> is the connection string to connect to the xstore image store.
        ///          <fileName> is the name of the file that contains list of files to be uploaded / downloaded
        ///          <xstoreTag> is the tag on xstore of the file to be uploaded / downloaded.
        ///          <localTag> is tag of the local file to be uploaded / downloaded.
        ///          <flag> is the flag for the copy (CopyIfDifferent|AtomicCopy).
        ///        For Upload/Download specify either the <filename> or <localTag> and <xStoreTag>.
        /// 
        ///   <param name="args">Command line parameters.</param>       
        ///   <returns>SuccessExitCode if program completes successfully, GenericImageStoreErrorExitCode otherwise.</returns>
        /// </summary>
        public static int Main(string[] args)
        {
            StringBuilder argumentsToTrace = new StringBuilder();
            for (var ix=0; ix<args.Length; ++ix)
            {
                if (args[ix] == "-c")
                {
                    // Skip ConnectionString argument, which may contain an XStore storage key
                    //
                    ++ix;
                    argumentsToTrace.Append("-c <ConnectionString>");
                }
                else
                {
                    argumentsToTrace.Append(args[ix] + " ");
                }
            }

            ImageStoreClient.TraceSource.WriteInfo("CommandLineParsing", "ImageStoreClient invoked with {0}", argumentsToTrace);

            CommandLineArgs commandLineArgs = new CommandLineArgs();
            try
            {

                if (!commandLineArgs.Parse(args))
                {
                    ImageStoreClient.TraceSource.WriteError("CommandLineParsing", "Command line parsing failed. Correct usage is \n {0}", GetUsage());
                    Console.WriteLine(GetUsage());
                    return GenericImageStoreErrorExitCode;
                }
            }
            catch (Exception e)
            {
                ImageStoreClient.TraceSource.WriteError("CommandLineParsing", "Command line parsing failed because {0}", e);
                return GenericImageStoreErrorExitCode;
            }

            if (commandLineArgs.XStoreMinTransferBPS > 0)
            {
                ImageStoreConfig.XStoreMinTransferBPS = commandLineArgs.XStoreMinTransferBPS;
            }

            List<string> storeTags = new List<string>();
            List<string> localTags = new List<string>();
            List<CopyFlag> flags = new List<CopyFlag>();            
            int exitCode = SuccessExitCode;
            Exception exception = null;
            try
            {
                switch (commandLineArgs.Command)
                {
                    case Commands.Delete:
                        if (!string.IsNullOrEmpty(commandLineArgs.FileName))
                        {
                            if (!ReadFile(commandLineArgs.FileName, storeTags, localTags, flags))
                            {
                                return GenericImageStoreErrorExitCode;
                            }
                        }
                        else
                        {
                            storeTags.Add(commandLineArgs.StoreTag);
                        }

                        Delete(commandLineArgs.ConnectionString, storeTags);
                        break;

                    case Commands.Download:
                        if (!string.IsNullOrEmpty(commandLineArgs.FileName))
                        {
                            if (!ReadFile(commandLineArgs.FileName, storeTags, localTags, flags))
                            {
                                return GenericImageStoreErrorExitCode;
                            }
                        }
                        else
                        {
                            storeTags.Add(commandLineArgs.StoreTag);
                            localTags.Add(commandLineArgs.LocalTag);
                            flags.Add(commandLineArgs.Flag);
                        }

                        Download(commandLineArgs.ConnectionString, storeTags, localTags, flags);
                        break;

                    case Commands.Upload:
                        if (!string.IsNullOrEmpty(commandLineArgs.FileName))
                        {
                            if (!ReadFile(commandLineArgs.FileName, storeTags, localTags, flags))
                            {
                                return GenericImageStoreErrorExitCode;
                            }
                        }
                        else
                        {
                            storeTags.Add(commandLineArgs.StoreTag);
                            localTags.Add(commandLineArgs.LocalTag);
                            flags.Add(commandLineArgs.Flag);
                        }

                        Upload(commandLineArgs.ConnectionString, storeTags, localTags, flags);
                        break;

                    case Commands.Exists:
                        if (!string.IsNullOrEmpty(commandLineArgs.FileName))
                        {
                            if (!ReadFile(commandLineArgs.FileName, storeTags, localTags, flags))
                            {
                                return GenericImageStoreErrorExitCode;
                            }
                        }
                        else
                        {
                            storeTags.Add(commandLineArgs.StoreTag);
                        }

                        Exists(commandLineArgs.ConnectionString, storeTags, commandLineArgs.OutputFile);
                        break;
                    default:
                        throw new ArgumentException(StringResources.Error_ArgumentInvalid, "command");
                }
            }
            catch (FileNotFoundException e)
            {
                exception = e;
                exitCode = FileNotFoundExitCode;
            }
            catch (Exception e)
            {
                exception = e;
                exitCode = GenericImageStoreErrorExitCode;
            }

            if (exception != null)
            {
                ImageStoreClient.TraceSource.WriteError(
                    TraceType,
                    "{0} failed because of {1}",
                    commandLineArgs.Command, exception);
            }

            return exitCode;
        }

        /// <summary>
        /// Method to generate the Store.
        /// </summary>
        /// <param name="connectionString">Connection string for the store.</param>
        /// <returns>An instance of the Image Store.</returns>
        private static IImageStore Store(string connectionString)
        {
            return ImageStoreFactoryProxy.CreateImageStore(connectionString);
        }

        /// <summary>
        /// Method to check if the content exists in ImageStore.
        /// </summary>
        /// <param name="connectionString">Connection string for the image store.</param>
        /// <param name="tag">The tags to be check.</param>
        /// <param name="outputFile">The output file where results are written into.</param>
        /// <returns>0 if the function succeeds, else GenericImageStoreErrorExitCode.</returns>
        private static void Exists(string connectionString, List<string> tags, string outputFile)
        {
            Dictionary<string, bool> tagExistsMap = new Dictionary<string, bool>();
            for (int i = 0; i < tags.Count; i++)
            {
                bool exists = Store(connectionString).DoesContentExist(tags[i], TimeSpan.MaxValue);
                tagExistsMap.Add(tags[i], exists);
            }

            if (!string.IsNullOrEmpty(outputFile))
            {
                using (StreamWriter writer = new StreamWriter(File.Open(outputFile, FileMode.Create), Encoding.Unicode))
                {
                    foreach (var tagExists in tagExistsMap)
                    {
                        writer.WriteLine(string.Format(CultureInfo.InvariantCulture, "{0}:{1}", tagExists.Key, tagExists.Value));
                    }
                }
            }
        }

        /// <summary>
        /// Wraps the delete functionality for the image store.
        /// </summary>
        /// <param name="connectionString">Connection string for the image store.</param>
        /// <param name="tag">The tag to be deleted.</param>
        /// <returns>0 if the function succeeds, GenericImageStoreErrorExitCode ow.</returns>
        private static void Delete(string connectionString, List<string> tags)
        {
            for (int i = 0; i < tags.Count; i++)
            {
                Store(connectionString).DeleteContent(tags[i], TimeSpan.MaxValue);
            }            
        }

        /// <summary>
        /// Wrapper to download a list of image store tags to local tags.
        /// </summary>
        /// <param name="connectionString">Connection string for the image store.</param>
        /// <param name="storeTags">The store tags to be downloaded</param>
        /// <param name="localTags">The local tags where the downloads should happen.</param>
        /// <returns>0 if download is successfull, GenericImageStoreErrorExitCode othrerwise</returns>
        private static void Download(string connectionString, List<string> storeTags, List<string> localTags, List<CopyFlag> flags)
        {
            for (int i = 0; i < storeTags.Count; i++)
            {
                Store(connectionString).DownloadContent(storeTags[i], localTags[i], TimeSpan.MaxValue, flags[i]);
            }
        }

        /// <summary>
        /// Wrapper to upload a list of local tags to image store tags.
        /// </summary>
        /// <param name="connectionString">Connection string for the image store.</param>
        /// <param name="storeTags">The store tags where uploads should happen</param>
        /// <param name="localTags">The local tags to upload.</param>
        /// <returns>0 if download is successfull, GenericImageStoreErrorExitCode othrerwise</returns>
        private static void Upload(string connectionString, List<string> storeTags, List<string> localTags, List<CopyFlag> flags)
        {
            for (int i = 0; i < storeTags.Count; i++)
            {
                Store(connectionString).UploadContent(storeTags[i], localTags[i], TimeSpan.MaxValue, flags[i], false);
            }
        }

        /// <summary>
        /// Function to read the file containing the list of tags.
        /// </summary>
        /// <param name="fileName">The name of the file to read.</param>
        /// <param name="storeTags">A reference parameter to return the list of store tags.</param>
        /// <param name="localTags">A reference parameter to return the list of local tags.</param>
        /// <returns>True if the file is in correct format and readable, false otherwise.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.WebSecurity", "CA3003:FileCanonicalizationRule", 
            Justification="Per MSDN documentation, File.ReadAllLines throws an ArgumentException if the path contains invalid characters. The caller (Main() method) has an exception handler for this.")]
        private static bool ReadFile(string fileName, List<string> storeTags, List<string> localTags, List<CopyFlag> flags)
        {
            string[] tagSet = File.ReadAllLines(fileName);
            if (tagSet.Length < 1)
            {
                return false;
            }

            foreach (string tag in tagSet)
            {
                string[] tags = tag.Split('|');
                if (tags.Length != 3)
                {
                    return false;
                }

                storeTags.Add(tags[0]);
                localTags.Add(tags[1]);
                flags.Add((CopyFlag)Enum.Parse(typeof(CopyFlag), tags[2]));
            }

            return true;
        }

        /// <summary>
        /// Prints the usage of this command.
        /// </summary>
        private static string GetUsage()
        {            
            StringBuilder builder = new StringBuilder();
            builder.AppendLine("ImageStoreClient is a program used to access the image store. Its usage is as follows:");
            builder.AppendLine("\n ImageStoreClient <command> -c <connectionString> [-f <fileName>] [-x <xstoreTag>] [-l <localTag>] [-g <flag>]");
            builder.AppendLine("\n\t <command>: Delete (just takes the <xstoreTag>), Download, Upload");
            builder.AppendLine("\t <connectionString>: xstore image store connection string.");
            builder.AppendLine("\t <fileName>: file that contains list of files to be uploaded/downloaded");
            builder.AppendLine("\t <xstoreTag>: tag on xstore of the file to be uploaded/downloaded.");
            builder.AppendLine("\t <localTag>: tag of the local file to be uploaded/downloaded.");
            builder.AppendLine("\t <flag>: CopyIfDifferent, AtomicCopy.");
            builder.AppendLine("\nFor Upload/Download specify either the <filename> or <localTag> and <xStoreTag>.");
            return builder.ToString();
        }

        /// <summary>
        /// This class provides a minimal command line parser.
        /// </summary>
        private class CommandLineArgs
        {
            /// <summary>
            /// The command passed in the command line.
            /// </summary>
            private Commands command;

            /// <summary>
            /// The connection string for the image store.
            /// </summary>
            private string connectionString;

            /// <summary>
            /// The file name containing the details list of local and remote tags to 
            /// be operated upon.
            /// </summary>
            private string fileName;

            /// <summary>
            /// The Image Store Tag.
            /// </summary>
            private string storeTag;

            /// <summary>
            /// The corresponding local tag.
            /// </summary>
            private string localTag;

            /// <summary>
            /// The copy flag for the store tag.
            /// </summary>
            private CopyFlag copyFlag;

            /// <summary>
            /// The output file.
            /// </summary>
            private string outputFile;

            /// <summary>
            /// Initializes a new instance of the CommandLineArgs class.
            /// </summary>
            internal CommandLineArgs()
            {
                this.connectionString = null;
                this.fileName = null;
                this.storeTag = null;
                this.localTag = null;
                this.XStoreMinTransferBPS = 0;
                this.copyFlag = CopyFlag.AtomicCopy;
                this.outputFile = null;
            }

            /// <summary>
            /// Gets the command passed in the command line.
            /// </summary>
            internal Commands Command
            {
                get
                {
                    return this.command;
                }
            }

            /// <summary>
            /// Gets the connection string for the image store.
            /// </summary>
            internal string ConnectionString
            {
                get
                {
                    return this.connectionString;
                }
            }

            /// <summary>
            /// Gets the file name containing the details list of local and remote tags to 
            /// be operated upon.
            /// </summary>
            internal string FileName
            {
                get
                {
                    return this.fileName;
                }
            }

            /// <summary>
            /// Gets the Image Store Tag.
            /// </summary>
            internal string StoreTag
            {
                get
                {
                    return this.storeTag;
                }
            }

            /// <summary>
            /// Gets the corresponding local tag.
            /// </summary>
            internal string LocalTag
            {
                get
                {
                    return this.localTag;
                }
            }

            /// <summary>
            /// Gets the corresponding local tag.
            /// </summary>
            internal CopyFlag Flag
            {
                get
                {
                    return this.copyFlag;
                }
            }

            internal string OutputFile
            {
                get
                {
                    return this.outputFile;
                }
            }

            /// <summary>
            /// Gets the minimum transfer rate in bits per second specified in the command line.
            /// </summary>
            internal int XStoreMinTransferBPS { get; private set; }

            /// <summary>
            /// Parses the command line arguments, applies the constraints and generates a structured accessor to the 
            /// command line arguments.
            /// </summary>
            /// <param name="args">The command line arguments.</param>
            /// <returns>True if the command line arguments were proper, false otherwise.</returns>
            internal bool Parse(string[] args)
            {
                if (args.Length < 1 || args[0] == "/?")
                {
                    return false;
                }

                int i = 1;
                while (i != args.Length)
                {
                    switch (args[i])
                    {
                        case "-c":
                            this.connectionString = args[++i];
                            break;
                        case "-f":
                            this.fileName = args[++i];
                            break;
                        case "-x":
                            this.storeTag = args[++i];
                            break;
                        case "-l":
                            this.localTag = args[++i];
                            break;
                        case "-g":
                            this.copyFlag = (CopyFlag)Enum.Parse(typeof(CopyFlag), args[++i]);
                            break;
                        case "-b":
                            this.XStoreMinTransferBPS = int.Parse(args[++i], CultureInfo.InvariantCulture);
                            break;
                        case "-o":
                            this.outputFile = args[++i];
                            break;
                        default:
                            return false;
                    }

                    i++;
                }

                this.command = (Commands)Enum.Parse(typeof(Commands), args[0]);
                if (string.IsNullOrEmpty(this.connectionString))
                {
                    bool isEncrypted;
                    NativeConfigStore configStore = NativeConfigStore.FabricGetConfigStore();
                    var configValue = configStore.ReadString("Management", "ImageStoreConnectionString", out isEncrypted);
                    if (isEncrypted)
                    {
                        var secureString = NativeConfigStore.DecryptText(configValue);
                        var secureCharArray = FabricValidatorUtility.SecureStringToCharArray(secureString);
                        this.connectionString = new string(secureCharArray);
                    }
                    else
                    {
                        this.connectionString = configValue;
                    }
                }

                if (this.command == Commands.Delete)
                {
                    if (string.IsNullOrEmpty(this.storeTag) && string.IsNullOrEmpty(this.fileName))
                    {
                        return false;
                    }
                }
                else if (this.command == Commands.Exists)
                {
                    if (string.IsNullOrEmpty(this.outputFile))
                    {
                        return false;
                    }
                }
                else
                {
                    if (string.IsNullOrEmpty(this.fileName))
                    {
                        if (string.IsNullOrEmpty(this.localTag) || string.IsNullOrEmpty(this.storeTag))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!string.IsNullOrEmpty(this.localTag) || !string.IsNullOrEmpty(this.storeTag))
                        {
                            return false;
                        }
                    }
                }

                return true;
            }
        }
    }
}