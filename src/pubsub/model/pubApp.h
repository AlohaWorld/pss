/*
 * pubApp.h
 *
 *  Created on: Mar 7, 2014
 *      Author: zhaohui
 */

#ifndef PUBAPP_H
#define PUBAPP_H

#include "ns3/applications-module.h"
#include "ns3/pubsub-core.h"
#include "ns3/persistence-manager.h"
#include "ns3/internet-module.h"
#include <deque>
#include <set>
#include <map>
namespace ns3 {

class PubApp: public Application {
private:
    typedef std::pair<t_topicNumberType,t_packetNumberType> t_msgId;

	t_nodesNumberType pubId;
	Ipv4Address pubIp;
	std::set<t_topicNumberType> topic;
	std::map<t_topicNumberType,appStatus> pubStatus;
	t_packetNumberType pubTotalSendedPackets;
	uint64_t pubTotalSendedBit;
	std::map<t_topicNumberType,t_packetNumberType> packetSendedPerTopic;
	std::map<t_topicNumberType,uint64_t> bitsSendedPerTopic;
	uint32_t pubMsgSize;
	Time pubSendInterval;
	ExponentialVariable randomMsgSize, randomSendInterval;
	std::map<t_topicNumberType,Ptr<Packet> > toSend;

	Ptr<Socket> pubRcvAckSocket;
	Address pubRcvAckAddress;

	std::map<t_topicNumberType,Ptr<Socket> > pubSendSocket;
	std::map<t_topicNumberType,Address> hubAddr;

	std::map<t_topicNumberType,t_packetNumberType> packetsToSend;
	Ptr<PersistenceManager> persistenceManager;

	std::map<std::pair<t_topicNumberType,t_packetNumberType>,Ptr<Packet> >  sendWindow;
	std::map<t_topicNumberType,std::list<Ptr<Packet> > > resendList;
	std::map<t_addrAndPort,Ptr<Packet> >tempAck;

    virtual void StartApplication(void);
    virtual void StopApplication(void);
//    void Send(void);
    void Send2Topic(t_topicNumberType theTopic);
    void ScheduleNext(t_topicNumberType theTopic);
    Ptr<Packet> MakePacket(t_topicNumberType theTopic);
    void SendRestart(Ptr<Socket> socket, uint32_t);
    void RcvAck(Ptr<Socket> socket);
    void HandleAckAccept(Ptr<Socket> s, const Address& from);
//    void HandleShutdown(Ptr<Socket> s);
//    void HandleShutdownErr(Ptr<Socket> s);

    void ConnectionSucceeded(Ptr<Socket> socket);
    void ConnectionFailed(Ptr<Socket> socket);

    void DoPublish(t_topicNumberType theTopic);
    void ConnectHub(t_topicNumberType theTopic);

    void GetResendPacket(t_topicNumberType topic);

public:
    static TypeId GetTypeId (void);
	PubApp();
	virtual ~PubApp();
	void Publish(t_nodesNumberType theTopic);
	void AddTopic(t_topicNumberType theTopic,t_packetNumberType packetNumber = c_defPacketNumber);
	void Reconnect(t_topicNumberType theTopic);
};

} //end namespace ns3

#endif /* PUBAPP_H_ */
