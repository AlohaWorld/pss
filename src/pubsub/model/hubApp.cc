/*
 * hubApp.cc
 *
 *  Created on: Mar 10, 2014
 *      Author: zhaohui
 */

#include "ns3/hubApp.h"
#include "ns3/persistence-manager.h"
namespace ns3{

NS_LOG_COMPONENT_DEFINE("pubsub-hub");
NS_OBJECT_ENSURE_REGISTERED (HubApp);

const uint16_t c_getPacketLengthInterval = 10;

TypeId
HubApp::GetTypeId(void) {
    static TypeId tid =TypeId("ns3::HubApp")
            .SetParent<Application>()
            .AddConstructor<HubApp>()
            .AddAttribute("Id","the unique id for each hub,do not change this manual",
                    UintegerValue(0),
                    MakeUintegerAccessor(&HubApp::hubId),
                    MakeUintegerChecker<uint32_t>(0))
            .AddAttribute("IP","hub IP",
                    Ipv4AddressValue(Ipv4Address("0.0.0.0")),
                    MakeIpv4AddressAccessor(&HubApp::hubIp),
                    MakeIpv4AddressChecker())
            .AddAttribute("capacity", "the capacity of hub's cache",
                    UintegerValue(100),
                    MakeUintegerAccessor(&HubApp::cacheCap),
                    MakeUintegerChecker<uint64_t>(1))
             .AddAttribute("cacheLength", "the capacity of hub's cache on byte",
                    UintegerValue(10485760),//10MB
                    MakeUintegerAccessor(&HubApp::cacheCapOnByte),
                    MakeUintegerChecker<uint64_t>(1))
            .AddAttribute("windowSize", "the up bound of hub send window",
                    UintegerValue(100),
                    MakeUintegerAccessor(&HubApp::hubSendWindowSize),
                    MakeUintegerChecker<uint64_t>(1))
            .AddAttribute("SecondThreshold", "The max packets number hub can send per second",
                    UintegerValue(10000000),//10Mpps
                    MakeUintegerAccessor(&HubApp::packetPerSecondThreshold),
                    MakeUintegerChecker<uint64_t>(1))
            .AddAttribute("BkReadDelay", "delay of reading a packet from bk",
                    UintegerValue(50),
                    MakeUintegerAccessor(&HubApp::readDelay),
                    MakeUintegerChecker<uint16_t>(1));
    return tid;
}

HubApp::HubApp():
      hubStatus(),
      RcvMsgSocket(0),
      RcvAckSocket(0),
      pub2socketMap(),
      sub2socketMap(),
      topic2publistMap(),
      RMsgAddress(),
      RAckAddress(),
      hubRcvBit(0),
      hubRcvPacket(0),
      hubRcvPacketPerPubTopic(),
      hubRcvPacketPerTopic(),
      hubSendPacketPerSubTopic(),
      temp(),
      toSend(),
      cache(),
      cacheCount(),
      cacheSizeNow(0),
      cacheOrder(),
      hubSendWindow(),
      hubSendBit(0),
      hubSendPacket(0),
      delay(),
      topicList(),
      hubSendInterval(0),
      getToSendPacketsNumber(),
      tempAck() {
    persistence = PersistenceManager::GetPersistenceManager();
    persistence->AddHub(this);
}

HubApp::~HubApp(){
    pub2socketMap.clear();
    sub2socketMap.clear();
    topic2publistMap.clear();
    hubRcvPacketPerPubTopic.clear();
    hubRcvPacketPerTopic.clear();
    hubSendPacketPerSubTopic.clear();
    temp.clear();
    toSend.clear();
    cache.clear();
    cacheCount.clear();
    cacheOrder.clear();
    hubSendWindow.clear();
    delay.clear();
    topicList.clear();
    tempAck.clear();
}


void
HubApp::StartApplication(void) {
    //the message receiving part
    RMsgAddress = Address(InetSocketAddress(Ipv4Address::GetAny(), c_messagePort));
    RcvMsgSocket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());

