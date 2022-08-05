#pragma once
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "UserManager.hpp"
#include "MsgQueue.hpp"
#include "ChatMsg.hpp"

#define THREAD_COUNT 4


struct Msg
{
    int sockfd;
    char buf[TCP_DATA_MAX_LEN];
};

//网络通信，创建工作线程，接受线程，发送线程
class ChatServer
{
    public:
        ChatServer()
        {
            tcp_sock_ = -1;
            tcp_port_ = TCP_PORT;
            user_mana_ = NULL;
            epoll_fd_ = -1;
            thread_count_ = THREAD_COUNT;

            send_que_ = NULL;
            ready_sockfd_que_ = NULL;
            recv_que_ = NULL;
        }

        ~ChatServer()
        {}

        //初始化资源的函数
        int InitChatServer(uint16_t tcp_port = TCP_PORT, int thread_count = THREAD_COUNT)
        {
            tcp_port_ = tcp_port;
            thread_count_ = thread_count;

            //tcp初始化
            tcp_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(tcp_sock_ < 0)
            {
                perror("socket");
                return -1;
            }

            //端口重用
            int opt = 1;
            setsockopt(tcp_sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(tcp_port_);
            addr.sin_addr.s_addr = inet_addr("0.0.0.0");
            int ret = bind(tcp_sock_, (struct sockaddr*)&addr, sizeof(addr));

            if(ret < 0)
            {
                perror("bind");
                return -1;
            }

            ret = listen(tcp_sock_, 1024);
            if(ret < 0)
            {
                perror("listen");
                return -1;
            }

            //epoll初始化
            epoll_fd_ = epoll_create(5);
            if(epoll_fd_ < 0)
            {
                return -1;
            }

            //用户管理系统
            user_mana_ = new UserManager();
            if(user_mana_ == NULL)
            {
                return -1;
            }

           if(false == user_mana_->InitUserMana())
           {
               return -1;
           }

           send_que_ = new MsgQueue<ChatMsg>();
           if(NULL == send_que_)
           {
               return -1;
           }

           ready_sockfd_que_ = new MsgQueue<int>();
           if(NULL == ready_sockfd_que_)
           {
               return -1;
           }

           recv_que_ = new MsgQueue<ChatMsg>();
           if(NULL == recv_que_)
           {
               return -1;
           }

           return 0;
        }

        //启动各类线程函数 - 主线程调用
        int StartChatServer()
        {
            //1.创建epoll等待线程
            pthread_t tid;
            int ret = pthread_create(&tid, NULL, epoll_wait_start, (void*)this);//this指针，为了使静态成员变量使用类中的属性
            if(ret < 0)
            {
                perror("pthread_create");
                return -1;
            }

            //2.创建接收线程
            ret = pthread_create(&tid, NULL, recv_mag_start, (void*)this);
            if(ret < 0)
            {
                perror("pthread_create");
                return -1;
            }

            //3.创建发送线程
            ret = pthread_create(&tid, NULL, send_msg_start, (void*)this);
            if(ret < 0)
            {
                perror("pthread_create");
                return -1;
            }

            //4.创建工作线程
            for(int i = 0; i < thread_count_; i++)
            {
                ret = pthread_create(&tid, NULL, deal_start, (void*)this);
                if(ret < 0)
                {
                    thread_count_--;
                }
            }

            //5.主线程循环接收新连接， 将新连接套接字放入epoll中
            struct sockaddr_in cli_addr;
            socklen_t cli_addr_len = sizeof(cli_addr);
            while(1)
            {
                 //监听有无新连接，有新连接请求时，返回新连接套接字
                int newsockfd = accept(tcp_sock_, (struct sockaddr*)&cli_addr,  &cli_addr_len);
                if(newsockfd < 0)
                {
                    continue;
                }

                //接受到了，放入epoll中监控
                struct epoll_event ee;
                ee.events = EPOLLIN;
                ee.data.fd = newsockfd;
                epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, newsockfd, &ee);
            }

            return 0;
        }

