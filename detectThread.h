#ifndef DETECTTHREAD_H
#define DETECTTHREAD_H

#include <QThread>
#include "stateTool.h"
#define BLACK_PIC 10
#pragma once 
class DetectThread : public QThread
{
	Q_OBJECT

public:
	DetectThread(QObject *parent);
	~DetectThread();
public:
	void run();
	void WaitThreadStop();
signals:
	void signals_updateActiveImg(int nCamNo,int nImgNo,double dCostTime,int tmpResult);
	void signals_updateCheckInfo(int IOCardNum,int checkResult,int nGrabImageCount,int type = 0);
	void signals_AddErrorTableView(int nCamSN,int nSignalNo,int nErrorType);
	void signals_upDateList(int iCam,int iErrorType);
	void signals_upDateCamera(int nCam,int nMode = 0);
	void signals_updateCameraFailureRate();
	void signals_updateImage(QImage*, QString, QString, QString, QString, QString, QList<QRect> ,int );
	void signals_ShowDebugLog(QString str,QColor color = Qt::black);
	void signals_showspeed(int);
private:
	void DetectNormal(CGrabElement *pElement=NULL);
	void DetectStress(CGrabElement *pElement=NULL);
	void rotateImage(CGrabElement *pElement);
	void checkImage(CGrabElement *pElement,int iCheckMode);
	bool getCheckResult(CGrabElement *pElement);

	void kickOutSample(int nSignalNo, int result);
	void kickOutBad(int nSignalNo);
	void CountDefectIOCard0(int nSignalNo,int tmpResult);
	void CountDefectIOCard1(int nSignalNo,int tmpResult);

	void saveImage(CGrabElement *pElement);
	void addErrorImageList(CGrabElement *pElement);
	void upDateState(QImage* myImage, int signalNo,double costTime, int nMouldID, QList<QRect>, int);
	//获取检测模点的总数据
	void GetModelDotData(CGrabElement *pElement);
	//统计运行时的数据
	void CountRuningData(int cameraNumber);
	//计算一体机的采集速度
	void CountGrabRate(int cameraNumber);
	//计算一体机检测个数
	void CountCheckNumber();
	//判断是否因为指定缺陷报警
	void ifSendAret(int);
private:

	int iStressCamera;
	bool m_bStopThread;					//结束检测

	bool bCheckResult[CAMERA_MAX_COUNT];
	int iErrorType;
	int iMaxErrorType;
	int iMaxErrorArea;

	bool isShowImage[CAMERA_MAX_COUNT];
	bool bIsStress;

	CSpendTime waitLocPos;
	CSpendTime checkTimecost;
	CSpendTime showImageDelay[CAMERA_MAX_COUNT];//显示错误图像延时

	QDir *dirSaveImagePath;
	//调用算法
	s_AlgCInP sAlgCInp;						//检测输入参数
	s_AlgCheckResult *pAlgCheckResult;		//返回检测结果结构体
	s_Status  sReturnStatus;				// 函数执行状态信息
public:
	int ThreadNumber;					//线程号，对应相机号
	int iCamera;
	int iImageNo;
};

#endif
