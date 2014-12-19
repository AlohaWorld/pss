/*
 * persistence-manager.cc
 *
 *  Created on: Feb 26, 2014
 *      Author: zhaohui
 */

#include "ns3/persistence-manager.h"
#include "ns3/hubApp.h"

namespace ns3{
NS_LOG_COMPONENT_DEFINE("pubsub-persistence");

Ptr<PersistenceManager> PersistenceManager::instance = 0;

Ptr<PersistenceManager>
PersistenceManager::GetPersistenceManager(){
    if(instance == 0){
        instance = Create<PersistenceManager>();
    }
    return instance;
}

PersistenceManager::PersistenceManager():
        hub_topic(),
        topic_hub(),
        topic_pubAddr(),
        pubId_topic(),
        topic_pubId(),
        hubList(),
        subInfo(),
        topic_sub(),
        subId_topic(),
        topic_packet(),
        topic_packetNum(),
        howToDoBalance(NONE),
        hubNumber(0),
        threshold(2),
        howToArrangeHub(IMBALANCE)
{
    if (c_balanceON) {
        if(c_balanceTime == 0){
            howToArrangeHub = TOPICBASED;
        } else {
            balanceTrigger.SetFunction(&PersistenceManager::DoBalance, this);
            balanceTrigger.Schedule(Seconds(c_balanceTime));
        }
    }
}

//void
//PersistenceManager::releasePersistenceManager(){
//    counter--;
//    if(counter == 0){
//        delete instance;
//    }
//}

PersistenceManager::~PersistenceManager(){
    hub_topic.clear();
    topic_hub.clear();
    topic_pubAddr.clear();
    pubId_topic.clear();
    topic_pubId.clear();
    hubList.clear();
    subInfo.clear();
    topic_sub.clear();
    subId_topic.clear();
    topic_packet.clear();
    topic_packetNum.clear();
    balanceTrigger.Cancel();

}

Ipv4Address
PersistenceManager::HandlePub(t_topicNumberType theTopic,t_nodesNumberType pubId,Ipv4Address addr) {
    //chack weather the pub already published this topic
    t_pubId2topicMltMap::iterator itor = pubId_topic.find(pubId);
    if(itor != pubId_topic.end()){
        t_pubId2topicMltMap::size_type counter = pubId_topic.count(pubId);
        while(counter){
            if(itor->second == theTopic){
                NS_LOG_WARN(Simulator::Now().GetSeconds()<<": I pub["<<pubId
                            <<"] already published topic:"<<theTopic<<".so change a broker.");
                t_pubId2topicMltMap::iterator temp = itor;
                itor++;
                pubId_topic.erase(temp);
            }
            counter --;
            itor++;
        }
    }

    //try to get the hub
    t_nodesNumberType targetHubId = GetHub(theTopic);
    if (targetHubId == 0) {//the first time publish this topic
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I publisher creates topic "<<theTopic);
        targetHubId = ArrangeHub(theTopic,howToArrangeHub);

    }

    topic_pubAddr.insert(std::pair<t_topicNumberType,Ipv4Address>(theTopic,addr));
    topic_pubId.insert(std::pair<t_topicNumberType,t_nodesNumberType>(theTopic,pubId));
    pubId_topic.insert(std::pair<t_nodesNumberType,t_topicNumberType>(pubId,theTopic));
    hubList[targetHubId]->ConnectPub(theTopic,pubId,addr);

    return GetHubAddress(targetHubId);
}

Ipv4Address
PersistenceManager::HandleSub(t_topicNumberType theTopic,t_nodesNumberType subId,Ipv4Address subAddr){
    //chack weather the sub already subscribed this topic
    t_subId2topicMltMap::iterator itor = subId_topic.find(subId);
    if(itor != subId_topic.end()){
        t_subId2topicMltMap::size_type counter = subId_topic.count(subId);
        while(counter){
            if(itor->second == theTopic){
                NS_LOG_WARN(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] already subscribed topic:"<<theTopic<<". Don't subscribe it again.");
                return Ipv4Address("0.0.0.0");
            }
            counter --;
            itor++;
        }
    }
    //try to get the hub
    t_nodesNumberType targetHubId = GetHub(theTopic);
    if(targetHubId == 0){//no hub holds the topic
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] creates topic "<<theTopic);
        targetHubId = ArrangeHub(theTopic,howToArrangeHub);
    }
    subInfo.insert(std::make_pair(subId,subAddr));
    topic_sub.insert(std::make_pair(theTopic,std::make_pair(subId,0)));
    subId_topic.insert(std::pair<t_nodesNumberType, t_topicNumberType>(subId, theTopic));
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I notify hub["<<targetHubId<<"] topic "
                <<theTopic<<" has a new subscribe by sub["<<subId<<"]");
    hubList[targetHubId]->ConnectSub(theTopic,subId,subAddr);


    return GetHubAddress(targetHubId);
}

