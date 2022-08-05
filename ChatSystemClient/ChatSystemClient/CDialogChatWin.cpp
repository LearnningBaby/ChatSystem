// CDialogChatWin.cpp: 实现文件
//

#include "pch.h"
#include "ChatSystemClient.h"
#include "CDialogChatWin.h"
#include "afxdialogex.h"

#include "TcpSvr.h"
#include "ChatMsg.h"
#include "MsgQueue.h"
#include <string.h>
#include "CDialogAddFriend.h"
// CDialogChatWin 对话框

IMPLEMENT_DYNAMIC(CDialogChatWin, CDialogEx)

CDialogChatWin::CDialogChatWin(int userid, CWnd* pParent /*=nullptr*/)
	: user_id_(userid)
	,CDialogEx(IDD_DIALOG1, pParent)
	, m_input_(_T(""))
{
}

CDialogChatWin::~CDialogChatWin()
{
}


void CDialogChatWin::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTUSER, m_userlist_);
	DDX_Control(pDX, IDC_LISTOUTPUT, m_output);
	DDX_Text(pDX, IDC_EDITINPUT, m_input_);
	DDX_Control(pDX, IDC_EDITINPUT, m_input_edit_);
}


BEGIN_MESSAGE_MAP(CDialogChatWin, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONADDFRIEND, &CDialogChatWin::OnBnClickedButtonaddfriend)
	ON_BN_CLICKED(IDC_BUTTONSENDMSG, &CDialogChatWin::OnBnClickedButtonsendmsg)
	ON_LBN_SELCHANGE(IDC_LISTUSER, &CDialogChatWin::OnLbnSelchangeListuser)
END_MESSAGE_MAP()


// CDialogChatWin 消息处理程序


void CDialogChatWin::OnBnClickedButtonaddfriend()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogAddFriend caf(user_id_);
	caf.DoModal();
}


//点击发送触发函数
void CDialogChatWin::OnBnClickedButtonsendmsg()
{
	// TODO: 在此添加控件通知处理程序代码
	// 1.获取输入框内容
	UpdateData(TRUE);

	if (m_input_.IsEmpty())
	{
		MessageBox("输入内容不能为空");
		return;
	}

	// 2.组织发送消息
	ChatMsg cm;
	cm.msg_type_ = SendMsg;
	//消息是谁发的
	cm.user_id_ = user_id_;
	//消息发给谁
	cm.json_msg_["recvmsgid"] = send_user_id_;
	//消息内容
	cm.json_msg_["msg"] = m_input_.GetString();

	std::string msg;
	cm.GetMsg(&msg);

	TcpSvr* ts = TcpSvr::getinstance();
	if (NULL == ts)
	{
		MessageBox("获取tcp服务失败，请重试");
		return;
	}

	// 3.消息发送
	ts->Send(msg);

	// 4.接收应答，判断消息发送是否成功
	MsgQueue* mq = MsgQueue::GetInstance();
	if (NULL == mq)
	{
		MessageBox("获取消息队列失败");
		return;
	}

	msg.clear();
	mq->Pop(SendMsg_Resp, &msg);
	cm.Clear();
	cm.PraseChatMsg(-1, msg);

	// 5.添加到该好友的历史消息中
	for (size_t i = 0; i < fri_vec_.size(); i++)
	{
		if (send_user_id_ == fri_vec_[i].user_id_)
		{
			std::string tmp = "我 : ";
			tmp += m_input_.GetString();
			if (SENDMSG_SUCCESS == cm.reply_status_)
			{
				tmp += "send success";
			}
			else
			{
				tmp += "send failed";
			}
			fri_vec_[i].history_msg_.push_back(tmp);
			m_output.AddString(tmp.c_str());
		}
	}

	// 6.清空编辑框
	m_input_.Empty();
	m_input_edit_.SetWindowTextA(0);
	// 

}


