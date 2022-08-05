#pragma once
#include<string>
#include<vector>
#include<unordered_map>
#include<string>
#include<pthread.h>
#include "DBserve.hpp"

enum UserStatus
{
    OFFLINE, //0，不在线
    ONLINE   //1,在线
};

class UserInfo
{
public:
    //注册时
    UserInfo(const std::string& fakename, const std::string& school, const std::string& telnum, const std::string& passwd, int userid)
        :fakename_(fakename)
        ,school_(school)
        ,telnum_(telnum)
        ,passwd_(passwd)
        ,userid_(userid)
        ,user_status_(OFFLINE)
        ,tcp_socket_(-1)
    {

        friend_id_.clear();
    }

    //从数据库拿时（初始化）
    UserInfo()
    {}

    ~UserInfo()
    {}

public:
    std::string fakename_;
    std::string school_;
    std::string telnum_;
    std::string passwd_;

    int userid_;
    //用户状态
    int user_status_;

    //登录客户端对应的套接字描述符，用来区分到底是哪一个客户
   int tcp_socket_;
    std::vector<int> friend_id_;

};

class UserManager
{
public:
    UserManager()
    {
        user_map_.clear();
        pthread_mutex_init(&map_lock_, NULL);
        //如果一开始就从0进行分配，一定是不对的
        //因为用户管理类还会从数据库中已经存在的用户信息读回来
        prepare_id_ = -1;
        db_ = NULL;
    }

    ~UserManager()
    {
        pthread_mutex_destroy(&map_lock_);

        if(db_)
        {
            delete db_;
            db_ = NULL;
        }
    }

    //初始化
    bool InitUserMana()
    {
        //1.连接数据库
        db_ = new DBserve();
        if(NULL == db_)
        {
            std::cout<< "数据库类创建失败"<< std::endl;
            return false;
        }

        if(false == db_->mysqlInit())
        {
            return false;
        }

        //2.查询所有用户信息，维护
        Json::Value all_user;
        if(false == db_->getAllUser(&all_user))
        {
            return false;
        }

        for(int i = 0; i < (int)all_user.size(); i++)
        {
            //个人信息
            UserInfo ui;
            ui.fakename_ = all_user[i]["fakename"].asString();
            ui.school_ = all_user[i]["school"].asString();
            ui.telnum_ = all_user[i]["telnum"].asString();
            ui.passwd_ = all_user[i]["passwd"].asString();
            ui.userid_ = all_user[i]["userid"].asInt();
            ui.user_status_ = OFFLINE;

            //好友信息
            //std::cout << "userid: "<< ui.userid_ << std::endl;
            db_->getFriend(ui.userid_, &ui.friend_id_);

            //加锁，防止插入数据时，用户查询迭代器失效
            pthread_mutex_lock(&map_lock_);
            user_map_[ui.userid_] = ui;
            if(prepare_id_ < ui.userid_)
            {
                prepare_id_ = ui.userid_ + 1;
            }
            pthread_mutex_unlock(&map_lock_);
        }

        return true;
    }

    //处理注册请求
    //如果注册成功，通过user_id告诉注册的客户端，id是什么
    int DealRegister(const std::string& fakename, const std::string& school, const std::string& telnum, const std::string& passwd, int* user_id)
    {
        //1.判断注册信息是否为空
        if(0 == fakename.size() || 0 == school.size() || 0 == telnum.size() || 0 == passwd.size())
        {
            *user_id = -2;
            return -1;
        }

        //2.判断用户是否注册过
        //加锁防止同时用同一电话注册,和同时注册
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.begin();
        while(iter != user_map_.end())
        {
            if(iter->second.telnum_ == telnum)
            {
                *user_id = -2;
                pthread_mutex_unlock(&map_lock_);
                return -1;
            }
            iter++;
        }

        //3.创建userinfo，分配user_id, 保存用户信息
        UserInfo ui(fakename, school, telnum, passwd, prepare_id_);
        *user_id = prepare_id_;

        user_map_[prepare_id_] = ui;
        prepare_id_++;
        pthread_mutex_unlock(&map_lock_);

        //4.插入数据库
        db_->insertUser(ui.userid_, ui.fakename_, ui.school_, ui.telnum_, ui.passwd_);
        return 0;
    }

