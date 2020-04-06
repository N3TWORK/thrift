package org.apache.thrift.protocol;

import org.apache.thrift.TException;
import org.apache.thrift.transport.TMemoryBuffer;
import org.apache.thrift.transport.TTransport;

import java.util.LinkedHashMap;

public class TTinyProtocol extends TCompactProtocol {

    public TTinyProtocol(TTransport tTransport) {
        super(tTransport);
    }

    int structDepth = 0;
    String[] stringArray;
    LinkedHashMap<String, Integer> stringDict;
    private TTransport realTrans;
    private TMemoryBuffer memoryTrans;

    @Override
    public void writeStructBegin(TStruct strct) throws TException {
        if (structDepth == 0) {
            stringDict = new LinkedHashMap<>();
            realTrans = trans_;
            memoryTrans = new TMemoryBuffer(32);
            trans_ = memoryTrans;
        }
        structDepth++;

        super.writeStructBegin(strct);
    }

    @Override
    public void writeStructEnd() throws TException {
        super.writeStructEnd();

        structDepth--;
        if (structDepth == 0) {
            trans_ = realTrans;
            writeVarint32((int) stringDict.size());
            for (String s : stringDict.keySet())
                super.writeString(s);
            trans_.write(memoryTrans.getArray(), 0, memoryTrans.length());
        }
    }

    @Override
    public void writeString(String str) throws TException {
        int slot;
        if (!stringDict.containsKey(str)) {
            slot = stringDict.size();
            stringDict.put(str, slot);
        } else {
            slot = stringDict.get(str);
        }

        writeVarint32(slot);
    }

    public TStruct readStructBegin() throws TException {
        // Read the string header.
        if (structDepth == 0) {
            int count = readVarint32();
            stringArray = new String[count];
            for (int i = 0; i < count; ++i) {
                stringArray[i] = super.readString();
            }
        }
        structDepth++;
        return super.readStructBegin();
    }

    public void readStructEnd() throws TException {
        super.readStructEnd();
        structDepth--;
    }

    public String readString() throws TException {
        int index = this.readVarint32();
        return stringArray[index];
    }

    private int readVarint32() throws TException {
        int result = 0;
        int shift = 0;
        if (this.trans_.getBytesRemainingInBuffer() >= 5) {
            byte[] var7 = this.trans_.getBuffer();
            int pos = this.trans_.getBufferPosition();
            int off = 0;

            while (true) {
                byte b1 = var7[pos + off];
                result |= (b1 & 127) << shift;
                if ((b1 & 128) != 128) {
                    this.trans_.consumeBuffer(off + 1);
                    break;
                }

                shift += 7;
                ++off;
            }
        } else {
            while (true) {
                byte b = this.readByte();
                result |= (b & 127) << shift;
                if ((b & 128) != 128) {
                    break;
                }

                shift += 7;
            }
        }

        return result;
    }

    /**
     * Temporary buffer used for various operations that would otherwise require a
     * small allocation.
     */
    private final byte[] temp = new byte[10];

    /**
     * Write an i32 as a varint. Results in 1-5 bytes on the wire.
     * TODO: make a permanent buffer like writeVarint64?
     */
    private void writeVarint32(int n) throws TException {
        int idx = 0;
        while (true) {
            if ((n & ~0x7F) == 0) {
                temp[idx++] = (byte) n;
                // writeByteDirect((byte)n);
                break;
                // return;
            } else {
                temp[idx++] = (byte) ((n & 0x7F) | 0x80);
                // writeByteDirect((byte)((n & 0x7F) | 0x80));
                n >>>= 7;
            }
        }
        trans_.write(temp, 0, idx);
    }

    public static class Factory implements TProtocolFactory {

        public TProtocol getProtocol(TTransport trans) {
            return new TTinyProtocol(trans);
        }
    }
}