void DealPushMsg(CDialogChatWin* cc)
{
	MsgQueue* mq = MsgQueue::GetInstance();
	if (NULL == mq)
	{
		return ;
	}
	while (1)
	{
		std::string msg;
		mq->Pop(PushMsg, &msg);

		ChatMsg cm;
		cm.PraseChatMsg(-1, msg);
		std::string peer_fakename = cm.json_msg_["peer_fakename"].asString();
		std::string peer_school = cm.json_msg_["peer_school"].asString();
		std::string peer_msg = cm.json_msg_["peer_msg"].asString();
		int peer_id = cm.json_msg_["peer_userid"].asInt();

		for (size_t i = 0; i < cc->fri_vec_.size(); i++)
		{
			if (peer_id == cc->fri_vec_[i].user_id_)
			{
				std::string tmp = peer_fakename + "-" + peer_school + " : " + peer_msg;
				cc->fri_vec_[i].history_msg_.push_back(tmp);
				if (peer_id == cc->send_user_id_)
				{
					cc->m_output.AddString(tmp.c_str());
				}
				else
				{
					cc->fri_vec_[i].msg_cnt_++;
				}
			}
		}
		cc->RefreshUserList();
	}
}


//被添加方
void DealPushAddFriendMsg(CDialogChatWin* cc)
{
	//
	// 1.获取消息队列实例化指针 & tcp实例化指针
	MsgQueue* mq = MsgQueue::GetInstance();
	if (NULL == mq)
	{
		return;
	}

	// 2.循环获取 PushAddFriendMsg 消息类型消息
	while (1)
	{
		std::string msg;
		mq->Pop(PushAddFriendMsg, &msg);

		ChatMsg cm;
		cm.PraseChatMsg(-1, msg);
		std::string adder_fakename = cm.json_msg_["adder_fakename"].asString();
		std::string adder_school = cm.json_msg_["adder_school"].asString();
		int adder_userid = cm.json_msg_["adder_userid"].asInt();

		// 3.通过获取的消息内容展示是哪个用户想要添加自己
		std::string show_msg = adder_fakename + " : " + adder_school + "希望添加你为好友";

		cm.Clear();
		int i = MessageBox(cc->m_hWnd, show_msg.c_str(), "添加好友", MB_YESNO);
		if (i == IDYES)
		{
			//同意添加
			//将新好友信息维护起来
			struct UserInfo ui;
			ui.fakename_ = adder_fakename;
			ui.school_ = adder_school;
			ui.user_id_ = adder_userid;
			ui.msg_cnt_ = 0;
			cc->fri_vec_.push_back(ui);

			//刷新用户列表
			cc->RefreshUserList();

			//组织应答
			cm.msg_type_ = PushADDFriendMsg_Resp;
			cm.reply_status_ = ADDFRIEND_SUCCESS;
			cm.user_id_ = cc->user_id_;//被添加方的id
			cm.json_msg_["userid"] = adder_userid;
		}
		else
		{
			//不同意
			//组织应答
			cm.msg_type_ = PushADDFriendMsg_Resp;
			cm.reply_status_ = ADDFRIEND_FAILED;
			cm.user_id_ = cc->user_id_;//被添加方的id
			cm.json_msg_["userid"] = adder_userid;
		}

		msg.clear();
		cm.GetMsg(&msg);

		// 4.根据不同结果返回应答
		TcpSvr* ts = TcpSvr::getinstance();
		if (NULL == ts)
		{
			continue;
		}

		ts->Send(msg);
	}

}


//添加方
void DealAddFriendResp(CDialogChatWin* cc)
{
	// 1.获取消息队列实例化指针 & tcp实例化指针
	MsgQueue* mq = MsgQueue::GetInstance();
	if (NULL == mq)
	{
		return;
	}

	// 2.循环获取 PushAddFriendMsg 消息类型消息
	while (1)
	{
		std::string msg;
		mq->Pop(AddFriend_Resp, &msg);

		ChatMsg cm;
		cm.PraseChatMsg(-1, msg);
		std::string content = cm.GetValue("content");
		MessageBox(cc->m_hWnd, content.c_str(), "添加好友应答", MB_OK);
		if (ADDFRIEND_FAILED == cm.reply_status_)
		{
			continue;
		}

		std::string be_adder_fakename = cm.json_msg_["peer_fakename"].asString();
		std::string be_adder_school = cm.json_msg_["peer_school"].asString();
		int be_adder_userid = cm.json_msg_["peer_userid"].asInt();

		//维护好友信息
		struct UserInfo ui;
		ui.fakename_ = be_adder_fakename;
		ui.school_ = be_adder_school;
		ui.user_id_ = be_adder_userid;
		ui.msg_cnt_ = 0;
		cc->fri_vec_.push_back(ui);

		//刷新用户列表
		cc->RefreshUserList();
	}
}


