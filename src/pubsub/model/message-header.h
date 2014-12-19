/*
 * packet-header.h
 *
 *  Created on: Feb 26, 2014
 *      Author: zhaohui
 */

#ifndef MESSAGE_HEADER_H_
#define MESSAGE_HEADER_H_

#include "ns3/header.h"
#include "ns3/pubsub-core.h"
#include "ns3/network-module.h"

namespace ns3 {

enum messageKind {
    NOTSET,
    PUB,
    SUB,
    MESSAGE,
    HUBACK,
    SUBACK,
    RESEND,
    SENDLENGTH,
};
//====================================== CtrlHeader =====================
class CtrlHeader: public Header{
private:

    messageKind kind;
    t_topicNumberType topic;
    t_nodesNumberType senderId;
    t_packetNumberType sequenceTillNow;
public:
    CtrlHeader(messageKind mkind = NOTSET);
    virtual ~CtrlHeader() {};

    void SetSequenceTillNow(t_packetNumberType sequence){sequenceTillNow = sequence;}
    t_packetNumberType GetSequenceTillNow(){return sequenceTillNow;}
    void SetTopic(t_topicNumberType top) {
        topic = top;
    }
    t_topicNumberType GetTopic() const {
        return topic;
    }

    void SetSenderId(t_nodesNumberType id) {
        senderId = id;
    }
    t_nodesNumberType GetSenderId() const {
        return senderId;
    }

    void SetKind(messageKind mkind) {
        kind = mkind;
    }
    messageKind GetKind() const {
        return kind;
    }


    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual void Print(std::ostream &os) const;
	virtual void Serialize(Buffer::Iterator start) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);
	virtual uint32_t GetSerializedSize(void) const;
};
//====================================== MsgHeader =====================
class MsgHeader: public Header {
private:
    messageKind kind;
    t_topicNumberType topic;
    t_nodesNumberType senderId;
    t_packetNumberType sequence;
    uint32_t contentSize;
    t_timestamp sendTime;
    t_timestamp hubRTime;
    t_timestamp hubSTime;
    t_timestamp subRTime;
    t_timestamp subSTime;
    uint8_t fromBK;
    uint16_t part;
    t_packetNumberType hubCacLen;
    t_packetNumberType hubSendWindowLen;
    t_packetNumberType hubTosend;

public:
    MsgHeader();
    virtual ~MsgHeader() {
    }
    ;

    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual void Print(std::ostream &os) const;
    virtual void Serialize(Buffer::Iterator start) const;
    virtual uint32_t Deserialize(Buffer::Iterator start);
    virtual uint32_t GetSerializedSize(void) const;

    void PrintTime(std::ostream &o) const;

    void SetContentSize(uint32_t size) {
        contentSize = size;
    }
    uint32_t GetContentSize(void) const {
        return contentSize;
    }

    void SetSequence(t_packetNumberType seq) {
        sequence = seq;
    }
    t_packetNumberType GetSequence(void) const {
        return sequence;
    }

    void SetFromBK(uint8_t isTrue) {
        fromBK = isTrue;
    }
    uint8_t isFromBK() {
        return fromBK;
    }

    void SetPart(uint8_t parts) {
        part = parts;
    }
    uint8_t GetPart() {
        return part;
    }

    void SetHubCacLen(t_packetNumberType length) {
        hubCacLen = length;
    }
    t_packetNumberType GetHubCacLen(void) const {
        return hubCacLen;
    }

    void SetHubSendWindowLen(t_packetNumberType length) {
        hubSendWindowLen = length;
    }
    t_packetNumberType GetHubSendWindowLen(void) const {
        return hubSendWindowLen;
    }

    void SetSendTime(t_timestamp time) {
        sendTime = time;
    }
    t_timestamp GetSendTime(void) const {
        return sendTime;
    }

    void SetHubRTime(t_timestamp time) {
        hubRTime = time;
    }
    t_timestamp GetHubRTime(void) const {
        return hubRTime;
    }

    void SetHubSTime(t_timestamp time) {
        hubSTime = time;
    }
    t_timestamp GetHubSTime(void) const {
        return hubSTime;
    }

    void SetSubRTime(t_timestamp time) {
        subRTime = time;
    }
    t_timestamp GetSubRTime(void) const {
        return subRTime;
    }

    void SetSubSTime(t_timestamp time) {
        subSTime = time;
    }
    t_timestamp GetSubSTime(void) const {
        return subSTime;
    }

    void SetTopic(t_topicNumberType top) {
        topic = top;
    }
    t_topicNumberType GetTopic() const {
        return topic;
    }

    void SetSenderId(t_nodesNumberType id) {
        senderId = id;
    }
    t_nodesNumberType GetSenderId() const {
        return senderId;
    }

    void SetKind(messageKind mkind) {
        kind = mkind;
    }
    messageKind GetKind() const {
        return kind;
    }

    void SetHubTosend(t_packetNumberType packets){
        hubTosend = packets;
    }

    t_packetNumberType GetHubTosend(){
        return hubTosend;
    }
};
}
#endif /* HEADWIG_HEADER_H_ */
