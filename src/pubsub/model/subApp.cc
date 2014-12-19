/*
 * subApp.cc
 *
 *  Created on: Mar 10, 2014
 *      Author: zhaohui
 */
#include "ns3/subApp.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("pubsub-sub");
NS_OBJECT_ENSURE_REGISTERED (SubApp);

SubApp::SubApp() :
        subTopic(),
        rcvMsgSocket(0),
        sendAckSocket(),
        rcvMsgAddress(),
        subTotalRcvBit(0),
        subTotalRcvMsg(0),
        subRcvMsgPerTopic(),
        totalFromBK(0),
        submited(),
        temp(),
        partition(),
        maxSeen(),
        toSubmit(),
        fileName(),
        toPrint(){
    persistence = PersistenceManager::GetPersistenceManager();
//    fileName = c_prefileName;
//    fileName += "_sub";
//    fileName += char(subId+48);
//    fileName += "_topic";
//    rcvMsgSocket = Socket::CreateSocket(,TcpSocketFactory::GetTypeId());
//    sendAckSocket = Socket::CreateSocket(theNode,TcpSocketFactory::GetTypeId());
//    rcvMsgAddress = Address(InetSocketAddress(Ipv4Address::GetAny(), c_messagePort));
}

SubApp::~SubApp() {
    rcvMsgSocket = 0;
    sendAckSocket.clear();
    for(std::map<t_topicNumberType,std::ofstream*>::iterator itor = dataCollection.begin();
            itor != dataCollection.end();itor++){
        itor->second->close();
        delete itor->second;
    }
//    dataCollection.close();
    dataCollection.clear();
    toPrint.clear();
}

TypeId
SubApp::GetTypeId(void){
    static TypeId tid =TypeId("ns3::SubApp")
            .SetParent<Application>()
            .AddConstructor<SubApp>()
            .AddAttribute("Id","the unique id for each sub,do not change this manual",
                    UintegerValue(0), MakeUintegerAccessor(&SubApp::subId),
                    MakeUintegerChecker<uint32_t>(0))
            .AddAttribute("IP","sub IP",
                    Ipv4AddressValue(Ipv4Address("0.0.0.0")),
                    MakeIpv4AddressAccessor(&SubApp::subIp),
                    MakeIpv4AddressChecker());
    return tid;
}

void SubApp::StartApplication(void) {
    //if there is no information of subscribe topic.We just subscribe topic with the same name of subId
//    if(subTopic.empty()){
//        subTopic.insert(subId);
//    }
    for(std::set<t_topicNumberType>::iterator itor = subTopic.begin();itor != subTopic.end();itor++){
        Subscribe(*itor);

        std::string temp;
        temp = c_prefileName;
        temp += "_sub";
        char middleTemp[16];
        sprintf(middleTemp, "%lu_topic%u", subId,*itor);
        temp += middleTemp;
        dataCollection.insert(std::make_pair(*itor,new std::ofstream()));
        dataCollection[*itor]->open(temp.c_str());

    }

    rcvMsgSocket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    rcvMsgAddress = Address(InetSocketAddress(Ipv4Address::GetAny(), c_messagePort));
    rcvMsgSocket -> Bind(rcvMsgAddress);
    rcvMsgSocket -> Listen();
    rcvMsgSocket -> ShutdownSend();
    rcvMsgSocket -> SetRecvCallback (MakeCallback(&SubApp::ReceiveMsg,this));
    rcvMsgSocket -> SetAcceptCallback (
          MakeNullCallback<bool,Ptr<Socket>,const Address &>(),
          MakeCallback(&SubApp::SubAccept,this));
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": sub["<<subId<<"] start");
//    std::cout << fileName << std::endl;
//    dataCollection.open(fileName.c_str());
}