BOOL CDialogChatWin::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//接收消息线程
	std::thread recv_msg(DealPushMsg, this);
	recv_msg.detach();

	//接收好友请求线程
	std::thread recv_addfriendmsg(DealPushAddFriendMsg, this);
	recv_addfriendmsg.detach();

	//接收好友请求回复线程
	std::thread recv_addfriendrespmsg(DealAddFriendResp, this);
	recv_addfriendrespmsg.detach();

	// TODO:  在此添加额外的初始化
	// 1.获取tcp实例化指针
	TcpSvr* ts = TcpSvr::getinstance();
	if (NULL == ts)
	{
		MessageBox("获取tcp服务失败，请重试");
		return false;
	}

	// 2.组织获取好友信息的数据
	ChatMsg cm;
	cm.msg_type_ = GetFriendMsg;
	cm.user_id_ = user_id_;
	std::string msg;
	cm.GetMsg(&msg);
	ts->Send(msg);

	// 3.解析应答
	MsgQueue* mq = MsgQueue::GetInstance();
	if (NULL == mq)
	{
		MessageBox("获取消息队列失败");
		return false;
	}
	msg.clear();
	mq->Pop(GetFriendMsg_Resp, &msg);

	// 4.展示好友信息到userlist
	cm.Clear();
	cm.PraseChatMsg(-1, msg);
	for (size_t i = 0; i < cm.json_msg_.size(); i++)
	{
		struct UserInfo ui;
		ui.fakename_ = cm.json_msg_[i]["fakename"].asString();
		ui.school_ = cm.json_msg_[i]["school"].asString();
		ui.user_id_ = cm.json_msg_[i]["userid"].asInt();
		ui.msg_cnt_ = 0;

		if (i == 0)
		{
			send_user_id_ = ui.user_id_;
		}

		fri_vec_.push_back(ui);

	}

	//  5.刷新userlist
	RefreshUserList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CDialogChatWin::RefreshUserList()
{
	int count = m_userlist_.GetCount();
	//先清空
	for (int i = count; i >= 0; i--)
	{
		m_userlist_.DeleteString(i);
	}
	
	//再展示
	for (size_t i = 0; i < fri_vec_.size(); i++)
	{
		std::string tmp = fri_vec_[i].fakename_;
		//当该好友未读消息个数为0时
		if (fri_vec_[i].msg_cnt_ > 0)
		{
			tmp += " : ";
			tmp += std::to_string(fri_vec_[i].msg_cnt_);
		}

		m_userlist_.AddString(tmp.c_str());
	}
}

//userlist
void CDialogChatWin::OnLbnSelchangeListuser()
{
	// TODO: 在此添加控件通知处理程序代码
	// 1.获取点击文本内容
	int cur = m_userlist_.GetCurSel();//获取下标
	CString strText;
	m_userlist_.GetText(cur, strText);

	// 2.判断当前点击用户是哪一个，更改发送id
	for (size_t i = 0; i < fri_vec_.size(); i++)
	{
		if (strstr(strText, fri_vec_[i].fakename_.c_str()) != NULL)
		{
			send_user_id_ = fri_vec_[i].user_id_;
		}
	}

	// 3.清空output区
	int count = m_output.GetCount();
	//先清空
	for (int i = count; i >= 0; i--)
	{
		m_output.DeleteString(i);
	}

	// 4.展示该用户的历史消息
	for (size_t i = 0; i < fri_vec_.size(); i++)
	{
		if (send_user_id_ == fri_vec_[i].user_id_)
		{
			for (size_t j = 0; j < fri_vec_[i].history_msg_.size(); j++)
			{
				m_output.AddString(fri_vec_[i].history_msg_[j].c_str());
			}

			fri_vec_[i].msg_cnt_ = 0;
		}
	}

	// 5.刷新userlist
	RefreshUserList();
	//
}


BOOL CDialogChatWin::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		OnBnClickedButtonsendmsg();
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
	{
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
