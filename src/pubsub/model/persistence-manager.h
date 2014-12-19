/*
 * persistence-manager.h
 *
 *  Created on: Feb 26, 2014
 *      Author: zhaohui
 */

#ifndef PERSISTENCE_MANAGER_H_
#define PERSISTENCE_MANAGER_H_

#include <vector>
#include <list>
#include <map>
#include "ns3/internet-module.h"
#include "ns3/pubsub-core.h"
#include <utility>
#include <set>

namespace ns3
{
class HubApp;
class PersistenceManager:public SimpleRefCount<PersistenceManager>{
private:

    typedef std::pair<t_topicNumberType,t_packetNumberType> t_packetId;
    typedef std::multimap<t_nodesNumberType,t_topicNumberType> t_hub2topicMltMap;
    typedef std::multimap<t_topicNumberType,Ipv4Address> t_topic2pubAddrMltMap;
    typedef std::multimap<t_nodesNumberType,t_topicNumberType> t_pubId2topicMltMap;
    typedef std::multimap<t_topicNumberType,t_nodesNumberType> t_topic2pubIdMltMap;
    typedef std::map<t_topicNumberType,t_nodesNumberType> t_topic2hubMap;
    typedef std::map<t_nodesNumberType,HubApp*> t_hubId2hubMap;
    typedef std::multimap<t_topicNumberType,std::pair<t_nodesNumberType,t_packetNumberType> > t_topic2subMltMap;
    typedef std::multimap<t_nodesNumberType,t_topicNumberType> t_subId2topicMltMap;
    typedef std::map<t_packetId,Ptr<Packet> > t_topic2packetMap;
    typedef std::map<t_topicNumberType,t_packetNumberType> t_topic2pakcetNumberMap;
    typedef std::map<std::pair<t_topicNumberType,t_nodesNumberType>,t_packetNumberType > t_topicPub2packetNumberMap;
    typedef std::map<t_nodesNumberType,Ipv4Address> t_sub2AddrMap;


    t_hub2topicMltMap hub_topic;
    t_topic2hubMap topic_hub;
    t_topic2pubAddrMltMap topic_pubAddr;
    t_pubId2topicMltMap pubId_topic;
    t_topic2pubIdMltMap topic_pubId;
    t_hubId2hubMap hubList;
    t_sub2AddrMap subInfo;
    t_topic2subMltMap topic_sub;
    t_subId2topicMltMap subId_topic;
    t_topic2packetMap topic_packet;
    t_topic2pakcetNumberMap topic_packetNum;
    t_topicPub2packetNumberMap pubTopic_packetNum;
    balanceKind howToDoBalance;
    t_nodesNumberType hubNumber;
    uint32_t threshold;
    Timer balanceTrigger;
    balanceKind howToArrangeHub;


    PersistenceManager(const PersistenceManager& persistenceManager);
    static Ptr<PersistenceManager> instance;

    t_nodesNumberType DoArrange(t_topicNumberType theTopic,balanceKind how);
    t_nodesNumberType MakeImbalance(t_topicNumberType theTopic);
    t_nodesNumberType ArrangeById(t_topicNumberType theTopic);
    t_nodesNumberType ArrangeBytopicNumber(t_topicNumberType theTopic);
public:
    PersistenceManager();
    ~PersistenceManager();
    Ipv4Address HandlePub(t_topicNumberType theTopic,t_nodesNumberType pubId,Ipv4Address addr);
    Ipv4Address HandleSub(t_topicNumberType theTopic,t_nodesNumberType subId,Ipv4Address subAddr);
    Ipv4Address GetHubAddress(t_nodesNumberType RequireId);
    t_nodesNumberType ArrangeHub(t_topicNumberType theTopic,balanceKind how = IMBALANCE);
    t_nodesNumberType GetPubAddress(t_nodesNumberType id,std::set<Ipv4Address>* ipv4,std::set<t_nodesNumberType> *outputId);
    t_nodesNumberType GetSubAddress(t_nodesNumberType, std::set<Ipv4Address>* ipv4,std::set<t_nodesNumberType> *outpubId);
    t_nodesNumberType GetHub(t_topicNumberType requireTopic);
    void AddHub(HubApp* hub);
    void DeleteHub(t_nodesNumberType theId);
    void DoBalance(void);
    void storeMessage(t_packetId theId,Ptr<Packet> thePacket);
    t_topicNumberType GetHeldSubedTopic(t_nodesNumberType hubId,t_nodesNumberType subId,std::set<t_topicNumberType> *theTopics);
    Ptr<Packet> GetPacket(t_packetNumberType packetId,t_topicNumberType topic);
    static Ptr<PersistenceManager> GetPersistenceManager();
//    static void releasePersistenceManager();
//    void SetBalance(balanceKind balance){howToDoBalance = balance;}
    void NotifyHubReleaseopic(t_nodesNumberType hubId);
    void DoReleaseTopic(t_topicNumberType topicId);
    Ipv4Address ReconnectHub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address);
    void SetSendNumber(t_topicNumberType topic,t_nodesNumberType subId,t_packetNumberType secquence);
    void SetRcvNumberOfPub(t_topicNumberType theTopic,t_nodesNumberType pubId,t_packetNumberType packetNumber);
    t_packetNumberType GetRcvNumber(t_topicNumberType theTopic);
    t_packetNumberType GetRcvNumberOfPub(t_topicNumberType theTopic,t_nodesNumberType pubId);
    t_packetNumberType GetSendNumber(t_topicNumberType theTopic,t_nodesNumberType subId);
};
}//end of namespace ns3



#endif /* PERSISTENCE_MANAGER_H_ */
