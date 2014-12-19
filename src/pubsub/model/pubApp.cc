/*
 * pubApp.cc
 *
 *  Created on: Mar 10, 2014
 *      Author: zhaohui
 */
#include "ns3/pubApp.h"
#include "ns3/message-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("pubsub-pub");
NS_OBJECT_ENSURE_REGISTERED (PubApp);

TypeId PubApp::GetTypeId(void) {
    static TypeId tid =TypeId("ns3::PubApp")
            .SetParent<Application>()
            .AddConstructor<PubApp>()
            .AddAttribute("Id","the unique id for each pub,do not change this manual",
                    UintegerValue(0), MakeUintegerAccessor(&PubApp::pubId),
                    MakeUintegerChecker<uint32_t>(0))
            .AddAttribute("PacketSize","The average packet size sent in pub", UintegerValue(1000),
                    MakeUintegerAccessor(&PubApp::pubMsgSize),
                    MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("SendInterval", "The average send interval of pub",
                    TimeValue(MicroSeconds(35)),
                    MakeTimeAccessor(&PubApp::pubSendInterval),
                    MakeTimeChecker(MicroSeconds(1)))
            .AddAttribute("IP","pub IP",
                    Ipv4AddressValue(Ipv4Address("0.0.0.0")),
                    MakeIpv4AddressAccessor(&PubApp::pubIp),
                    MakeIpv4AddressChecker());
    return tid;
}

PubApp::PubApp() :
        topic(),
        pubStatus(),
        pubTotalSendedPackets(0),
        pubTotalSendedBit(0),
        packetSendedPerTopic(),
        bitsSendedPerTopic(),
        randomMsgSize(0),
        randomSendInterval(0),
        toSend(),
        pubRcvAckSocket(0),
        pubRcvAckAddress(),
        pubSendSocket(),
        hubAddr(),
        packetsToSend(),
        sendWindow(),
        resendList(),
        tempAck(){
//    if (theNode == 0) {
//        NS_LOG_ERROR("pub init error");
//        exit(0);
//    }
//    pubRcvAckSocket = Socket::CreateSocket(theNode,
//            TcpSocketFactory::GetTypeId());
//    pubRcvAckAddress = Address(InetSocketAddress(Ipv4Address::GetAny(), c_ackPort));
//
//    tempSendSocket = Socket::CreateSocket(theNode,
//            TcpSocketFactory::GetTypeId());

    persistenceManager = PersistenceManager::GetPersistenceManager();
}
PubApp::~PubApp() {
    sendWindow.clear();
    packetsToSend.clear();
    pubSendSocket.clear();
    toSend.clear();
    bitsSendedPerTopic.clear();
    packetSendedPerTopic.clear();
    pubStatus.clear();
    topic.clear();
    hubAddr.clear();
}

void PubApp::StartApplication(void) {
    //the ack receiving part
    pubRcvAckSocket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    pubRcvAckAddress = Address( InetSocketAddress(Ipv4Address::GetAny(), c_ackPort));

    pubRcvAckSocket->Bind(pubRcvAckAddress);
    pubRcvAckSocket->Listen();
    pubRcvAckSocket->ShutdownSend();
    pubRcvAckSocket->SetRecvCallback(MakeCallback(&PubApp::RcvAck, this));
    pubRcvAckSocket->SetAcceptCallback(
            MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
            MakeCallback(&PubApp::HandleAckAccept, this));

    //the packet sending part
    //there is no topic in this pub ,we set the topic the same as the pub id
    if (0 == topic.size()) {
        topic.insert(pubId);
    }


    for (std::set<t_topicNumberType>::iterator itor = topic.begin();
            itor != topic.end(); itor++) {
       ConnectHub(*itor);
    }

    randomMsgSize = ExponentialVariable(pubMsgSize, 2 * pubMsgSize);
    randomSendInterval = ExponentialVariable(pubSendInterval.GetMicroSeconds(),2*pubSendInterval.GetMicroSeconds());
    NS_LOG_DEBUG(Simulator::Now().GetSeconds()<<": pub["<<pubId<<"] start");
}

void PubApp::ConnectHub(t_topicNumberType theTopic){
    //create socket for each topic
    pubSendSocket.insert(std::make_pair(theTopic,Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId())));
    //notify the hub cluster and get the target hub address
    DoPublish(theTopic);
    pubStatus[theTopic] = RUNNING;
    pubSendSocket[theTopic]->Bind();
    pubSendSocket[theTopic]->Connect(hubAddr[theTopic]);
    pubSendSocket[theTopic]->ShutdownRecv();
    pubSendSocket[theTopic]->SetConnectCallback(
            MakeCallback(&PubApp::ConnectionSucceeded, this),
            MakeCallback(&PubApp::ConnectionFailed, this));
    pubSendSocket[theTopic]->SetSendCallback(
            MakeCallback(&PubApp::SendRestart, this));
}

