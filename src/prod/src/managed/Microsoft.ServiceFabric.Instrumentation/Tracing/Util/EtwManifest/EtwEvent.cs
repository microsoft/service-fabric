// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System.Collections.Generic;
    using System.Text;
    using System.Xml;

    public sealed class EtwEvent
    {
        private EtwProvider etwProvider;
        private int lineNum;
        private string opcodeId;
        private string keywordsId;
        private string taskId;

        public EtwProvider Provider
        {
            get { return this.etwProvider; }
        }

        public ushort Id { get; private set; }

        public ulong Keywords { get; private set; }

        public string KeywordsString
        {
            get { return this.Provider.GetKeywordSetString(this.Keywords); }
        }

        public byte Version { get; private set; }

        public byte Level { get; private set; }

        public byte Opcode { get; private set; }

        public ushort Task { get; private set; }

        public string Channel { get; private set; }

        /// <summary>
        /// The fields associated with this event.  This can be null if there are no fields.
        /// </summary>
        public IList<Field> Fields { get; private set; }

        public string TaskName
        {
            get { return this.etwProvider.GetTaskName(this.Task); }
        }

        public string OpcodeName
        {
            get { return this.etwProvider.GetOpcodeName(this.Task, this.Opcode); }
        }

        public string EventName
        {
            get
            {
                return this.Symbol.Substring(this.Symbol.LastIndexOf('_') + 1);
            }
        }

        public string Message { get; internal set; }

        public string Symbol { get; internal set; }

        public string TemplateName { get; internal set; }

        /// <summary>
        /// Get the line number in the file.  Used for error messages.
        /// </summary>
        public int LineNum { get; private set; }

        public override string ToString()
        {
            var basemessage = string.Format("Id: {0}, Message: {1}, Name: {2}, Symbol: {3}", this.Id, this.Message, this.EventName, this.Symbol);
            var fieldMessage = new StringBuilder();
            foreach (var oneFiled in this.Fields)
            {
                fieldMessage.AppendLine(oneFiled.ToString());
            }

            return string.Format("{0}. \n Fields: {1}", basemessage, fieldMessage);
        }

        #region private

        internal static string MakeUpperCase(string str)
        {
            if (0 < str.Length)
                str = str.Substring(0, 1).ToUpper() + str.Substring(1);
            return str;
        }

        internal static string MakeCamelCase(string str)
        {
            if (str.IndexOf(' ') < 0)
                return str;

            var parts = str.Split(' ');
            for (int i = 1; i < parts.Length; i++)
                parts[i] = MakeUpperCase(parts[i]);
            return string.Join("", parts);
        }

        private static byte ParseLevel(string levelStr)
        {
            switch (levelStr)
            {
                case "win:Always":
                    return 0;

                case "win:Critical":
                    return 1;

                case "win:Error":
                    return 2;

                case "win:Warning":
                    return 3;

                case "win:Informational":
                    return 4;

                case "win:Verbose":
                    return 5;

                default:
                    int ret;
                    if (int.TryParse(levelStr, out ret) && (byte)ret == ret)
                        return (byte)ret;
                    return 4; // Informational
            }
        }

        internal EtwEvent(XmlReader reader, EtwProvider provider, int lineNum)
        {
            this.lineNum = lineNum;
            this.etwProvider = provider;

            this.LineNum = ((IXmlLineInfo)reader).LineNumber;

            for (bool doMore = reader.MoveToFirstAttribute(); doMore; doMore = reader.MoveToNextAttribute())
            {
                switch (reader.Name)
                {
                    case "symbol":
                        this.Symbol = reader.Value;
                        break;

                    case "version":
                        this.Version = (byte)EtwProvider.ParseNumber(reader.Value);
                        break;

                    case "value":
                        this.Id = (ushort)EtwProvider.ParseNumber(reader.Value);
                        break;

                    case "task":
                        this.taskId = reader.Value;
                        break;

                    case "opcode":
                        this.opcodeId = reader.Value;
                        break;

                    case "keyword":
                        this.keywordsId = reader.Value;
                        break;

                    case "level":
                        this.Level = ParseLevel(reader.Value);
                        break;

                    case "template":
                        this.TemplateName = reader.Value;
                        break;

                    case "message":
                        this.Message = reader.Value;
                        break;

                    case "channel":
                        this.Channel = reader.Value;
                        break;

                    default:
                        break;
                }
            }
        }

        /// <summary>
        /// We need a two pass system where after all the defintions are parsed, we go back and link
        /// up uses to their defs.   This routine does this for Events.
        /// </summary>
        internal void ResolveIdsInEvent()
        {
            var id = string.Empty;
            id = this.taskId;
            this.taskId = null;
            if (id != null)
                this.Task = (ushort)this.etwProvider.TaskValues[id];

            id = this.opcodeId;
            this.opcodeId = null;
            if (id != null)
                this.Opcode = (byte)this.etwProvider.OpCodeValues[id];

            id = this.keywordsId;
            this.keywordsId = null;
            if (id != null)
                this.Keywords = this.etwProvider.KeyworkValues[id];

            id = this.TemplateName;
            if (id != null)
            {
                List<Field> fields;
                if (!this.etwProvider.TemplateValues.TryGetValue(id, out fields))
                    this.etwProvider.TemplateValues[id] = fields = new List<Field>();
                this.Fields = fields;
                foreach (var field in this.Fields)
                {
                    if (field.MapId != null)
                        field.Enumeration = this.etwProvider.Enums[field.MapId];
                }
            }
        }

        internal void UpdateStrings(Dictionary<string, string> stringMap)
        {
            var message = this.Message;
            if (EtwProvider.Replace(ref message, stringMap))
                this.Message = message;

            if (this.Fields != null)
            {
                foreach (var parameter in this.Fields)
                {
                    if (parameter.Enumeration != null)
                        parameter.Enumeration.UpdateStrings(stringMap);
                }
            }
        }

        #endregion private
    }
}