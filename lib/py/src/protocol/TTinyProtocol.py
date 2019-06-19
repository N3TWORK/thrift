#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#	 http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

from TCompactProtocol import TCompactProtocol
from TProtocol import *
from ..transport import TTransport
from struct import pack, unpack

__all__ = ['TTinyProtocol', 'TTinyProtocolFactory']

class TTinyProtocol(TCompactProtocol):
	"""Tiny implementation of the Thrift protocol driver."""

	structDepth = 0

	def getStringsTable(self):
		return self.__strings_array

	def writeStructBegin(self, name):
		# Begin accumulating strings and hide our true protocol
		if self.structDepth == 0:
			self.__strings_array = []
			self.__strings_dict = {}
			self.__real_trans = self.trans
			self.trans = TTransport.TMemoryBuffer()
		self.structDepth += 1

		# Call super
		TCompactProtocol.writeStructBegin(self, name)

	def writeStructEnd(self):
		def encodeIntToUnicode(i):
			result = u''
			while True:
				result += unichr(i & 0xFFFF)
				i >>= 16
				if i == 0:
					break

			return result.encode('utf-8')
		# Call super
		TCompactProtocol.writeStructEnd(self)

		# If this is the end of the struct, output the strings table.
		self.structDepth -= 1
		if self.structDepth == 0:
			fake_trans = self.trans
			self.trans = self.__real_trans
			self._TCompactProtocol__writeVarint(len(self.__strings_array))
			rewritten = 0
			for s in self.__strings_array:
				if s.startswith('\x1b\x1a'):
					rewritten += 1
					rs = encodeIntToUnicode(rewritten)
					self._TCompactProtocol__writeSize(len(rs))
					self.trans.write(rs)
				else:
					self._TCompactProtocol__writeSize(len(s))
					self.trans.write(s)
			self.trans.write(fake_trans.getvalue())

	def writeString(self, s):
		if not s in self.__strings_dict:
			slot = len(self.__strings_array)
			self.__strings_array.append(s)
			self.__strings_dict[s] = slot
		else:
			slot = self.__strings_dict[s]
		self._TCompactProtocol__writeVarint(slot)

	def readStructBegin(self):
		if self.structDepth == 0:
			self.__strings_array = []
			count = self._TCompactProtocol__readVarint()
			for i in range(count):
				len = self._TCompactProtocol__readSize()
				s = self.trans.readAll(len)
				self.__strings_array.append(s)
		self.structDepth += 1

		# Call super
		TCompactProtocol.readStructBegin(self)

	def readStructEnd(self):
		self.structDepth -= 1

		# Call super
		TCompactProtocol.readStructEnd(self)

	def readString(self):
		i = self._TCompactProtocol__readVarint()
		return self.__strings_array[i]

class TTinyProtocolFactory:
	def __init__(self):
		pass

	def getProtocol(self, trans):
		return TTinyProtocol(trans)

class TTinyProtocolAccelerated(TTinyProtocol):
    def __init__(self, *args, **kwargs):
        fallback = kwargs.pop('fallback', True)
        super(TTinyProtocolAccelerated, self).__init__(*args, **kwargs)
        try:
            from thrift.protocol import fastbinary
        except ImportError:
            if not fallback:
                raise
        else:
            self._fast_decode = fastbinary.decode_tiny
            self._fast_encode = fastbinary.encode_tiny
