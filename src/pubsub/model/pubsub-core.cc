/*
 * pubsub-core.cc
 *
 *  Created on: May 20, 2014
 *      Author: zhaohui
 */



#include "ns3/pubsub-core.h"
#include <string>
#include <algorithm>
#include <cctype>

namespace ns3{

NS_LOG_COMPONENT_DEFINE("pubsub-core");

bool Configure::configureDone = false;

Configure::Configure()
            :packetNumber(1000),
             balanceTime(0.3),
             balanceOn(false),
             pubNumber(0),
             hubNumber(0),
             subNumber(0),
             pubSendInterval(0),
             hubSendInterval(0),
             hubCacheLength(100000),//100kB
             BkReadDelay(50),
             hubWindowCapacity(1000),
             outFile("log/on-1051-10000/resault"),
             inFile("./src/pubsub/pubsub.config")
{
    ReadConf();
    if (!configureDone) {
        PrintInfo();
        configureDone = true;
    }
}
Configure::~Configure(){
    inFile.close();
}
void
Configure::ReadConf(){
    if(!inFile.is_open()){
        NS_LOG_ERROR("can't find configure file : \"./src/pubsub/pubsub.config\" using default value");
    }else{
        std::string line;
        uint16_t lineNumber = 0;
        while(std::getline(inFile,line)){
            lineNumber++;
            std::istringstream record(line);
            std::string word;
            record >> word;
            if(word == "\0" || *word.c_str() == '#'){
                continue;
            }else{
                std::transform(word.begin(), word.end(), word.begin(), ::toupper);
                if(word == "DEF"){
                    if(!DelDefine(record)){
                        word = "ERROR";
                    }
                }else if(word == "PUB"){
                    if(!DelPub(record)){
                        word = "ERROR";
                    }
                }else if(word == "SUB"){
                    if(!DelSub(record)){
                        word = "ERROR";
                    }
                }else{
                    word ="ERROR";
                }
                if(word == "ERROR"){
                    exit(1);
                }
            }
        }
    }

}

bool
Configure::DelDefine(std::istringstream& input){
    std::string word;
    input>>word;
    if(word == "balance"){
        input>>word;
        if(word == "on"){
            balanceOn = true;
        }
    }else if(word == "pubnumber"){
        t_nodesNumberType temp;
        input>>temp;
        pubNumber = temp;
    }else if(word == "hubnumber"){
        t_nodesNumberType temp;
        input>>temp;
        hubNumber = temp;
    }else if(word == "subnumber"){
        t_nodesNumberType temp;
        input>>temp;
        subNumber = temp;
    }else if(word == "outfile"){
        input>>word;
        outFile = word;
    }else if(word == "pubSendInterval"){
        uint64_t temp;
        input>>temp;
        pubSendInterval = temp;
    }else if(word == "hubSendInterval"){
        uint64_t temp;
        input >> temp;
        hubSendInterval = temp;
    }else if(word == "hubCacheLength"){
        uint64_t temp;
        input >> temp;
        hubCacheLength = temp;
    }else if(word == "hubWindowCapacity"){
        t_packetNumberType temp;
        input >> temp;
        hubWindowCapacity = temp;
    }else if(word == "BKreadDelay"){
        uint16_t temp;
        input >> temp;
        BkReadDelay = temp;
    }else{
        return false;
    }
    return true;
}

bool
Configure::DelPub(std::istringstream& input){
    pubInfoNode temp;
    input>>temp.pubId;
    input>>temp.topicId;
    input>>temp.packetNumber;
    pubInfo.push_back(temp);
    return true;
}

bool
Configure::DelSub(std::istringstream& input){
    std::pair<t_nodesNumberType,t_topicNumberType> temp;
    input>>temp.first;
    input>>temp.second;
    subInfo.push_back(temp);
    return true;
}

void
Configure::PrintInfo(){
    std::cout<<pubNumber<<" pubs | "<<hubNumber<<" hubs | "<<subNumber<<" subs\n";
    std::vector<pubInfoNode>::size_type i = 0,end = pubInfo.size();
    for(;i<end;i++){
        std::cout<<"pub["<<pubInfo[i].pubId<<"] + topic"<<pubInfo[i].topicId<<"X"<<pubInfo[i].packetNumber<<" | ";
    }
    std::cout<<std::endl;
    end = subInfo.size();
    for(i = 0;i<end;i++){
        std::cout<<"sub["<<subInfo[i].first<<"] + topic"<<subInfo[i].second<<" | ";
    }
    std::cout<<std::endl;
    std::cout<<"pubSendSpeed "<<pubSendInterval<<" | hubMaxSendPerSecond "<<hubSendInterval
             <<" | hubCacheLength "<<hubCacheLength<<" | hubWindowCapacity "<<hubWindowCapacity
             <<" | BkReadDelay "<<BkReadDelay<<" | balance ";
    if(balanceOn){
        std::cout<<"on at "<<balanceTime<<std::endl;
    }else{
        std::cout<<"off\n";
    }

}

}
