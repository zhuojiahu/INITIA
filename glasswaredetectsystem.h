#ifndef GLASSWAREDETECTSYSTEM_H
#define GLASSWAREDETECTSYSTEM_H

#define ZUZHITONGSHI	//接口卡同时触发
#include <QtGui/QMainWindow>
#include <qlabel.h>

#include <QWidget>
#include <QDesktopWidget>
#include <QTranslator>
#include <QIcon>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QFileSystemWatcher>

#include "widget_title.h"
#include "widget_image.h"
#include "widget_info.h"
#include "UserManegeWidget.h"
#include "Widget_CarveSetting.h"
#include "widget_Management.h"
#include "widget_Help.h"
#include "widget_test.h"
#include "widget_count.h"
#include "widget_debug.h"
#include "widget_plc.h"
#include "widgetwarning.h"
#include "Widget_TestWighet.h"
#include "g_soap_listener.h"

#include "CombineResult.h"
#include "ConfigInfo.h"
#include "myQueue.h"
#include "clogFile.h"
#include "DetectThread.h"
#include "CIOCard.h"
#include "cmyerrorlist.h"
#include "stateTool.h"
#include <Mmsystem.h>
#pragma comment( lib,"winmm.lib" )

struct ImageSave
{
	QImage* pThat;
	QImage m_Picture;
};

class GlasswareDetectSystem : public QDialog
{
	Q_OBJECT

public:
	GlasswareDetectSystem(QWidget *parent = 0, Qt::WFlags flags = 0);
	~GlasswareDetectSystem();

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);
public:
	void Init();
	void InitImage();//初始化图像
	void CloseCam();//关闭相机
	void InitIOCard();
	void ReleaseIOCard();
	void ReleaseImage();
	void ReleaseAll();//释放资源
	void GrabCallBack(const s_GBSIGNALINFO *SigInfo);//采集回调函数
	void PutImagetoDetectList(int iRealCameraSN, int m_iImageCount);
	QString getVersion(QString strFullName);
	void StartDetectThread();	//开启检测线程
	void initDetectThread();	//开启检测线程
	void AlertFunction(int,int,int);	//报警功能
	void ShowCheckSet(int nCamIdx = 0,int signalNumber = 0);
	void writeLogText(QString string,e_SaveLogType eSaveLogType);
	bool changeLanguage(int nLangIdx);
	void InitCamImage(int iCameraNo);
	void StartCamGrab();
	e_CurrentMainPage getCurrentPage(){return m_eCurrentMainPage;};
	void CarveImage(uchar* pRes,uchar* pTar,int iResWidth,int iResHeight,int iTarX,int iTarY,int iTarWidth,int iTarHeight);
	bool RoAngle(uchar* pRes,uchar* pTar,int iResWidth,int iResHeight,int iAngle);
	void InitLastData();
	bool CheckLicense();
	void MonitorLicense();
	void showAllert();
	//高低踢废率报警
	void ShowAlelertStatus(int,int,int,QString=0);
signals:
	void signals_intoInfoWidget();
	void signals_intoCarveSettingWidget();
	void signals_intoManagementWidget();
	void signals_intoTestWidget();
	void signals_intoPLCWidget();
	void signals_intoCountWidget();
	void signals_intoDebugWidget();
	void signals_MessageBoxMainThread(s_MSGBoxInfo);	//通知主线程弹出提示对话框
	void signals_writeLogText(QString string,e_SaveLogType eSaveLogType);
	void signals_clear();
	void signals_ShowWarning(int , QString );
	void signals_HideWarning(int);
public slots:
	void slots_turnPage(int current_page, int iPara = 0);
	void slots_MessageBoxMainThread(s_MSGBoxInfo msgbox);				//子线程弹出对话框
	void slots_OnBtnStar();
	void slots_OnExit(bool ifyanz=false);
	void slots_ShowPLCStatus(int iStatus);
private slots:
	void slots_UpdateCoderNumber();
	void slots_updateCameraState(int nCam,int nMode = 0);
	void slots_SetCameraStatus(int nCam,int mode);
	void directoryChanged(QString path);
	void fileChanged(QString path);
	  
public:
	//初始化
	void Initialize();
	void InitParameter();//初始化参数
	void ReadIniInformation();
	void LoadParameterAndCam();//加载参数初始化相机
	void InitGrabCard(s_GBINITSTRUCT struGrabCardPara,int index);//初始化采集卡
	void InitCam();//初始化相机
	void SetCarvedCamInfo();
	void SetCombineInfo();
	void InitCheckSet();
	void ReadCorveConfig();
	void initInterface();	//初始化界面
	bool leaveWidget();
	void closeWidget();
	int ReadImageSignal(int nImageNum);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
private:
	QFileSystemWatcher fsWatcher;
	int iLastStatus;
	int iLastAbnormalCamera;
	bool bSaveImage;
public:
	int iLastAbnormalStep;
