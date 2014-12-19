/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PUBSUB_HELPER_H
#define PUBSUB_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/pubApp.h"
#include "ns3/hubApp.h"
#include "ns3/subApp.h"
#include <vector>
typedef uint64_t t_nodesNumberType;

namespace ns3 {
class PubsubHelper {
private:
    PubsubHelper() {};
    NodeContainer pubNodes;
    NodeContainer hubNodes;
    NodeContainer subNodes;
    ApplicationContainer pubApps;
    ApplicationContainer hubApps;
    ApplicationContainer subApps;
    void InitPub(const NodeContainer &nodes);
    void InitHub(const NodeContainer &nodes);
    void InitSub(const NodeContainer &nodes);
    std::vector<ObjectFactory> pubFactory,hubFactory,subFactory;

    ApplicationContainer Install (NodeContainer c,std::vector<ObjectFactory>  &factory);
    Ptr<Application> InstallPriv (Ptr<Node> node,ObjectFactory &factory);
public:

    PubsubHelper(t_nodesNumberType pubNumber,t_nodesNumberType hubNumber,
            t_nodesNumberType subNumber,const NodeContainer &nodes = NodeContainer());
    ApplicationContainer GetPubApps() {
        return pubApps;
    }
    ApplicationContainer GetHubApps() {
        return hubApps;
    }
    ApplicationContainer GetSubApps() {
        return subApps;
    }
//    void AddPub(const Ptr<Node> node);
//    void AddHub(const Ptr<Node> node);
//    void AddSub(const Ptr<Node> node);
    void Start(std::string kind,Time time);
    void Stop(std::string kind,Time time);
    void SetTime(std::string kind,Time startTime,Time stopTime);

    ApplicationContainer InstallPub (NodeContainer &c) ;
    ApplicationContainer InstallHub (NodeContainer &c) ;
    ApplicationContainer InstallSub (NodeContainer &c) ;

    Ptr<Application> InstallPrivPub (Ptr<Node> node) ;
    Ptr<Application> InstallPrivHub (Ptr<Node> node) ;
    Ptr<Application> InstallPrivSub (Ptr<Node> node) ;

    void SetPubAttribute (std::string name, const AttributeValue &value, t_nodesNumberType id = 0);
    void SetHubAttribute (std::string name, const AttributeValue &value, t_nodesNumberType id = 0);
    void SetSubAttribute (std::string name, const AttributeValue &value, t_nodesNumberType id = 0);

    void Publish(t_nodesNumberType id,t_topicNumberType topic,t_packetNumberType number = c_defPacketNumber);
    void Subscribe(t_nodesNumberType id,t_topicNumberType topic);
    void Init();

};

} //end of namespace ns3

#endif /* PUBSUB_HELPER_H */