    RcvMsgSocket->Bind(RMsgAddress);
    RcvMsgSocket->Listen();
    RcvMsgSocket->ShutdownSend();
    RcvMsgSocket->SetRecvCallback(MakeCallback(&HubApp::RcvMsg, this));
    RcvMsgSocket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
            MakeCallback(&HubApp::Accept, this));

    //the ACK receiving part
    RAckAddress = Address(InetSocketAddress(Ipv4Address::GetAny(), c_ackPort));
    RcvAckSocket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());

    RcvAckSocket->Bind(RAckAddress);
    RcvAckSocket->Listen();
    RcvAckSocket->ShutdownSend();
    RcvAckSocket->SetRecvCallback(MakeCallback(&HubApp::RcvAck, this));
    RcvAckSocket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
            MakeCallback(&HubApp::AcceptAck, this));

    randomSendInterval = ExponentialVariable(hubSendInterval.GetMicroSeconds());

    char middleTemp[17];
    sprintf(middleTemp, "_list_hub%lu", hubId);
    std::string fileName = c_prefileName + middleTemp;
    wfile.open(fileName.c_str());
    getToSendPacketsNumber.SetFunction(&HubApp::GetToSendPacketNumber,this);
    getToSendPacketsNumber.SetArguments(true);
    getToSendPacketsNumber.Schedule(MilliSeconds( c_balanceON?(c_getPacketLengthInterval+c_balanceTime):c_getPacketLengthInterval ) );

    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] start");
}

void
HubApp::StopApplication(void) {
    getToSendPacketsNumber.Cancel();
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] stopped and totally received "
            <<hubRcvPacket<<" messages of "<<hubRcvBit<<" Bytes message.Send "<<hubSendPacket<<" of "<<hubSendBit<<" Bytes.");
}



void
HubApp::RcvMsg(Ptr<Socket> socket) {
//    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] socket receive queue has "
//                <<socket->GetRxAvailable()<<" bytes");
    Address from;
    Ptr<Packet> packet;
    while((packet = socket -> RecvFrom(from))){
        if (packet->GetSize() == 0) {
            break;
        }
        hubRcvBit += packet->GetSize();
        RebuildPac(packet, from);
    }
//    NS_LOG_DEBUG(Simulator::Now().GetSeconds()<<": I out RcvMsg hub["<<hubId<<"] socket receive queue has "
//                <<socket->GetRxAvailable()<<" bytes");
}

void
HubApp::RebuildPac(Ptr<Packet> p, Address from){
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    t_addrAndPort addr(address.GetIpv4(),address.GetPort());
    if (temp.find(addr) == temp.end()) {
            temp.insert(std::make_pair(addr,p->Copy()));
        } else {
            temp[addr]->AddAtEnd(p);
        }

    MsgHeader header;
    while (temp[addr]->GetSize() > header.GetSerializedSize()) {
        //full head has been received,just peek it to see the packet size do not remove it!
        temp[addr]->PeekHeader(header);
        //full packet has been received
        if (temp[addr]->GetSize() >= header.GetContentSize() + header.GetSerializedSize() ) {
            temp[addr]->RemoveHeader(header);
            t_topicId topicId(header.GetTopic(), header.GetSenderId());
            //check the sequence
            if (hubRcvPacketPerPubTopic[topicId] < header.GetSequence()) {
                //if the topic is not held by this hub , tell pub to resend
                if (topicList.find(header.GetTopic()) == topicList.end()) {
                    SendAck(header, true);
                } else {
                    NS_LOG_INFO(Simulator::Now().GetSeconds()
                            <<": r hub["<<hubId<<"] receive the "<<hubRcvPacketPerTopic[header.GetTopic()]+1
                            <<" packet of topic "<<header.GetTopic()
                            <<" and sequence is "<<header.GetSequence()
                            <<" and content size is "<<header.GetContentSize());

                    ++hubRcvPacketPerPubTopic[topicId];
                    ++hubRcvPacketPerTopic[header.GetTopic()];
                    ++hubRcvPacket;
                    SendAck(header);
                    header.SetHubRTime(Simulator::Now().GetMicroSeconds());
                    t_topicNumberType tempTopic = header.GetTopic();
                    //hub reset the sequence according to all the pub packets under this topic
                    header.SetSequence(hubRcvPacketPerTopic[tempTopic]);
                    DoPersistence(header);
                }
            }

            temp[addr]->RemoveAtStart(header.GetContentSize());
            Transport(header.GetTopic());
        } else {
            NS_LOG_INFO(Simulator::Now().GetSeconds()
                        <<": r hub["<<hubId<<"] topic "<<header.GetTopic()
                        <<"("<<header.GetSequence()<<") is waiting for other partes.");
            break;
        }
    } //end of while
}

