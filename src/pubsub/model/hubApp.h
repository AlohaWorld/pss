/*
 * HubApp.h
 *
 *  Created on: Mar 7, 2014
 *      Author: zhaohui
 */

#ifndef HUBAPP_H
#define HUBAPP_H

#include "ns3/pubsub-core.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/message-header.h"
#include <vector>
#include <map>
#include <set>
#include <list>
#include <iostream>

namespace ns3{

class PersistenceManager;

class HubApp:public Application{
private:
    typedef std::pair<t_topicNumberType,t_packetNumberType> t_packetId;
    typedef std::pair<t_topicNumberType,t_nodesNumberType> t_topicId;
//    typedef std::pair<t_topicNumberType,t_packetNumberType> t_cacheType;

    t_nodesNumberType hubId;
    Ipv4Address hubIp;
    std::map<t_topicId,appStatus> hubStatus;
    Ptr<Socket> RcvMsgSocket;
    Ptr<Socket> RcvAckSocket;
    std::map<t_topicId,Ptr<Socket> > pub2socketMap;
    std::map<t_topicId,Ptr<Socket> > sub2socketMap;
    std::map<t_topicNumberType,std::list<t_nodesNumberType> > topic2publistMap;
    Address RMsgAddress;
//    std::vector<Address> SMsgAddress;
    Address RAckAddress;
//    std::vector<Address> SAckAddress;
//    std::set<t_topicNumberType> Topic;
//    std::set<t_nodesNumberType> connectedPub;
//    std::set<t_nodesNumberType> connectedSub;
    uint64_t hubRcvBit;
    t_packetNumberType hubRcvPacket;
    std::map<t_topicId,t_packetNumberType> hubRcvPacketPerPubTopic;
    std::map<t_topicNumberType,t_packetNumberType> hubRcvPacketPerTopic;
    std::map<t_topicId,t_packetNumberType> hubSendPacketPerSubTopic;
    Ptr<PersistenceManager> persistence;
    std::map<t_addrAndPort,Ptr<Packet> >temp;//for recreating received packets
    std::map<t_topicId,Ptr<Packet> > toSend;
    std::map<t_packetId,Ptr<Packet> > cache;
    std::map<t_packetId,t_nodesNumberType> cacheCount;//for multiple subscribe of one topic
    uint64_t cacheSizeNow;
    std::list<t_topicId> cacheOrder;
    t_packetNumberType cacheCap;
    uint64_t cacheCapOnByte;
    uint16_t readDelay;
    std::map<t_topicId,std::set<t_packetNumberType> > hubSendWindow;
    t_packetNumberType hubSendWindowSize;
//    std::map<t_topicNumberType,t_nodesNumberType> subInfo;
//    std::map<t_topicNumberType,std::list<Ptr<Socket> > > socketList;
    uint64_t hubSendBit;
    t_packetNumberType hubSendPacket;
    std::map<t_topicId,Timer> delay;
//    std::map<t_topicId,Timer> releaseSocket;
    std::map<t_topicNumberType,std::set<t_nodesNumberType> > topicList;
    Time hubSendInterval;
    uint64_t packetPerSecondThreshold;
    Timer getToSendPacketsNumber;
    ExponentialVariable randomSendInterval;
    std::map<t_addrAndPort,Ptr<Packet> >tempAck;
    std::ofstream wfile;


    virtual void StartApplication (void);
    virtual void StopApplication (void);

    void Transport(t_topicNumberType topicId);
    void DoSendMsg(t_topicId id);
    void SendAck(MsgHeader theHeader,bool isReSend = false);
    void RcvMsg(Ptr<Socket> socket);
    void RcvAck(Ptr<Socket> socket);
    void Accept(Ptr<Socket> s ,const Address& from);
    void AcceptAck(Ptr<Socket> s ,const Address& from);
    void RebuildPac(Ptr<Packet> p, Address from);
    int GetHub(t_nodesNumberType id);
    void GetPub(t_nodesNumberType id);
    void GetSub(t_nodesNumberType id);
    void SendRebuild (Ptr<Socket> socket, uint32_t txSpace);
    void Send2SubWithTopic(t_topicId subId);
    Ptr<Packet> ReadPacket(t_topicNumberType theTopic,t_packetNumberType seq,uint64_t & delay);
    void DoPersistence(MsgHeader theHeader);
    void DeleteSocket(t_topicNumberType theTopic);

    void ConnectionSucceeded(Ptr<Socket> socket);
    void ConnectionFailed(Ptr<Socket> socket);
    void GetToSendPacketNumber(bool firstTime);

public:
    static TypeId GetTypeId (void);
	HubApp();
	virtual ~HubApp();

    t_nodesNumberType GetId(void)const{return hubId;}
	Ipv4Address GetIp(void)const{return hubIp;}
	void OwnTopic(t_topicNumberType topic,t_nodesNumberType subId = 0);
    void ConnectPub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address);
    void ReConnectPub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address);
    void ReConnectSub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address);
    void ConnectSub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address);
    void DoBalance(t_topicNumberType topicNumber);
    void CaculatSendInterval();
};
}//end of namespace ns3




#endif /* HUBAPP_H_ */
