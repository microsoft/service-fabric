// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace CommandLineParser
{
    using System;

    /// <summary>
    /// Indicates that this argument is the default argument.
    /// '/' or '-' prefix only the argument value is specified.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field)]
    public class DefaultCommandLineArgumentAttribute : CommandLineArgumentAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DefaultCommandLineArgumentAttribute"/> class.
        /// </summary>
        /// <param name="type"> Specifies the error checking to be done on the argument. </param>
        public DefaultCommandLineArgumentAttribute(CommandLineArgumentType type)
            : base(type)
        {
        }
    }
}
