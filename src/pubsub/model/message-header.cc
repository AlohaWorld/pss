/*
 * -header.cc
 *
 *  Created on: Feb 26, 2014
 *      Author: zhaohui
 */

#include "ns3/message-header.h"

namespace ns3 {

//============================  control header ===========================

CtrlHeader::CtrlHeader(messageKind mkind) :
        kind(NOTSET),
        topic(0),
        senderId(0),
        sequenceTillNow(0){
}

TypeId CtrlHeader::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::CtrlHeader")
            .SetParent<Header>()
            .AddConstructor<CtrlHeader>();
    return tid;
}

TypeId CtrlHeader::GetInstanceTypeId(void) const {
    return GetTypeId();
}

void CtrlHeader::Serialize(Buffer::Iterator start) const {
    Buffer::Iterator i = start;
    i.WriteHtonU32(uint32_t(kind));
    i.WriteHtonU32(topic);
    i.WriteHtonU64(senderId);
    i.WriteHtonU64(sequenceTillNow);
}

uint32_t CtrlHeader::Deserialize(Buffer::Iterator start) {
    Buffer::Iterator i = start;
    kind=messageKind(i.ReadNtohU32());
    topic=i.ReadNtohU32();
    senderId=i.ReadNtohU64();
    sequenceTillNow = i.ReadNtohU64();
    return 24;
}

uint32_t CtrlHeader::GetSerializedSize(void) const {
    return 24;
}


void CtrlHeader::Print(std::ostream &os) const {
    switch (kind) {
    case SUB:
        os << "sub[" << senderId << "] subscribe " << topic
                << "["<<sequenceTillNow<<"]";
        break;
    case PUB:
        os << "pub[" << senderId << "] publish " << topic
                << "["<<sequenceTillNow<<"]";
        break;
    case HUBACK:
        os<<"hub["<<senderId<<"] ACK of "<<topic<<"("<<sequenceTillNow<<")";
        break;
    case SUBACK:
        os<<"sub["<<senderId<<"] ACK of "<<topic<<"("<<sequenceTillNow<<")";
        break;
    default:
        os << "illegal control message.";
    }
}

//========================================== message Header ========================================


MsgHeader::MsgHeader() :
        kind(NOTSET),
        topic(0),
        senderId(0),
        sequence(0),
        contentSize(0),
        sendTime(0),
        hubRTime(0),
        hubSTime(0),
        subRTime(0),
        subSTime(0),
        fromBK(0),
        part(0),
        hubCacLen(0),
        hubSendWindowLen(0),
        hubTosend(0)
{}

TypeId MsgHeader::GetTypeId(void) {
    static TypeId tid =TypeId("ns3::MsgHeader")
            .SetParent<Header>()
            .AddConstructor<MsgHeader>();
    return tid;
}

TypeId MsgHeader::GetInstanceTypeId(void) const {
    return GetTypeId();
}

void MsgHeader::Print(std::ostream &os) const {
    os <<"send " << topic <<"("<<sequence <<")";
}

void MsgHeader::PrintTime(std::ostream &o) const {
    if(sequence == 1){
        o<<"sequence topic PSTime HRTime HSTime SRTime SSTime fromBK partition Hcaclen hubSendWindowLen hubToSendPacketsNum packetSize\n";
    }
    o << sequence << " "<<topic<<" " << sendTime << " " << hubRTime << " " << hubSTime
            << " " << subRTime << " " << subSTime << " " << (int) fromBK << " "<<part<<" "
            << hubCacLen << " " << hubSendWindowLen <<" "<<hubTosend<<" "<<contentSize<<std::endl;
}

void MsgHeader::Serialize(Buffer::Iterator start) const {
    Buffer::Iterator i = start;

    i.WriteHtonU32(uint32_t(kind));
    i.WriteHtonU32(topic);
    i.WriteHtonU64(senderId);
    i.WriteHtonU64(sequence);
    i.WriteHtonU32(contentSize);
    i.WriteHtonU64(sendTime);
    i.WriteHtonU64(hubRTime);
    i.WriteHtonU64(hubSTime);
    i.WriteHtonU64(subRTime);
    i.WriteHtonU64(subSTime);
    i.WriteU8(fromBK);
    i.WriteHtonU16(part);
    i.WriteHtonU64(hubCacLen);
    i.WriteHtonU64(hubSendWindowLen);
    i.WriteHtonU64(hubTosend);
}

uint32_t MsgHeader::Deserialize(Buffer::Iterator start) {
    Buffer::Iterator i = start;

    kind=messageKind(i.ReadNtohU32());
    topic=i.ReadNtohU32();
    senderId=i.ReadNtohU64();
    sequence = i.ReadNtohU64();
    contentSize = i.ReadNtohU32();
    sendTime = i.ReadNtohU64();
    hubRTime = i.ReadNtohU64();
    hubSTime = i.ReadNtohU64();
    subRTime = i.ReadNtohU64();
    subSTime = i.ReadNtohU64();
    fromBK = i.ReadU8();
    part = i.ReadNtohU16();
    hubCacLen = i.ReadNtohU64();
    hubSendWindowLen = i.ReadNtohU64();
    hubTosend = i.ReadNtohU64();

    return 95;
}

uint32_t MsgHeader::GetSerializedSize(void) const {
    return 95;
}
} //end of namespace ns3

