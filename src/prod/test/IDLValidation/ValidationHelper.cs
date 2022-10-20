// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Test;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace System.Fabric.Setup
{
    public class ValidationHelper
    {
        /// <summary>
        /// Read .h file 
        /// </summary>
        /// <param name="path">Abosolute path of .h file</param>
        /// <returns>String list of file content</returns>
        public static List<string> ReadAllLines(string path)
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException(string.Format("Path {0} doesn't exist.", path));
            }

            LogHelper.Log("Start reading file {0}",path);
            List<string> lines = new List<string>();
            using (StreamReader sr = new StreamReader(path))
            {
                bool notComment = true;
                while (sr.Peek() > 0 && notComment)
                {
                    string line = sr.ReadLine();
                   
                    if (!string.IsNullOrWhiteSpace(line))
                    {
                        lines.Add(line.Trim());
                    }
                }
            }

            LogHelper.Log("Complete reading file {0}", path);
            return lines;
        }

        /// <summary>
        /// Read "Forward Declarations" of .h file and parse interface names
        /// </summary>
        /// <param name="lines">Content of .h file</param>
        /// <returns>InterfaceObject collection which has names</returns>
        public static Interfaces ParseInterfaceNames(List<string> lines)
        {
            LogHelper.Log("Start getting interfaces:");
            Interfaces interfaces = new Interfaces();

            const string sFlag = "/* Forward Declarations */";
            const string eFlag = "/* header files for imported files */";
            const string match = "typedef interface";
            const char space = ' ';

            int i = 0;

            // Read till "Forward Declarations"
            while (i < lines.Count && lines[i] != sFlag)
            {
                i++;
                continue;
            }

            // Get interface name if that line contains key words "typedef interface"
            // Skip duplicated ones
            while (i < lines.Count && lines[i] != eFlag)
            {
                if (lines[i].Contains(match))
                {                    
                    string name = lines[i].Split(space)[2];

                    if (!interfaces.ContainsName(name))
                    {
                        interfaces.Add(name, null);
                    }
                }

                i++;
            }

            LogHelper.Log("Completed getting interfaces: {0} interfaces.", interfaces.Count);
            return interfaces;
        }

        /// <summary>
        /// Read all content of .h and parse interface body
        /// </summary>
        /// <param name="lines">Content of .h file</param>
        /// <param name="interfaces">InterfaceObject collection</param>
        public static void ParseInterfaceBody(List<string> lines, Interfaces interfaces)
        {
            LogHelper.Log("Start parsing interface content:");

            const string sFlag = "MIDL_INTERFACE";
            const string cplusplusEFlag = "};";
            const string cEnd = "Vtbl;";
            const char space = ' ';            

            // Parse Guid
            // MIDL_INTERFACE("
            const int startIndex = 16;

            // skip to C++ interface body
            const int cppCursor = 2;

            // skip to C interface body 
            const int cCursor = 4;

            Dictionary<Guid, string> ids = new Dictionary<Guid, string>();           

            int i = 0;
            while (i < lines.Count)
            {
                // Read till interface definition
                if (!lines[i].Contains(sFlag))
                {
                    i++;
                    continue;
                }

                InterfaceObject ic = new InterfaceObject();
                string line = lines[i++];                                
                string guid = line.Substring(startIndex, line.Length - startIndex - 2);
                Guid id = new Guid(guid);
                ic.ID = id;
               
                StringBuilder sb = new StringBuilder();

                // Parse interface name                
                string name = lines[i].Split(space)[0];
                ic.Name = name;

                // Save Guid and interfaces to dictionary
                if (!ids.ContainsKey(id))
                {
                    ids.Add(id, name);
                }
                else if (interfaces.SameGuidInterfaces.ContainsKey(id))
                {
                    interfaces.SameGuidInterfaces[id].Add(name);
                }
                else
                {
                    interfaces.SameGuidInterfaces.Add(id, new List<string>() { ids[id] });
                    interfaces.SameGuidInterfaces[id].Add(name);
                }

                // skip to methods
                i += cppCursor;
                while (i < lines.Count && !lines[i].Contains(cplusplusEFlag))
                {
                    sb.AppendLine(lines[i++]);
                }
                ic.CplusplusContent = sb.ToString();

                // skip to BEGIN_INTERFACE
                i += cCursor;
                sb.Clear();
                while (i < lines.Count && !lines[i].Contains(cEnd))
                {
                    sb.AppendLine(lines[i]);
                    i++;
                }
                ic.CContent = sb.ToString();
                
                if (interfaces.ContainsName(name))
                {
                    interfaces[name] = ic;
                }

                i++;
            }

            LogHelper.Log("Complete parsing interface content.");
        }

        public static Interfaces GetInterfaceObjects(string path)
        {             
            List<string> file = ValidationHelper.ReadAllLines(path);

            Interfaces interfaces = ParseInterfaceNames(file);
            ParseInterfaceBody(file, interfaces);

            return interfaces;          
        }        

        /// <summary>
        /// Parse method information from C++ interface body
        /// </summary>
        /// <param name="source">C++ interface body</param>
        /// <returns>Collection of method name and body</returns>
        public static Methods ParseMethodInformation(string source)
        {
            if(source == null || string.IsNullOrWhiteSpace(source))
            {
                return null;
            }
            
            Dictionary<string, string> infos = new Dictionary<string, string>();
            Methods methods = new Methods(infos);

            string patternMethod = @"virtual.*?= 0;";
            string patternName = @"\b(\S+)\(";
            Regex regex = new Regex(patternMethod, RegexOptions.Singleline);
            MatchCollection mc = regex.Matches(source);

            if (mc.Count > 0)
            {
                foreach (Match match in mc)
                {
                   Regex regexName = new Regex(patternName);                   
                    string method = match.Value;                
                    Match mcc = regexName.Match(method);

                    string methodName = mcc.Groups[1].Value;
                    infos.Add(methodName, method);
                }
            }
            
            return methods;
        }

        /// <summary>
        /// Read all content of .h and parse struct name and body
        /// </summary>
        /// <param name="lines">Content of .h file</param>
        /// <returns>StructObject collection</returns>
        public static Structs ParseStructs(List<string> lines)
        {
            LogHelper.Log("Start getting structs:");
            Structs structs = new Structs();
            
            const string match = "typedef struct";
            const string sFlag = "{";
            const string notMatch = "Vtbl";
            const string endFlag = "}";
            const char space = ' ';

            int i = 0;
         
            while (i < lines.Count)
            {
                string line = lines[i++];
                if (!line.StartsWith(match) || line.Contains(notMatch) || !lines[i].Contains(sFlag))
                {
                    continue;
                }

                StructObject so = new StructObject();
                so.Name = line.Split(space)[2];
                StringBuilder sb = new StringBuilder();

                while (++i < lines.Count && !lines[i].Contains(endFlag))
                {
                    sb.AppendLine(lines[i]);
                }

                so.Body = sb.ToString();
                structs.Add(so.Name, so);
            }

            LogHelper.Log("Completed getting structs: {0} structs.", structs.Count);           

            return structs;
        }

        /// <summary>
        /// Parse member information from struct, used for getting the detailed description when
        /// printing the error.
        /// </summary>
        /// <param name="source">struct body</param>
        /// <returns>Collection of member name and content</returns>
        public static Members ParseMemberInformation(string source)
        {
            if (source == null || string.IsNullOrWhiteSpace(source))
            {
                return null;
            }

            const string semicolon = "\r\n";
            const char space = ' ';

            string[] seprator = new string[]{semicolon};

            Dictionary<string, string> infos = new Dictionary<string, string>();
            Members members = new Members(infos);

            string[] memArray = source.Split(seprator, StringSplitOptions.RemoveEmptyEntries);

            foreach (string member in memArray)
            {
                string memberName = member.Substring(0, member.Length - 1).Split(space).Last();
                infos.Add(memberName, member);
            }

            return members;
        }        
    }
}