#pragma once


// CDialogRegister 对话框

class CDialogRegister : public CDialogEx
{
	DECLARE_DYNAMIC(CDialogRegister)

public:
	CDialogRegister(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogRegister();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REGISTER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
private:
	// 用户昵称
	CString m_fakename;
	// 学校
	CString m_school;
	// 电话
	CString m_telnum;
	// 密码
	CString m_passwd;
public:
	afx_msg void OnBnClickedButtoncommit();
};
