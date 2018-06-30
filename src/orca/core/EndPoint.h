#ifndef  ORCA_CORE_ENDPOINT_H
#define  ORCA_CORE_ENDPOINT_H

#include <string>
#include <vector>
#include <queue>
#include "../base/libuv_cpp11/uv/uv11.h"
#include "../base/thread/Thread.h"
#include "net/ActorClient.h"
#include "net/ActorServer.h"
#include "RemoteMail.h"
#include "../base/error/ErrorHandle.h"

namespace orca
{

namespace core
{

struct EndPointAddress
{
    enum IPV
    {
        Ipv4 = 0,
        Ipv6
    };
    std::string ip;
    uint16_t port;
    IPV ipv;
};

template<typename MessageType>
class EndPoint
{
public:
    using RemoteMailPtr = std::shared_ptr<RemoteMail<MessageType>>;

    EndPoint(EndPointAddress& addr,uint32_t id);
    ~EndPoint();

    void run();
    void appendRemoteEndPoint(struct EndPointAddress& addr);
    void clear();

    void registerActorClient(uint32_t id, ActorClientPtr client);

    void send(const std::shared_ptr<MessageType> message, Address& from, Address& destination);
    void send(const std::shared_ptr<MessageType> message, Address& from, std::string& name, uint32_t framework);

    static const int MessageProcessPeriodMS;
private:
    uint32_t id_;
    uv::EventLoop loop_;
    std::atomic<bool> remoteRegisterCompleted_;
    std::vector<ActorClientPtr> endPoints_;
    std::map<uint32_t, ActorClientPtr> endPointMap_;
    std::shared_ptr<ActorServer> server_;
    uv::Timer<void*>* timer_;
    std::queue<RemoteMailPtr> sendCache_;

    void appendMail(RemoteMailPtr mail);
    void processMail();
};
template<typename MessageType>
const int EndPoint<MessageType>::MessageProcessPeriodMS = 10;

template<typename MessageType>
inline EndPoint<MessageType>::EndPoint(EndPointAddress & addr, uint32_t id)
    :id_(id),
    remoteRegisterCompleted_(false),
    timer_(nullptr)
{
    uv::SocketAddr uvaddr(addr.ip, addr.port, static_cast<uv::SocketAddr::IPV>(addr.ipv));
    server_ = std::make_shared<ActorServer>(&loop_, uvaddr, id_);
    timer_ = new uv::Timer<void*>(&loop_, MessageProcessPeriodMS, MessageProcessPeriodMS,
        [this](uv::Timer<void*>* ,void*)
    {
        processMail();
    }, nullptr);
}

template<typename MessageType>
inline EndPoint<MessageType>::~EndPoint()
{
    timer_->close(
        [](uv::Timer<void*>* timer)
    {
        //release timer pointer in loop callback.
        delete timer;
    });
}

template<typename MessageType>
inline void EndPoint<MessageType>::run()
{
    remoteRegisterCompleted_ = endPoints_.empty() ? true : false;
    for (auto it = endPoints_.begin(); it != endPoints_.end(); it++)
    {
        (*it)->connect();
    }

    server_->start();
    timer_->start();
    loop_.run();
}

template<typename MessageType>
inline void EndPoint<MessageType>::appendRemoteEndPoint(EndPointAddress & addr)
{
    uv::SocketAddr socketAddr(addr.ip, addr.port, static_cast<uv::SocketAddr::IPV>(addr.ipv));
    ActorClientPtr client = std::make_shared<ActorClient>(&loop_, socketAddr, id_);
    client->setRegisterRemoteFrameworkCallback(std::bind(&EndPoint::registerActorClient, this, std::placeholders::_1, std::placeholders::_2));
    endPoints_.push_back(client);

}

template<typename MessageType>
inline void EndPoint<MessageType>::clear()
{
    endPoints_.clear();
}

template<typename MessageType>
inline void EndPoint<MessageType>::registerActorClient(uint32_t id, ActorClientPtr client)
{
    //thread Safety
    loop_.runInThisLoop([this, client, id]()
    {
        if (id == id_)
        {
            base::ErrorHandle::Instance()->error(base::ErrorInfo::RepeatedRemoteFrameworkID, std::string("remote framework id repeated :") + std::to_string(id));
        }
        else
        {
            auto it = endPointMap_.find(id);
            if (it != endPointMap_.end())
            {
                endPointMap_[id] = client;
                if (endPointMap_.size() == endPoints_.size())
                {
                    remoteRegisterCompleted_ = true;
                }
            }
            else
            {
                base::ErrorHandle::Instance()->error(base::ErrorInfo::RepeatedRemoteFrameworkID, std::string("remote framework id repeated :") + std::to_string(id));
            }
        }
    });
}

template<typename MessageType>
inline void EndPoint<MessageType>::send(const std::shared_ptr<MessageType> message, Address& from, Address& destination)
{
    auto ptr = std::make_shared<RemoteMail<MessageType>>(from,destination,message);
}

template<typename MessageType>
inline void EndPoint<MessageType>::send(const std::shared_ptr<MessageType> message, Address& from, std::string& name, uint32_t framework)
{
    auto ptr = std::make_shared<RemoteMail<MessageType>>(from, framework, name, message);
}

template<typename MessageType>
inline void EndPoint<MessageType>::appendMail(RemoteMailPtr mail)
{
    //thread safe.
    loop_.runInThisLoop([mail,this]() 
    {
        sendCache_.push(mail);
    });
}

template<typename MessageType>
inline void EndPoint<MessageType>::processMail()
{

    //��ѯ
    //auto last
}

}
}
#endif
