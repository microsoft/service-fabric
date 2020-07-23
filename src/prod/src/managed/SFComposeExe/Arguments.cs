// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace SFCompose
{
    using System;
    using System.IO;
    using CommandLineParser;

    internal class Arguments
    {
        public enum Type
        {
            Generate,
            Validate,
            SingleInstance
        }

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description =
                "This parameter controls the tool's operating mode. \n" +
                "Generate - generates the Service Fabric App and Service manifests from the docker-compose yaml file.\n" +
                "Validate - validates if the yaml file can be converted to valid App and service manifests without actually generating them. This is the default option.",
            LongName = "Operation",
            ShortName = "Op")] public Type Operation;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description =
                "This optional parameter gives the file path to the docker compose file. If this is omitted, then the tool looks for a file \n" +
                " named docker-compose.yaml in the same directory as the tool.",
            LongName = "ComposeFilePath",
            ShortName = "F")] public string DockerComposeFilePath;

        public string OverridesFilePath;

        public string ApplicationDescriptionPath = null;

        public bool VolumeDescriptionRequired;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description =
                "This optional parameter gives the path for the generated app package. If this is omitted, then the tool generates the app package\n" +
                "in a folder named 'pkg' in the same directory as the tool.",
            LongName = "Output",
            ShortName = "Out")] public string GeneratedApplicationPackagePath;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description =
                "This optional parameter gives the user name to connect to the private repository. This parameter needs to be specified if the container images specified" +
                "are stored in a private repository",
            LongName = "RepositoryUserName",
            ShortName = "U")] public string RepositoryUserName;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description =
                "This optional parameter gives the password for the user name to connect to the private repository. This parameter needs to be specified if the container images specified" +
                "are stored in a private repository",
            LongName = "RepositoryPassword",
            ShortName = "Pwd")] public string RepositoryPassword;

        [CommandLineArgument(
            CommandLineArgumentType.AtMostOnce,
            Description =
                "This optional parameter indicates if the password specified is encrypted. The certificate used to encrypt the password should be installed on the cluster.",
            LongName = "IsPasswordEncrypted",
            ShortName = "PwdEncrypted")] public bool IsPasswordEncrypted;

        public Arguments()
        {
            this.Operation = Type.Validate;
            this.DockerComposeFilePath = @".\Docker-compose.yml";
            this.OverridesFilePath = "";
            this.GeneratedApplicationPackagePath = @".\pkg";
            this.RepositoryUserName = "";
            this.RepositoryPassword = "";
            this.IsPasswordEncrypted = false;
            this.VolumeDescriptionRequired = false;
        }

        public bool IsValid()
        {
            if ((this.Operation== Type.Generate
                || this.Operation == Type.Validate)
                && !File.Exists(this.DockerComposeFilePath))
            {
                Console.WriteLine("Input file '{0}' doesnot exist", this.DockerComposeFilePath);
                return false;
            }

            if (this.RepositoryUserName.Length != 0 && this.RepositoryPassword.Length == 0)
            {
                Console.WriteLine("Repository user name specified without password.");
                return false;
            }

            if (this.RepositoryPassword.Length != 0 && this.RepositoryUserName.Length == 0)
            {
                Console.WriteLine("Repository password specified without username.");
                return false;
            }

            return true;
        }
    }
}