        static void* epoll_wait_start(void* arg)
        {
            pthread_detach(pthread_self());
            ChatServer* cs = (ChatServer*)arg;
            while(1)
            {
                struct epoll_event arr[10];

                //接收到客户端发来的消息，不在阻塞
                int ret = epoll_wait(cs->epoll_fd_, arr, sizeof(arr)/sizeof(arr[0]), -1);
                if(ret < 0)
                {
                    continue;
                }

                //获取了就绪的事件结构,一定获取到的是新连接套接字
                for(int i = 0; i < ret; i++)
                {
                    //cs->ready_sockfd_que_->Push(arr[i].data.fd);
                    //2.接收数据
                    char buf[TCP_DATA_MAX_LEN] = {0};
                    //隐藏问题-tcp粘包
                    //接收处理客户端端的消息
                    size_t recv_size = recv(arr[i].data.fd, buf, sizeof(buf) - 1, 0);
                    if(recv_size < 0)
                    {
                        //接收失败
                        continue;
                    }
                    else if(recv_size == 0)
                    {
                        //对端关闭连接
                        epoll_ctl(cs->epoll_fd_, EPOLL_CTL_DEL, arr[i].data.fd, NULL);
                        close(arr[i].data.fd);

                        //组织一个更改用户状态的消息
                        cs->user_mana_->SetUserOffLine(arr[i].data.fd);
                        continue;
                    }

                    printf("epoll_wait_start recv msg : %s from sockfd %d\n", buf, arr[i].data.fd);
                    //接收成功,将接收回来的数据放到接收线程的队列当中，等到工作线程从队列当中获取消息，进而进行处理
                    //3.将接收的到的数据放到接收队列当中
                    std::string msg;
                    msg.assign(buf, strlen(buf));

                    ChatMsg cm;
                    cm.PraseChatMsg(arr[i].data.fd, msg);
                    cs->recv_que_->Push(cm);
                }   
            }


            return NULL;
        
        }

        static void* recv_mag_start(void* arg)
        {
            pthread_detach(pthread_self());
            ChatServer* cs = (ChatServer*)arg;
            while(1)
            {
                /*
                    //1.从队列当中获取就绪的文件描述符
                    struct Msg msg;
                    int readyfd;
                    cs->ready_sockfd_que_->Pop(&readyfd);
                    
                    //2.接收数据
                    msg.sockfd = readyfd;

                    //隐藏问题-tcp粘包
                    
                    //接收处理客户端端的消息
                    size_t recv_size = recv(msg.sockfd, msg.buf, sizeof(msg.buf) - 1, 0);
                    if(recv_size < 0)
                    {
                        //接收失败
                        continue;
                    }
                    else if(recv_size == 0)
                    {
                        //对端关闭连接
                        epoll_ctl(cs->epoll_fd_, EPOLL_CTL_DEL, msg.sockfd, NULL);
                        close(msg.sockfd);
                        continue;
                    }:

                    printf("epoll_wait_start recv msg : %s from sockfd %d\n", msg.buf, msg.sockfd);
                    //接收成功,将接收回来的数据放到接收线程的队列当中，等到工作线程从队列当中获取消息，进而进行处理
                    //3.将接收的到的数据放到接收队列当中
                    cs->send_que_->Push(msg);
                    */

            }
            return NULL;
        }

        static void* send_msg_start(void* arg)
        {
            pthread_detach(pthread_self());
            ChatServer* cs = (ChatServer*)arg;
            while(1)
            {
                //1.从队列拿出数据
                ChatMsg cm;
                cs->send_que_->Pop(&cm);

                //序列化
                std::string msg;
                cm.GetMsg(&msg);
                std::cout << "send thread:" << msg << std::endl;

                //2.发送数据
                send(cm.sockfd_, msg.c_str(), msg.size(), 0);

            }
            return NULL;
        }

