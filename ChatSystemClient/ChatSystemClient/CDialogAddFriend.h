#pragma once


// CDialogAddFriend 对话框

class CDialogAddFriend : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogAddFriend)

public:
	CDialogAddFriend(int user_id, CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogAddFriend();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADDFRIEND };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	// 好友电话号码
	CString m_fri_telnum_;
	int user_id_;
	afx_msg void OnBnClickedButtonaddfriend();
};
