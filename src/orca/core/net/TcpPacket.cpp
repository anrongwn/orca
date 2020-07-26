/*
Copyright 2017, orcaer@yeah.net  All rights reserved.

Author: orcaer@yeah.net

Last modified: 2020-7-26

Description:
*/
#include "TcpPacket.h"

#include "../../base/error/ErrorHandle.h"

using namespace orca::core;
using namespace std;

void TcpPacket::packWithType(const char * data, uint16_t size)
{
    dataSize_ = size;
    buffer_.resize(size + PacketMinSize()+sizeof(messageType_));

    buffer_[0] = HeadByte;
    PackNum(&buffer_[1], size);
    PackNum(&buffer_[3], messageType_);
    std::copy(data, data + size, &buffer_[sizeof(HeadByte) + sizeof(dataSize_) + sizeof(messageType_)]);
    buffer_.back() = EndByte;
}

const char* TcpPacket::getData()
{
    return buffer_.c_str() + sizeof(HeadByte) + sizeof(dataSize_)+sizeof(messageType_);
}


int TcpPacket::ReadTcpBuffer(uv::PacketBuffer* packetbuf, void* out)
{
    TcpPacket* packet = (TcpPacket*)out;
    packet->buffer_.clear();
    while (true)
    {
        auto size = packetbuf->readSize();
        //����С�ڰ�ͷ��С
        if (size < PacketMinSize()+sizeof(uint64_t))
        {
            return -1;
        }
        //�Ұ�ͷ
        packetbuf->readBufferN(packet->buffer_, sizeof(packet->dataSize_) + 1+ sizeof(uint64_t));
        if ((uint8_t)packet->buffer_[0] != HeadByte) //��ͷ����ȷ������һ���ֽڿ�ʼ������
        {
            packet->buffer_.clear();
            packetbuf->clearBufferN(1);
            continue;
        }
        UnpackNum((uint8_t*)packet->buffer_.c_str() + 1, packet->dataSize_);
        UnpackNum((uint8_t*)packet->buffer_.c_str() + 3, packet->messageType_);
        uint16_t msgsize = packet->dataSize_ + PacketMinSize()+ sizeof(uint64_t);
        //��������
        if (size < msgsize)
        {
            return -1;
        }
        packetbuf->clearBufferN(sizeof(packet->dataSize_) + 1 + sizeof(packet->messageType_));
        packetbuf->readBufferN(packet->buffer_, packet->dataSize_ + 1);
        //����β
        if ((uint8_t)packet->buffer_.back() == EndByte)
        {
            packetbuf->clearBufferN(packet->dataSize_ + 1);
            break;
        }
    }
    return 0;
}