void
HubApp::Transport(t_topicNumberType topicId){
    std::map<t_topicNumberType, std::set<t_nodesNumberType> >::iterator itor =
            topicList.find(topicId);
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] check the transport situation of topic:"<<topicId);
    if (itor != topicList.end()) {
        std::set<t_nodesNumberType>::iterator end = itor->second.end();
        for (std::set<t_nodesNumberType>::iterator subItor =itor->second.begin() ; subItor != end ; subItor++) {
            t_topicId tempId(topicId,*subItor);
//            if (!delay[tempId].IsRunning()) {
            if (hubStatus[tempId] == READY) { //if not in the sending circulation, just start it
                NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] schedule to send packet of topic:"<<topicId);
                Send2SubWithTopic(tempId);
            }else{
                NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] topic:"<<
                            topicId<<" transport of sub["<<*subItor<<"] is in situation "
                            <<hubStatus[tempId]);
            }
        }//end of all sub
        //remove packet from cache
    }
}


//send ACK packet to pub. if isReSend is set to true means this ACK packet is a resend packet
//isReSend default set to false
void
HubApp::SendAck(MsgHeader theHeader , bool isReSend) {
    //find ACK socketDoPersistence
    t_topicId combinedId(theHeader.GetTopic(),theHeader.GetSenderId());
    std::map<t_topicId,Ptr<Socket> >::iterator itor = pub2socketMap.find(combinedId);
    //if the socket has been deleted ,which means RESEND has been sent,just return
    if(itor == pub2socketMap.end() ){
        return;
    }
    //create a head
    CtrlHeader ackHeader;
    if(isReSend){
        ackHeader.SetKind(RESEND);
    }else{
        ackHeader.SetKind(HUBACK);
    }
    ackHeader.SetSenderId(hubId);
    ackHeader.SetSequenceTillNow(theHeader.GetSequence());
    ackHeader.SetTopic(theHeader.GetTopic());

    Ptr<Packet> packet = Create<Packet>(1);
    packet -> AddHeader(ackHeader);
    //send ack
    itor->second->Send(packet);
    if (isReSend) {
        if (itor != pub2socketMap.end()) {
                (itor->second)->Close();
                pub2socketMap.erase(itor);
            }
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": - hub["<<hubId<<"] send RESEND of topic:"
                    <<theHeader.GetTopic()<<" at sequence "<<theHeader.GetSequence());
    } else {
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": - hub["<<hubId<<"] send ACK of:"<<theHeader.GetTopic()
                <<"("<<theHeader.GetSequence()<<")");
    }
}


void
HubApp::DoPersistence(MsgHeader theHeader){
    //recreate the packet
    Ptr<Packet> packet= Create<Packet>(theHeader.GetContentSize());
    t_topicId id(theHeader.GetTopic(),theHeader.GetSequence());
    packet->AddHeader(theHeader);
//    if(cache.size() == cacheCap){
    while(cacheSizeNow + packet->GetSize() > cacheCapOnByte){
        if(cacheOrder.empty()){
            NS_LOG_ERROR("cache capacity("<<cacheCapOnByte<<") is too small to hold packet of size "<<packet->GetSize());
            exit(1);
        }
        t_topicId toErase = cacheOrder.front();
        cacheSizeNow -= cache.find(toErase)->second->GetSize();
        cache.erase(toErase);
        cacheCount.erase(toErase);
        cacheOrder.pop_front();
    }

    cache.insert(std::pair<t_topicId,Ptr<Packet> >(id,packet));
    cacheSizeNow += packet->GetSize();
    //record order
    cacheOrder.push_back(id);
    cacheCount[id] = topicList[id.first].size();
    persistence->storeMessage(id,packet);
}