t_nodesNumberType
PersistenceManager::GetHub(t_topicNumberType requireTopic) {
    t_topic2hubMap::iterator itor = topic_hub.find(requireTopic);
    if (itor == topic_hub.end()) {
        return 0;
    } else {
        return itor->second;
    }
}


t_nodesNumberType
PersistenceManager::ArrangeHub(t_topicNumberType theTopic,balanceKind how){
    t_nodesNumberType targetHubId = DoArrange(theTopic,how);
    topic_hub[theTopic] = targetHubId;
    hub_topic.insert(std::make_pair(targetHubId,theTopic));
    return targetHubId;
}


Ipv4Address
PersistenceManager::GetHubAddress(t_nodesNumberType Id){
    t_hubId2hubMap::iterator hubItor = hubList.find(Id);
     if (hubItor != hubList.end()) {
         return hubItor->second->GetIp();
     } else {
         NS_LOG_ERROR("hub["<<Id<<"] does not exist.");
         exit(0);
     }
}

t_nodesNumberType
PersistenceManager::GetPubAddress(t_nodesNumberType id,std::set<Ipv4Address>* ipv4,std::set<t_nodesNumberType> *outputId){
    t_topicNumberType  topicNumber = hub_topic.count(id),temp = topicNumber;
    t_hub2topicMltMap::iterator itor = hub_topic.find(id);
    while(temp){
        ipv4->insert(topic_pubAddr.find(itor->second)->second);
        outputId->insert(topic_pubId.find(itor->second)->second);
    }
    return outputId->size();
}

t_nodesNumberType
PersistenceManager::GetSubAddress(t_nodesNumberType hubId, std::set<Ipv4Address>* ipv4,std::set<t_nodesNumberType> *outputId){
    t_topicNumberType topicNumber = hub_topic.count(hubId);
    t_hub2topicMltMap::iterator topicItor= hub_topic.find(hubId);
    while(topicNumber){
        t_nodesNumberType nodesNumber = topic_sub.count(topicItor->second);
        t_topic2subMltMap::iterator subItor = topic_sub.find(topicItor->second);
        while (nodesNumber) {
            if (outputId->insert(subItor->second.first).second) {
                ipv4->insert(subInfo.find(subItor->second.first)->second);
            }
        }
    }
    return outputId->size();
}

void
ns3::PersistenceManager::AddHub(HubApp* hub) {
    hubList.insert(std::pair<t_nodesNumberType,HubApp* >(hubNumber+1,hub));
    hubNumber++;
}

void
PersistenceManager::storeMessage(t_packetId theId,Ptr<Packet> thePacket){
    if(topic_packet.find(theId) == topic_packet.end()){
        topic_packet.insert(std::make_pair(theId,thePacket));
        topic_packetNum[theId.first]++;
    }
}

t_topicNumberType
PersistenceManager::GetHeldSubedTopic(t_nodesNumberType hubId,t_nodesNumberType subId,std::set<t_topicNumberType> *theTopics){
    t_subId2topicMltMap::iterator itor = subId_topic.find(subId);
    t_subId2topicMltMap::size_type topicNumber = subId_topic.count(subId),temp;
    temp = topicNumber;
    while(topicNumber){
        if(topic_hub.find(itor->second)->second != hubId){
            topicNumber--;
        }
        temp--;
        itor++;
    }
    return topicNumber;
}

Ptr<Packet>
PersistenceManager::GetPacket(t_packetNumberType packetId,t_topicNumberType topic){
    t_packetId id(topic,packetId);
    if(topic_packet.find(id) != topic_packet.end()){
        return topic_packet.find(id)->second;
    }
    return 0;
}

void
PersistenceManager::DeleteHub(t_nodesNumberType theId) {
}

void
PersistenceManager::NotifyHubReleaseopic(t_nodesNumberType hubId) {
    t_topicNumberType average = topic_hub.size()/hubNumber;
    t_topicNumberType topicHeldNow = hub_topic.count(hubId);
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I notify hub["<<hubId<<"] to release "
                <<topicHeldNow - average<<"topics");
    hubList[hubId]->DoBalance(topicHeldNow - average);

}

void
PersistenceManager::DoReleaseTopic(t_topicNumberType topicId) {
    //delete the relate of target topic and the hub hold it at the certain time
    t_topic2hubMap::iterator it = topic_hub.find(topicId);
    t_nodesNumberType hubId = 0;
    if(it != topic_hub.end()){
         hubId = it->second;
         topic_hub.erase(it);
    }else{
        NS_LOG_ERROR(this<<"can't find information of topic "<<topicId);
    }
    t_hub2topicMltMap::iterator itor = hub_topic.find(hubId);
    if(itor != hub_topic.end()){
        hub_topic.erase(itor);
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<"delete topic "<<topicId<<" information waiting for rearrange");
    }
}

