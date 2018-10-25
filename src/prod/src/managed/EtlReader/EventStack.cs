// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System.Collections.Generic;

    internal class EventStack
    {
        class EventEntry
        {
            private string contextSequence;
            private int expectedCount;
            private string data;
            private int count;

            public EventEntry(string contextSequence, string data)
            {
                this.contextSequence = contextSequence;
                this.data = data;
                this.expectedCount = GetContextCount(contextSequence);
                this.count = 1;
            }

            public string ContextSequence
            {
                get
                {
                    return this.contextSequence;
                }
            }

            public string Data
            {
                get
                {
                    if (this.expectedCount == (this.count & 0xff))
                    {
                        return this.data;
                    }

                    return data + " !!!Incomplete context!!! ";
                }
            }

            public void Add(string data)
            {
                this.data += data;
                this.count++;
            }
        }

        private Dictionary<string, List<EventEntry>> threads;

        public EventStack()
        {
            threads = new Dictionary<string, List<EventEntry>>();
        }

        private static int GetContextCount(string contextSequence)
        {
            return int.Parse(contextSequence, System.Globalization.CultureInfo.InvariantCulture) & 0xff;
        }

        public void Push(uint process, uint thread, string contextSequence, string data)
        {
            string threadId = GetThreadId(process, thread);

            List<EventEntry> stack;
            if (!threads.TryGetValue(GetThreadId(process, thread), out stack))
            {
                stack = new List<EventEntry>();
                threads.Add(threadId, stack);
            }

            if (stack.Count > 0 && stack[stack.Count - 1].ContextSequence == contextSequence)
            {
                stack[stack.Count - 1].Add(data);
            }
            else
            {
                stack.Add(new EventEntry(contextSequence, data));
            }
        }

        public string Pop(uint process, uint thread, string contextSequence, bool clearStack)
        {
            if (GetContextCount(contextSequence) == 0)
            {
                return string.Empty;
            }

            string result = null;

            List<EventEntry> stack;
            if (threads.TryGetValue(GetThreadId(process, thread), out stack))
            {
                for (int i = stack.Count - 1; i >= 0; i--)
                {
                    if (stack[i].ContextSequence == contextSequence)
                    {
                        result = stack[i].Data;
                        stack.RemoveRange(i, stack.Count - i);

                        break;
                    }
                }
                
                if (clearStack)
                {
                    stack.Clear();
                }
            }

            return result;
        }

        private static string GetThreadId(uint process, uint thread)
        {
            return process.ToString(System.Globalization.CultureInfo.InvariantCulture) + "." + thread.ToString(System.Globalization.CultureInfo.InvariantCulture);
        }
    }
}