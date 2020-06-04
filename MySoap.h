////////////////////////////////
//SOAP通讯类 Server端 V1.0
//实现功能：初始化SOAP组件并开启线程等待获取数据命令
//当收到获取数据命令后，调用回调函数，从检测程序获取检测数据进行发送
//编写：阴同    日期：2017年1月
////////////////////////////////
#pragma once
#include <QThread>

#ifndef WINAPI
#define WINAPI __stdcall
#endif

typedef struct STRUCT_DETECT_INFO 
{
    int nMouldNumber;//模具序号
	QString sMouldID;//模具号
	int nCheckCount;//检测总数
	int nKickCount;//踢废总数

	int nCameraCount;//相机总数
	int nCameraCheckCount[50];//每个相机检测总数
	int nCameraKickCount[50];//每个相机踢废总数

	int nErrorTypeCount;//缺陷类型总数
	QString sErrorTypeName[50];//每个缺陷类型名称
	int nErrorTypeClass[50];//每个缺陷类型分类（瓶口、瓶底、瓶身）
	int nErrorTypeKickCount[50];//每个缺陷类型踢废个数

	int nStationCount;//工位总数
	QString sStationName[50];//工位名称
	int nStationCheckCount[50];//每个工位检测个数
	int nStationKickCount[50];//每个工位踢废个数

	STRUCT_DETECT_INFO()
	{
		nMouldNumber = 0;
		sMouldID = "";
		nCheckCount = 0;
		nKickCount = 0;
		nCameraCount = 0;
		nErrorTypeCount = 0;
		nStationCount = 0;
		for (int i=0;i<50;i++)
		{
			nCameraCheckCount[i] = 0;
			nCameraKickCount[i] = 0;
			sErrorTypeName[i] = "";
			nErrorTypeClass[i] = 0;
			nErrorTypeKickCount[i] = 0;
			sStationName[i] = "";
			nStationCheckCount[i] = 0;
			nStationKickCount[i] = 0;
		}
	}
}s_DetectInfo;

//回调函数指针声明
typedef void (WINAPI *SOAPCALLBACK)(s_DetectInfo* DetectInfo);

class MySoap
{
public:
	MySoap(void);
	MySoap(int nPort,SOAPCALLBACK pCallBackFunc);
	~MySoap(void);

	bool InitSoapServer();//初始化

	bool OpenSoap();//开启监听
	bool CloseSoap();//关闭监听
	bool IsOpen();//获取状态

	bool m_bIsOpen; //是否开启
	SOAPCALLBACK m_pCallBackFunc;				//回调函数指针

	int m_nSoapPort;//SOAP端口号

	class SoapServerThread : public QThread
	{
	public:
		bool m_isFreeze;
		bool m_bExit;
		SoapServerThread()
		{
			m_bExit=false;
			m_isFreeze=false;
		}
		void run();	
		void DetectThreadClosed();
	}m_SoapServerThread;//SOAP服务器线程 开启后打开端口监听并发送消息
};

