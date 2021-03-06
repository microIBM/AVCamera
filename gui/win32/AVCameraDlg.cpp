
// AVCameraDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "win32.h"
#include "AVCameraDlg.h"
#include "afxdialogex.h"

#include <opencv2/core/core.hpp>

#include <boost/signals2/signal.hpp>
#include <boost/bind.hpp>
#include <winuser.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// wchar_t to string
void Wchar_tToString(std::string& szDst, wchar_t *wchar)
{
	wchar_t * wText = wchar;
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP,NULL,wText,-1,NULL,0,NULL,FALSE);// WideCharToMultiByte的运用
	char *psText; // psText为char*的临时数组，作为赋值给std::string的中间变量
	psText = new char[dwNum];
	WideCharToMultiByte (CP_OEMCP,NULL,wText,-1,psText,dwNum,NULL,FALSE);// WideCharToMultiByte的再次运用
	szDst = psText;// std::string赋值
	delete []psText;// psText的清除
}

// string to wstring
void StringToWstring(std::wstring& szDst, std::string str)
{
	std::string temp = str;
	int len=MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, NULL,0); 
	wchar_t * wszUtf8 = new wchar_t[len+1]; 
	memset(wszUtf8, 0, len * 2 + 2); 
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, len);
	szDst = wszUtf8;
	std::wstring r = wszUtf8;
	delete[] wszUtf8;
}

// CAVCameraDlg 对话框

CAVCameraDlg::CAVCameraDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAVCameraDlg::IDD, pParent)
	, _file_path(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAVCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_FILE_PATH, _file_path);
}

BEGIN_MESSAGE_MAP(CAVCameraDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CAVCameraDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CAVCameraDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CAVCameraDlg::OnBnClickedButtonStop)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_FILE_BROWSER, &CAVCameraDlg::OnBnClickedButtonFileBrowser)
END_MESSAGE_MAP()


// CAVCameraDlg 消息处理程序

BOOL CAVCameraDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标


	CButton* pButton = (CButton*)GetDlgItem(IDC_BUTTON_START);
	ASSERT(pButton);
	pButton->ModifyStyle(0, BS_BITMAP);
	pButton->SetBitmap(LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_PNG_START)));

	// TODO: 在此添加额外的初始化代码
	_dc = GetDlgItem(IDC_VIDEO_PLAYBACK)->GetDC();

	_camera.refresh_sign.connect(boost::bind(&CAVCameraDlg::UpdateVideoFrame, this));
	UpdateStatus(VideoMgr::CREATED);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAVCameraDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CAVCameraDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CAVCameraDlg::UpdateStatus(VideoMgr::CameraSatus status)
{
	switch (status)
	{
	case VideoMgr::CREATED:
	case VideoMgr::STOPPED:
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(true);
		GetDlgItem(IDC_BUTTON_PAUSE)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(false);
		break;
	case VideoMgr::PAUSED:
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(true);
		GetDlgItem(IDC_BUTTON_PAUSE)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(true);
		break;
	case VideoMgr::RECORDING:
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON_PAUSE)->EnableWindow(true);
		GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(true);
		break;
	}
}

void CAVCameraDlg::OnBnClickedButtonStart()
{
	UpdateData(TRUE);
	if(_file_path.GetLength()<=0) {AfxMessageBox(L"请设置视频保存位置");return;}
	UpdateStatus(VideoMgr::RECORDING);
	int width = 640, height = 480, channel = 3, bit_rate = 1815484;
	if(_img.IsNull())
	{
		_img.Create(width, height, channel * 8);
	}
	else
	{
		if(_img.GetWidth() != width || _img.GetHeight() != height || _img.GetBPP() == channel)
		{
			_img.Destroy();
			_img.Create(width, height, channel * 8);
		}
	}
	std::string tempstr;
	Wchar_tToString(tempstr, _file_path.GetBuffer());
	_camera.start(tempstr, 640, 480, 1815484);
}

void CAVCameraDlg::OnBnClickedButtonPause()
{
	UpdateStatus(VideoMgr::PAUSED);
	_camera.pause();
}

void CAVCameraDlg::OnBnClickedButtonStop()
{
	UpdateStatus(VideoMgr::STOPPED);
	_camera.stop();
}

void CAVCameraDlg::UpdateVideoFrame()
{
	cv::Mat mat;
	_camera.get_curr_frame(mat);
	if(mat.data == nullptr || _img.IsNull()) return;
	
	//convert Mat to CImage
	uchar* ps;
	uchar* pimg = (uchar*)_img.GetBits();
	int step = _img.GetPitch();
	for (int i = 0; i < mat.rows; ++i)
	{
		ps = (mat.ptr<uchar>(i));
		for ( int j = 0; j < mat.cols; ++j )
		{
			if ( mat.channels() == 3 ) //3 channels
			{
				for (int k = 0 ; k < 3; ++k )
				{
					*(pimg + i*step + j*3 + k ) = ps[j*3 + k];
				}
			}
		}
	}
	RECT pic_rect,dst_rect;
	GetDlgItem(IDC_VIDEO_PLAYBACK)->GetWindowRect(&pic_rect);
	dst_rect.left = dst_rect.top = 0;
	dst_rect.right = pic_rect.right - pic_rect.left;
	dst_rect.bottom = pic_rect.bottom - pic_rect.top;
	_img.Draw(_dc->GetSafeHdc(), dst_rect, Gdiplus::InterpolationModeBilinear);

}

void CAVCameraDlg::OnClose()
{
	_camera.exit();
	ReleaseDC(_dc);
	CDialogEx::OnClose();
}


void CAVCameraDlg::OnBnClickedButtonFileBrowser()
{
	UpdateData(TRUE);
	TCHAR szFilters[]= _T("MP4 Files (*.mp4)|*.mp4|All Files (*.*)|*.*||");
	CFileDialog fileDlg(FALSE, _T("mp4"), _file_path//_T("*.mp4")
		, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters
		, this, 512);
	if(fileDlg.DoModal() == IDOK)
	{
		_file_path = fileDlg.GetPathName();
	}
	UpdateData(FALSE);
}