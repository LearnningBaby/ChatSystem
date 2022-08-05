// CDialogAddFriend.cpp: 实现文件
//

#include "pch.h"
#include "ChatSystemClient.h"
#include "CDialogAddFriend.h"
#include "afxdialogex.h"

#include "TcpSvr.h"
#include "ChatMsg.h"
#include "MsgQueue.h"

// CDialogAddFriend 对话框

IMPLEMENT_DYNAMIC(CDialogAddFriend, CDialogEx)

CDialogAddFriend::CDialogAddFriend(int user_id, CWnd* pParent /*=nullptr*/)
	:user_id_(user_id)
	,CDialogEx(IDD_ADDFRIEND, pParent)
	, m_fri_telnum_(_T(""))
{

}

CDialogAddFriend::~CDialogAddFriend()
{
}

void CDialogAddFriend::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ADDFRIENDNUM, m_fri_telnum_);
}


BEGIN_MESSAGE_MAP(CDialogAddFriend, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONADDFRIEND, &CDialogAddFriend::OnBnClickedButtonaddfriend)
END_MESSAGE_MAP()


// CDialogAddFriend 消息处理程序


void CDialogAddFriend::OnBnClickedButtonaddfriend()
{
	// TODO: 在此添加控件通知处理程序代码
		// 1.获取输入框内容
	UpdateData(TRUE);

	if (m_fri_telnum_.IsEmpty())
	{
		MessageBox("输入内容不能为空");
		return;
	}

	// 2.组织发送消息
	ChatMsg cm;
	cm.msg_type_ = AddFriend;
	//消息是谁发的
	cm.user_id_ = user_id_;
	//消息发给谁
	cm.json_msg_["telnum"] = m_fri_telnum_.GetString();

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
	CDialog::OnCancel();

	//此处不能立即拿到回复，写一个线程单独接收
}
