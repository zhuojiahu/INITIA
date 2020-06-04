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
	//��ȡ���ģ���������
	void GetModelDotData(CGrabElement *pElement);
	//ͳ������ʱ������
	void CountRuningData(int cameraNumber);
	//����һ����Ĳɼ��ٶ�
	void CountGrabRate(int cameraNumber);
	//����һ���������
	void CountCheckNumber();
	//�ж��Ƿ���Ϊָ��ȱ�ݱ���
	void ifSendAret(int);
private:

	int iStressCamera;
	bool m_bStopThread;					//�������

	bool bCheckResult[CAMERA_MAX_COUNT];
	int iErrorType;
	int iMaxErrorType;
	int iMaxErrorArea;

	bool isShowImage[CAMERA_MAX_COUNT];
	bool bIsStress;

	CSpendTime waitLocPos;
	CSpendTime checkTimecost;
	CSpendTime showImageDelay[CAMERA_MAX_COUNT];//��ʾ����ͼ����ʱ

	QDir *dirSaveImagePath;
	//�����㷨
	s_AlgCInP sAlgCInp;						//����������
	s_AlgCheckResult *pAlgCheckResult;		//���ؼ�����ṹ��
	s_Status  sReturnStatus;				// ����ִ��״̬��Ϣ
public:
	int ThreadNumber;					//�̺߳ţ���Ӧ�����
	int iCamera;
	int iImageNo;
};

#endif
