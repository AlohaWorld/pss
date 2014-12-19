/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/pubsub-helper.h"

NS_LOG_COMPONENT_DEFINE("pubsub-helper");

namespace ns3 {

PubsubHelper::PubsubHelper(
        t_nodesNumberType pubNumber, t_nodesNumberType hubNumber,
        t_nodesNumberType subNumber,const NodeContainer& nodes) {
    for(t_nodesNumberType i = 0;i<pubNumber;i++){
        pubFactory.push_back(ObjectFactory("ns3::PubApp"));
        //Id 0 is obligated for representing empty
        pubFactory[i].Set("Id",UintegerValue(i+1));
    }
    for(t_nodesNumberType i = 0;i<hubNumber;i++){
            hubFactory.push_back(ObjectFactory("ns3::HubApp"));
            hubFactory[i].Set("Id",UintegerValue(i+1));
        }
    for(t_nodesNumberType i = 0;i<subNumber;i++){
            subFactory.push_back(ObjectFactory("ns3::SubApp"));
            subFactory[i].Set("Id",UintegerValue(i+1));
        }
    if(nodes.GetN() != 0){
        if (nodes.GetN()-1 < pubNumber + hubNumber + subNumber) {
            NS_LOG_ERROR("by passing nodeContainer to PubsubHelper constructor,each node can have only one instance of pub hub or sub.\n"
                         <<"if you want to add more than one instance in one node ,separate the nodeContainer and use installPub/Sub.");
            exit(0);
        }
        for (t_nodesNumberType i = 0; i < pubNumber; i++) {
            pubNodes.Add(nodes.Get(i + 1));  //the first node is router
        }
        for (t_nodesNumberType i = pubNumber; i < pubNumber + hubNumber; i++) {
            hubNodes.Add(nodes.Get(i + 1));
        }
        for (t_nodesNumberType i = pubNumber + hubNumber;
                i < pubNumber + hubNumber + subNumber; i++) {
            subNodes.Add(nodes.Get(i + 1));
        }
    }
}

//void PubsubHelper::InitPub(const NodeContainer& nodes) {
//    for (t_nodesNumberType i = 0; i < nodes.GetN(); i++) {
//        pubFactory[i].SetTypeId("ns3:PubApp");
//    }
//    pubApps.Add(InstallPub(nodes));
//    Start("Pub",Seconds(0.01));
//    Stop("Pub",Seconds(10));
//}
//
//void PubsubHelper::InitHub(const NodeContainer& nodes) {
//    for (t_nodesNumberType i = 0; i < nodes.GetN(); i++) {
//        hubFactory[i].SetTypeId("ns3:HubApp");
//    }
//    hubApps.Add(InstallPub(nodes));
//    Start("Hub",Seconds(0.0));
//    Stop("Hub",Seconds(11));
//}
//
//void PubsubHelper::InitSub(const NodeContainer& nodes) {
//    for (t_nodesNumberType i = 0; i < nodes.GetN(); i++) {
//        subFactory[i].SetTypeId("ns3:SubApp");
//    }
//    subApps.Add(InstallPub(nodes));
//    Start("Sub",Seconds(0.02));
//    Stop("Sub",Seconds(10));
//}

void
PubsubHelper::Start(std::string kind,Time time){
    if("pub"==kind||"Pub"==kind){
        pubApps.Start(time);
    }else if("hub"==kind||"Hub"==kind){
        hubApps.Start(time);
    }else if("sub"==kind||"Sub"==kind){
        subApps.Start(time);
    }else{
        NS_LOG_ERROR("can't not set start time of "<<kind<<" use pub hub or sub.");
    }
}

void
PubsubHelper::Stop(std::string kind, Time time) {
    if ("pub" == kind || "Pub" == kind) {
        pubApps.Stop(time);
    } else if ("hub" == kind || "Hub" == kind) {
        hubApps.Stop(time);
    } else if ("sub" == kind || "Sub" == kind) {
        subApps.Stop(time);
    } else {
        NS_LOG_ERROR("can't not set stop time of "<<kind<<" use pub hub or sub.");
    }
}

void
PubsubHelper::SetTime(std::string kind,Time startTime,Time stopTime){
    Start(kind,startTime);
    Stop(kind,stopTime);
}


ApplicationContainer
PubsubHelper::Install (NodeContainer c,std::vector<ObjectFactory>  &factory){
    ApplicationContainer appc;
    t_nodesNumberType temp;
    uint8_t isOverflow = 0;
    if(c.GetN() < factory.size()){
        NS_LOG_ERROR(" Some Apps will be added into one node");
        isOverflow = 1;
    }
    for(t_nodesNumberType i =0;i<factory.size();i++){
        temp =( (isOverflow == 0)?i:i%c.GetN() );
        appc.Add( InstallPriv(c.Get(temp),factory[i]) );
    }
    return appc;
}

Ptr<Application>
PubsubHelper::InstallPriv (Ptr<Node> node,ObjectFactory &factory){
    factory.Set("IP",Ipv4AddressValue(node->GetObject<Ipv4> ()->GetAddress(1,0).GetLocal()));
    Ptr<Application> app = factory.Create<Application> ();
    node->AddApplication(app);
    return app;
}

ApplicationContainer
PubsubHelper::InstallPub (NodeContainer &c){
    pubApps.Add(Install(c,pubFactory));
    return pubApps;
}

ApplicationContainer
PubsubHelper::InstallHub (NodeContainer &c){
    if(hubFactory.size() > c.GetN()){
        NS_LOG_ERROR("can not create several hubs in one node");
        exit(0);
    }
    hubApps.Add(Install(c,hubFactory));
    return hubApps;
}

ApplicationContainer
PubsubHelper:: InstallSub (NodeContainer &c){
    subApps.Add(Install(c,subFactory));
    return subApps;
}

void
PubsubHelper::SetPubAttribute (std::string name, const AttributeValue &value, t_nodesNumberType id){
    if (id > pubFactory.size()){
        NS_LOG_ERROR("do not have pub["<<id<<"]");
        exit(0);
    }
    if(id == 0){
        for(std::vector<ObjectFactory>::size_type i = 0;i<pubFactory.size();i++){
            pubFactory[i].Set(name, value);
        }
    }
    pubFactory[id].Set (name, value);
}
void
PubsubHelper::SetHubAttribute (std::string name, const AttributeValue &value, t_nodesNumberType id){
    if (id > hubFactory.size()){
        NS_LOG_ERROR("do not have hub["<<id<<"]");
        exit(0);
    }
    if(id == 0){
        for(std::vector<ObjectFactory>::size_type i = 0;i<hubFactory.size();i++){
            hubFactory[i].Set(name, value);
        }
    }
    hubFactory[id].Set (name, value);
}
void
PubsubHelper::SetSubAttribute (std::string name, const AttributeValue &value, t_nodesNumberType id){
    if (id > subFactory.size()){
        NS_LOG_ERROR("do not have sub["<<id<<"]");
        exit(0);
    }
    if(id == 0){
        for(std::vector<ObjectFactory>::size_type i = 0;i<subFactory.size();i++){
            subFactory[i].Set(name, value);
        }
    }
    subFactory[id].Set (name, value);
}

void
PubsubHelper::Publish(t_nodesNumberType pubId,t_topicNumberType topic,t_packetNumberType number){
    if(0 == pubApps.GetN()){
        NS_LOG_ERROR("you must install the app first");
        exit(0);
    }else if(pubId > pubApps.GetN()){
        NS_LOG_ERROR("do not have pub["<<pubId<<"]");
        exit(0);
    }

    Ptr<PubApp> pub = DynamicCast<PubApp> ( pubApps.Get(pubId-1) );
    pub->AddTopic(topic,number);
}
void
PubsubHelper::Subscribe(t_nodesNumberType subId,t_topicNumberType topic){
    if(0 == subApps.GetN()){
        NS_LOG_ERROR("you must install the app first");
        exit(0);
    }else if(subId > subApps.GetN()){
        NS_LOG_ERROR("do not have sub["<<subId<<"]");
        exit(0);
    }

    Ptr<SubApp> sub = DynamicCast<SubApp> ( subApps.Get(subId-1) );
    sub->AddTopic(topic);
}

void
PubsubHelper::Init(){
    InstallPub(pubNodes);
    InstallHub(hubNodes);
    InstallSub(subNodes);
}

} //end namespace ns3
