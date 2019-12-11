/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * Contains some contributions under the Thrift Software License.
 * Please see doc/old-thrift-license.txt in the Thrift distribution for
 * details.
 */

using System;
using System.Text;
using Thrift.Transport;
using System.Collections;
using System.IO;
using System.Collections.Generic;

namespace Thrift.Protocol
{
    public class TTinyProtocol : TCompactProtocol
    {
        #region CompactProtocol Factory

        public new class Factory : TProtocolFactory
        {
            public Factory() { }

            public TProtocol GetProtocol(TTransport trans)
            {
                return new TTinyProtocol(trans);
            }
        }

        #endregion

        public TTinyProtocol(TTransport trans) : base(trans) {}

		private string[] stringArray;
		private List<string> stringList;
		private Dictionary<string, int> stringDict;
		private int structDepth;
		private TTransport realTrans;
		private MemoryStream realStream;

		public override void WriteStructBegin(TStruct strct)
		{
			if (structDepth == 0)
			{
				stringList = new List<string> ();
				stringDict = new Dictionary<string, int> ();
				realTrans = trans;
				realStream = new MemoryStream ();
				trans = new TStreamTransport (null, realStream);
			}
			structDepth++;

			base.WriteStructBegin (strct);
		}

		public override void WriteStructEnd()
		{
			base.WriteStructEnd ();

			structDepth--;
			if (structDepth == 0)
			{
				trans = realTrans;
				WriteVarint32 ((uint)stringList.Count);
				foreach (var s in stringList)
					WriteBinary (UTF8Encoding.UTF8.GetBytes (s));
				var fullBytes = realStream.GetBuffer(); // XXX(ek): shouldn't this be ToArray()?
				trans.Write(fullBytes, 0, fullBytes.Length);
			}
		}

        public override void WriteString(String str)
        {
			int slot;
			if (!stringDict.TryGetValue (str, out slot))
			{
				slot = stringList.Count;
				stringList.Add (str);
				stringDict [str] = slot;
			}

			WriteVarint32 ((uint)slot);
        }

        public override TStruct ReadStructBegin()
        {
			// Read the string header.
			if (structDepth == 0)
			{
				int count = (int)ReadVarint32();
				stringArray = new string[count];

				for (int i = 0; i < count; ++i)
				{
					var str = Encoding.UTF8.GetString(ReadBinary());
					stringArray [i] = str;
				}
			}
			structDepth++;

			return base.ReadStructBegin();
        }

        public override void ReadStructEnd()
        {
			base.ReadStructEnd ();
			structDepth--;
        }

        public override String ReadString()
        {
			int index = (int)ReadVarint32();
			return stringArray [index];
        }
    }
}
