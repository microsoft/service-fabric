// ------------------------------------------------------------
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT License (MIT).See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace CommandLineParser
{
    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Text;

    /// <summary>
    /// Parser for command line arguments.
    /// <para>
    /// The parser specification is inferred from the instance fields of the object
    /// specified as the destination of the parse.
    /// Valid argument types are: <see cref="int"/>, <see cref="uint"/>, <see cref="string"/>, <see cref="bool"/>, enumerations
    /// Also argument types of Array of the above types are also valid.
    /// </para>
    /// <para>
    /// Error checking options can be controlled by adding a CommandLineArgumentAttribute
    /// to the instance fields of the destination object.
    /// </para>
    /// At most one field may be marked with the DefaultCommandLineArgumentAttribute
    /// indicating that arguments without a '-' or '/' prefix will be parsed as that argument.
    /// <para>
    /// If not specified then the parser will infer default options for parsing each
    /// instance field. The default long name of the argument is the field name. The
    /// default short name is the first character of the long name. Long names and explicitly
    /// specified short names must be unique. Default short names will be used provided that
    /// the default short name does not conflict with a long name or an explicitly
    /// specified short name.
    /// </para>
    /// <para>
    /// Arguments which are array types are collection arguments. Collection arguments can be specified multiple times.
    /// </para>
    /// </summary>
    internal class CommandLineArgumentParser
    {
        private readonly ArrayList arguments;
        private readonly Hashtable argumentMap;
        private readonly Argument defaultArgument;
        private readonly ErrorReporter reporter;

        /// <summary>
        ///  Initializes a new instance of the <see cref="CommandLineArgumentParser"/> class.
        /// </summary>
        /// <param name="argumentSpecification"> The type of object to  parse. </param>
        /// <param name="reporter"> The destination for parse errors. </param>
        public CommandLineArgumentParser(Type argumentSpecification, ErrorReporter reporter)
        {
            this.reporter = reporter;
            this.arguments = new ArrayList();
            this.argumentMap = new Hashtable();

            foreach (var field in argumentSpecification.GetFields())
            {
                if (!field.IsStatic && !field.IsInitOnly && !field.IsLiteral)
                {
                    var attribute = GetAttribute(field);
                    if (attribute is DefaultCommandLineArgumentAttribute)
                    {
                        Debug.Assert(this.defaultArgument == null, "The default argument must not be null.");
                        this.defaultArgument = new Argument(attribute, field, reporter);
                    }
                    else
                    {
                        this.arguments.Add(new Argument(attribute, field, reporter));
                    }
                }
            }

            // add explicit names to map
            foreach (Argument argument in this.arguments)
            {
                Debug.Assert(
                    !this.argumentMap.ContainsKey(argument.LongName),
                    "The argument must exist in the argument map.");

                this.argumentMap[argument.LongName] = argument;

                if (argument.ExplicitShortName && !string.IsNullOrEmpty(argument.ShortName))
                {
                    Debug.Assert(
                        !this.argumentMap.ContainsKey(argument.ShortName),
                        "The argument must exists for the short name");

                    this.argumentMap[argument.ShortName] = argument;
                }
            }

            // add implicit names which don't collide to map
            foreach (Argument argument in this.arguments)
            {
                if (!argument.ExplicitShortName && !string.IsNullOrEmpty(argument.ShortName))
                {
                    if (!this.argumentMap.ContainsKey(argument.ShortName))
                    {
                        this.argumentMap[argument.ShortName] = argument;
                    }
                }
            }
        }

        /// <summary>
        /// Gets a user friendly usage string describing the command line argument syntax.
        /// </summary>
        public string Usage
        {
            get
            {
                var builder = new StringBuilder();

                foreach (Argument arg in this.arguments)
                {
                    var oldLength = builder.Length;

                    builder.Append("    /");
                    builder.Append(arg.LongName);
                    var valueType = arg.ValueType;
                    if (valueType == typeof(int))
                    {
                        builder.Append(":<int>");
                    }
                    else if (valueType == typeof(uint))
                    {
                        builder.Append(":<uint>");
                    }
                    else if (valueType == typeof(bool))
                    {
                        builder.Append("[+|-]");
                    }
                    else if (valueType == typeof(string))
                    {
                        builder.Append(":<string>");
                    }
                    else
                    {
                        Debug.Assert(valueType.IsEnum, "The value type must be an enumeration.");

                        builder.Append(":{");
                        var first = true;
                        foreach (var field in valueType.GetFields())
                        {
                            if (field.IsStatic)
                            {
                                if (first)
                                {
                                    first = false;
                                }
                                else
                                {
                                    builder.Append('|');
                                }

                                builder.Append(field.Name);
                            }
                        }

                        builder.Append('}');
                    }

                    if (arg.ShortName != arg.LongName && this.argumentMap[arg.ShortName] == arg)
                    {
                        builder.Append(' ', IndentLength(builder.Length - oldLength));
                        builder.Append("short form /");
                        builder.Append(arg.ShortName);
                    }

                    builder.Append(CommandLineUtility.NewLine);

                    if (arg.Description != null)
                    {
                        builder.Append("        " + arg.Description);
                        builder.Append(CommandLineUtility.NewLine);
                        builder.Append(CommandLineUtility.NewLine);
                    }
                }

                builder.Append(CommandLineUtility.NewLine);

                if (this.defaultArgument != null)
                {
                    builder.Append("    <");
                    builder.Append(this.defaultArgument.LongName);
                    builder.Append(">");
                    builder.Append(CommandLineUtility.NewLine);
                }

                return builder.ToString();
            }
        }

        /// <summary>
        /// Parses an argument list.
        /// </summary>
        /// <param name="args"> The arguments to parse. </param>
        /// <param name="destination"> The destination of the parsed arguments. </param>
        /// <returns> true if no parse errors were encountered. </returns>
        public bool Parse(string[] args, object destination)
        {
            var hadError = this.ParseArgumentList(args, destination);

            // check for missing required arguments
            foreach (Argument arg in this.arguments)
            {
                hadError |= arg.Finish(destination);
            }

            if (this.defaultArgument != null)
            {
                hadError |= this.defaultArgument.Finish(destination);
            }

            return !hadError;
        }

        private static CommandLineArgumentAttribute GetAttribute(FieldInfo field)
        {
            var attributes = field.GetCustomAttributes(typeof(CommandLineArgumentAttribute), false);
            if (attributes.Length == 1)
            {
                return (CommandLineArgumentAttribute)attributes[0];
            }

            Debug.Assert(
                attributes.Length == 0,
                "The field must have CommandrgumentAttribute.");

            return null;
        }

        private static int IndentLength(int lineLength)
        {
            return Math.Max(4, 40 - lineLength);
        }

        private static string LongName(CommandLineArgumentAttribute attribute, FieldInfo field)
        {
            return (attribute == null || attribute.DefaultLongName) ? field.Name : attribute.LongName;
        }

        private static string Description(CommandLineArgumentAttribute attribute)
        {
            if (attribute != null)
            {
                return attribute.Description;
            }
            else
            {
                return null;
            }
        }

        private static string ShortName(CommandLineArgumentAttribute attribute, FieldInfo field)
        {
            return !ExplicitShortName(attribute) ? LongName(attribute, field).Substring(0, 1) : attribute.ShortName;
        }

        private static bool ExplicitShortName(CommandLineArgumentAttribute attribute)
        {
            return attribute != null && !attribute.DefaultShortName;
        }

        private static Type ElementType(FieldInfo field)
        {
            if (IsCollectionType(field.FieldType))
            {
                return field.FieldType.GetElementType();
            }
            else
            {
                return null;
            }
        }

        private static CommandLineArgumentType Flags(CommandLineArgumentAttribute attribute, FieldInfo field)
        {
            if (attribute != null)
            {
                return attribute.Type;
            }
            else if (IsCollectionType(field.FieldType))
            {
                return CommandLineArgumentType.MultipleUnique;
            }
            else
            {
                return CommandLineArgumentType.AtMostOnce;
            }
        }

        private static bool IsCollectionType(Type type)
        {
            return type.IsArray;
        }

        private static bool IsValidElementType(Type type)
        {
            return type != null && (
                       type == typeof(int) ||
                       type == typeof(uint) ||
                       type == typeof(string) ||
                       type == typeof(bool) ||
                       type.IsEnum);
        }

        private bool LexFileArguments(string fileName, out string[] argumentsStrings)
        {
            string args;

            try
            {
                using (var file = new FileStream(fileName, FileMode.Open, FileAccess.Read))
                {
                    args = (new StreamReader(file)).ReadToEnd();
                }
            }
            catch (Exception e)
            {
                this.reporter(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        SR.Error_CannotOpenCommandLineArgumentFile,
                        fileName,
                        e.Message));
                argumentsStrings = null;
                return false;
            }

            var hadError = false;
            var argArray = new ArrayList();
            var currentArg = new StringBuilder();
            var inQuotes = false;
            var index = 0;

            // while (index < args.Length)
            try
            {
                while (true)
                {
                    // skip whitespace
                    while (char.IsWhiteSpace(args[index]))
                    {
                        index += 1;
                    }

                    // # - comment to end of line
                    if (args[index] == '#')
                    {
                        index += 1;
                        while (args[index] != '\n')
                        {
                            index += 1;
                        }

                        continue;
                    }

                    // do one argument
                    do
                    {
                        if (args[index] == '\\')
                        {
                            var slashes = 1;
                            index += 1;
                            while (index == args.Length && args[index] == '\\')
                            {
                                slashes += 1;
                            }

                            if (index == args.Length || args[index] != '"')
                            {
                                currentArg.Append('\\', slashes);
                            }
                            else
                            {
                                currentArg.Append('\\', slashes >> 1);
                                if ((slashes & 1) != 0)
                                {
                                    currentArg.Append('"');
                                }
                                else
                                {
                                    inQuotes = !inQuotes;
                                }
                            }
                        }
                        else if (args[index] == '"')
                        {
                            inQuotes = !inQuotes;
                            index += 1;
                        }
                        else
                        {
                            currentArg.Append(args[index]);
                            index += 1;
                        }
                    }
                    while (!char.IsWhiteSpace(args[index]) || inQuotes);

                    argArray.Add(currentArg.ToString());
                    currentArg.Length = 0;
                }
            }
            catch (IndexOutOfRangeException)
            {
                // got EOF
                if (inQuotes)
                {
                    this.reporter(
                        string.Format(
                            SR.Error_UnbalancedSlashes,
                            fileName));
                    hadError = true;
                }
                else if (currentArg.Length > 0)
                {
                    // valid argument can be terminated by EOF
                    argArray.Add(currentArg.ToString());
                }
            }

            argumentsStrings = (string[])argArray.ToArray(typeof(string));
            return hadError;
        }

        private void ReportUnrecognizedArgument(string argument)
        {
            this.reporter(
                string.Format(
                    CultureInfo.InvariantCulture,
                    SR.Error_UnrecognizedCommandLineArgument,
                    argument));
        }

        private bool ParseArgumentList(string[] args, object destination)
        {
            var hadError = false;
            if (args != null)
            {
                foreach (var argument in args)
                {
                    if (argument.Length > 0)
                    {
                        switch (argument[0])
                        {
                            case '-':
                            case '/':
                                {
                                    var endIndex = argument.IndexOfAny(
                                        new[]
                                        {
                                        ':',
                                        '+',
                                        '-',
                                        },
                                        1);
                                    var option = argument.Substring(
                                        1,
                                        endIndex == -1 ? argument.Length - 1 : endIndex - 1);
                                    string optionArgument;
                                    if (endIndex == -1)
                                    {
                                        optionArgument = null;
                                    }
                                    else if (argument.Length > 1 + option.Length && argument[1 + option.Length] == ':')
                                    {
                                        optionArgument = argument.Substring(option.Length + 2);
                                    }
                                    else
                                    {
                                        optionArgument = argument.Substring(option.Length + 1);
                                    }

                                    var arg = (Argument)this.argumentMap[option];
                                    if (arg == null)
                                    {
                                        this.ReportUnrecognizedArgument(argument);
                                        hadError = true;
                                    }
                                    else
                                    {
                                        hadError |= !arg.SetValue(optionArgument, destination);
                                    }

                                    break;
                                }

                            case '@':
                                {
                                    hadError |= this.LexFileArguments(argument.Substring(1), out var nestedArguments);
                                    hadError |= this.ParseArgumentList(nestedArguments, destination);
                                    break;
                                }

                            default:
                                {
                                    if (this.defaultArgument != null)
                                    {
                                        hadError |= !this.defaultArgument.SetValue(argument, destination);
                                    }
                                    else
                                    {
                                        this.ReportUnrecognizedArgument(argument);
                                        hadError = true;
                                    }

                                    break;
                                }
                        }
                    }
                }
            }

            return hadError;
        }

        private class Argument
        {
            private readonly string longName;
            private readonly string shortName;
            private readonly string description;
            private readonly bool explicitShortName;
            private readonly FieldInfo field;
            private readonly Type elementType;
            private readonly CommandLineArgumentType flags;
            private readonly ArrayList collectionValues;
            private readonly ErrorReporter reporter;
            private readonly bool isDefault;
            private bool seenValue;

            public Argument(CommandLineArgumentAttribute attribute, FieldInfo field, ErrorReporter reporter)
            {
                this.longName = CommandLineArgumentParser.LongName(attribute, field);
                this.description = CommandLineArgumentParser.Description(attribute);
                this.explicitShortName = CommandLineArgumentParser.ExplicitShortName(attribute);
                this.shortName = CommandLineArgumentParser.ShortName(attribute, field);
                this.elementType = ElementType(field);
                this.flags = Flags(attribute, field);
                this.field = field;
                this.seenValue = false;
                this.reporter = reporter;
                this.isDefault = (attribute != null) && (attribute is DefaultCommandLineArgumentAttribute);

                if (this.IsCollection)
                {
                    this.collectionValues = new ArrayList();
                }

                Debug.Assert(
                    !string.IsNullOrEmpty(this.longName),
                    "The long name must be provided for the argument.");

                Debug.Assert(
                    !this.IsCollection || this.AllowMultiple,
                    "Collection arguments must have allow multiple");

                Debug.Assert(
                    !this.Unique || this.IsCollection,
                    "Unique only applicable to collection arguments");

                Debug.Assert(
                    IsValidElementType(this.Type) || IsCollectionType(this.Type),
                    "The argument type is not valid.");

                Debug.Assert(
                    (this.IsCollection && IsValidElementType(this.elementType)) || (!this.IsCollection && this.elementType == null),
                    "The argument type is not valid.");
            }

            internal Type ValueType
            {
                get { return this.IsCollection ? this.elementType : this.Type; }
            }

            internal string LongName
            {
                get { return this.longName; }
            }

            internal string Description
            {
                get { return this.description; }
            }

            internal bool ExplicitShortName
            {
                get { return this.explicitShortName; }
            }

            internal string ShortName
            {
                get { return this.shortName; }
            }

            internal bool IsRequired
            {
                get
                {
                    return (this.flags & CommandLineArgumentType.Required) != 0;
                }
            }

            internal bool SeenValue
            {
                get { return this.seenValue; }
            }

            internal bool AllowMultiple
            {
                get
                {
                    return (this.flags & CommandLineArgumentType.Multiple) != 0;
                }
            }

            internal bool Unique
            {
                get
                {
                    return (this.flags & CommandLineArgumentType.Unique) != 0;
                }
            }

            internal Type Type
            {
                get { return this.field.FieldType; }
            }

            internal bool IsCollection
            {
                get { return IsCollectionType(this.Type); }
            }

            internal bool IsDefault
            {
                get { return this.isDefault; }
            }

            public bool Finish(object destination)
            {
                if (this.IsCollection)
                {
                    this.field.SetValue(destination, this.collectionValues.ToArray(this.elementType));
                }

                return this.ReportMissingRequiredArgument();
            }

            internal bool SetValue(string value, object destination)
            {
                if (this.SeenValue && !this.AllowMultiple)
                {
                    this.reporter(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            SR.Error_DuplicateCommandLineArgument,
                            this.LongName));

                    return false;
                }

                this.seenValue = true;

                if (!this.ParseValue(this.ValueType, value, out var newValue))
                {
                    return false;
                }

                if (this.IsCollection)
                {
                    if (this.Unique && this.collectionValues.Contains(newValue))
                    {
                        this.ReportDuplicateArgumentValue(value);
                        return false;
                    }
                    else
                    {
                        this.collectionValues.Add(newValue);
                    }
                }
                else
                {
                    this.field.SetValue(destination, newValue);
                }

                return true;
            }

            private bool ParseValue(Type type, string stringData, out object value)
            {
                // null is only valid for bool variables
                // empty string is never valid
                if ((stringData != null || type == typeof(bool)) && (stringData == null || stringData.Length > 0))
                {
                    try
                    {
                        if (type == typeof(string))
                        {
                            value = stringData;
                            return true;
                        }
                        else if (type == typeof(bool))
                        {
                            if (stringData == null || stringData == "+")
                            {
                                value = true;
                                return true;
                            }
                            else if (stringData == "-")
                            {
                                value = false;
                                return true;
                            }
                        }
                        else if (type == typeof(int))
                        {
                            if (stringData != null)
                            {
                                value = int.Parse(stringData);
                                return true;
                            }

                            value = 0;
                            return false;
                        }
                        else if (type == typeof(uint))
                        {
                            if (stringData != null)
                            {
                                value = int.Parse(stringData);
                                return true;
                            }

                            value = 0;
                            return false;
                        }
                        else
                        {
                            Debug.Assert(type.IsEnum, "The type must be an enumeration.");
                            if (stringData != null)
                            {
                                value = Enum.Parse(type, stringData, true);
                                return true;
                            }

                            value = 0;
                            return false;
                        }
                    }
                    catch
                    {
                        // catch parse errors
                    }
                }

                this.ReportBadArgumentValue(stringData);
                value = null;
                return false;
            }

            private void ReportBadArgumentValue(string value)
            {
                this.reporter(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        SR.Error_BadArgumentValue,
                        value,
                        this.LongName));
            }

            private bool ReportMissingRequiredArgument()
            {
                if (this.IsRequired && !this.SeenValue)
                {
                    if (this.IsDefault)
                    {
                        this.reporter(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                SR.Error_MissingRequiredDefaultArgument,
                                this.LongName));
                    }
                    else
                    {
                        this.reporter(
                            string.Format(
                                CultureInfo.InvariantCulture,
                                SR.Error_MissingRequiredDefaultArgument,
                                this.LongName));
                    }

                    return true;
                }

                return false;
            }

            private void ReportDuplicateArgumentValue(string value)
            {
                this.reporter(
                    string.Format(
                        SR.Error_DuplicateArgumentValue,
                        this.LongName,
                        value));
            }
        }
    }
}
