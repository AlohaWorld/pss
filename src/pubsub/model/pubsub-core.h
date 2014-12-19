/*
 * pubsub-core.h
 *
 *  Created on: Mar 10, 2014
 *      Author: zhaohui
 */

#ifndef PUBSUB_CORE_H
#define PUBSUB_CORE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <iostream>
#include <vector>

namespace ns3{

#define   CALLBACK_ ;

enum balanceKind{
    NONE,
    IMBALANCE,
    TOPICBASED,
    MESSAGEBASED,
    PERIODICALTOPICBASED,
    PERIODICALMESSAGEBASED,
    ROUNDROBIN,
};

enum appStatus{
    UNSET,
    STOP,
    WAITING,
    WAITACK,
    RSEDWAIT,
    RUNNING,
    READY,
    RESENDING,
    WAITRECONNECT,
};

typedef uint64_t t_nodesNumberType;
typedef uint32_t t_topicNumberType;
typedef uint64_t t_packetNumberType;
typedef uint64_t t_timestamp;
typedef std::pair<Ipv4Address,uint16_t> t_addrAndPort;

struct pubInfoNode{
    t_nodesNumberType pubId;
    t_topicNumberType topicId;
    t_packetNumberType packetNumber;
};

class Configure{
public:
    Configure();
    ~Configure();

    void ReadConf();

    static bool configureDone;
    t_packetNumberType packetNumber;
    double balanceTime;
    bool balanceOn;

    t_nodesNumberType pubNumber;
    t_nodesNumberType hubNumber;
    t_nodesNumberType subNumber;

    uint64_t pubSendInterval;
    uint64_t hubSendInterval;
    t_packetNumberType hubCacheLength;
    uint16_t BkReadDelay;
    t_packetNumberType hubWindowCapacity;

    std::string outFile;
    std::ifstream inFile;

    std::vector<pubInfoNode> pubInfo;
    std::vector<std::pair<t_nodesNumberType,t_topicNumberType> > subInfo;
private:
    Configure(const Configure&){}
    bool DelDefine(std::istringstream& input);
    bool DelPub(std::istringstream& input);
    bool DelSub(std::istringstream& input);
    void PrintInfo();
};

const std::string c_runInfoFile = "~/log/new/runInfo";

const static Configure configure;

const static uint16_t c_basePort = 30000;
const static uint16_t c_ackPort = c_basePort+1;
const static uint16_t c_messagePort = c_basePort+2;


const static t_nodesNumberType c_pubNumber = configure.pubNumber;
const static t_nodesNumberType c_hubNumber = configure.hubNumber;
const static t_nodesNumberType c_subNumber = configure.subNumber;

const static std::vector<pubInfoNode> pubInfo = configure.pubInfo;
const static std::vector<std::pair<t_nodesNumberType,t_topicNumberType> > subInfo = configure.subInfo;

const static t_packetNumberType c_defPacketNumber = configure.packetNumber;
const static double c_balanceTime = configure.balanceTime;
const static bool c_balanceON = configure.balanceOn;

const static uint64_t c_pubSendInterval = configure.pubSendInterval;
const static uint64_t c_hubSendInterval = configure.hubSendInterval;
const static uint64_t c_hubCacheLength = configure.hubCacheLength;
const static uint16_t c_BkreadDelay = configure.BkReadDelay;
const static t_packetNumberType c_hubWindowCapacity = configure.hubWindowCapacity;

const std::string c_prefileName = configure.outFile;


}//namespace ns3


#endif /* PUBSUB_CORE_H_ */
