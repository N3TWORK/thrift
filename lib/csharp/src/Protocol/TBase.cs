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
 */

using System;

namespace Thrift.Protocol
{
    public interface TBase : TAbstractBase
    {
        /// <summary>
        /// Reads the TObject from the given input protocol.
        /// </summary>
        void Read(TProtocol tProtocol);
    }


    //Required by csharp:leg compilation
    public abstract class ThriftFactory<T> where T : TBase, new()
    {
        public static T CreateInstance(TProtocol tProtocol)
        {
            T inst = new T();
            inst.Read(tProtocol);
            return inst;
        }
    }


    //Provided by csharp:leg compilation
    [AttributeUsage(AttributeTargets.All)]
    public class DataMemberAttribute : System.Attribute
    {
        public int Index { get; set; }
        public DataMemberAttribute()
        {
        }
    }

}