void
HubApp::RcvAck(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet;
    CtrlHeader header;
    while((packet = socket -> RecvFrom(from))){
        InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
        t_addrAndPort addr(address.GetIpv4(),address.GetPort());
        if(tempAck[addr] == 0){
            tempAck[addr] = packet->Copy();
        } else {
            tempAck[addr]->AddAtEnd(packet);
        }
        while (tempAck[addr]->GetSize() >= header.GetSerializedSize() + 1) {
            tempAck[addr]->RemoveHeader(header);
            tempAck[addr]->RemoveAtStart(1);
            t_topicId id(header.GetTopic(), header.GetSenderId());
            //erase all packet number < sequence
            std::set<t_packetNumberType>::iterator it = hubSendWindow[id].find(
                    header.GetSequenceTillNow());
            hubSendWindow[id].erase(hubSendWindow[id].begin(), it);
            if(hubStatus[id] == WAITACK){
                hubStatus[id] = RUNNING;
                Time tNext(MicroSeconds(randomSendInterval.GetValue()));
                Simulator::Schedule(tNext, &HubApp::Send2SubWithTopic, this, id);
            }
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] receive ACK of topic "<<header.GetTopic()<<"("
                    <<header.GetSequenceTillNow()<<") from sub["<<header.GetSenderId()
                    <<"].The window length now is "<<hubSendWindow[id].size());
        }
    }
}
void
HubApp::Accept(Ptr<Socket> s ,const Address& from){
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] accept a socket from pub");
    s->SetRecvCallback (MakeCallback (&HubApp::RcvMsg, this));
}

void
HubApp::AcceptAck(Ptr<Socket> s ,const Address& from){
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] accept a new ACK socket");
    s->SetRecvCallback (MakeCallback (&HubApp::RcvAck, this));
}

void
HubApp::SendRebuild (Ptr<Socket> socket, uint32_t txSpace)
{
    std::map<t_topicId,Ptr<Socket> >::iterator itor;
    for(itor = sub2socketMap.begin();itor != sub2socketMap.end();itor++){
        if(itor->second == socket){
            break;
        }
    }
    if(itor == sub2socketMap.end()){
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] a topic is waiting for rearrange");
    }
    if(hubStatus[itor->first] == WAITING){
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] socket queue of topic:"
                                <<itor->first.first<<" is available now.");
        DoSendMsg(itor->first);
    }
}

void
HubApp::OwnTopic(t_topicNumberType topic,t_nodesNumberType subId){
    if (topicList.find(topic) == topicList.end()){
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] own topic "<<topic);
        std::cout<<"hub["<<hubId<<"] ++ topic"<<topic<<std::endl;
        topicList.insert(std::make_pair(topic, std::set<t_nodesNumberType>()));
        hubRcvPacketPerTopic[topic] = persistence->GetRcvNumber(topic);
        topic2publistMap.insert(std::make_pair( topic,std::list<t_nodesNumberType>() ));
    }
    if (subId) {
        topicList[topic].insert(subId);
    }
    CaculatSendInterval();
}

void
HubApp::ConnectPub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address) {

    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] connecting pub["<<id<<"]");
    OwnTopic(topic);
    topic2publistMap[topic].push_back(id);
    Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    Address addr = Address(InetSocketAddress(address, c_ackPort));
    socket->Bind();
    socket->Connect(addr);

    t_topicId combinedId(topic,id);
    pub2socketMap.insert(std::pair<t_topicId, Ptr<Socket> >(combinedId, socket));
    hubRcvPacketPerPubTopic[combinedId] = persistence->GetRcvNumberOfPub(topic,id);

/*    releaseSocket[combinedId].SetFunction(&HubApp::DeleteSocket,this);
    releaseSocket[combinedId].SetArguments(combinedId)*/;
}

void
HubApp::ReConnectPub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address){
    t_topicId combinedId(topic,id);
    ConnectPub(topic,id,address);
}


void
HubApp::ConnectSub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address){
    OwnTopic(topic,id);
    Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    Address addr = Address(InetSocketAddress(address, c_messagePort));
    socket->Bind();
    socket->Connect(addr);
    socket->ShutdownRecv();
    socket->SetSendCallback(
            MakeCallback(&HubApp::SendRebuild, this));
    socket->SetConnectCallback(
            MakeCallback(&HubApp::ConnectionSucceeded, this),
            MakeCallback(&HubApp::ConnectionFailed, this));

    t_topicId combinedId(topic,id);
    sub2socketMap.insert(std::make_pair(combinedId,socket));
    if(hubStatus.find(combinedId) !=hubStatus.end() && hubStatus[combinedId] == RESENDING){
        hubSendPacketPerSubTopic[combinedId] = persistence->GetSendNumber(topic,id);
    } else {
        hubSendPacketPerSubTopic[combinedId] = hubRcvPacketPerTopic[topic];
        hubStatus[combinedId] = READY;
    }
    hubSendWindow.insert(std::make_pair(std::make_pair(topic,id),std::set<t_packetNumberType>()));
    delay[combinedId].SetFunction(&HubApp::DoSendMsg,this);
    delay[combinedId].SetArguments(combinedId);
}

