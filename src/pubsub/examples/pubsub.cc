/*
 * pubsub.cpp
 *
 *  Created on: Mar 3, 2014
 *      Author: zhaohui
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/pubsub-helper.h"
#include "ns3/pubsub-core.h"
#include "ns3/netanim-module.h"
#include <string>
const uint16_t HUBCAPACITY = 100;
const uint16_t BKCAPACITY = -1;

NS_LOG_COMPONENT_DEFINE("pubsub");

using namespace ns3;

void PrintDropedPacket(Ptr<const Packet>  thePacket){
    Ptr<Packet> packet = thePacket->Copy();
    Ipv4Header ipHeader;
    TcpHeader tcpHeader;
    packet->RemoveHeader(ipHeader);
    packet->RemoveHeader(tcpHeader);
    std::cout<<"packet ("<<ipHeader.GetSource()<<"-"
             <<ipHeader.GetDestination()<<":"<<tcpHeader.GetSequenceNumber()<<") dropped\n";
}

void PrintIpv4DropedPacket(const Ipv4Header &header, Ptr<const Packet> thePacket
        , Ipv4L3Protocol::DropReason reason, Ptr<Ipv4> ip, uint32_t interface){
    Ptr<Packet> packet = thePacket->Copy();
    Ipv4Header ipHeader;
    TcpHeader tcpHeader;
    packet->RemoveHeader(ipHeader);
    packet->RemoveHeader(tcpHeader);
    std::cout<<"ip packet ("<<ipHeader.GetSource()<<"-"
             <<ipHeader.GetDestination()<<":"<<tcpHeader.GetSequenceNumber()<<") dropped\n";
}

//void testTrace(Ptr<const Packet>, Ptr<Ipv4>,  uint32_t){
//    std::cout<<"testIpTrace\n";
//}

int main(int argc, char *argv[]) {

    CommandLine cmd;
    cmd.Parse (argc,argv);

    //1 is the router
    //total nodes number are up to 6553
    t_nodesNumberType nodesNumber = c_pubNumber + c_hubNumber + c_subNumber + 1;

    NodeContainer allNodes;
    allNodes.Create(nodesNumber);

    PointToPointHelper p2p;

    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", TimeValue(MicroSeconds(50)));



    InternetStackHelper internet;
    internet.InstallAll();
    for(uint32_t i = 0;i < allNodes.GetN();i++){
        allNodes.Get(i)->GetObject<Ipv4L3Protocol>()
                       ->TraceConnectWithoutContext("Drop",MakeCallback(&PrintIpv4DropedPacket));
    }


    //the last one used to contain all the device
    NetDeviceContainer deviceContainer[nodesNumber];
//
    Ipv4AddressHelper address;

    for(t_nodesNumberType i = 0; i<nodesNumber-1; i++){
        deviceContainer[i] = p2p.Install(allNodes.Get(0), allNodes.Get(i + 1));

        deviceContainer[i].Get(1)->TraceConnectWithoutContext("PhyRxDrop",MakeCallback(&PrintDropedPacket));
        deviceContainer[i].Get(0)->TraceConnectWithoutContext("PhyRxDrop",MakeCallback(&PrintDropedPacket));
        deviceContainer[i].Get(1)->TraceConnectWithoutContext("PhyTxDrop",MakeCallback(&PrintDropedPacket));
        deviceContainer[i].Get(0)->TraceConnectWithoutContext("PhyTxDrop",MakeCallback(&PrintDropedPacket));
        deviceContainer[i].Get(1)->TraceConnectWithoutContext("MacTxDrop",MakeCallback(&PrintDropedPacket));
        deviceContainer[i].Get(0)->TraceConnectWithoutContext("MacTxDrop",MakeCallback(&PrintDropedPacket));

        char addr[14];
        sprintf(addr, "1.%ld.%ld.0", i / 255, i % 255 + 10);
        Ipv4Address ip(addr);
        address.SetBase(ip, "255.255.255.0");
        address.Assign(deviceContainer[i]);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    PubsubHelper pubSub(c_pubNumber, c_hubNumber, c_subNumber,allNodes);
    if (c_pubSendInterval != 0) {
        pubSub.SetPubAttribute("SendInterval",TimeValue(MicroSeconds(c_pubSendInterval)));
    }
    if(c_hubSendInterval != 0){
        pubSub.SetHubAttribute("SecondThreshold",UintegerValue(c_hubSendInterval));
    }
    pubSub.SetHubAttribute("BkReadDelay",UintegerValue(c_BkreadDelay));
    pubSub.SetHubAttribute("cacheLength",UintegerValue(c_hubCacheLength));
    pubSub.SetHubAttribute("windowSize",UintegerValue(c_hubWindowCapacity));
    pubSub.Init();

//    for(t_nodesNumberType i=1; i<=c_pubNumber ;i++){
//        pubSub.Publish(i,i,1000);
//
//        pubSub.Subscribe(1,i);
//    }
    std::vector<pubInfoNode>::const_iterator itor = pubInfo.begin();
    for(;itor != pubInfo.end();itor++){
        pubSub.Publish(itor->pubId,itor->topicId,itor->packetNumber);
    }
    std::vector<std::pair<t_nodesNumberType,t_topicNumberType> >::const_iterator it = subInfo.begin();
    for(;it != subInfo.end();it++){
        pubSub.Subscribe(it->first,it->second);
    }
//    pubSub.Subscribe(1,1);
//    pubSub.Subscribe(1,2);
//    pubSub.Subscribe(1,3);
//    pubSub.Subscribe(1,4);


//    AsciiTraceHelper ascii;
//    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("hedwig.tr");
//    p2p.EnableAsciiAll (stream);

    pubSub.SetTime("pub",Seconds(0.1),Seconds(39.0));
    pubSub.SetTime("hub",Seconds(0.0),Seconds(39.0));
    pubSub.SetTime("sub",Seconds(0.0),Seconds(39.0));

    Simulator::Stop (Seconds (40));

    Simulator::Run ();
    Simulator::Destroy ();
    std::cout<<"===========================================================================\n";



    return 0;
}