Ipv4Address
PersistenceManager::ReconnectHub(t_topicNumberType theTopic,t_nodesNumberType pubId,const Ipv4Address& addr){
    t_nodesNumberType hubId = ArrangeHub(theTopic,TOPICBASED);
    hubList[hubId]->ReConnectPub(theTopic,pubId,addr);
    t_topic2subMltMap::iterator itor = topic_sub.find(theTopic);
    t_topic2subMltMap::size_type count = topic_sub.count(theTopic);
    while(count){
        hubList[hubId]->ReConnectSub(theTopic,(itor->second).first,subInfo[(itor->second).first]);
        count -- ;
        itor ++;
    }
    return GetHubAddress(hubId);
}

void
PersistenceManager::SetSendNumber(t_topicNumberType topic,t_nodesNumberType subId,t_packetNumberType secquence){
    t_topic2subMltMap::iterator itor = topic_sub.find(topic);
    t_topic2subMltMap::size_type count = topic_sub.count(topic);
    if(count == 0){
        NS_LOG_ERROR(this<<"can't find subscribe information of topic "<<topic);
        exit(1);
    }
    while(count != 0){
        if((itor->second).first == subId){
            (itor->second).second = secquence;
            break;
        }
        itor++;
        count--;
    }
    if(count == 0){
        NS_LOG_ERROR(this<<"can't find subscribe information of sub["<<subId<<"] on topic "<<topic);
        exit(1);
    }
}

t_packetNumberType
PersistenceManager::GetRcvNumber(t_topicNumberType theTopic){
    if(topic_packetNum.find(theTopic)!=topic_packetNum.end()){
        return topic_packetNum[theTopic];
    }else{
        return 0;
    }

}

t_packetNumberType
PersistenceManager::GetSendNumber(t_topicNumberType theTopic,t_nodesNumberType subId){
    t_topic2subMltMap::iterator itor = topic_sub.find(theTopic);
    t_topic2subMltMap::size_type count = topic_sub.count(theTopic);
    if(count == 0){
        NS_LOG_ERROR(this<<"can't find subscribe information of topic "<<theTopic);
        exit(1);
    }
    while(count != 0){
        if((itor->second).first == subId){
            return (itor->second).second;
        }
        itor++;
        count--;
    }
    if(count == 0){
        NS_LOG_ERROR(this<<"can't find subscribe information of sub["<<subId<<"] on topic "<<theTopic);
        exit(1);
    }
    return 0;
}

void
PersistenceManager::SetRcvNumberOfPub(t_topicNumberType theTopic,t_nodesNumberType pubId,t_packetNumberType packetNumber){
    std::pair<t_topicNumberType,t_nodesNumberType> combinedId(theTopic,pubId);
    pubTopic_packetNum[combinedId] = packetNumber;
}

t_packetNumberType
PersistenceManager::GetRcvNumberOfPub(t_topicNumberType theTopic,t_nodesNumberType pubId){
    std::pair<t_topicNumberType,t_nodesNumberType> combinedId(theTopic,pubId);
    if(pubTopic_packetNum.find(combinedId) != pubTopic_packetNum.end()){
        return pubTopic_packetNum[combinedId];
    }else{
        return 0;
    }
}

//-----------------------Balance----------------------------------------//

void
PersistenceManager::DoBalance(void){
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I load balance detection");
    t_topicNumberType average = topic_hub.size()/hubNumber;
    for(t_nodesNumberType i = 1;i<=hubNumber;i++){
        if(hub_topic.count(i)>average && hub_topic.count(i) > 1){
            // >1 in case of travel the only topic around hubs
            NotifyHubReleaseopic(i);
        }
    }
}


t_nodesNumberType
PersistenceManager::DoArrange(t_topicNumberType theTopic,balanceKind how){
    switch (how) {
    case TOPICBASED:
        return ArrangeBytopicNumber(theTopic);
    case IMBALANCE:
        return MakeImbalance(theTopic);
    case ROUNDROBIN:
        return ArrangeById(theTopic);
    default:
        return ArrangeById(theTopic);
    }
}

t_nodesNumberType
PersistenceManager::MakeImbalance(t_topicNumberType theTopic){
    //just make it imbalance by connect to single hub
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<"arrange hub to make it imbalance");
    return 1;
}

t_nodesNumberType
PersistenceManager::ArrangeById(t_topicNumberType theTopic){
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<"arrange hub by topic ID");
    return theTopic%hubNumber +1 ;
}

t_nodesNumberType
PersistenceManager::ArrangeBytopicNumber(t_topicNumberType theTopic){
    t_topicNumberType min = 100;
    t_nodesNumberType targetHub = 0;
    for(t_nodesNumberType i = 1;i<=hubNumber;i++){
        t_topicNumberType temp = hub_topic.count(i);
        if(temp < min){
            min = temp;
            targetHub = i;
        }
    }
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I arrange hub:"<<targetHub<<" by topic based load balance");
    return targetHub;
}

} //end of namespace ns3