public:
	QStackedWidget *statked_widget;
	WidgetTitle *title_widget;				//标题栏 
	QWidget *widgetInfoContainer;//系统信息界面
	widget_info *info_widget;//QWT控件
	UserManegeWidget *widget_UserManege;
	WidgetCarveSetting *widget_carveSetting;			//剪切设置
	WidgetManagement *widget_Management;			//品种设置
	widget_Help *widgetHelp;			//品种设置
	WidgetTest *test_widget;
	Widget_Count *widget_count;
	Widget_Debug *widget_Debug;
	Widget_PLC *widget_PLC;
	QWidget *widget_alg;
	WidgetWarning *widget_Warning;
	Widget_TestWighet *widget_testWidget;
	QString skin_name;						//主窗口背景图片的名称
	QPixmap skin;
	//状态栏控件
	QList<CameraStatusLabel *> cameraStatus_list;
	QTimer *timerUpdateCoder;
	QVector<QString> m_vstrPLCInfoType;		//错误类型
	QLabel *labelCoder;
	QLabel *labelPLC;
	QLabel *labelSpeed;
	QLabel *labelUnknowMoldNumber;
	QWidget *stateBar;
	CLogFile Logfile;
public:
	e_CurrentMainPage m_eCurrentMainPage;
	e_CurrentMainPage m_eLastMainPage;
	int iLastPage;
	s_Permission sPermission;
	//配置信息结构体
	s_ConfigInfo m_sConfigInfo;
	s_SystemInfo m_sSystemInfo;
	s_ErrorInfo m_sErrorInfo;
	s_GBINITSTRUCT struGrabCardPara[CAMERA_MAX_COUNT];
	//运行信息结构体
	s_RunningInfo m_sRunningInfo;
	//取样结构体
	s_SampleInfo m_sSampleInfo;
	//相机结构体
	s_RealCamInfo m_sRealCamInfo[CAMERA_MAX_COUNT];
	s_CarvedCamInfo m_sCarvedCamInfo[CAMERA_MAX_COUNT];
	CMyQueue m_queue[CAMERA_MAX_COUNT];			//图像存储链表		
	CDetectElement m_detectElement[CAMERA_MAX_COUNT];
	QMutex mutexDetectElement[CAMERA_MAX_COUNT];
	QMutex m_stressStatue[CAMERA_MAX_COUNT];
	CIOCard *m_vIOCard[IOCard_MAX_COUNT];		//IO卡队列
	CMyErrorList m_ErrorList;
	int m_iGrabCounter[CAMERA_MAX_COUNT];
	int m_iImgGrabCounter[CAMERA_MAX_COUNT];
	int m_CameraBuffer;
	int m_iNewsetTriggerSignalCount2;
	//图像综合相关参数
	CCombineRlt m_cCombine;
	CCombineRlt m_cCombine1;
	int m_CombineCameraCount;
	int m_CombineCameraCount1;
	//互斥操作
	QMutex m_mutexmGrab;						//相机操作互斥
	QMutex m_mutexmCarve;						//相机操作互斥
	QMutex m_mutexmIOCardOperation;				//IO卡操作互斥
	QMutex m_mutexmSendResult;
	QMutex m_mutexmCheckSet;					//检测设置互斥
	QMutex m_mutexmLogfile;					
	QMutex mutexListDetect[CAMERA_MAX_COUNT];
	QMutex m_mutexGlasswareNumber;
	QMutex m_logSqlite;
	QMutex m_mutexFailNum;
	QMutex m_CountNumber;
	GrabberEvent m_EventGrab;					//采集完成事件
	IOCardEvent m_EventIOCard;					//接口卡发送事件
	QVector<QRgb> m_vcolorTable;				//生成灰度颜色表
	//算法使用的检测列表
	s_InputCheckerAry CherkerAry;

	//检测线程相关参数
	DetectThread *pdetthread[CAMERA_MAX_COUNT];
	int nCPUNumber;
	int m_iLastKickNo0;
	int m_iLastKickNo1;

	int m_iDetectStep[CAMERA_MAX_COUNT];//0进入检测,1旋转，2检测，3获取结果，4踢废，5存图，6刷错误列表，7更新图像，8结束。
	int m_iDetectSignalNo[CAMERA_MAX_COUNT];
	bool bIsGetSample[256];
	QMutex mutexSetSample;
	BOOL m_bIsThreadDead;			//杀死线程，关闭窗口结束线程
	BOOL m_bIsIOCardThreadDead;			//杀死线程，关闭窗口结束线程
	BOOL m_bShowImage[CAMERA_MAX_COUNT];
	BOOL m_bSaveImage[CAMERA_MAX_COUNT];//是否保存图像

	CBottleCheck m_cBottleCheck[CAMERA_MAX_COUNT];	// 算法类对象 [10/26/2010 GZ]
	CBottleModel m_cBottleModel;// 模板设置对话框 [10/26/2010 GZ]
	CBottleCheck m_cBottleRotate[CAMERA_MAX_COUNT];	
	CBottleCheck m_cBottleStress[CAMERA_MAX_COUNT];	
	CBottleCheck m_cBottleRotateCarve;
	g_soap_listener *pg_soap_listener;
	DWORD m_iGrabCallBackCount[CAMERA_MAX_COUNT];		//采集回调次数
	//测试用
	bool blostImage[CAMERA_MAX_COUNT];
	QString sVersion;
	int imgTime;
	bool uniqueFlag;
	QString SaveDataPath;
public:
	ImageSave m_SavePicture[CAMERA_MAX_COUNT];
	double MaxRate;
	double MinRate;
	QList<time_t> AertNumber[ERRORTYPE_MAX_COUNT];
	CSpendTime timeCost[CAMERA_MAX_COUNT];
	int surplusDays;
};
#endif // GLASSWAREDETECTSYSTEM_H