        static void* deal_start(void* arg)
        {
            pthread_detach(pthread_self());
            ChatServer* cs = (ChatServer*)arg;

            while(1)
            {
                //1.从接收队列当中获取消息
                ChatMsg cm;
                cs->recv_que_->Pop(&cm);

                //2.通过消息类型分业务处理
                int msg_type = cm.msg_type_;
                std::cout << SendMsg << " --- "  << cm.msg_type_ << std::endl;
                switch(msg_type)
                {
                    case Register:
                        {
                            cs->DealRegister(cm);
                            break;
                        }
                    case Login:
                        {
                            cs->DealLogin(cm);
                            break;
                        }
                    case AddFriend:
                        {
                            cs->DealAddFriend(cm);
                            break;
                        }
                    case PushADDFriendMsg_Resp:
                        {
                            cs->DealAddFriendResp(cm);
                            break;
                        }
                    case SendMsg:
                        {
                            cs->DealSendMsg(cm);
                            break;
                        }
                    case GetFriendMsg:
                        {
                            cs->GetAllFriendInfo(cm);
                            break;
                        }
                }

                //3.组织应答

            }

            return NULL;
        }


        void DealRegister(ChatMsg& cm)
        {
            //1.获取注册信息
            std::string fakename = cm.GetValue("fakename");
            std::string school = cm.GetValue("school");
            std::string telnum = cm.GetValue("telnum");
            std::string passwd = cm.GetValue("passwd");

            //2.调用用户管理系统当中的注册接口
            int userid = -1;
            int ret = user_mana_->DealRegister(fakename, school, telnum, passwd, &userid);

            //3.回复应答
            cm.Clear();
            cm.msg_type_ = Register_Resp;
            if(ret < 0)
            {
                cm.reply_status_ = REGISTER_FAILED;
            }
            else
            {
                cm.reply_status_ = REGISTER_SUCCESS;
            }
            cm.user_id_ = userid;

            send_que_->Push(cm);
        }

        void DealLogin(ChatMsg& cm)
        {
            //1.获取数据
            std::string telnum = cm.GetValue("telnum");
            std::string passwd = cm.GetValue("passwd");

            //2.调用用户管理系统当中的登录接口
            int ret = user_mana_->DealLogin(telnum, passwd, cm.sockfd_);

            //3.回复应答
            cm.Clear();
            cm.msg_type_ = Login_Resp;
            if(ret < 0)
            {
                cm.reply_status_ = LOGIN_FAILED;
            }
            else
            {
                cm.reply_status_ = LOGIN_SUCCESS;
            }
            cm.user_id_ = ret;

            send_que_->Push(cm);
        }


        void DealAddFriend(ChatMsg& cm)
        {
            //1.获取被添加方的电话号码
            std::string telnum = cm.GetValue("telnum");
            //添加方的userid
            int add_userid = cm.user_id_;

            //2.查询被添加方是否是登录状态
            cm.Clear();
            UserInfo be_add_ui;
            int ret = user_mana_->IsLogin(telnum, &be_add_ui);
            if(-1 == ret)
            {
                //用户不存在
                cm.json_msg_ = AddFriend_Resp;
                cm.reply_status_ = ADDFRIEND_FAILED;
                cm.SetValue("continue", "用户不存在");
                send_que_->Push(cm);
                return;
            }
            else if(OFFLINE == ret)
            {
                //不在线
                return;
            }
            //ONLINE状态
            
            //3.被添加方推送添加好友请求
            UserInfo add_ui;
            user_mana_->GetUseInfo(add_userid, &add_ui);

            cm.sockfd_ = be_add_ui.tcp_socket_;
            cm.msg_type_ = PushAddFriendMsg;
            cm.SetValue("adder_fakename", add_ui.fakename_);
            cm.SetValue("addr_school", add_ui.school_);
            cm.SetValue("adder_userid", add_ui.userid_);

            send_que_->Push(cm);
            
        }

