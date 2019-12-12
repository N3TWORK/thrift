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
using System.IO;

#if UNITY_ANDROID && !UNITY_EDITOR
using Unity.IO.Compression;
#else
using System.IO.Compression;
#endif

namespace Thrift.Transport
{
  public class TZlibTransport : TTransport, IDisposable
    {
		private DeflateStream inputBuffer;
		private DeflateStream outputBuffer;
        private TStreamTransport transport;

		public TZlibTransport(TStreamTransport transport)
        {
            this.transport = transport;
            InitBuffers();
        }

        private void InitBuffers()
        {
            if (transport.InputStream != null)
            {
				inputBuffer = new DeflateStream(transport.InputStream, CompressionMode.Decompress);
            }
            if (transport.OutputStream != null)
            {
				outputBuffer = new DeflateStream(transport.OutputStream, CompressionMode.Compress);
            }
        }

        private void CloseBuffers()
        {
            if (inputBuffer != null && inputBuffer.CanRead)
            {
                inputBuffer.Close();
            }
            if (outputBuffer != null && outputBuffer.CanWrite)
            {
                outputBuffer.Close();
            }
        }

        public TTransport UnderlyingTransport
        {
            get { return transport; }
        }

        public override bool IsOpen
        {
            get { return transport.IsOpen; }
        }

        public override void Open()
        {
            transport.Open();
            InitBuffers();
        }

        public override void Close()
        {
            CloseBuffers();
            transport.Close();
        }

        public override int Read(byte[] buf, int off, int len)
        {
            return inputBuffer.Read(buf, off, len);
        }

        public override void Write(byte[] buf, int off, int len)
        {
            outputBuffer.Write(buf, off, len);
        }

        public override void Flush()
        {
            outputBuffer.Flush();
        }

    #region " IDisposable Support "
    private bool _IsDisposed;

    // IDisposable
    protected override void Dispose(bool disposing)
    {
      if (!_IsDisposed)
      {
        if (disposing)
        {
          if (inputBuffer != null)
            inputBuffer.Dispose();
          if (outputBuffer != null)
            outputBuffer.Dispose();
        }
      }
      _IsDisposed = true;
    }
    #endregion
  }
}
