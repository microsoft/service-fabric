// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text.RegularExpressions;
    using System.Xml;

    // Portions of code borrowed from Parse Gen Tool.
    public sealed class EtwProvider
    {
        // opcodes live inside a particular task, or you can have 'global' scope for the
        // opcode.   'GlobalScope' is a 'task' for this global scope.  Just needs to be an illegal task number.
        private const int GlobalScope = 0x10000;
        private List<EtwEvent> m_events = new List<EtwEvent>();
        private string[] keywordNames;
        private Dictionary<int, string> taskNames;
        private Dictionary<int, string> opcodeNames;
        private Dictionary<string, int> taskValues;
        private Dictionary<string, int> opcodeValues;
        private Dictionary<string, ulong> keywordValues;
        private Dictionary<string, List<Field>> templateValues;
        private Dictionary<string, Enumeration> enums;

        public Dictionary<string, int> OpCodeValues
        {
            get { return this.opcodeValues; }
        }

        public Dictionary<string, ulong> KeyworkValues
        {
            get { return this.keywordValues; }
        }

        public Dictionary<string, Enumeration> Enums
        {
            get { return this.enums; }
        }

        public Dictionary<string, List<Field>> TemplateValues
        {
            get { return this.templateValues; }
        }

        public Dictionary<string, int> TaskValues
        {
            get { return this.taskValues; }
        }

        public Dictionary<int, string> OpCodeNames
        {
            get { return this.opcodeNames; }
        }

        public Dictionary<int, string> TaskNames
        {
            get { return this.taskNames; }
        }

        public static List<EtwProvider> LoadProviders(string fullPathToManifestFile)
        {
            XmlReaderSettings settings = new XmlReaderSettings()
            {
                DtdProcessing = DtdProcessing.Prohibit,
                IgnoreComments = true,
                IgnoreWhitespace = true,
                XmlResolver = null
            };

            var reader = XmlReader.Create(fullPathToManifestFile, settings);

            var providers = new List<EtwProvider>();
            if (reader.ReadToDescendant("events"))
            {
                using (var events = reader.ReadSubtree())
                {
                    if (events.ReadToDescendant("provider"))
                    {
                        do
                        {
                            providers.Add(new EtwProvider(reader));
                        } while (events.ReadToNextSibling("provider"));
                    }
                }
            }

            // Resolve all the localization strings that may have been used.
            var stringMap = new Dictionary<string, string>();
            while (reader.Read())
            {
                if (reader.NodeType == XmlNodeType.Element && reader.Name == "string")
                {
                    var id = reader.GetAttribute("id");
                    var value = reader.GetAttribute("value");
                    stringMap[id] = value;
                }
            }
            if (0 < stringMap.Count)
            {
                foreach (var provider in providers)
                {
                    if (provider.keywordNames != null)
                    {
                        for (int i = 0; i < provider.keywordNames.Length; i++)
                            Replace(ref provider.keywordNames[i], stringMap);
                    }

                    if (provider.taskNames != null)
                    {
                        foreach (var taskId in new List<int>(provider.taskNames.Keys))
                        {
                            var taskName = provider.taskNames[taskId];
                            if (Replace(ref taskName, stringMap))
                                provider.taskNames[taskId] = taskName;
                        }
                    }

                    if (provider.opcodeNames != null)
                    {
                        foreach (var opcodeId in new List<int>(provider.opcodeNames.Keys))
                        {
                            var opcodeName = provider.opcodeNames[opcodeId];
                            if (Replace(ref opcodeName, stringMap))
                                provider.opcodeNames[opcodeId] = opcodeName;
                        }
                    }

                    foreach (EtwEvent ev in provider.Events)
                        ev.UpdateStrings(stringMap);
                }
            }

            return providers;
        }

        /// <summary>
        /// The name of the ETW provider
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// The GUID that uniquely identifies the ETW provider
        /// </summary>
        public Guid Id { get; private set; }

        /// <summary>
        /// The events for the
        /// </summary>
        public IList<EtwEvent> Events
        {
            get { return this.m_events; }
        }

        public string GetKeywordName(int bitPos)
        {
            Debug.Assert(0 <= bitPos && bitPos < 64);
            if (this.keywordNames == null)
                return null;

            return this.keywordNames[bitPos];
        }

        public string GetKeywordSetString(ulong keywords, string separator = ",")
        {
            var ret = "";
            ulong bitsWithNoName = 0;
            ulong bit = 1;
            for (int bitPos = 0; bitPos < 64; bitPos++)
            {
                if (keywords == 0)
                    break;
                if ((bit & keywords) != 0)
                {
                    var name = this.keywordNames[bitPos];
                    if (name != null)
                    {
                        if (ret.Length != 0)
                            ret += separator;
                        ret += name;
                    }
                    else
                        bitsWithNoName |= bit;
                }
                bit = bit << 1;
            }
            if (bitsWithNoName != 0)
            {
                if (ret.Length != 0)
                    ret += separator;
                ret += "0x" + bitsWithNoName.ToString("x");
            }
            if (ret.Length == 0)
                ret = "0";
            return ret;
        }

        public string GetTaskName(ushort taskId)
        {
            if (this.taskNames == null)
                return "";
            string ret;
            if (this.taskNames.TryGetValue(taskId, out ret))
                return ret;
            if (taskId == 0)
                return "";
            return "Task" + taskId.ToString();
        }

        public string GetOpcodeName(ushort taskId, byte opcodeId)
        {
            int value = opcodeId + (taskId * 256);
            string ret;
            if (this.opcodeNames.TryGetValue(value, out ret))
                return ret;

            value = opcodeId + (GlobalScope * 256);
            if (this.opcodeNames.TryGetValue(value, out ret))
                return ret;
            return "Opcode" + opcodeId;
        }

        /// <summary>
        /// For debugging
        /// </summary>
        public override string ToString()
        {
            return this.Name + " " + this.Id;
        }

        #region private

        // create a manifest from a stream or a file
        internal EtwProvider(XmlReader reader)
        {
            var lineInfo = (IXmlLineInfo)reader;
            Debug.Assert(
                reader.NodeType == XmlNodeType.Element && reader.Name == "provider",
                "Must advance to provider element (e.g. call ReadToDescendant)");

            this.enums = new Dictionary<string, Enumeration>();
            this.templateValues = new Dictionary<string, List<Field>>();
            this.InitStandardOpcodes();

            this.Name = reader.GetAttribute("name");
            this.Id = Guid.Parse(reader.GetAttribute("guid"));

            var inputDepth = reader.Depth;
            reader.Read(); // Advance to children
            var curTask = GlobalScope;
            var curTaskDepth = int.MaxValue;
            while (inputDepth < reader.Depth)
            {
                if (reader.NodeType == XmlNodeType.Element)
                {
                    switch (reader.Name)
                    {
                        case "keyword":
                            {
                                if (this.keywordNames == null)
                                {
                                    this.keywordNames = new string[64];
                                    this.keywordValues = new Dictionary<string, ulong>();
                                }
                                string name = reader.GetAttribute("name");
                                string valueString = reader.GetAttribute("mask");
                                ulong value = ParseNumber(valueString);
                                int keywordIndex = GetBitPosition(value);

                                string message = reader.GetAttribute("message");
                                if (message == null)
                                    message = name;

                                this.keywordNames[keywordIndex] = message;
                                this.keywordValues.Add(name, value);
                                reader.Skip();
                            }
                            break;

                        case "task":
                            {
                                if (this.taskNames == null)
                                {
                                    this.taskNames = new Dictionary<int, string>();
                                    this.taskValues = new Dictionary<string, int>();
                                }
                                string name = reader.GetAttribute("name");
                                int value = (int)ParseNumber(reader.GetAttribute("value"));

                                string message = reader.GetAttribute("message");
                                if (message == null)
                                    message = name;

                                this.taskNames.Add(value, message);
                                this.taskValues.Add(name, value);

                                // Remember enuough to resolve opcodes nested inside this task.
                                curTask = value;
                                curTaskDepth = reader.Depth;
                                reader.Skip();
                            }
                            break;

                        case "opcode":
                            {
                                string name = reader.GetAttribute("name");
                                int value = (int)ParseNumber(reader.GetAttribute("value"));
                                int taskForOpcode = GlobalScope;
                                if (reader.Depth > curTaskDepth)
                                    taskForOpcode = curTask;

                                string message = reader.GetAttribute("message");
                                if (message == null)
                                    message = name;

                                this.AddOpcode(taskForOpcode, value, message, name);
                                reader.Skip();
                                break;
                            }
                        case "event":
                            this.m_events.Add(new EtwEvent(reader, this, lineInfo.LineNumber));
                            break;

                        case "template":
                            this.ReadTemplate(reader);
                            break;

                        case "bitMap":
                            this.ReadMap(reader, true);
                            break;

                        case "valueMap":
                            this.ReadMap(reader, false);
                            break;

                        default:
                            Debug.WriteLine("Skipping unknown element {0}", reader.Name);
                            reader.Read();
                            break;
                    }
                }
                else if (!reader.Read())
                    break;
            }

            // Second pass, look up any Ids used in the events.
            foreach (var ev in this.m_events)
                ev.ResolveIdsInEvent();

            // We are done with these.  Free up some space.
            this.taskValues = null;
            this.opcodeValues = null;
            this.keywordValues = null;
            this.templateValues = null;
            this.enums = null;
        }

        private void AddOpcode(int taskId, int opcodeId, string name, string manifestName = null)
        {
            Debug.Assert(0 <= taskId && taskId <= GlobalScope);
            Debug.Assert(0 <= opcodeId && opcodeId < 256);
            int value = opcodeId + (taskId * 256);
            this.opcodeNames.Add(value, name);
            if (manifestName == null)
                manifestName = name;
            this.opcodeValues.Add(manifestName, value);
        }

        internal static ulong ParseNumber(string valueString)
        {
            if (valueString.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
                return ulong.Parse(valueString.Substring(2), System.Globalization.NumberStyles.AllowHexSpecifier);
            return ulong.Parse(valueString);
        }

        private static int GetBitPosition(ulong value)
        {
            if (value == 0)
                throw new ApplicationException("Keyword is not a power of 2");
            int ret = 0;
            for (; ; )
            {
                if (((int)value & 1) != 0)
                {
                    if (value == 0)
                        throw new ApplicationException("Keyword is not a power of 2");
                    break;
                }
                value = value >> 1;
                ret++;
            }
            return ret;
        }

        internal static bool Replace(ref string value, Dictionary<string, string> map)
        {
            if (value == null)
                return false;
            if (!value.Contains("$(string."))
                return false;

            var ret = false;
            value = Regex.Replace(
                value,
                @"\$\(string\.(.*)\)",
                delegate (Match m)
                {
                    ret = true;
                    var key = m.Groups[1].Value;
                    return map[key];
                });
            return ret;
        }

        private void InitStandardOpcodes()
        {
            this.opcodeNames = new Dictionary<int, string>();
            this.opcodeValues = new Dictionary<string, int>();

            this.AddOpcode(GlobalScope, 0, "", "win:Info");
            this.AddOpcode(GlobalScope, 1, "Start", "win:Start");
            this.AddOpcode(GlobalScope, 2, "Stop", "win:Stop");
            this.AddOpcode(GlobalScope, 3, "DC_Start", "win:DC_Start");
            this.AddOpcode(GlobalScope, 4, "DC_Stop", "win:DC_Stop");
            this.AddOpcode(GlobalScope, 5, "Extension", "win:Extension");
            this.AddOpcode(GlobalScope, 6, "Reply", "win:Reply");
            this.AddOpcode(GlobalScope, 7, "Resume", "win:Resume");
            this.AddOpcode(GlobalScope, 8, "Suspend", "win:Suspend");
            this.AddOpcode(GlobalScope, 9, "Send", "win:Send");
            this.AddOpcode(GlobalScope, 240, "Receive", "win:Receive");
        }

        private void ReadMap(XmlReader reader, bool isBitMap)
        {
            Debug.Assert(
                reader.NodeType == XmlNodeType.Element && (reader.Name == "bitMap" || reader.Name == "valueMap"),
                "Must advance to bitMap/valueMap element (e.g. call ReadToDescendant)");

            string name = reader.GetAttribute("name");
            var enumeration = new Enumeration(name, isBitMap);
            var inputDepth = reader.Depth;
            reader.Read(); // Advance to children
            while (inputDepth < reader.Depth)
            {
                if (reader.NodeType == XmlNodeType.Element)
                {
                    switch (reader.Name)
                    {
                        case "map":
                            {
                                int value = (int)ParseNumber(reader.GetAttribute("value"));
                                string message = reader.GetAttribute("message");
                                enumeration.Add(value, message);
                            }
                            break;

                        default:
                            Debug.WriteLine("Skipping unknown element {0}", reader.Name);
                            break;
                    }
                }
                if (!reader.Read())
                    break;
            }
            this.enums.Add(name, enumeration);
        }

        private void ReadTemplate(XmlReader reader)
        {
            Debug.Assert(reader.NodeType == XmlNodeType.Element && reader.Name == "template", "Must advance to template element (e.g. call ReadToDescendant)");

            string tid = reader.GetAttribute("tid");
            List<Field> template;
            if (this.templateValues.TryGetValue(tid, out template))
            {
                if (template.Count != 0)
                    throw new ApplicationException("Template " + tid + " is defined twice");
            }
            else
                this.templateValues[tid] = template = new List<Field>();

            var inputDepth = reader.Depth;
            reader.Read(); // Advance to children
            int position = 0;
            while (inputDepth < reader.Depth)
            {
                if (reader.NodeType == XmlNodeType.Element)
                {
                    switch (reader.Name)
                    {
                        case "data":
                            {
                                string name = reader.GetAttribute("name");
                                string inTypeStr = reader.GetAttribute("inType");
                                string ouTypeStr = reader.GetAttribute("outType");
                                var mapId = reader.GetAttribute("map");
                                template.Add(new Field(name, inTypeStr, mapId, position++));
                            }
                            break;

                        default:
                            Debug.WriteLine("Skipping unknown element {0}", reader.Name);
                            break;
                    }
                }
                if (!reader.Read())
                    break;
            }
        }

        #endregion private
    }
}