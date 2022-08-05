#pragma once

#include<string>
#include<vector>
#include<thread>

struct UserInfo
{
public:
	std::string fakename_;
	std::string school_;
	int user_id_;
	//历史和好友发送的消息
	std::vector<std::string> history_msg_;
	//未读的消息个数
	int msg_cnt_;
};

// CDialogChatWin 对话框

class CDialogChatWin : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogChatWin)

public:
	CDialogChatWin(int userid, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogChatWin();

	void RefreshUserList();//刷新函数(刷新userlist区域)

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif
	 
	int user_id_;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonaddfriend();
	afx_msg void OnBnClickedButtonsendmsg();
	virtual BOOL OnInitDialog();
	//userlist区
	CListBox m_userlist_;

	//好友信息
	std::vector<struct UserInfo> fri_vec_;
	//当前消息发送给该id好友，随着用户点击，动态变化
	int send_user_id_;
	afx_msg void OnLbnSelchangeListuser();
	// 消息展示区
	CListBox m_output;
	// 输入的消息
	CString m_input_;
	// 负责清空发送窗口消息
	CEdit m_input_edit_;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