void PubApp::Reconnect(t_topicNumberType theTopic){
    NS_LOG_DEBUG(Simulator::Now().GetSeconds()<<": pub["<<pubId<<"] reconnecting socket of topic:"<<theTopic);
    Ipv4Address ipv4 = persistenceManager->ReconnectHub(theTopic,pubId,pubIp);
    Address addr = Address(InetSocketAddress(ipv4, c_messagePort));
    hubAddr[theTopic] = addr;
    std::map<t_topicNumberType,Ptr<Socket> >::iterator itor = pubSendSocket.find(theTopic);
    if(itor != pubSendSocket.end()){
        itor->second->Close();
    }
    pubSendSocket[theTopic] = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    pubSendSocket[theTopic]->Bind();
    pubSendSocket[theTopic]->Connect(addr);
    pubSendSocket[theTopic]->ShutdownRecv();
    pubSendSocket[theTopic]->SetConnectCallback(
            MakeCallback(&PubApp::ConnectionSucceeded, this),
            MakeCallback(&PubApp::ConnectionFailed, this));
    pubSendSocket[theTopic]->SetSendCallback(
            MakeCallback(&PubApp::SendRestart, this));
    GetResendPacket(theTopic);
    pubStatus[theTopic] = WAITRECONNECT;
}

void PubApp::GetResendPacket(t_topicNumberType topic){
    std::map<std::pair<t_topicNumberType,t_packetNumberType>,Ptr<Packet> >::iterator itor ;
    bool finded = false;
    for (itor = sendWindow.begin(); itor != sendWindow.end(); itor++) {
        if((itor->first).first == topic){
            finded = true ;
            resendList[topic].push_back(itor->second);

            uint32_t tempSize = itor->second->GetSize();
            bitsSendedPerTopic[topic] -= tempSize;
            pubTotalSendedBit -= tempSize;
            packetSendedPerTopic[topic]--;
            pubTotalSendedPackets--;

        }else if(finded){
            break;
        }
    }
}

void PubApp::DoPublish(t_topicNumberType theTopic) {
    Ipv4Address IpAddress = persistenceManager->HandlePub(theTopic, pubId, pubIp);
    if (IpAddress != "0.0.0.0") {
        hubAddr[theTopic] = Address(InetSocketAddress(IpAddress, c_messagePort));
    }
}

void PubApp::StopApplication(void) {
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": pub["<<pubId<<"] stopped");
    NS_LOG_INFO("pub["<<pubId<<"] "<<pubTotalSendedPackets<<" "<<pubTotalSendedBit);
    for (std::set<t_topicNumberType>::iterator itor = topic.begin();
            itor != topic.end(); itor++) {
        NS_LOG_INFO("topic["<<*itor<<"] "<<packetSendedPerTopic[*itor]<<" "<<bitsSendedPerTopic[*itor]);
        pubStatus[*itor] = STOP;
        pubSendSocket[*itor]->Close();
    }
    pubRcvAckSocket->Close();
}