        void DealAddFriendResp(ChatMsg& cm)
        {
            //1.获取双方的用户信息
            int reply_status = cm.reply_status_;
            //获取被添加方用户信息
            int be_add_user = cm.user_id_;
            UserInfo be_userinfo;
            user_mana_->GetUseInfo(be_add_user, &be_userinfo);

            //获取添加方用户信息-通过应答获取添加方Userid
            UserInfo ui;
            int addr_add_user = atoi(cm.GetValue("userid").c_str());
            user_mana_->GetUseInfo(addr_add_user, &ui);

            //2.获取响应状态
            cm.Clear();
            cm.sockfd_ = ui.tcp_socket_;
            cm.msg_type_ = AddFriend_Resp;
            if(ADDFRIEND_FAILED == reply_status)
            {
                cm.reply_status_ = ADDFRIEND_FAILED;
                std::string content = "add user " + be_userinfo.fakename_ + " faild";
                cm.SetValue("content", content);
            }
            else if(ADDFRIEND_SUCCESS == reply_status)
            {
                cm.reply_status_ = ADDFRIEND_SUCCESS;
                std::string content = "add user " + be_userinfo.fakename_ + " success";
                cm.SetValue("content", content);
                cm.SetValue("peer_fakename", be_userinfo.fakename_);
                cm.SetValue("peer_school", be_userinfo.school_);
                cm.SetValue("peer_userid", be_userinfo.userid_);

                user_mana_->SetFriend(addr_add_user, be_add_user);
            }

            //TODO
            if(OFFLINE == ui.user_status_)
            {
                //消息放到缓存队列中，择机发送
                
            }

            //3.给添加方回复响应
            send_que_->Push(cm);
        }

        void GetAllFriendInfo(ChatMsg& cm)
        {
            //好友信息的数据从用户管理模块获取
            int user_id = cm.user_id_;

            cm.Clear();
            std::vector<int> fri;
            bool ret = user_mana_->GetFriends(user_id, &fri);
            if(false == ret)
            {
                cm.reply_status_ = GETFRIEND_FAILED;
            }
            else
            {
                cm.reply_status_ = GETFRIEND_SUCCESS;
            }

            cm.msg_type_ = GetFriendMsg_Resp;

            for(size_t i = 0; i < fri.size(); i++)
            {
                UserInfo tmp;
                user_mana_->GetUseInfo(fri[i], &tmp);

                Json::Value val;
                val["fakename"] = tmp.fakename_;
                val["school"] = tmp.school_;
                val["userid"] = tmp.userid_;

                cm.json_msg_.append(val);
            }

            send_que_->Push(cm);

        }

        void DealSendMsg(ChatMsg& cm)
        {
            int send_id = cm.user_id_;
            int recv_id = cm.json_msg_["recvmsgid"].asInt();
            std::string send_msg = cm.json_msg_["msg"].asString();

            cm.Clear();

            //消息发送失败时，给发送方返回
            UserInfo recv_ui;
            bool ret = user_mana_->GetUseInfo(recv_id, &recv_ui);

            //区分用户不存在和不在线
            //用户不存在：消息发送失败
            //用户不在线：消息保存下来。择机发送
            if(false == ret || OFFLINE == recv_ui.user_status_)
            {
                cm.msg_type_ = SendMsg_Resp;
                cm.reply_status_ = SENDMSG_FAILED;
                send_que_->Push(cm);
                return;
            }

            //消息发送成功时，给发送方返回
            cm.Clear();
            cm.msg_type_ = SendMsg_Resp;
            cm.reply_status_ = SENDMSG_SUCCESS;
            send_que_->Push(cm);

            //给接收方发送
            UserInfo send_ui;
            user_mana_->GetUseInfo(send_id, &send_ui);

            cm.Clear();
            cm.msg_type_ = PushMsg;
            cm.sockfd_ = recv_ui.tcp_socket_;
            cm.SetValue("peer_fakename", send_ui.fakename_);
            cm.SetValue("peer_school", send_ui.school_);
            cm.SetValue("peer_msg", send_msg);
            cm.json_msg_["peer_userid"] = send_ui.userid_;
            send_que_->Push(cm);

        }

    private:
        //侦听套接字
        int tcp_sock_;
        //侦听端口
        int tcp_port_;

        //用户管理模块实例化
        UserManager* user_mana_;

        //epoll操作句柄
        int epoll_fd_;

        //工作线程数量
        int thread_count_;

        //发送线程的队列
        MsgQueue<ChatMsg>* send_que_;

        //接收线程队列
        MsgQueue<ChatMsg>* recv_que_;

        //就绪的文件描述符队列
        MsgQueue<int>* ready_sockfd_que_;


};
