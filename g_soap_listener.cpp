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
	DetectInfo->nCheckCount = pMainFrm->m_sRunningInfo.nGSOAP_PassCount;//检测总数
	DetectInfo->nKickCount = pMainFrm->m_sRunningInfo.nGSOAP_KickCount;//踢废总数
	DetectInfo->nCameraCount = pMainFrm->m_sSystemInfo.iCamCount;//相机总数
	DetectInfo->nErrorTypeCount = pMainFrm->m_sErrorInfo.m_iErrorTypeCount;//缺陷类型总数

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
		// 		QByteArray qbaErrorTypeName =  pMainFrm->m_sErrorInfo.m_vstrErrorType.at(i).toLocal8Bit();//模具号
		// 		DetectInfo->sErrorTypeName[i] = qbaErrorTypeName.data();//每个缺陷类型名称
		DetectInfo->sErrorTypeName[i] = pMainFrm->m_sRunningInfo.sGSoap_ErrorTypeName[i+1];//每个缺陷类型名称
		//		DetectInfo->nErrorTypeClass[i] = ;//每个缺陷类型分类（瓶口、瓶底、瓶身）
		DetectInfo->nErrorTypeKickCount[i] = pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[i+1];//每个缺陷类型踢废个数
	}

	//发送完毕计数清零
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