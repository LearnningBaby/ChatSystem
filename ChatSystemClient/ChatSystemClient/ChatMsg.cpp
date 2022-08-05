#include "pch.h"
#include"ChatMsg.h"


class JsonUtil {
public:
    static bool Serialize(const Json::Value& value, std::string* body) {
        Json::StreamWriterBuilder swb;
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

        std::stringstream ss;
        int ret = sw->write(value, &ss);
        if (ret != 0) {
            return false;
        }
        *body = ss.str();
        return true;
    }

    static bool UnSerialize(const std::string& body, Json::Value* value) {
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

        std::string err;
        bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), value, &err);
        if (ret == false) {
            return false;
        }
        return true;
    }
};



ChatMsg::ChatMsg()
{
    sockfd_ = -1;
    msg_type_ = -1;
    user_id_ = -1;
    reply_status_ = -1;
    json_msg_.clear();
}

ChatMsg::~ChatMsg()
{}

//�ṩ�����л��ӿ� - �������֮���ṩ�����л�
int ChatMsg::PraseChatMsg(int sockfd, const std::string& msg)
{
    //1.����jsoncpp�ķ����л��ӿ�
    Json::Value tmp;
    bool ret = JsonUtil::UnSerialize(msg, &tmp);
    if (false == ret)
    {
        return -1;
    }

    //2.��ֵ
    sockfd_ = sockfd;
    msg_type_ = tmp["msg_type"].asInt();
    user_id_ = tmp["user_id"].asInt();
    reply_status_ = tmp["reply_status"].asInt();
    json_msg_ = tmp["json_msg"];

    return 0;
}

//�ṩ���л��ӿ�-�ظ�Ӧ��ʱʹ��
//msg��ȡ���л���ϵ��ַ���
bool ChatMsg::GetMsg(std::string* msg)
{
    Json::Value tmp;
    tmp["msg_type"] = msg_type_;
    tmp["user_id"] = user_id_;
    tmp["reply_status"] = reply_status_;
    tmp["json_msg"] = json_msg_;

    return JsonUtil::Serialize(tmp, msg);
}

//��ȡjson_msg_���е�valueֵ
std::string ChatMsg::GetValue(const std::string& key)
{
    if (!json_msg_.isMember(key))
    {
        return "";
    }

    return json_msg_[key].asString();
}

//����json_msg_���е�kv��ֵ��
void ChatMsg::SetValue(const std::string& key, const std::string& value)
{
    json_msg_[key] = value;
}

void ChatMsg::SetValue(const std::string& key, int value)
{
    json_msg_[key] = value;
}

void ChatMsg::Clear()
{
    user_id_ = -1;
    msg_type_ = -1;
    reply_status_ = -1;
    json_msg_.clear();
}