void
HubApp::ReConnectSub(t_topicNumberType topic,t_nodesNumberType id,const Ipv4Address& address){
    hubStatus[std::make_pair(topic,id)] = RESENDING;
    ConnectSub(topic,id,address);
}


void
HubApp::Send2SubWithTopic(t_topicId subId){
    if(hubStatus[subId] == WAITING || hubStatus[subId] == WAITACK){
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["
                    <<hubId<<"] socket to sub["<<subId.second<<"] of topic "
                    <<subId.first<<" is waiting for ACK");
        return;
    }
    if(hubSendPacketPerSubTopic[subId] >= hubRcvPacketPerTopic[subId.first]){
        hubStatus[subId] = READY;
        return;
    }
    if(!delay[subId].IsRunning()){//is sending previous packet
        uint64_t delayTime = 0;
        t_packetNumberType toSendSeq = hubSendPacketPerSubTopic[subId] + 1;
        toSend[subId] = ReadPacket(subId.first, toSendSeq, delayTime);
        if (toSend[subId] != 0) {
            //delay here is the packet reading delay
            delay[subId].SetDelay(MicroSeconds(delayTime));
            delay[subId].Schedule();
        } else {
            NS_LOG_ERROR(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] topic:"<<subId.first<<"has no subscribe");
            exit(1);
        }
    }

//    if (hubStatus[subId] != READY) {
//        Time tNext(MicroSeconds(randomSendInterval.GetValue()));
//        Simulator::Schedule(tNext, &HubApp::Send2SubWithTopic, this, subId);
//    }
}

Ptr<Packet>
HubApp::ReadPacket(t_topicNumberType theTopic,t_packetNumberType seq,uint64_t & delay){
    //this topic has no subscribe
    if(topicList.find(theTopic) == topicList.end()||
       topicList.find(theTopic)->second.empty()){
        return 0;
    }else{//have subscribe
        t_packetId id(theTopic,seq);
        if(cache.find(id) != cache.end()){//packet is in cache list
            Ptr<Packet> packet = cache[id];
//            if (cacheCount[id] == 1) {
//                cacheOrder.remove(id);
//            }
            return packet;
        }else{//packet is not in cache list

            delay += readDelay;
            MsgHeader header;
            Ptr<Packet> packet = persistence->GetPacket(seq, theTopic);
            if(packet == 0){
                NS_LOG_ERROR(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] can't find packet:"<<seq
                            <<" of topic:"<<theTopic);
                exit(1);
            }else{
                NS_LOG_INFO(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] pick packet:"<<seq
                            <<" of topic:"<<theTopic<<" from persistence layer.the cache length:"<<
                            cache.size());
            }
            packet -> RemoveHeader(header);
            header.SetFromBK(1);
            packet->AddHeader(header);
            /*put the packet into cache.
              So if the left space in cache is not large enough ,delete the earliest packets until
              space is enough.
            */
//            if (cache.size() == cacheCap) {
            while (cacheSizeNow >= cacheCapOnByte) {
                t_packetId deleteId = cacheOrder.front();
                cacheSizeNow -= cache[deleteId]->GetSize();
                cache.erase(deleteId);
                cacheCount.erase(deleteId);
                cacheOrder.pop_front();
            }

            cache[id] = packet;
            cacheSizeNow += packet->GetSize();
            cacheCount[id] = topicList[id.first].size();
            cacheOrder.push_back(id);
            return packet;
        }
    }

}