void SubApp::AddTopic(t_topicNumberType theTopic){
    if (subTopic.find(theTopic) == subTopic.end()) {
            subTopic.insert(theTopic);
//            std::string temp;
//            temp += "_sub";
//            temp += char(subId+48);
//            temp += "_topic";
//            temp = fileName;
//            temp += char(theTopic+48);
//            dataCollection.insert(std::make_pair(theTopic,new std::ofstream()));
//            std::cout<<temp<<std::endl;
//            dataCollection[theTopic]->open(temp.c_str());
            } else {
                NS_LOG_INFO("topic:"<<theTopic<<" already subbed in sub["<<subId<<"]");
            }
        if(Simulator::Now().GetSeconds()!=0){
            Subscribe(theTopic);
        }
}


void SubApp::Subscribe(t_topicNumberType theTopic) {
/*    Ipv4Address IpAddr = persistence -> HandleSub(theTopic,subId,subIp);
    Address sendAckAddress = InetSocketAddress(IpAddr, c_ackPort);
    if(IpAddr.IsEqual("0.0.0.0")){//if this topic already sub by this topic
        return;
    }
    Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    socket->Bind();
    socket->Connect(sendAckAddress);
    socket->ShutdownRecv();
    socket->SetConnectCallback(
                MakeCallback(&SubApp::ConnectionSucceeded, this),
                MakeCallback(&SubApp::ConnectionFailed, this));
    sendAckSocket.insert(std::make_pair(theTopic,socket));*/
    persistence -> HandleSub(theTopic,subId,subIp);
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": sub["<<subId<<"] subscribe topic "<<theTopic);
}

Ptr<Socket>
SubApp::MakeAckSocket(const Ipv4Address& ip){
    Address sendAckAddress = InetSocketAddress(ip, c_ackPort);
    Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),TcpSocketFactory::GetTypeId());
    socket->Bind();
    socket->Connect(sendAckAddress);
    socket->ShutdownRecv();
    socket->SetConnectCallback(MakeCallback(&SubApp::ConnectionSucceeded, this),
            MakeCallback(&SubApp::ConnectionFailed, this));
    return socket;
}

void SubApp::StopApplication(void) {
    std::map<t_topicNumberType,std::vector<std::string> >::iterator itor = toPrint.begin();
    for(;itor != toPrint.end();itor++){
        if(itor->second.size() != 0 ){
            LogToFile(itor->first);
        }
    }
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": sub["<<subId<<"] stop and totally received "<<subTotalRcvMsg);

}


void SubApp::ReceiveMsg(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet = socket->RecvFrom(from);
    subTotalRcvBit += packet->GetSize();
    RebuildPac(packet, from);
}

void SubApp::RebuildPac(Ptr<Packet> p, const Address &from) {
    InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
    t_addrAndPort addr(address.GetIpv4(),address.GetPort());

    if (temp.find(addr) == temp.end()) {
        temp.insert(std::pair<t_addrAndPort,Ptr<Packet> >(addr,p->Copy()));
    } else {
        temp[addr]->AddAtEnd(p);
    }
    MsgHeader header;
    while (temp[addr]->GetSize() >= header.GetSerializedSize()) {
        ++ partition[addr];
        //has already received a total message head
        temp[addr]->PeekHeader(header);
        if (temp[addr]->GetSize() >=
                uint32_t(header.GetContentSize() + header.GetSerializedSize())) {
            //has already received a total message
            temp[addr]->RemoveHeader(header);

            //if the ACK address of this topic has been updated,update the record
            std::map<Address,Ptr<Socket> >::iterator itor = tempRecord.find(from);
            if(itor != tempRecord.end()){
                sendAckSocket[header.GetTopic()] = tempRecord.find(from)->second;
                tempRecord.erase(itor);
            }

            NS_LOG_INFO(Simulator::Now().GetSeconds()
                    <<": r sub["<<subId<<"] receive the "
                    <<header.GetSequence()<<" packet topic is "
                    <<header.GetTopic()<<" and content size is "
                    <<header.GetContentSize());
            header.SetSubRTime(Simulator::Now().GetMicroSeconds());
            header.SetPart(partition[addr]);
            partition[addr] = 0;
            SubSubmit(header);
            subTotalRcvMsg++;
            if (header.isFromBK()) {
                ++totalFromBK;
            }
            temp[addr]->RemoveAtStart(header.GetContentSize());
        } else {
            break;
        }
    }
    if(temp[addr] != NULL && temp[addr]->GetSize() < header.GetSerializedSize()
                          && temp[addr] ->GetSize() != 0){
        ++ partition[addr];
    }
}