/*
void PubApp::Send(void) {
    for (std::set<t_topicNumberType>::iterator itor = topic.begin();
            itor != topic.end(); itor++) {
        Send2Topic(*itor);
    }
}
*/

void PubApp::Send2Topic(t_topicNumberType theTopic) {
    if(pubStatus[theTopic] == WAITRECONNECT){
        return;
    }
    if (packetSendedPerTopic[theTopic] < packetsToSend[theTopic]) {
        //the first packet to send
        if (0 == packetSendedPerTopic[theTopic]) {
            toSend.insert(std::pair<t_topicNumberType, Ptr<Packet> >(theTopic, MakePacket(theTopic)));
        }
        //if is resending case ,recover toSend packet with the resend one
        if(pubStatus[theTopic] == RESENDING){
            toSend[theTopic] = MakePacket(theTopic);
        }
//        MsgHeader header;
        if (pubSendSocket[theTopic]->GetTxAvailable() >= toSend[theTopic]->GetSize())
        {
            pubStatus[theTopic] = (pubStatus[theTopic] == RSEDWAIT)?RESENDING:RUNNING;
            //add pub send time to header
            MsgHeader header;
            toSend[theTopic]->RemoveHeader(header);
            header.SetSendTime(Simulator::Now().GetMicroSeconds());
            header.SetHubTosend(131072 - pubSendSocket[theTopic]->GetTxAvailable());
            toSend[theTopic]->AddHeader(header);

            pubSendSocket[theTopic]->Send(toSend[theTopic]);
            //update the recording information
            uint32_t tempSize = toSend[theTopic]->GetSize();
            bitsSendedPerTopic[theTopic] += tempSize;
            pubTotalSendedBit += tempSize;
            packetSendedPerTopic[theTopic]++;
            pubTotalSendedPackets++;

            //put the just sended message into sending buffer
            t_msgId msgId(theTopic, packetSendedPerTopic[theTopic]);
            sendWindow[msgId] = toSend[theTopic];
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": - pub["<<pubId<<"] send "
                    <<packetSendedPerTopic[theTopic]<<" of "<<toSend[theTopic]->GetSize()
                    <<" bytes to topic "<<theTopic);
            //the next send packet
            toSend[theTopic] = MakePacket(theTopic);
            if (packetSendedPerTopic[theTopic] < packetsToSend[theTopic]) {
                ScheduleNext(theTopic);
            }
        } else {//the sending buffer dosen't have enough place for the packet
            pubStatus[theTopic] = (pubStatus[theTopic] == RESENDING)?RSEDWAIT:WAITING;
        }

    }
}

void PubApp::ScheduleNext(t_topicNumberType theTopic) {
    if (pubStatus[theTopic] != STOP) {
        Time tNext(MicroSeconds(randomSendInterval.GetValue()));
        Simulator::Schedule(tNext, &PubApp::Send2Topic, this, theTopic);
    }
}

Ptr<Packet> PubApp::MakePacket(t_topicNumberType theTopic) {
    Ptr<Packet> packet = 0;
    if ( resendList.find(theTopic) != resendList.end()
         && resendList[theTopic].size() !=0) { // is in resending case
        packet = resendList[theTopic].front();
        resendList[theTopic].pop_front();
        MsgHeader header;
        packet->PeekHeader(header);
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I pub["<<pubId<<"] resend topic:"
                <<theTopic<<" at sequence"<<header.GetSequence());
    } else {//normal
        uint64_t size = randomMsgSize.GetValue();
        size++;                 //in case of packet size is 0.
        packet = Create<Packet>(size);
        MsgHeader header;
        header.SetKind(MESSAGE);
        header.SetTopic(theTopic);
        header.SetSenderId(pubId);
        header.SetSequence(packetSendedPerTopic[theTopic] + 1);
        header.SetContentSize(size);
        packet->AddHeader(header);
    }
    return packet;
}

