#include "ChatServer.hpp"

int main()
{
    ChatServer cs;
    int ret = cs.InitChatServer();
    if(ret < 0)
    {
        return 0;
    }

    ret = cs.StartChatServer();
    if(ret < 0)
    {
        return 0;
    }

    return 0;
}