void SubApp::SubSubmit(MsgHeader theHeader) {
    t_topicNumberType topic = theHeader.GetTopic();
    uint32_t comein = theHeader.GetSequence();
    if(comein <= submited[topic]){
        //this packet has already been received
        SendAck(theHeader);
    }else if (comein == submited[topic] + 1) {
            submited[topic]++;
            SendAck(theHeader);
            Print(theHeader);
            for (uint32_t i = submited[topic]+1; i <= maxSeen[topic]; i++) {
                if (toSubmit[topic][i & LENGTH].GetSequence() == submited[topic] + 1) {
                    submited[topic]++;
                    SendAck(toSubmit[topic][i & LENGTH]);
                    Print(toSubmit[topic][i & LENGTH]);
                    toSubmit[topic][i & LENGTH].SetSequence(0);
                } else {
                    break;
                }
            }
        } else if (submited[topic] + 1 < comein && comein < maxSeen[topic]) {
            toSubmit[topic][comein&LENGTH] = theHeader;
        }else{
            for(uint32_t i = maxSeen[topic]+1; i<=comein; i++){
                toSubmit[topic][i&LENGTH].SetSequence(0);
            }
            toSubmit[topic][comein&LENGTH] = theHeader;
            maxSeen[topic] = comein;
        }
}

void SubApp::SendAck(MsgHeader theHeader) {
    CtrlHeader ackHeader;
    ackHeader.SetSequenceTillNow(theHeader.GetSequence());
    ackHeader.SetKind(SUBACK);
    ackHeader.SetSenderId(subId);
    ackHeader.SetTopic(theHeader.GetTopic());
    Ptr<Packet> packet = Create<Packet>(1);
    packet -> AddHeader(ackHeader);
    sendAckSocket[theHeader.GetTopic()] -> Send(packet);
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] send ACK of topic:"
                <<ackHeader.GetTopic()<<"("<<ackHeader.GetSequenceTillNow()<<")");
}

void SubApp::Print(MsgHeader theHeader) {
    theHeader.SetSubSTime(Simulator::Now().GetMicroSeconds());
//        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] add log to file");
//        theHeader.PrintTime(*dataCollection[theHeader.GetTopic()]);
        std::ostringstream temp;
        theHeader.PrintTime(temp);
        t_topicNumberType topic = theHeader.GetTopic();
        toPrint[topic].push_back(temp.str());

    if (toPrint[topic].size() == 1000) {
        if (dataCollection[topic]->is_open()) {
            LogToFile(topic);
        } else {
            NS_LOG_ERROR("can\'t open log file "<<c_prefileName);
            exit(1);
        }
    }
}

void
SubApp::LogToFile(t_topicNumberType theTopic){
    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] add log to file");
    for(t_packetNumberType i = 0;i<toPrint[theTopic].size();i++){
        (*dataCollection[theTopic]) << toPrint[theTopic][i];
//        std::cout<<toPrint[theTopic][i];
    }
    toPrint[theTopic].clear();
}


void
SubApp::SubAccept(Ptr<Socket> s ,const Address& from){
/*    NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] accept a socket connection");*/
    s->SetRecvCallback (MakeCallback (&SubApp::ReceiveMsg, this));
    if( incomeAddress.find(from) == incomeAddress.end() ){
        InetSocketAddress address = InetSocketAddress::ConvertFrom(from);
        Ptr<Socket> socket = MakeAckSocket(address.GetIpv4());
        incomeAddress.insert(from);
        tempRecord.insert(std::make_pair(from,socket));
        NS_LOG_INFO(Simulator::Now().GetSeconds()<<": I sub["<<subId<<"] accept a new connection from "
                    <<address.GetIpv4());
    }
}
void
SubApp::ConnectionSucceeded (Ptr<Socket> socket){

}
void
SubApp::ConnectionFailed(Ptr<Socket> socket){

}
} //namespace ns3