void
HubApp::DoSendMsg(t_topicId id) {
    if(hubSendWindow[id].size() == hubSendWindowSize){
        if(hubStatus[id] != WAITACK){//this log just print send window is full at the first time
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": hub["<<hubId<<"] sendwindow to"
                    <<" sub["<<id.second<<"] topic:"<<id.first<<" is full stop sending");
            hubStatus[id] = WAITACK;
        }
        return;
    }
    Ptr<Socket> socket = sub2socketMap[id];
    Ptr<Packet> packet = toSend[id];
    if(socket->GetTxAvailable() >= packet->GetSize()){
        hubStatus[id] = RUNNING;

        MsgHeader header;
        packet->RemoveHeader(header);
        header.SetHubSTime(Simulator::Now().GetMicroSeconds());
        header.SetHubCacLen(cache.size());
        header.SetHubSendWindowLen(hubSendWindow[id].size());
//        header.SetHubTosend(131072 - socket->GetTxAvailable());
        packet->AddHeader(header);
        socket->Send(packet);
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": - hub["<<hubId<<"] send "
                <<header.GetSequence()<<" of "<<packet->GetSize()
                <<" bytes to sub["<<id.second<<"](topic "<<header.GetTopic()<<")");

        //update several queues
        t_packetId tempId(header.GetTopic(),header.GetSequence());
        if (--cacheCount[tempId] == 0) {
            //just delete once , in case of sevel sub subs one topic
            cache.erase(tempId);
            cacheCount.erase(tempId);
            cacheSizeNow -= packet->GetSize();
            cacheOrder.remove(tempId);
        }
        hubSendWindow[id].insert(header.GetSequence());
        ++hubSendPacketPerSubTopic[id];
        hubSendBit += packet->GetSize();
        ++hubSendPacket;
        if (hubSendPacketPerSubTopic[id] < hubRcvPacketPerTopic[header.GetTopic()]) {
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId
                    <<"] schedule to send next packet of topic:"<<header.GetTopic());
            Time tNext(MicroSeconds(randomSendInterval.GetValue()));
            Simulator::Schedule(tNext, &HubApp::Send2SubWithTopic, this, id);
        }else{
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId
                                <<"] send all received packets of topic:"<<header.GetTopic()<<" waiting for next packets.");
            hubStatus[id] = READY;
        }
    } else {
        hubStatus[id] = WAITING;
    }

}

void
HubApp::DeleteSocket(t_topicNumberType theTopic){
//    //for pub
//    std::map<t_topicId,Ptr<Socket> >::iterator itor = pub2socketMap.begin();
    bool found = false;
//    while(itor != pub2socketMap.end()){
//        if ((itor->first).first == theTopic) {
//            itor->second->Close();
//            pub2socketMap.erase(itor++);
//            found = true;
//        } else if (found) {
//            break;
//        }else {
//            itor++;
//        }
//    }
    //for sub
    std::map<t_topicId,Ptr<Socket> >::iterator itor = sub2socketMap.begin();
    found = false;
    while(itor != sub2socketMap.end()){
        if ((itor->first).first == theTopic) {
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": - hub["<<hubId<<"] delete socket to sub["
                    <<(itor->first).second<<"] and sent "<<hubSendPacketPerSubTopic[itor->first]<<"packets");
            hubSendPacketPerSubTopic[itor->first] = 0;
            itor->second->Close();
            sub2socketMap.erase(itor++);
            found = true;
        } else if (found) {
            break;
        }else {
            itor++;
        }
    }
}

