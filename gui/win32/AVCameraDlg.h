
// AVCameraDlg.h : 头文件
//

#pragma once
#include "camera.hpp"

// CAVCameraDlg 对话框
class CAVCameraDlg : public CDialogEx
{
// 构造
public:
	CAVCameraDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_WIN32_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonFileBrowser();
	DECLARE_MESSAGE_MAP()

protected:
	void UpdateStatus(VideoMgr::CameraSatus status);
	void UpdateVideoFrame();

private:
	VideoMgr::Camera _camera;
	CImage _img;
	CDC* _dc;
	CString _file_path;
};
