#pragma once
#include<mysql/mysql.h>
#include<vector>
#include<string>
#include<iostream>
#include <jsoncpp/json/json.h>
#include <mutex>

#define HOST "127.0.0.1"
#define DB "mychat"
#define USER "root"
#define PASSWD "123456"
#define DBPORT 3306

class DBserve
{
    public:
        DBserve()
        {
            mysql_ = NULL;
        }

        ~DBserve()
        {
            if(mysql_ != NULL)
            {
                mysql_close(mysql_);
            }
        }

        // 初始化mysql操作句柄，连接后台mysql服务器
        bool mysqlInit()
        {
            mysql_ = mysql_init(NULL);
            if(NULL == mysql_)
            {
                std::cout << "mysql句柄初始化失败" << std::endl;
                return false;
            }

            if(NULL == mysql_real_connect(mysql_, HOST, USER, PASSWD, DB, DBPORT, NULL, 0))
            {
                std::cout << "mysql连接失败" << std::endl;
                mysql_close(mysql_);
                return false;
            }
            mysql_set_character_set(mysql_, "utf8");
            return true;
        }

        //获取所有用户信息
        bool getAllUser(Json::Value* all_user)
        {
#define GETALLUSER "select * from user;"

            lock_.lock();
            //1.数据库查询
            if(false == mysqlQuey(GETALLUSER))
            {
                lock_.unlock();
                return false;
            }
            //2.获取结果集
            MYSQL_RES* res = mysql_store_result(mysql_);
            if(NULL == res)
            {
                lock_.unlock();
                return false;
            }

            lock_.unlock();

            //3.获取单行数据
            int row_nums = mysql_num_rows(res);//满足条件的数据有几行
            for(int i = 0; i < row_nums; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                //4.将单行数据调用起来，返回给调用者
                Json::Value tmp;
                tmp["userid"] = atoi(row[0]);
                tmp["fakename"] = row[1];
                tmp["school"] = row[2];
                tmp["telnum"] = row[3];
                tmp["passwd"] = row[4];
                all_user->append(tmp);
            }

            mysql_free_result(res);

            return true;
        }

        //获取单个用户所有好友信息
        //初始化阶段让用户管理模块维护起来
        //userid 用户id
        //f_id所有好友id
        bool getFriend(int userid, std::vector<int>* f_id)
        {
#define GETFRIEND "select friend from friendinfo where userid = '%d';"

            //1.格式化sql语句
            char sql[1024] = {0};
            sprintf(sql, GETFRIEND, userid);

            lock_.lock();
            //2.查询
            if(false == mysqlQuey(sql))
            {
                lock_.unlock();
               // std::cout << "in" << userid << std::endl;
                return false;
            }
            //3.获取结果集
            MYSQL_RES* res = mysql_store_result(mysql_);
            if(NULL == res)
            {
                lock_.unlock();
                //std::cout << sql << std::endl;
                return false;
            }

            lock_.unlock();

            //4.获取单行数据
            int row_nums = mysql_num_rows(res);//满足条件的数据有几行
            std::cout << row_nums << " " << sql << std::endl;
            for(int i = 0; i < row_nums; i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                f_id->push_back(atoi(row[0]));
                //std::cout << row[0] << std::endl;
            }

            mysql_free_result(res);
            return true;
        }

        //用户注册时，插入
        bool insertUser(int userid, std::string& fakename, const std::string& school, const std::string& telnum, const std::string& passwd)
        {
#define INSERTUSER "insert into user(userid, fakename, school, telnum, passwd) values('%d', '%s', '%s', '%s', '%s');"

            //1.格式化sql语句
            char sql[1024] = {0};
            sprintf(sql, INSERTUSER, userid, fakename.c_str(), school.c_str(), telnum.c_str(), passwd.c_str());

            //2.查询
            if(mysqlQuey(sql))
            {
                return false;
            }

            return true;
        }

        //添加好友
        bool insertFriend(int userid, int friendid)
        {
#define INSERTFRIEND "insert into friendinfo values('%d', '%d');"
            char sql[1024] = {0};
            sprintf(sql, INSERTFRIEND, userid, friendid);

            if(mysqlQuey(sql))
            {
                return false;
            }
            return true;
        }


    private:
        //判断sql请求
        bool mysqlQuey(const std::string& sql)
        {
            if(0 != mysql_query(mysql_, sql.c_str()))
            {
                std::cout << "sql语句请求失败" << sql << std::endl;
                std::cout << mysql_error(mysql_) << std::endl;
                return false;
            }
            return true;
        }
    private:
        MYSQL* mysql_;
        std::mutex lock_;
};
