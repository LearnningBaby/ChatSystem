// CDialogRegister.cpp: 实现文件
//

#include "pch.h"
#include "ChatSystemClient.h"
#include "CDialogRegister.h"
#include "afxdialogex.h"

#include "ChatMsg.h"
#include "MsgQueue.h"
#include "TcpSvr.h"

// CDialogRegister 对话框

IMPLEMENT_DYNAMIC(CDialogRegister, CDialogEx)

CDialogRegister::CDialogRegister(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REGISTER, pParent)
	, m_fakename(_T(""))
	, m_school(_T(""))
	, m_telnum(_T(""))
	, m_passwd(_T(""))
{

}

CDialogRegister::~CDialogRegister()
{
}

void CDialogRegister::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDITFAKENAME, m_fakename);
	DDX_Text(pDX, IDC_EDITSCHOOL, m_school);
	DDX_Text(pDX, IDC_EDITTELNUM, m_telnum);
	DDX_Text(pDX, IDC_EDITPASSWD, m_passwd);
}


BEGIN_MESSAGE_MAP(CDialogRegister, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTONCOMMIT, &CDialogRegister::OnBnClickedButtoncommit)
END_MESSAGE_MAP()


// CDialogRegister 消息处理程序


void CDialogRegister::OnBnClickedButtoncommit()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	if (m_fakename.IsEmpty() || m_passwd.IsEmpty() || m_school.IsEmpty() || m_telnum.IsEmpty())
	{
		MessageBox("输入内容不能为空");
		return;
	}

	// 2.组织登录消息（ChatMsg）
	ChatMsg cm;
	cm.msg_type_ = Register;
	//cm.SetValue("telnum", m_telnum.GetString());
	//cm.SetValue("passwd", m_passwd.GetString());
	cm.json_msg_["telnum"] = m_telnum.GetString();
	cm.json_msg_["passwd"] = m_passwd.GetString();
	cm.json_msg_["school"] = m_school.GetString();
	cm.json_msg_["fakename"] = m_fakename.GetString();
	std::string msg;
	cm.GetMsg(&msg);

	// 3.获取tcp服务实例化指针
	TcpSvr* ts = TcpSvr::getinstance();
	if (NULL == ts)
	{
		MessageBox("获取tcp服务失败，请重试");
		return;
	}

	// 4.发送登录消息
	ts->Send(msg);

	// 5.获取消息队列的实例化指针
	MsgQueue* mq = MsgQueue::GetInstance();
	if (NULL == mq)
	{
		MessageBox("获取消息队列失败");
		return;
	}

	msg.clear();
	mq->Pop(Register_Resp, &msg);
	// 6.获取登录应答当中的应答状态
	cm.Clear();
	cm.PraseChatMsg(-1, msg);

	// 7.判断登录应答当中的应答状态（LOFIN_SUCCESS/LOGINFILED）
	if (REGISTER_SUCCESS == cm.reply_status_)
	{
		MessageBox("注册成功");

		//退出当前注册页面----回到登录界面
		CDialog::OnCancel();
	}
	else
	{
		MessageBox("注册失败，该电话已被注册，请重试");
	}
}
