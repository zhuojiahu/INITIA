////////////////////////////////
//SOAPͨѶ�� Server�� V1.0
//ʵ�ֹ��ܣ���ʼ��SOAP����������̵߳ȴ���ȡ��������
//���յ���ȡ��������󣬵��ûص��������Ӽ������ȡ������ݽ��з���
//��д����ͬ    ���ڣ�2017��1��
////////////////////////////////
#pragma once
#include <QThread>

#ifndef WINAPI
#define WINAPI __stdcall
#endif

typedef struct STRUCT_DETECT_INFO 
{
    int nMouldNumber;//ģ�����
	QString sMouldID;//ģ�ߺ�
	int nCheckCount;//�������
	int nKickCount;//�߷�����

	int nCameraCount;//�������
	int nCameraCheckCount[50];//ÿ������������
	int nCameraKickCount[50];//ÿ������߷�����

	int nErrorTypeCount;//ȱ����������
	QString sErrorTypeName[50];//ÿ��ȱ����������
	int nErrorTypeClass[50];//ÿ��ȱ�����ͷ��ࣨƿ�ڡ�ƿ�ס�ƿ��
	int nErrorTypeKickCount[50];//ÿ��ȱ�������߷ϸ���

	int nStationCount;//��λ����
	QString sStationName[50];//��λ����
	int nStationCheckCount[50];//ÿ����λ������
	int nStationKickCount[50];//ÿ����λ�߷ϸ���

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

//�ص�����ָ������
typedef void (WINAPI *SOAPCALLBACK)(s_DetectInfo* DetectInfo);

class MySoap
{
public:
	MySoap(void);
	MySoap(int nPort,SOAPCALLBACK pCallBackFunc);
	~MySoap(void);

	bool InitSoapServer();//��ʼ��

	bool OpenSoap();//��������
	bool CloseSoap();//�رռ���
	bool IsOpen();//��ȡ״̬

	bool m_bIsOpen; //�Ƿ���
	SOAPCALLBACK m_pCallBackFunc;				//�ص�����ָ��

	int m_nSoapPort;//SOAP�˿ں�

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
	}m_SoapServerThread;//SOAP�������߳� ������򿪶˿ڼ�����������Ϣ
};

