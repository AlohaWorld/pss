/*
 * subApp.h
 *
 *  Created on: Mar 7, 2014
 *      Author: zhaohui
 */

#ifndef SUBAPP_H
#define SUBAPP_H

#include "ns3/pubsub-core.h"
#include "ns3/applications-module.h"
#include "ns3/message-header.h"
#include "ns3/persistence-manager.h"
#include <set>
#include <map>
namespace ns3 {
class SubApp: public Application {
private:
    const static uint32_t LENGTH = 127; //should be (2^n)-1

    t_nodesNumberType subId;
    std::set<t_topicNumberType> subTopic;
    Ipv4Address subIp;

    Ptr<Socket> rcvMsgSocket;
    std::map<t_topicNumberType,Ptr<Socket> > sendAckSocket;
    Address rcvMsgAddress;

    uint64_t subTotalRcvBit;
    t_packetNumberType subTotalRcvMsg;
    std::map<t_topicNumberType,t_packetNumberType> subRcvMsgPerTopic;
    t_packetNumberType totalFromBK;
    std::set<Address> incomeAddress;
    std::map<Address,Ptr<Socket> > tempRecord;
    //record the new created socket temporarily.Use when connected broker has changed


    std::map<t_topicNumberType,uint32_t> submited;
    //save the received packets parts
    std::map<t_addrAndPort,Ptr<Packet> > temp;
    //record how many parts a packet has been cutted
    std::map<t_addrAndPort,uint16_t> partition;
    std::map<t_topicNumberType,uint32_t> maxSeen;
    std::map<t_topicNumberType,MsgHeader> toSubmit[LENGTH + 1];

    Ptr<PersistenceManager> persistence;
//    std::ofstream dataCollection;
    std::map<t_topicNumberType,std::ofstream*> dataCollection;
    std::string fileName;
    std::map<t_topicNumberType,std::vector<std::string> > toPrint;

    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void ReceiveMsg(Ptr<Socket> socket);
    void SubAccept(Ptr<Socket> s ,const Address& from);
    void ConnectionSucceeded (Ptr<Socket> socket);
    void ConnectionFailed(Ptr<Socket> socket);
    void SendAck(MsgHeader TheHeader);
    void RebuildPac(Ptr<Packet> p, const Address &from);
    void SubSubmit(MsgHeader TheHeader);
    void Print(MsgHeader theHeader);
    Ptr<Socket> MakeAckSocket(const Ipv4Address& ip);
    void LogToFile(t_topicNumberType theTopic);


public:
    static TypeId GetTypeId(void);
    SubApp();
    ~SubApp();
    void AddTopic(t_topicNumberType theTopic);
    void Subscribe(t_topicNumberType theTopic);
};
} //namespace ns3

#endif /* SUBAPP_H_ */