void PubApp::SendRestart(Ptr<Socket> socket, uint32_t) {
    std::map<t_topicNumberType,Ptr<Socket> >::iterator itor;
    //find key-topic using value-socket in map pubsendSocket
    for(itor = pubSendSocket.begin();itor != pubSendSocket.end();itor++){
        if(itor->second == socket){
            break;
        }
    }
    //restart the topic's sending
    if(itor != pubSendSocket.end() &&
            (pubStatus[itor->first] == WAITING || pubStatus[itor->first] == RSEDWAIT)){
        Send2Topic(itor->first);
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I pub["<<pubId<<"] socket of topic:"
                        <<itor->first<<" start send again.");
    }
}

void PubApp::RcvAck(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet;
    CtrlHeader header;
    while ((packet = socket->RecvFrom(from))) {
        InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
        t_addrAndPort addr(address.GetIpv4(),address.GetPort());
        if(tempAck[addr] == 0){
            tempAck[addr] = packet->Copy();
        } else {
            tempAck[addr]->AddAtEnd(packet);
        }
        while(tempAck[addr]->GetSize() >= header.GetSerializedSize()+1){ // the ACK packet size is set to 1
            tempAck[addr]->RemoveHeader(header);
            tempAck[addr]->RemoveAtStart(1);
            if (header.GetKind() == RESEND) {
                NS_LOG_INFO(Simulator::Now().GetSeconds()<<": + pub["<<pubId<<"] received the RESENDING ACK of topic:"<<header.GetTopic());
                if (pubStatus[header.GetTopic()] != RESENDING) {
                    pubStatus[header.GetTopic()] = RESENDING;
                    Reconnect(header.GetTopic());
                } else {
                    return;
                }
            } else {
                t_msgId msgId(header.GetTopic(), header.GetSequenceTillNow());
                NS_LOG_INFO(Simulator::Now().GetSeconds()<<": + Pub["<<pubId<<"] received the "
                        <<header.GetSequenceTillNow()<<" packet ACK of topic:"<<header.GetTopic());
                if (sendWindow.find(msgId) != sendWindow.end()) {
                    sendWindow.erase(msgId);
                }
            }
        }
    }
}
void PubApp::HandleAckAccept(Ptr<Socket> s, const Address& from) {
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I pub["<<pubId<<"] accept a new ACK socket");
    s->SetRecvCallback(MakeCallback(&PubApp::RcvAck, this));
}

void PubApp::ConnectionSucceeded(Ptr<Socket> socket) {
    NS_LOG_INFO("pub["<<pubId<<"] socket get a successfully connect");
    std::map<t_topicNumberType,Ptr<Socket> >::iterator itor;
    //find key-topic using value-socket in map pubsendSocket
    for(itor = pubSendSocket.begin();itor != pubSendSocket.end();itor++){
        if(itor->second == socket){
            break;
        }
    }
    //restart the topic's sending
    if(itor != pubSendSocket.end()){
        if (pubStatus[itor->first] == WAITRECONNECT) {
            pubStatus[itor->first] = RESENDING;
            NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I pub["<<pubId<<"] reconnect socket of topic:"
                        <<itor->first<<"successfully");
        }
        Send2Topic(itor->first);
    }
}

void PubApp::ConnectionFailed(Ptr<Socket> socket) {
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<":I pub["<<pubId<<"] do connecting failed ");
}

void PubApp::AddTopic(t_topicNumberType theTopic,t_packetNumberType packetNumber) {
    if (topic.find(theTopic) == topic.end()) {
        topic.insert(theTopic);
        packetsToSend[theTopic] = packetNumber;
        packetSendedPerTopic.insert(std::make_pair(theTopic,0));
        bitsSendedPerTopic.insert(std::make_pair(theTopic,0));
    } else {
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<":I topic:"<<theTopic<<" already in pub["<<pubId<<"]");
    }
    if (Simulator::Now().GetSeconds() != 0) {
        ConnectHub(theTopic);
    }
}

} //namespace ns3