void
HubApp::DoBalance(t_topicNumberType topicNumber){
    while(topicNumber){//how many topic need to be removed
        std::map<t_topicNumberType,std::set<t_nodesNumberType> >::iterator itor = topicList.begin();
        t_topicNumberType totalTopic = topicList.size();
        if(topicNumber > totalTopic){
            NS_LOG_ERROR(this<<Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] don't have "
                             <<topicNumber<<" topics to be released.");
            exit(1);
        }
        //delete  random topics
        //and here we can add some strategies to pick up certain topics
        uint32_t moveStaps = UniformVariable().GetInteger(0, totalTopic - 1);
        while(moveStaps){
            itor ++;
            moveStaps--;
        }
        t_topicNumberType topicId = itor->first;

        //record how many packets have been sent to certain subs of this topic so far
        std::set<t_nodesNumberType>::iterator subItor = (itor->second).begin();
        for(;subItor != (itor->second).end();subItor++){
            t_packetNumberType leftPacket = hubSendWindow[std::make_pair(topicId,*subItor)].size();
            t_packetNumberType sendedNumber = hubSendPacketPerSubTopic.find( std::make_pair(topicId,*subItor) )->second;
            persistence->SetSendNumber(topicId,*subItor,sendedNumber - leftPacket);

            //stop sending
            delay[std::make_pair(topicId,*subItor)].Cancel();
        }
        //record how many packets have been received by a certain pub of a certain topic
        std::list<t_nodesNumberType>::iterator pubItor = topic2publistMap[topicId].begin();
        for(;pubItor != topic2publistMap[topicId].end() ; pubItor++){
            t_packetNumberType packetNumber = hubRcvPacketPerPubTopic[std::make_pair(topicId,*pubItor)];
            persistence->SetRcvNumberOfPub(topicId,*pubItor,packetNumber);
        }

        //close all socket related to this topic
        DeleteSocket(topicId);
        NS_LOG_INFO(this<<Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] released  the topic:"<<topicId);
        std::cout<<"hub["<<hubId<<"] -- topic"<<topicId<<std::endl;
        topicList.erase(topicId);
        hubRcvPacketPerTopic[topicId] = 0;
        persistence->DoReleaseTopic(topicId);
        topicNumber --;
    }
    CaculatSendInterval();
}

void HubApp::ConnectionSucceeded(Ptr<Socket> socket) {
    std::map<t_topicId,Ptr<Socket> >::iterator itor;
    //find key-topic using value-socket in map pubsendSocket
    for(itor = sub2socketMap.begin();itor != sub2socketMap.end();itor++){
        if(itor->second == socket){
            break;
        }
    }
    //start the topic's sending
    if(itor != sub2socketMap.end()){
        if (hubStatus[itor->first] == RESENDING) {
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I hub["<<hubId<<"] resending topic:"
                        <<(itor->first).first<<" to sub["<<(itor->first).second<<"]");
//            Send2SubWithTopic(itor->first);
            Time tNext(MicroSeconds(randomSendInterval.GetValue()));
            Simulator::Schedule(tNext, &HubApp::Send2SubWithTopic, this, itor->first);
        }else{
            NS_LOG_INFO("hub["<<hubId<<"] sending socket get a successfully connect");
        }
    }
}

void HubApp::ConnectionFailed(Ptr<Socket> socket) {
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<":I hub["<<hubId<<"] do connecting failed ");
}

void
HubApp::GetToSendPacketNumber(bool firstTime){
    if(!wfile.is_open()){
        NS_LOG_ERROR("hub["<<hubId<<"] can\'t open packet length file.");
        exit(1);
    }
    std::map<t_topicId,t_packetNumberType>::iterator itor = hubSendPacketPerSubTopic.begin();
    std::map<t_topicId,t_packetNumberType>::iterator end = hubSendPacketPerSubTopic.end();
    if(firstTime){
        wfile<<"timeStap  ";
        for (; itor != end; itor++) {
            wfile<<"topic"<<itor->first.first<<"_sub"<<itor->first.second<<"  ";
        }
        wfile<<"  total" <<std::endl;
        getToSendPacketsNumber.SetArguments(false);
        itor = hubSendPacketPerSubTopic.begin();
    }
    wfile<<Simulator::Now().GetMicroSeconds()<<"  ";
    t_packetNumberType total = 0;
    for(; itor != end; itor++){
        t_packetNumberType received = hubRcvPacketPerTopic.find( (itor->first).first )->second;
        wfile<<received - itor->second<<"  ";
        total += (received - itor->second);
    }
    wfile<<total<<std::endl;
    getToSendPacketsNumber.Schedule(MilliSeconds(c_getPacketLengthInterval));
}

void
HubApp::CaculatSendInterval(){
    std::map<t_topicNumberType,std::set<t_nodesNumberType> >::iterator itor = topicList.begin();
    std::map<t_topicNumberType,std::set<t_nodesNumberType> >::iterator end = topicList.end();
    double count = 0 ;
    for(;itor != end ; itor++){
        count += itor->second.size();
    }
    double temp = (count*1000000)/ packetPerSecondThreshold;
    hubSendInterval = MicroSeconds(temp >1?temp:1);//avoid 0 delay
    randomSendInterval = ExponentialVariable(hubSendInterval.GetMicroSeconds());
}


} //namespace ns3
