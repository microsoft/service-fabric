// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace CommandLineParser
{
    using System;

    /// <summary>
    /// Allows control of command line parsing.
    /// Attach this attribute to instance fields of types used
    /// as the destination of command line argument parsing.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field)]
    public class CommandLineArgumentAttribute : Attribute
    {
        private readonly CommandLineArgumentType type;

        private string shortName;
        private string longName;

        /// <summary>
        /// Initializes a new instance of the <see cref="CommandLineArgumentAttribute"/> class.
        /// </summary>
        /// <param name="type"> Specifies the error checking to be done on the argument. </param>
        public CommandLineArgumentAttribute(CommandLineArgumentType type)
        {
            this.type = type;
            this.Description = null;
        }

        /// <summary>
        /// Gets the type of the command line argument.
        /// </summary>
        public CommandLineArgumentType Type
        {
            get { return this.type; }
        }

        /// <summary>
        /// Gets a value indicating whether an explicit short name was specified or not.
        /// </summary>
        public bool DefaultShortName
        {
            get { return this.shortName == null; }
        }

        /// <summary>
        /// Gets or sets the short name of the argument.
        /// </summary>
        public string ShortName
        {
            get { return this.shortName; }

            set { this.shortName = value; }
        }

        /// <summary>
        /// Gets a value indicating whether an explicit long name was specified for this argument or not.
        /// </summary>
        public bool DefaultLongName
        {
            get { return this.longName == null; }
        }

        /// <summary>
        /// Gets or sets the long name of the argument.
        /// </summary>
        public string LongName
        {
            get { return this.longName; }

            set { this.longName = value; }
        }

        /// <summary>
        /// Gets or sets the description of the command line argument
        /// </summary>
        public string Description { get; set; }
    }
}