    //处理登录请求
    //sockfd:服务端为客户端创建的新连接套接字
    int DealLogin(const std::string& telnum, const std::string& passwd, int sockfd)
    {
        //1.判断字段是否为空
        if(telnum.size() == 0 || passwd.size() == 0)
        {
            return -1;
        }

        //2.判断用户是否存在
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.begin();
        while(iter != user_map_.end())
        {
            if(iter->second.telnum_ == telnum)
            {
                break;
            }
            iter++;
        }
        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return -1;
        }

        //3.判断密码是否正确
        if(iter->second.passwd_ != passwd)
        {
            pthread_mutex_unlock(&map_lock_);
            return -1;
        }

        //4.更改用户登录状态信息
        iter->second.user_status_ = ONLINE;
        int userid = iter->second.userid_;
        iter->second.tcp_socket_ = sockfd;
        pthread_mutex_unlock(&map_lock_);
        return userid;
    }

    //是否在线
    int IsLogin(int userid)
    {
        //判断用户是否存在
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.find(userid);
        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return -1;
        }

        if(iter->second.user_status_ == OFFLINE)
        {
            pthread_mutex_unlock(&map_lock_);
            return OFFLINE;
        }

        pthread_mutex_unlock(&map_lock_);
        return ONLINE;
    }

    int IsLogin(const std::string& telnum, UserInfo* ui)
    {
        //判断用户是否存在
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.begin();
        while(iter != user_map_.end())
        {
            if(iter->second.telnum_ == telnum)
            {
                break;
            }
            iter++;
        }

        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return -1;
        }

        *ui = iter->second;
        if(OFFLINE == iter->second.user_status_)
        {
            pthread_mutex_unlock(&map_lock_);
            return OFFLINE;

        }
        pthread_mutex_unlock(&map_lock_);

        return ONLINE;

    }

     // 获取用户信息（发送消息）
    bool GetUseInfo(int userid, UserInfo* ui)
    {
        //判断用户是否存在
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.find(userid);
        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return false;
        }

        //将找到的数据传递回去
        *ui = iter->second;
        pthread_mutex_unlock(&map_lock_);
        return true;
    }

    //拉取好友信息
    bool GetFriends(int userid, std::vector<int>* friendid)
    {
        //判断用户是否存在
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.find(userid);
        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return false;
        }

        //将找到的数据传递回去
        *friendid = iter->second.friend_id_;
        pthread_mutex_unlock(&map_lock_);
        return true;
    }

    //加好友
    //如果没有同意，不会进入此程序
    //因此两用户一定存在
    void SetFriend(int userid1, int userid2)
    {
        //1.找userid1，把userid2放到userid1中
        //判断用户是否存在
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.find(userid1);
        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return;
        }
        iter->second.friend_id_.push_back(userid2);

        //2.找userid2，把userid1放到userid2中
        iter = user_map_.find(userid1);
        if(iter == user_map_.end())
        {
            pthread_mutex_unlock(&map_lock_);
            return;
        }
        iter->second.friend_id_.push_back(userid1);
        
        //3.插入到数据库
        db_->insertFriend(userid1, userid2);
        db_->insertFriend(userid2, userid1);
        pthread_mutex_unlock(&map_lock_);
    }

    void SetUserOffLine(int sockfd)
    {
        pthread_mutex_lock(&map_lock_);
        auto iter = user_map_.begin();
        while(iter != user_map_.end())
        {
            if(iter->second.tcp_socket_ == sockfd)
            {
                iter->second.user_status_ = OFFLINE;
            }
            iter++;
        }
        pthread_mutex_unlock(&map_lock_);

    }
private:
    std::unordered_map<int, UserInfo> user_map_;
    pthread_mutex_t map_lock_;

    //针对注册用户分配的id
    int prepare_id_;

    //数据库管理模块实例化指针
    DBserve* db_;

};
