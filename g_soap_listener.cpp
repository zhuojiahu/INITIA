#include "g_soap_listener.h"

#include "glasswaredetectsystem.h"
#include "common.h"
extern GlasswareDetectSystem *pMainFrm;

g_soap_listener::g_soap_listener(QObject *parent)
	: QObject(parent)
{
	init();
}

g_soap_listener::~g_soap_listener()
{
	pMySoap->CloseSoap();
	delete pMySoap;
}

static void WINAPI GetInfoCallback(s_DetectInfo* DetectInfo)
{
	DetectInfo->sMouldID =pMainFrm->m_sSystemInfo.m_strModelName;
	DetectInfo->nCheckCount = pMainFrm->m_sRunningInfo.nGSOAP_PassCount;//�������
	DetectInfo->nKickCount = pMainFrm->m_sRunningInfo.nGSOAP_KickCount;//�߷�����
	DetectInfo->nCameraCount = pMainFrm->m_sSystemInfo.iCamCount;//�������
	DetectInfo->nErrorTypeCount = pMainFrm->m_sErrorInfo.m_iErrorTypeCount;//ȱ����������

	DetectInfo->nStationCount = 3;
	DetectInfo->sStationName[0] = QString("Body");
	DetectInfo->sStationName[1] = QString("Finish");
	DetectInfo->sStationName[2] = QString("Bottom");

	DetectInfo->nStationCheckCount[0] = pMainFrm->m_sRunningInfo.nGSOAP_PassCount;
	DetectInfo->nStationCheckCount[1] = pMainFrm->m_sRunningInfo.nGSOAP_PassCount;
	DetectInfo->nStationCheckCount[2] = pMainFrm->m_sRunningInfo.nGSOAP_PassCount;
	DetectInfo->nStationKickCount[0] = 0;
	DetectInfo->nStationKickCount[1] = 0;
	DetectInfo->nStationKickCount[2] = 0;

	for (int i=0; i<DetectInfo->nCameraCount ;i++)
	{
		DetectInfo->nCameraCheckCount[i] = pMainFrm->m_sRunningInfo.nGSOAP_CheckCount;
		DetectInfo->nCameraKickCount[i] = pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[i];

		if(0 == pMainFrm->m_sCarvedCamInfo[i].m_iImageType)
		{
			DetectInfo->nStationKickCount[0] += pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[i]; 
		}
		else if(1 == pMainFrm->m_sCarvedCamInfo[i].m_iImageType)
		{
			DetectInfo->nStationKickCount[1]+= pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[i]; 
		}		
		else if(2 == pMainFrm->m_sCarvedCamInfo[i].m_iImageType)
		{
			DetectInfo->nStationKickCount[2]  += pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[i]; 
		}			
	}
	for (int i = 0; i<DetectInfo->nErrorTypeCount ; i++)
	{
		// 		QByteArray qbaErrorTypeName =  pMainFrm->m_sErrorInfo.m_vstrErrorType.at(i).toLocal8Bit();//ģ�ߺ�
		// 		DetectInfo->sErrorTypeName[i] = qbaErrorTypeName.data();//ÿ��ȱ����������
		DetectInfo->sErrorTypeName[i] = pMainFrm->m_sRunningInfo.sGSoap_ErrorTypeName[i+1];//ÿ��ȱ����������
		//		DetectInfo->nErrorTypeClass[i] = ;//ÿ��ȱ�����ͷ��ࣨƿ�ڡ�ƿ�ס�ƿ��
		DetectInfo->nErrorTypeKickCount[i] = pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[i+1];//ÿ��ȱ�������߷ϸ���
	}

	//������ϼ�������
	pMainFrm->m_sRunningInfo.nGSOAP_PassCount = 0;
	pMainFrm->m_sRunningInfo.nGSOAP_KickCount = 0;
	pMainFrm->m_sRunningInfo.nGSOAP_CheckCount = 0;
	for(int i=0; i<DetectInfo->nCameraCount ;i++)
	{
		pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[i] = 0;
	}
	for (int i = 0; i<DetectInfo->nErrorTypeCount ; i++)
	{
		pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[i+1] = 0;
	}
}
void g_soap_listener::init()
{
	pMySoap = new MySoap(pMainFrm->m_sSystemInfo.iWebServerPort,GetInfoCallback);
	pMySoap->OpenSoap();
}