﻿#include "glasswaredetectsystem.h"
#include <Mmsystem.h>
#pragma comment( lib,"winmm.lib" )
#include <QTextCodec>
#include <QDateTime>
#include <QPropertyAnimation>
//是否加密宏开关 JIAMI_INITIA
//#define  JIAMI_INITIA
#ifdef JIAMI_INITIA
#ifdef WIN_32_1_10
#include "ProgramLicense.h"
#include "pushLicense.h"
CProgramLicense m_ProgramLicense;
#pragma comment( lib,"DHProgramLicense.lib" )
#else
#include "ProgramLicense.h"
#include "pushLicense.h"
CProgramLicense m_ProgramLicense;
#pragma comment( lib,"DHProgramLicense64.lib" )
#endif
#endif

GlasswareDetectSystem * pMainFrm;
QString appPath;  //更新路径


GlasswareDetectSystem::GlasswareDetectSystem(QWidget *parent, Qt::WFlags flags)
	: QDialog(parent)
{
	pMainFrm = this;
	iLastStatus = -1;
	m_iLastKickNo0 = 0;
	m_iLastKickNo1 = 0;
	MinRate=0;
	MaxRate=0;
	iLastAbnormalCamera = -1;
	iLastAbnormalStep = -1;
	bSaveImage = false;
	for (int i=0;i<CAMERA_MAX_COUNT;i++)
	{
		m_queue[i].listDetect.clear();
		m_iDetectStep[i] = 0;
		m_iDetectSignalNo[i] = 0;
		m_iGrabCounter[i] = 0;
		m_iImgGrabCounter[i] = 0;
		pdetthread[i] = NULL;
		blostImage[i] = false;
		m_SavePicture[i].pThat=NULL;
	}

	m_vcolorTable.clear();
	for (int i = 0; i < 256; i++)  
	{  
		m_vcolorTable.append(qRgb(i, i, i)); 
	}
	uniqueFlag = false;
	imgTime = 0;
	CherkerAry.pCheckerlist=NULL;
	surplusDays=0;
}

GlasswareDetectSystem::~GlasswareDetectSystem()
{
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		pdetthread[i]->WaitThreadStop();
	}
	ReleaseAll();
	pMainFrm->Logfile.write(tr("Release Over"),OperationLog);
}
//功能：获取文件版本号
QString GlasswareDetectSystem::getVersion(QString strFullName)
{
#ifdef WIN_32_1_10
	QString temp(tr("Version:")+"3.32.");
#else
	QString temp(tr("Version:")+"3.64.");
#endif
	return temp+"1.12";
}

void GlasswareDetectSystem::closeWidget()
{
	close();
}
void GlasswareDetectSystem::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Escape) 
	{
		return;
	}
}

void GlasswareDetectSystem::Init()
{
	m_sRunningInfo.m_iPermission = 0;
	pMainFrm->Logfile.write(tr("InitParameter!"),OperationLog);
	InitParameter();
	pMainFrm->Logfile.write(tr("ReadIniInformation!"),OperationLog);
	ReadIniInformation();
	pMainFrm->Logfile.write(tr("Initialize!"),OperationLog);
	Initialize();
	pMainFrm->Logfile.write(tr("initInterface!"),OperationLog);
	initInterface();
	pMainFrm->Logfile.write(tr("startdetect!"),OperationLog);
	StartDetectThread();
	StartCamGrab();
	//pg_soap_listener = new g_soap_listener(this);
	pMainFrm->Logfile.write(tr("Enter system!"),OperationLog);
	InitLastData();
}
//初始化
void GlasswareDetectSystem::Initialize()
{
	pMainFrm->Logfile.write(tr("LoadParameterAndCam!"),OperationLog);
	LoadParameterAndCam();
	pMainFrm->Logfile.write(tr("InitImage!"),OperationLog);
	InitImage();	//初始化图像
	pMainFrm->Logfile.write(tr("InitIOCard!"),OperationLog);
	InitIOCard();
	pMainFrm->Logfile.write(tr("InitCheckSet!"),OperationLog);
	InitCheckSet();
	initDetectThread();
}
//采集回调函数
void WINAPI GlobalGrabOverCallback (const s_GBSIGNALINFO* SigInfo)
{
	if (SigInfo && SigInfo->Context)
	{
		GlasswareDetectSystem* pGlasswareDetectSystem = (GlasswareDetectSystem*) SigInfo->Context;
		pGlasswareDetectSystem->GrabCallBack(SigInfo);
	}
}
//采集回调函数
void GlasswareDetectSystem::GrabCallBack(const s_GBSIGNALINFO *SigInfo)
{
	int iRealCameraSN = SigInfo->nGrabberSN;//读取相机序号
	if (iRealCameraSN==-1 || !m_sRunningInfo.m_bCheck)
	{
		return;
	}
	m_iGrabCounter[iRealCameraSN]++;
	if(SigInfo->nErrorCode != GBOK)
	{
		pMainFrm->Logfile.write(QString("Camera:%1 have Residual frame").arg(iRealCameraSN+1),CheckLog);
		return;
	}

	CMachineSignal m_cMachioneSignalCurrent;//获得机器信号
	if (!m_sSystemInfo.m_bIsIOCardOK)//生成机器号
	{
		m_cMachioneSignalCurrent.m_iImageCount = m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast+1;
 		if (m_cMachioneSignalCurrent.m_iImageCount >= 256)
 		{
 			m_cMachioneSignalCurrent.m_iImageCount = 0;
 		}
	}
	else if (m_sSystemInfo.m_bIsIOCardOK && m_sRealCamInfo[iRealCameraSN].m_bGrabIsTrigger)// 读机器信号
	{
		//m_mutexGlasswareNumber.lock();
		m_cMachioneSignalCurrent.m_iImageCount = ReadImageSignal(iRealCameraSN);
		//m_mutexGlasswareNumber.unlock();
	}

	//误触发判断，测试相机时不能判断误触发
	if (!m_sSystemInfo.m_bIsTest && m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast > -1
		&& m_cMachioneSignalCurrent.m_iImageCount == m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast)
	{
		pMainFrm->Logfile.write(QString("camera %1 False triggering,ImageNumber=%2!").
			arg(iRealCameraSN+1).arg(m_cMachioneSignalCurrent.m_iImageCount),CheckLog);
		return;
	}
	/*if (m_sSystemInfo.m_bIsIOCardOK)
	{
		pMainFrm->m_sRunningInfo.m_mutexRunningInfo.lock();
		int nIOCard1Checked = pMainFrm->m_vIOCard[0]->ReadCounter(0);
		if (nIOCard1Checked > pMainFrm->m_sRunningInfo.m_iLastIOCard1IN0)
		{
			pMainFrm->m_sRunningInfo.m_checkedNum += nIOCard1Checked - pMainFrm->m_sRunningInfo.m_iLastIOCard1IN0;
		}
		pMainFrm->m_sRunningInfo.m_iLastIOCard1IN0 = nIOCard1Checked;
		pMainFrm->m_sRunningInfo.m_mutexRunningInfo.unlock();
	}*/
	if(!m_sSystemInfo.m_bIsTest && iRealCameraSN == 0)
	{
		pMainFrm->m_sRunningInfo.m_checkedNum +=1;
	}
	//图像号读取异常时，打印调试信息
	if(m_cMachioneSignalCurrent.m_iImageCount-m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast != 1&&
		m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast != 255)
	{
		pMainFrm->Logfile.write(QString("first read ImageNumber =%1,lastImageNumber=%2,icamera=%3!").
			arg(m_cMachioneSignalCurrent.m_iImageCount).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast).arg(iRealCameraSN+1),CheckLog);

	}
	m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast = m_cMachioneSignalCurrent.m_iImageCount;

	//******************采集:得到图像缓冲区地址****************************//
	uchar* pImageBuffer = NULL;
	uchar* pImageRo = NULL;
	int nAddr = 0;
	int nWidth, nHeight, nBitCount;
	m_mutexmGrab.lock();
	m_sRealCamInfo[iRealCameraSN].m_pGrabber->GetParamInt(GBImageBufferAddr, nAddr);
	pImageBuffer = (uchar*)nAddr;
	m_sRealCamInfo[iRealCameraSN].m_pGrabber->GetParamInt(GBImageWidth, nWidth);
	m_sRealCamInfo[iRealCameraSN].m_pGrabber->GetParamInt(GBImageHeight, nHeight);
	m_sRealCamInfo[iRealCameraSN].m_pGrabber->GetParamInt(GBImagePixelSize, nBitCount);

	m_mutexmGrab.unlock();
	if (90 == pMainFrm->m_sRealCamInfo[iRealCameraSN].m_iImageRoAngle || 180 == pMainFrm->m_sRealCamInfo[iRealCameraSN].m_iImageRoAngle|| 270 == pMainFrm->m_sRealCamInfo[iRealCameraSN].m_iImageRoAngle)
	{
		RoAngle(pImageBuffer,m_sRealCamInfo[iRealCameraSN].m_pRealImage->bits(),\
		nWidth,	nHeight,pMainFrm->m_sRealCamInfo[iRealCameraSN].m_iImageRoAngle);
	} 
	else
	{
		long lImageSize = nWidth * nHeight;
		memcpy(m_sRealCamInfo[iRealCameraSN].m_pRealImage->bits(),pImageBuffer,lImageSize);
	}
	//m_sRealCamInfo[iRealCameraSN].m_pRealImage->save("./abc.dmp");

	PutImagetoDetectList(iRealCameraSN,m_cMachioneSignalCurrent.m_iImageCount);
}

void GlasswareDetectSystem::PutImagetoDetectList(int iRealCameraSN,int m_iImageCount)
{
	try
	{
		CGrabElement *pGrabElement = NULL;
		for(int i = 0; i < m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageCount; i++)
		{
			int iCameraSN = m_sRealCamInfo[iRealCameraSN].m_sCorves.i_GrabSN[i];
			if(m_queue[iCameraSN].listGrab.count()<=0)
			{
				pMainFrm->Logfile.write(QString("There is no queue(%1) available!").arg(iCameraSN),CheckLog);
				continue;
			}
			m_queue[iCameraSN].mGrabLocker.lock();
			pGrabElement = (CGrabElement *) m_queue[iCameraSN].listGrab.first();
			m_queue[iCameraSN].listGrab.removeFirst();
			m_queue[iCameraSN].mGrabLocker.unlock();
			if (pGrabElement != NULL)
			{
				long lImageSize = m_sCarvedCamInfo[iCameraSN].m_iImageWidth * m_sCarvedCamInfo[iCameraSN].m_iImageHeight;
				if (lImageSize != pGrabElement->myImage->byteCount())
				{
					pMainFrm->Logfile.write(tr("ImageSize unsuitable, Thread:Grab, camera:%1.lImageSize = %2,myImage byteCount = %3").arg(iCameraSN).arg(lImageSize).arg(pGrabElement->myImage->byteCount()),AbnormityLog);
					delete []pGrabElement->sImgLocInfo.sXldPoint.nColsAry;
					delete []pGrabElement->sImgLocInfo.sXldPoint.nRowsAry;
					delete pGrabElement->myImage;
					return;
				}
				m_mutexmCarve.lock();
				//剪切图像
				CarveImage(m_sRealCamInfo[iRealCameraSN].m_pRealImage->bits(),m_sCarvedCamInfo[iCameraSN].m_pGrabTemp,\
					m_sRealCamInfo[iRealCameraSN].m_iImageWidth,m_sRealCamInfo[iRealCameraSN].m_iImageHeight, m_sCarvedCamInfo[iCameraSN].i_ImageX,m_sCarvedCamInfo[iCameraSN].i_ImageY,\
					m_sCarvedCamInfo[iCameraSN].m_iImageWidth,m_sCarvedCamInfo[iCameraSN].m_iImageHeight);			

			//m_iImgGrabCounter[iCameraSN]++;

				memcpy(pGrabElement->myImage->bits(), m_sCarvedCamInfo[iCameraSN].m_pGrabTemp, \
					m_sCarvedCamInfo[iCameraSN].m_iImageWidth*m_sCarvedCamInfo[iCameraSN].m_iImageHeight);
				m_mutexmCarve.unlock();

				//将图像数据填人到元素队列中
				m_sRealCamInfo[iRealCameraSN].m_iGrabImageCount++;
				pGrabElement->bHaveImage=TRUE;
				pGrabElement->nCheckRet = 0;
				pGrabElement->nSignalNo = m_iImageCount;
				pGrabElement->nCamSN = iCameraSN;
				pGrabElement->nCheckRet = FALSE;
				pGrabElement->cErrorParaList.clear();

 				if (pMainFrm->m_queue[iCameraSN].InitID == pGrabElement->initID)
				{
					if (0 == m_sCarvedCamInfo[iCameraSN].m_iStress)
					{
						int nNormalCameraNo = iCameraSN;
						int nStressCameraNo = m_sCarvedCamInfo[iCameraSN].m_iToStressCamera;
						mutexDetectElement[nNormalCameraNo].lock();
						if (!m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && !m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
 							m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElement);
						}
						else if (m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && !m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
 							//pMainFrm->Logfile.write(tr("add black stress image because get one more normal image , CamNo:%1,imageNo:%2").arg(nNormalCameraNo).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast),AbnormityLog);
 							m_queue[nStressCameraNo].mGrabLocker.lock();
 							CGrabElement *pGrabElementStrss = (CGrabElement *) m_queue[nStressCameraNo].listGrab.takeFirst();
 							m_queue[nStressCameraNo].mGrabLocker.unlock();
 							memset(pGrabElementStrss->myImage->bits(), 0, \
 								m_sCarvedCamInfo[nStressCameraNo].m_iImageWidth*m_sCarvedCamInfo[nStressCameraNo].m_iImageHeight);
							pGrabElementStrss->bHaveImage=TRUE;
							pGrabElementStrss->nCheckRet = 0;
							pGrabElementStrss->nSignalNo = m_detectElement[nNormalCameraNo].SignalNoNormal();//m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast;
							pGrabElementStrss->nCamSN = nStressCameraNo;
							pGrabElementStrss->nCheckRet = FALSE;
							pGrabElementStrss->cErrorParaList.clear();

							m_detectElement[nNormalCameraNo].AddStressImage(pGrabElementStrss);
							CDetectElement DetectElement;
							m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
							DetectElement.setType(0);
							m_queue[nNormalCameraNo].mDetectLocker.lock();
							m_queue[nNormalCameraNo].listDetect.append(DetectElement);
							m_queue[nNormalCameraNo].mDetectLocker.unlock();
							m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElement);
						}
						else if (!m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
							if (pGrabElement->nSignalNo < m_detectElement[nNormalCameraNo].SignalNoStress())
							{
								m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElement);

								//pMainFrm->Logfile.write(tr("add black stress image because normal imageNo. is smaller than Stress , CamNo:%1,imageNo:%2").arg(nNormalCameraNo).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast),AbnormityLog);
								m_queue[nStressCameraNo].mGrabLocker.lock();
								CGrabElement *pGrabElementStrss = (CGrabElement *) m_queue[nStressCameraNo].listGrab.takeFirst();
								m_queue[nStressCameraNo].mGrabLocker.unlock();
								memset(pGrabElementStrss->myImage->bits(), 0, \
									m_sCarvedCamInfo[nStressCameraNo].m_iImageWidth*m_sCarvedCamInfo[nStressCameraNo].m_iImageHeight);
								pGrabElementStrss->bHaveImage=TRUE;
								pGrabElementStrss->nCheckRet = 0;
								pGrabElementStrss->nSignalNo = pGrabElement->nSignalNo;
								pGrabElementStrss->nCamSN = nStressCameraNo;
								pGrabElementStrss->nCheckRet = FALSE;
								pGrabElementStrss->cErrorParaList.clear();

								CGrabElement *pGrabElementTempStress = m_detectElement[nNormalCameraNo].ImageStress;

								m_detectElement[nNormalCameraNo].AddStressImage(pGrabElementStrss);
								CDetectElement DetectElement;
								m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
								DetectElement.setType(0);
								m_queue[nNormalCameraNo].mDetectLocker.lock();
								m_queue[nNormalCameraNo].listDetect.append(DetectElement);
								m_queue[nNormalCameraNo].mDetectLocker.unlock();

								m_detectElement[nNormalCameraNo].AddStressImage(pGrabElementTempStress);
							}
							else if (pGrabElement->nSignalNo > m_detectElement[nNormalCameraNo].SignalNoStress())
							{
								//pMainFrm->Logfile.write(tr("add black normal image because normal imageNo. is bigger than Stress , CamNo:%1,imageNo:%2").arg(nNormalCameraNo).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast),AbnormityLog);
								m_queue[nNormalCameraNo].mGrabLocker.lock();
								CGrabElement *pGrabElementNormal = (CGrabElement *) m_queue[nNormalCameraNo].listGrab.takeFirst();
								m_queue[nNormalCameraNo].mGrabLocker.unlock();
								memset(pGrabElementNormal->myImage->bits(), 0, \
									m_sCarvedCamInfo[nNormalCameraNo].m_iImageWidth*m_sCarvedCamInfo[nNormalCameraNo].m_iImageHeight);
								pGrabElementNormal->bHaveImage=TRUE;
								pGrabElementNormal->nCheckRet = 0;
								pGrabElementNormal->nSignalNo = m_detectElement[nNormalCameraNo].SignalNoStress();
								pGrabElementNormal->nCamSN = nNormalCameraNo;
								pGrabElementNormal->nCheckRet = FALSE;
								pGrabElementNormal->cErrorParaList.clear();

								m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElementNormal);
								CDetectElement DetectElement;
								m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
								DetectElement.setType(0);
								m_queue[nNormalCameraNo].mDetectLocker.lock();
								m_queue[nNormalCameraNo].listDetect.append(DetectElement);
								m_queue[nNormalCameraNo].mDetectLocker.unlock();

								m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElement);
							} 
							else
							{
								m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElement);
							}
						}

						if (m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
							CDetectElement DetectElement;
							m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
							DetectElement.setType(0);
							m_queue[nNormalCameraNo].mDetectLocker.lock();
							m_queue[nNormalCameraNo].listDetect.append(DetectElement);
							m_queue[nNormalCameraNo].mDetectLocker.unlock();
						}
						mutexDetectElement[nNormalCameraNo].unlock();
					}
					else if (2 == m_sCarvedCamInfo[iCameraSN].m_iStress )
					{
						int nNormalCameraNo = m_sCarvedCamInfo[iCameraSN].m_iToNormalCamera;
						int nStressCameraNo = iCameraSN;
						mutexDetectElement[nNormalCameraNo].lock();
						if (!m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && !m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
							m_detectElement[nNormalCameraNo].AddStressImage(pGrabElement);
						}
						else if (!m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
							//pMainFrm->Logfile.write(tr("add normal stress image because get one more stress image , CamNo:%1,imageNo:%2").arg(nStressCameraNo).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast),AbnormityLog);
							m_queue[nNormalCameraNo].mGrabLocker.lock();
							CGrabElement *pGrabElementNormal = (CGrabElement *) m_queue[nNormalCameraNo].listGrab.takeFirst();
							m_queue[nNormalCameraNo].mGrabLocker.unlock();
							memset(pGrabElementNormal->myImage->bits(), 0, \
								m_sCarvedCamInfo[nNormalCameraNo].m_iImageWidth*m_sCarvedCamInfo[nNormalCameraNo].m_iImageHeight);
							pGrabElementNormal->bHaveImage=TRUE;
							pGrabElementNormal->nCheckRet = 0;
							pGrabElementNormal->nSignalNo = m_detectElement[nNormalCameraNo].SignalNoStress();//m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast;
							pGrabElementNormal->nCamSN = nNormalCameraNo;
							pGrabElementNormal->nCheckRet = FALSE;
							pGrabElementNormal->cErrorParaList.clear();

							m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElementNormal);
							CDetectElement DetectElement;
							m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
							DetectElement.setType(0);
							m_queue[nNormalCameraNo].mDetectLocker.lock();
							m_queue[nNormalCameraNo].listDetect.append(DetectElement);
							m_queue[nNormalCameraNo].mDetectLocker.unlock();
							m_detectElement[nNormalCameraNo].AddStressImage(pGrabElement);
						}
						else if (m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && !m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
							if (pGrabElement->nSignalNo < m_detectElement[nNormalCameraNo].SignalNoNormal())
							{
								m_detectElement[nNormalCameraNo].AddStressImage(pGrabElement);

								//pMainFrm->Logfile.write(tr("add black normal image because normal imageNo. is smaller than normal , CamNo:%1,imageNo:%2").arg(nNormalCameraNo).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast),AbnormityLog);
								m_queue[nNormalCameraNo].mGrabLocker.lock();
								CGrabElement *pGrabElementNormal = (CGrabElement *) m_queue[nNormalCameraNo].listGrab.takeFirst();
								m_queue[nNormalCameraNo].mGrabLocker.unlock();
								memset(pGrabElementNormal->myImage->bits(), 0, \
									m_sCarvedCamInfo[nNormalCameraNo].m_iImageWidth*m_sCarvedCamInfo[nNormalCameraNo].m_iImageHeight);
								pGrabElementNormal->bHaveImage=TRUE;
								pGrabElementNormal->nCheckRet = 0;
								pGrabElementNormal->nSignalNo = pGrabElement->nSignalNo;
								pGrabElementNormal->nCamSN = nNormalCameraNo;
								pGrabElementNormal->nCheckRet = FALSE;
								pGrabElementNormal->cErrorParaList.clear();

								CGrabElement *pGrabElementTempNormal = m_detectElement[nNormalCameraNo].ImageNormal;

								m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElementNormal);
								CDetectElement DetectElement;
								m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
								DetectElement.setType(0);
								m_queue[nNormalCameraNo].mDetectLocker.lock();
								m_queue[nNormalCameraNo].listDetect.append(DetectElement);
								m_queue[nNormalCameraNo].mDetectLocker.unlock();

								m_detectElement[nNormalCameraNo].AddNormalImage(pGrabElementTempNormal);
							}
							else if (pGrabElement->nSignalNo > m_detectElement[nNormalCameraNo].SignalNoNormal())
							{
								//pMainFrm->Logfile.write(tr("add black stress image because normal imageNo. is bigger than normal , CamNo:%1,imageNo:%2").arg(nNormalCameraNo).arg(m_sRealCamInfo[iRealCameraSN].m_iImageIdxLast),AbnormityLog);
								m_queue[nStressCameraNo].mGrabLocker.lock();
								CGrabElement *pGrabElementStress = (CGrabElement *) m_queue[nStressCameraNo].listGrab.takeFirst();
								m_queue[nStressCameraNo].mGrabLocker.unlock();
								memset(pGrabElementStress->myImage->bits(), 0, \
									m_sCarvedCamInfo[nStressCameraNo].m_iImageWidth*m_sCarvedCamInfo[nStressCameraNo].m_iImageHeight);
								pGrabElementStress->bHaveImage=TRUE;
								pGrabElementStress->nCheckRet = 0;
								pGrabElementStress->nSignalNo = m_detectElement[nNormalCameraNo].SignalNoNormal();
								pGrabElementStress->nCamSN = nStressCameraNo;
								pGrabElementStress->nCheckRet = FALSE;
								pGrabElementStress->cErrorParaList.clear();

								m_detectElement[nNormalCameraNo].AddStressImage(pGrabElementStress);
								CDetectElement DetectElement;
								m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
								DetectElement.setType(0);
								m_queue[nNormalCameraNo].mDetectLocker.lock();
								m_queue[nNormalCameraNo].listDetect.append(DetectElement);
								m_queue[nNormalCameraNo].mDetectLocker.unlock();

								m_detectElement[nNormalCameraNo].AddStressImage(pGrabElement);
							} 
							else
							{
								m_detectElement[nNormalCameraNo].AddStressImage(pGrabElement);
							}
						}
						if (m_detectElement[nNormalCameraNo].IsImageNormalCompelet() && m_detectElement[nNormalCameraNo].IsImageStressCompelet())
						{
							CDetectElement DetectElement;
							m_detectElement[nNormalCameraNo].copyDatato(DetectElement);
							DetectElement.setType(0);
							m_queue[nNormalCameraNo].mDetectLocker.lock();
							m_queue[nNormalCameraNo].listDetect.append(DetectElement);
							m_queue[nNormalCameraNo].mDetectLocker.unlock();
						}
						mutexDetectElement[nNormalCameraNo].unlock();
					}
					else if	(1 == m_sCarvedCamInfo[iCameraSN].m_iStress )
					{
						m_detectElement[iCameraSN].ImageNormal = pGrabElement;
						m_detectElement[iCameraSN].iCameraNormal = iCameraSN;
						m_detectElement[iCameraSN].iSignalNoNormal = pGrabElement->nSignalNo;

						m_detectElement[iCameraSN].iType = 1;
						m_queue[iCameraSN].mDetectLocker.lock();
						m_queue[iCameraSN].listDetect.append(m_detectElement[iCameraSN]);
						m_queue[iCameraSN].mDetectLocker.unlock();
					}
				}
				else
				{
					delete pGrabElement;
					//pMainFrm->Logfile.write(tr("delete GrabElement! Camera number %1").arg(iCameraSN),AbnormityLog);
				}
			}	
		}
	}
	catch (...)
	{
	}
}
//配置初始化信息
void GlasswareDetectSystem::InitParameter()
{
	// 注册s_MSGBoxInfo至元对象系统,否则s_MSGBoxInfo,s_ImgWidgetShowInfo，s_StatisticsInfo无法作为参数进行传递
	qRegisterMetaType<s_MSGBoxInfo>("s_MSGBoxInfo"); 
	qRegisterMetaType<e_SaveLogType>("e_SaveLogType");  
	qRegisterMetaType<QList<QRect>>("QList<QRect>");   
	//初始化路径
	QString path = QApplication::applicationFilePath();  
	appPath = path.left(path.findRev("/")+1);
	m_sConfigInfo.m_strAppPath = appPath;
	pMainFrm->Logfile.write(tr("Get Version"),AbnormityLog);

	//获取文件版本号
	sVersion = getVersion(path);

	//配置文件在run目录中位置
	m_sConfigInfo.m_strConfigPath = "Config/Config.ini";
	m_sConfigInfo.m_strDataPath = "Config/Data.ini";
	m_sConfigInfo.m_strErrorTypePath = "Config/ErrorType.ini";
	m_sConfigInfo.m_strPLCStatusTypePath = "Config/PLCStatusType.ini";
	m_sConfigInfo.m_sAlgFilePath = "ModelInfo";// 算法路径 [10/26/2010 GZ]	

	//配置文件绝对路径
	m_sConfigInfo.m_strConfigPath = appPath + m_sConfigInfo.m_strConfigPath;
	m_sConfigInfo.m_strDataPath = appPath + m_sConfigInfo.m_strDataPath;
	m_sConfigInfo.m_strErrorTypePath = appPath + m_sConfigInfo.m_strErrorTypePath;
	m_sConfigInfo.m_strPLCStatusTypePath = appPath + m_sConfigInfo.m_strPLCStatusTypePath;
	m_sConfigInfo.m_sAlgFilePath = appPath + m_sConfigInfo.m_sAlgFilePath;

	SaveDataPath=appPath+"./LastData.ini";
	
	//初始化相机参数
	for (int i = 0;i<CAMERA_MAX_COUNT;i++)
	{
		m_bSaveImage[i] = FALSE;
		m_bShowImage[i] = TRUE;
		m_sRealCamInfo[i].m_bGrabIsStart = FALSE;
	}
	connect(this,SIGNAL(signals_MessageBoxMainThread(s_MSGBoxInfo)),this,SLOT(slots_MessageBoxMainThread(s_MSGBoxInfo)));
}
//读取配置信息
void GlasswareDetectSystem::ReadIniInformation()
{
	QSettings iniset(m_sConfigInfo.m_strConfigPath,QSettings::IniFormat);
	iniset.setIniCodec(QTextCodec::codecForName("GBK"));
	QSettings erroriniset(m_sConfigInfo.m_strErrorTypePath,QSettings::IniFormat);
	erroriniset.setIniCodec(QTextCodec::codecForName("GBK"));
	QSettings PLCStatusiniset(m_sConfigInfo.m_strPLCStatusTypePath,QSettings::IniFormat);
	PLCStatusiniset.setIniCodec(QTextCodec::codecForName("GBK"));
	QSettings iniDataSet(m_sConfigInfo.m_strDataPath,QSettings::IniFormat);
	iniDataSet.setIniCodec(QTextCodec::codecForName("GBK"));

	QString sGSoap_ErrorTypePath = "Config/ErrorType_en.ini";//阴同添加
	QSettings GSoap_erroriniset(sGSoap_ErrorTypePath,QSettings::IniFormat);//阴同添加
	GSoap_erroriniset.setIniCodec(QTextCodec::codecForName("GBK"));//阴同添加

	QString strSession;
	strSession = QString("/ErrorType/total");
	m_sErrorInfo.m_iErrorTypeCount = erroriniset.value(strSession,0).toInt();

	for (int i=0;i<=m_sErrorInfo.m_iErrorTypeCount;i++)
	{
		if (0 == i)
		{
			m_sErrorInfo.m_bErrorType[i] = false;
			m_sErrorInfo.m_vstrErrorType.append(tr("Good"));//.toLatin1().data()));
		}
		else
		{
			m_sErrorInfo.m_bErrorType[i] = true;
			strSession = QString("/ErrorType/%1").arg(i);
			m_sErrorInfo.m_vstrErrorType.append(QString::fromLocal8Bit(erroriniset.value(strSession,"NULL").toString()));//.toLatin1().data()));
			m_sErrorInfo.m_cErrorReject.iErrorCountByType[i] = 0;
		}
		strSession = QString("/ErrorType/%1").arg(i+1);//阴同添加
		m_sRunningInfo.sGSoap_ErrorTypeName[i+1] = QString::fromLocal8Bit(GSoap_erroriniset.value(strSession,"Other").toString());//阴同添加

		strSession = QString("/TrackErrorType/IsTrack%1").arg(i);
		m_sSystemInfo.m_iIsTrackErrorType[i]= iniset.value(strSession,0).toInt();	//是否报警统计的错误类型
		strSession = QString("/TrackErrorType/MaxRate%1").arg(i);
		m_sSystemInfo.m_iTrackAlertRateMax[i]= iniset.value(strSession,0).toInt();	//是否报警统计的错误类型
		strSession = QString("/TrackErrorType/MinRate%1").arg(i);
		m_sSystemInfo.m_iTrackAlertRateMin[i]= iniset.value(strSession,0).toInt();	//是否报警统计的错误类型
	}
	m_sErrorInfo.m_vstrErrorType.append(tr("Other"));//.toLatin1().data()));

	strSession = QString("/StatusType/total");
	int  StatusTypeNumber= PLCStatusiniset.value(strSession,0).toInt();
	for (int i=0;i<StatusTypeNumber;i++)
	{
		strSession = QString("/StatusType/%1").arg(i);
		m_vstrPLCInfoType.append(QString::fromLocal8Bit(PLCStatusiniset.value(strSession,"NULL").toString()));//.toLatin1().data()));
	}
	//读取系统参数
	m_sRunningInfo.m_failureNumFromIOcard = iniDataSet.value("/system/failureNum",0).toInt();	
	m_sRunningInfo.m_checkedNum = iniDataSet.value("/system/checkedNum",0).toInt();	
	imgTime = iniset.value("/system/TrigerTime",300).toInt();
	iLastAbnormalStep = iniset.value("/system/TrickMaxNumber",10).toInt(); 

	m_sSystemInfo.nLoginHoldTime = iniset.value("/system/nLoginHoldTime",10).toInt();	//是否报警统计
	m_sSystemInfo.m_strWindowTitle = QObject::tr("Glass Bottle Detect System");//读取系统标题
	m_sSystemInfo.m_iTest = iniset.value("/system/Test",0).toInt();
	m_sSystemInfo.m_bDebugMode = iniset.value("/system/DebugMode",0).toInt();	//读取是否debug
	m_sSystemInfo.m_iSystemType = iniset.value("/system/systemType",0).toInt();	//读取系统类型
	m_sSystemInfo.m_bIsUsePLC=iniset.value("/system/isUsePLC",1).toBool();	//是否使用PLC
	m_sSystemInfo.m_bIsIOCardOK=iniset.value("/system/isUseIOCard",1).toInt();	//是否使用IO卡
	m_sSystemInfo.m_bIsStopNeedPermission=iniset.value("/system/IsStopPermission",0).toBool();	//是否使用IO卡
	m_sSystemInfo.iIOCardCount=iniset.value("/system/iIOCardCount",1).toInt();	//读取IO卡个数
	m_sSystemInfo.iRealCamCount = iniset.value("/GarbCardParameter/DeviceNum",2).toInt();	//真实相机个数
	m_sSystemInfo.m_bIsCarve = iniset.value("/system/IsCarve",0).toInt();	//获取是否切割设置
	m_sSystemInfo.m_bIsTest = iniset.value("/system/IsTest",0).toInt();//是否是测试模式
	m_sSystemInfo.iIsButtomStress = iniset.value("/system/IsButtomStree",0).toInt();//是否有瓶底应力
	m_sSystemInfo.iIsSaveCountInfoByTime = iniset.value("/system/IsSaveCountInfoByTime",0).toInt();//是否保存指定时间段内的统计信息
	m_sSystemInfo.iIsSample = iniset.value("/system/IsSample",0).toInt();//是否取样
	m_sSystemInfo.iIsCameraCount = iniset.value("/system/IsCameraCount",0).toInt();//是否统计各相机踢废率
	m_sSystemInfo.LastModelName = iniset.value("/system/LastModelName","default").toString();	//读取上次使用模板
	m_sSystemInfo.m_iIsTrackStatistics = iniset.value("/system/isTrackStatistics",0).toInt();	//是否报警统计
	m_sSystemInfo.m_iTrackNumber = iniset.value("/system/TrackNumber",10).toInt();	//报警统计个数
	m_sSystemInfo.m_NoKickIfNoFind = iniset.value("/system/NoKickIfNoFind",0).toInt();	//报警类型
	m_sSystemInfo.m_NoKickIfROIFail = iniset.value("/system/NoKickIfROIFail",0).toInt();	//报警类型	
	m_sSystemInfo.m_iTwoImagePage = iniset.value("/system/twoImagePage",1).toInt();	//是否两页显示图像
	m_sSystemInfo.m_iImageStyle = iniset.value("/system/ImageStyle",0).toInt();	//图像横向排布还是上下排布
	m_sSystemInfo.m_iImageStretch = iniset.value("/system/ImageStretch",1).toInt();	//图像横向排布还是上下排布
	m_sSystemInfo.m_iSaveNormalErrorImageByTime = iniset.value("/system/SaveNormalErrorImageByTime",0).toInt();	
	m_sSystemInfo.m_iSaveStressErrorImageByTime = iniset.value("/system/SaveStressErrorImageByTime",0).toInt();	
	m_sSystemInfo.m_iStopOnConveyorStoped = iniset.value("/system/stopCheckWhenConveyorStoped",0).toBool();	//输送带停止是否停止检测
	m_sSystemInfo.fPressScale = iniset.value("/system/fPressScale",1).toDouble();	//瓶身应力增强系数
	m_sSystemInfo.fBasePressScale = iniset.value("/system/fBasePressScale",1).toDouble();	//瓶底应力增强系数
	m_sSystemInfo.m_strModelName = m_sSystemInfo.LastModelName;
	m_sSystemInfo.bSaveRecord = iniset.value("/system/bSaveRecord",1).toBool();
	m_sSystemInfo.iSaveRecordInterval = iniset.value("/system/iSaveRecordInterval",60).toInt();
	m_sSystemInfo.bAutoSetZero = iniset.value("/system/bAutoSetZero",1).toBool();
	m_sSystemInfo.iIsSampleAndAlamConflict = iniset.value("/system/IsSampleAndAlamConflict",0).toInt();
	m_sSystemInfo.m_iIs3Sensor = iniset.value("/system/Is3Sensor",0).toInt();
	m_sSystemInfo.bCameraOffLineSurveillance = iniset.value("/system/bCameraOffLineSurveillance",0).toBool();	
	m_sSystemInfo.bCameraContinueRejectSurveillance = iniset.value("/system/bCameraContinueRejectSurveillance",0).toBool();	
	m_sSystemInfo.iCamOfflineNo = iniset.value("/system/iCamOfflineNo",10).toInt();	
	m_sSystemInfo.iCamContinueRejectNumber = iniset.value("/system/iCamContinueRejectNumber",10).toInt();	
	m_sSystemInfo.iWebServerPort = iniset.value("/system/iWebServerPort",8000).toInt();	
	m_sSystemInfo.iIfCountDetectNumberByCamera = iniset.value("/system/iIfCountDetectNumberByCamera",0).toInt();	
	m_sSystemInfo.iIOCardOffSet = iniset.value("/system/iIOCardOffSet",200).toInt();	

	for (int i=0;i<CAMERA_MAX_COUNT;i++)
	{
		strSession = QString("/NoRejectIfNoOrigin/Device_%1").arg(i+1);
		m_sSystemInfo.m_iNoRejectIfNoOrigin[i] = iniset.value(strSession,0).toInt();
		strSession = QString("/NoKickIfROIFail/Device_%1").arg(i+1);
		m_sSystemInfo.m_iNoRejectIfROIfail[i] = iniset.value(strSession,0).toInt();
		strSession = QString("/NoStaticIfNoOrigin/Device_%1").arg(i+1);
		m_sSystemInfo.m_iNoStaticIfNoOrigin[i] = iniset.value(strSession,0).toInt();
	}
	int iShift[3];
	iShift[0] = iniset.value("/system/shift1",000000).toInt();
	iShift[1] = iniset.value("/system/shift2",80000).toInt();
	iShift[2] = iniset.value("/system/shift3",160000).toInt();
	for (int i = 0; i<2; i++)
	{
		if (iShift[i] > iShift[i+1] )
		{
			int temp =iShift[i];
			iShift[i] = iShift[i+1];
			iShift[i+1] = temp;
		}
	}
	m_sSystemInfo.shift1.setHMS(iShift[0]/10000,(iShift[0]%10000)/100,iShift[0]%100);
	m_sSystemInfo.shift2.setHMS(iShift[1]/10000,(iShift[1]%10000)/100,iShift[1]%100);
	m_sSystemInfo.shift3.setHMS(iShift[2]/10000,(iShift[2]%10000)/100,iShift[2]%100);

	//设置剪切参数路径
	m_sConfigInfo.m_strGrabInfoPath = m_sConfigInfo.m_strAppPath + "ModelInfo/" + m_sSystemInfo.m_strModelName + "/GrabInfo.ini";

	//切割后相机个数
	if(m_sSystemInfo.m_bIsCarve)
	{		
		m_sSystemInfo.iCamCount = iniset.value("/system/CarveDeviceCount",1).toInt();
	}
	else
	{
		m_sSystemInfo.iCamCount = m_sSystemInfo.iRealCamCount;
	}

	m_CameraBuffer = iniset.value("/GarbCardParameter/CameraBuffer",8).toInt(); 
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		//strSession = QString("/YUV422TORGB/Device_%1").arg(i+1);
		//m_iGrabYUVtoRGB[i] = iniset.value(strSession,0).toInt();
		struGrabCardPara[i].iGrabberTypeSN = 1;
		strSession = QString("/GarbCardParameter/Device%1ID").arg(i+1);
		struGrabCardPara[i].nGrabberSN = iniset.value(strSession,-1).toInt();
		strSession = QString("/GarbCardParameter/Device%1Name").arg(i+1);
		strcpy_s(struGrabCardPara[i].strDeviceName,iniset.value(strSession,"").toString().toLocal8Bit().data());
		strSession = QString("/GarbCardParameter/Device%1Mark").arg(i+1);
		strcpy_s(struGrabCardPara[i].strDeviceMark,iniset.value(strSession,"").toString().toLocal8Bit().data());
		QString strGrabInitFile;//存储相机初始化位置
		strSession = QString("/GarbCardParameter/Device%1InitFile").arg(i+1);
		strGrabInitFile = iniset.value(strSession,"").toString();

		strSession = QString("/GarbCardParameter/Device%1Station").arg(i+1);
		m_sRealCamInfo[i].m_iGrabPosition = iniset.value(strSession,0).toInt();
		strSession = QString("/RoAngle/Device_%1").arg(i+1);
		m_sRealCamInfo[i].m_iImageRoAngle = iniset.value(strSession,0).toInt();
		strSession = QString("/ImageType/Device_%1").arg(i+1);
		m_sRealCamInfo[i].m_iImageType = iniset.value(strSession,0).toInt();
		strSession = QString("/IOCardSN/Device_%1").arg(i+1);
		m_sRealCamInfo[i].m_iIOCardSN = iniset.value(strSession,0).toInt();
		//采集卡文件路径与config所在文件夹相同
		strGrabInitFile = m_sConfigInfo.m_strConfigPath.left(m_sConfigInfo.m_strConfigPath.findRev("/")+1) + strGrabInitFile;
		memcpy(struGrabCardPara[i].strGrabberFile,strGrabInitFile.toLocal8Bit().data(),GBMaxTextLen);
		if(m_sRealCamInfo[i].m_iGrabPosition == 9)
		{
			//显示瓶底应力工位
			uniqueFlag = true;
		}
	}

	QSettings iniCameraSet(m_sConfigInfo.m_strGrabInfoPath,QSettings::IniFormat);
	QString strShuter,strTrigger;
	for(int i = 0; i < m_sSystemInfo.iRealCamCount; i++)
	{
		strShuter = QString("/Shuter/Grab_%1").arg(i);
		strTrigger = QString("/Trigger/Grab_%1").arg(i);
		m_sRealCamInfo[i].m_iShuter=iniCameraSet.value(strShuter,20).toInt();
		m_sRealCamInfo[i].m_iTrigger=iniCameraSet.value(strTrigger,1).toInt();//默认外触发
	}
	//InitLastData();
}
//读取切割信息
void GlasswareDetectSystem::ReadCorveConfig()
{
	for(int i=0; i<m_sSystemInfo.iRealCamCount; i++)
	{
		m_sRealCamInfo[i].m_sCorves.i_ImageCount = 0;
	}

	QSettings iniCarveSet(m_sConfigInfo.m_strGrabInfoPath,QSettings::IniFormat);
	QString strSession;
	int iRealCameraSN;
	if(m_sSystemInfo.m_bIsCarve)//如果剪切图像
	{
		//为对应应力相机赋初值-1；防止切换相机对应正常相机时多个正常相机对应一个应力相机
		for(int i=0; i<m_sSystemInfo.iCamCount; i++)
		{
			m_sCarvedCamInfo[m_sCarvedCamInfo[i].m_iToNormalCamera].m_iToStressCamera = -1;
		}

		for(int i=0; i<m_sSystemInfo.iCamCount; i++)
		{
			strSession = QString("/convert/Grab_%1").arg(i);
			if (2 == m_sSystemInfo.m_iSystemType )
			{
				if (0 == m_sSystemInfo.iIsButtomStress)
				{
					if (i < 20)
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount).toInt();
					}
					else
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount+2).toInt();
					}
				}
				else
				{
					if (i < 21)
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount).toInt();
					}
					else
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount+3).toInt();
					}
				}
			}
			else if (3 == m_sSystemInfo.m_iSystemType)
			{	
				if (0 == m_sSystemInfo.iIsButtomStress)
				{
					if (i < 14)
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount).toInt();
					}
					else
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount+2).toInt();
					}
				}
				else
				{
					if (i < 15)
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount).toInt();
					}
					else
					{
						iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount+3).toInt();
					}
				}
			}
			else if (6 == m_sSystemInfo.m_iSystemType )
			{
				iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount).toInt();
			}
			else
			{
				iRealCameraSN = iniCarveSet.value(strSession,i%m_sSystemInfo.iRealCamCount).toInt();
			}
			if (iRealCameraSN<0 || iRealCameraSN >= m_sSystemInfo.iRealCamCount)//配置文件写错不至于崩溃
			{
				iRealCameraSN = 0;
				pMainFrm->Logfile.write(tr("Error realCameraNo"),AbnormityLog);
			}
			m_sCarvedCamInfo[i].m_iToRealCamera = iRealCameraSN;

			int index = m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageCount;
			m_sRealCamInfo[iRealCameraSN].m_sCorves.i_GrabSN[index] = i;
			strSession = QString("/pointx/Grab_%1").arg(i);
			m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageX[index] = iniCarveSet.value(strSession,0).toInt();
			strSession = QString("/pointy/Grab_%1").arg(i);
			m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageY[index] = iniCarveSet.value(strSession,0).toInt();
			strSession = QString("/width/Grab_%1").arg(i);
			m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageWidth[index] = iniCarveSet.value(strSession,m_sRealCamInfo[iRealCameraSN].m_iImageWidth).toInt();
			strSession = QString("/height/Grab_%1").arg(i);
			m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageHeight[index] = iniCarveSet.value(strSession,m_sRealCamInfo[iRealCameraSN].m_iImageHeight).toInt();
			m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageCount++;

			//加载剪切后参数
			strSession = QString("/angle/Grab_%1").arg(i);
			m_sCarvedCamInfo[i].m_iImageAngle = iniCarveSet.value(strSession,0).toInt();
			strSession = QString("/Stress/Device_%1").arg(i+1);
			if (i<m_sSystemInfo.iRealCamCount)
			{
				m_sCarvedCamInfo[i].m_iStress = iniCarveSet.value(strSession,0).toInt();
			}
			else
			{
				m_sCarvedCamInfo[i].m_iStress = iniCarveSet.value(strSession,2).toInt();
			}
			if ((2 == m_sSystemInfo.m_iSystemType || 6 == m_sSystemInfo.m_iSystemType) && 1 == m_sSystemInfo.iIsButtomStress && 8 == i )
			{
				m_sCarvedCamInfo[8].m_iStress = iniCarveSet.value(strSession,2).toInt();
			}
			if (3 == m_sSystemInfo.m_iSystemType && 1 == m_sSystemInfo.iIsButtomStress && 6 == i  )
			{
				m_sCarvedCamInfo[6].m_iStress = iniCarveSet.value(strSession,2).toInt();
			}
			//配置对应正常相机
			strSession = QString("/tonormal/Grab_%1").arg(i);
			if (2 == m_sSystemInfo.m_iSystemType )
			{
				if (0 == m_sSystemInfo.iIsButtomStress)
				{
					if (i<m_sSystemInfo.iRealCamCount)
					{
						m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
					}
					else
					{
						if (i < 20)
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount).toInt();
						}
						else
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount+2).toInt();
						}
					}
				}
				else
				{
					if (i<m_sSystemInfo.iRealCamCount)
					{
						m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
						if (8 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,7).toInt();
						}
					}
					else
					{
						if (i < 21)
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount).toInt();
						}
						else
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount+3).toInt();
						}
					}
				}
			}
			else if (3 == m_sSystemInfo.m_iSystemType)
			{	
				if (0 == m_sSystemInfo.iIsButtomStress)
				{
					if (i<m_sSystemInfo.iRealCamCount)
					{
						m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
					}
					else
					{
						if (i < 14)
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount).toInt();
						}
						else
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount+2).toInt();
						}
					}
				}
				else
				{
					if (i<m_sSystemInfo.iRealCamCount)
					{
						m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
						if (6 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,7).toInt();
						}
					}
					else
					{
						if (i < 15)
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount).toInt();
						}
						else
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount+3).toInt();
						}
					}
				}
				
			}
			else if (6 == m_sSystemInfo.m_iSystemType)
			{
 				if (0 == m_sSystemInfo.iIsButtomStress)
				{

					m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
				}
				else
				{
					if (i<m_sSystemInfo.iRealCamCount)
					{
						m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
						if (8 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,7).toInt();
						}
						if (15 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,0).toInt();
						}		
						if (16 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,2).toInt();
						}
						if (17 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,4).toInt();
						}	
						if (18 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,9).toInt();
						}		
						if (19 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,11).toInt();
						}		
						if (20 == i )
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,13).toInt();
						}

					}
					else
					{
						if (i < 21)
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount).toInt();
						}
						else
						{
							m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount+3).toInt();
						}
					}
				}
			}
			else
			{
				if (i<m_sSystemInfo.iRealCamCount)
				{
					m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i).toInt();
				}
				else
				{
					m_sCarvedCamInfo[i].m_iToNormalCamera = iniCarveSet.value(strSession,i-m_sSystemInfo.iRealCamCount).toInt();
				}
			}

			//检测剪切参数是否合适
			if (m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageX[index]<0)
			{
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageX[index] = 0;
				pMainFrm->Logfile.write(tr("x is smaller than 0----Camera%1").arg(i+1),AbnormityLog);

			}
			if (m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageY[index]<0)
			{
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageY[index] = 0;
				pMainFrm->Logfile.write(tr("y is smaller than 0----Camera%1").arg(i+1),AbnormityLog);

			}
			if (m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageWidth[index]<4)
			{
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageWidth[index] =  m_sRealCamInfo[iRealCameraSN].m_iImageWidth;;
				pMainFrm->Logfile.write(tr("Width pix is smaller than 4----Camera%1").arg(i+1),AbnormityLog);

			}
			if (m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageHeight[index]<4)
			{
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageHeight[index] = m_sRealCamInfo[iRealCameraSN].m_iImageHeight;
				pMainFrm->Logfile.write(tr("Height pix is smaller than 4----Camera%1").arg(i+1),AbnormityLog);

			}
			if ((m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageX[index] + m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageWidth[index]) > m_sRealCamInfo[iRealCameraSN].m_iImageWidth)
			{
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageX[index] = 0;
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageWidth[index] = m_sRealCamInfo[iRealCameraSN].m_iImageWidth;
				pMainFrm->Logfile.write(tr("Width pix is too big----Camera%1").arg(i+1),AbnormityLog);
			}
			if ((m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageY[index] + m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageHeight[index]) > m_sRealCamInfo[iRealCameraSN].m_iImageHeight)
			{
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageY[index] = 0;
				m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageHeight[index] = m_sRealCamInfo[iRealCameraSN].m_iImageHeight;
				pMainFrm->Logfile.write(tr("Height pix is too big----Camera%1").arg(i+1),AbnormityLog);
			}
			m_sCarvedCamInfo[i].i_ImageX = m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageX[index];
			m_sCarvedCamInfo[i].i_ImageY = m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageY[index];
			m_sCarvedCamInfo[i].m_iImageWidth = m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageWidth[index];
			m_sCarvedCamInfo[i].m_iImageHeight = m_sRealCamInfo[iRealCameraSN].m_sCorves.i_ImageHeight[index];
		}
		//填入正常相机对应的应力相机
		for(int i=0; i<m_sSystemInfo.iCamCount; i++)
		{
			if (m_sCarvedCamInfo[i].m_iStress == 2)
			{
				if ((m_sCarvedCamInfo[i].m_iToNormalCamera<0))
				{
					pMainFrm->Logfile.write(tr("Normal Camera Set Error!___Camera%1").arg(i),AbnormityLog);
					m_sCarvedCamInfo[i].m_iToNormalCamera = 0;
				}
				else
				{
					m_sCarvedCamInfo[m_sCarvedCamInfo[i].m_iToNormalCamera].m_iToStressCamera = i;
				}
			}
		}
		for(int i=0; i<m_sSystemInfo.iCamCount; i++)
		{
			if (m_sCarvedCamInfo[i].m_iStress != 2)
			{
				if (-1 == m_sCarvedCamInfo[i].m_iToStressCamera)
				{
					m_sCarvedCamInfo[i].m_iStress = 1;
				}
			}
		}
	}
	else//不剪切
	{
		for(int i=0; i<m_sSystemInfo.iRealCamCount; i++)
		{
			m_sCarvedCamInfo[i].m_iToRealCamera = i;
			int index = m_sRealCamInfo[i].m_sCorves.i_ImageCount;
			m_sRealCamInfo[i].m_sCorves.i_GrabSN[index] = i;
			m_sRealCamInfo[i].m_sCorves.i_ImageX[index] = 0;
			m_sRealCamInfo[i].m_sCorves.i_ImageY[index] = 0;
			m_sRealCamInfo[i].m_sCorves.i_ImageWidth[index] = m_sCarvedCamInfo[i].m_iResImageWidth;
			m_sRealCamInfo[i].m_sCorves.i_ImageHeight[index] = m_sCarvedCamInfo[i].m_iResImageHeight;
			m_sRealCamInfo[i].m_sCorves.i_ImageCount = 1;

			m_sCarvedCamInfo[i].m_iToRealCamera = i;
			m_sCarvedCamInfo[i].i_ImageX = 0;
			m_sCarvedCamInfo[i].i_ImageY = 0;
			m_sCarvedCamInfo[i].m_iImageWidth = m_sRealCamInfo[i].m_iImageWidth;
			m_sCarvedCamInfo[i].m_iImageHeight = m_sRealCamInfo[i].m_iImageHeight;

			m_sCarvedCamInfo[i].m_iStress = 1;
		}
	}
	//测试图像应力检测配置是否正确
	for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
	{
		if (2 == m_sCarvedCamInfo[i].m_iStress)
		{
			int iNormalCamera = m_sCarvedCamInfo[i].m_iToNormalCamera;
			if (iNormalCamera>m_sSystemInfo.iCamCount)
			{
				QMessageBox::information(this,tr("Error"),tr("Stress camera%1 Error,camera%2 is not a available camera!").arg(i+1).arg(iNormalCamera+1));
				return;
			}
			if (m_sCarvedCamInfo[iNormalCamera].m_iStress > 1)
			{
				QMessageBox::information(this,tr("Error"),tr("Stress camera%1 Error,camera%2 is not a normal camera!").arg(i+1).arg(iNormalCamera+1));
				return;
			}
		}
		if (0 == m_sCarvedCamInfo[i].m_iStress)
		{
			int iStressCamera = m_sCarvedCamInfo[i].m_iToStressCamera;
			if (iStressCamera>m_sSystemInfo.iCamCount)
			{
				QMessageBox::information(this,tr("Error"),tr("Stress camera%1 Error,camera%2 is not a available camera!").arg(i+1).arg(iStressCamera+1));
				return;
			}

			if (m_sCarvedCamInfo[iStressCamera].m_iStress < 1)
			{
				QMessageBox::information(this,tr("Error"),tr("camera%1 Error,camera%2 is not a Stress camera!").arg(i+1).arg(iStressCamera+1));
				return;
			}
		}
	}
}
//加载参数和相机
void GlasswareDetectSystem::LoadParameterAndCam()
{
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		//回调
		struGrabCardPara[i].CallBackFunc = GlobalGrabOverCallback;
		struGrabCardPara[i].Context = this;
		//初始化采集卡
		InitGrabCard(struGrabCardPara[i],i);

	}
}
//初始化采集卡（：初始化相机）
void GlasswareDetectSystem::InitGrabCard(s_GBINITSTRUCT struGrabCardPara,int index)
{
	QString strDeviceName = QString(struGrabCardPara.strDeviceName);
	if (strDeviceName=="SimulaGrab")
	{
		m_sRealCamInfo[index].m_pGrabber = new CDHGrabberSG;
		m_sRealCamInfo[index].m_bSmuGrabber = true;
		m_sRealCamInfo[index].m_iGrabType = 0;
	}
	else if (strDeviceName == "AVT")
	{
		m_sRealCamInfo[index].m_pGrabber = new CDHGrabberAVT;
		m_sRealCamInfo[index].m_bSmuGrabber = false;
		m_sRealCamInfo[index].m_iGrabType = 1;
	}
	
	else if (strDeviceName == "Baumer")
	{
		m_sRealCamInfo[index].m_pGrabber = new CDHGrabberBaumer;
		m_sRealCamInfo[index].m_bSmuGrabber = false;
		m_sRealCamInfo[index].m_iGrabType = 3;
	}
	else if (strDeviceName == "MER")
	{
		m_sRealCamInfo[index].m_pGrabber = new CDHGrabberMER;
		m_sRealCamInfo[index].m_bSmuGrabber = false;
		m_sRealCamInfo[index].m_iGrabType = 8;
	}else if (strDeviceName == "Basler")
	{
		//m_sRealCamInfo[index].m_pGrabber = new CDHGrabberBasler;
		m_sRealCamInfo[index].m_bSmuGrabber = false;
		m_sRealCamInfo[index].m_iGrabType = 2;
	}else
	{
		m_sRealCamInfo[index].m_pGrabber = new CDHGrabberSG;
		m_sRealCamInfo[index].m_bSmuGrabber = true;
		m_sRealCamInfo[index].m_iGrabType = 0;
	}	
	BOOL bRet = FALSE;
	int iErrorPosition = 0;
	try
	{
		bRet = m_sRealCamInfo[index].m_pGrabber->Init(&struGrabCardPara);	
		
		if(bRet)
		{
			iErrorPosition = 1;
			m_sRealCamInfo[index].m_bCameraInitSuccess=TRUE;
			bRet = m_sRealCamInfo[index].m_pGrabber->GetParamInt(GBImageWidth, m_sRealCamInfo[index].m_iImageWidth);
			if(bRet)
			{
				iErrorPosition = 2;
				bRet = m_sRealCamInfo[index].m_pGrabber->GetParamInt(GBImageHeight, m_sRealCamInfo[index].m_iImageHeight);
				if(bRet)
				{
					iErrorPosition = 3;
					bRet = m_sRealCamInfo[index].m_pGrabber->GetParamInt(GBImageBufferSize, m_sRealCamInfo[index].m_iImageSize);	
					if(bRet)
					{
						iErrorPosition = 4;
						int nImagePixelSize = 0;
						bRet = m_sRealCamInfo[index].m_pGrabber->GetParamInt(GBImagePixelSize, nImagePixelSize);
						if(bRet)
						{
							int result=0;
							bRet = m_sRealCamInfo[index].m_pGrabber->GetParamInt(GBImageBufferAddr, result);
							if(bRet)
							{
								iErrorPosition = 5;
								m_sRealCamInfo[index].m_iImageBitCount =8* nImagePixelSize;
								if(strDeviceName == "AVT")
								{
									iErrorPosition = 6;
									((CDHGrabberAVT*)m_sRealCamInfo[index].m_pGrabber)->AVTSetTriggerParam(AVTTriggerOn);
								}
								if(strDeviceName == "Baumer")
								{
									iErrorPosition = 6;
									((CDHGrabberBaumer*)m_sRealCamInfo[index].m_pGrabber)->BaumerSetTriggerParam(BaumerTriggerOn);
								}
								if(strDeviceName == "MER")
								{
									iErrorPosition = 6;
									((CDHGrabberMER*)m_sRealCamInfo[index].m_pGrabber)->MERSetParamInt(MERSnapMode,1);
									//设备相机buffer大小
									//((CDHGrabberMER*)m_sRealCamInfo[index].m_pGrabber)->MERSetParamInt(MERBufferSize,m_CameraBuffer);
								}
							}
						}
					}
				}
			}		
		}
	}
	catch (...)
	{
		QString strError;
		strError = QString("catch camera%1initial error").arg(index);
		pMainFrm->Logfile.write(strError,OperationLog);
		m_sRealCamInfo[index].m_bCameraInitSuccess = FALSE;
		//QMessageBox::information(this,tr("Error"),QString::fromLocal8Bit("相机%1初始化失败").arg(index));
	}
	//pMainFrm->Logfile.write(QString("%1---%2----%3").arg(m_sRealCamInfo[index].m_iImageWidth).arg(m_sRealCamInfo[index].m_iImageHeight).arg(m_sRealCamInfo[index].m_iImageBitCount),OperationLog);

	if (bRet)
	{
		InitCam();
		m_sRealCamInfo[index].m_bCameraInitSuccess = TRUE;

	}
	else
	{
		m_sRealCamInfo[index].m_bCameraInitSuccess = FALSE;

		s_GBERRORINFO ErrorInfo;
		QString str;			
		m_sRealCamInfo[index].m_pGrabber->GetLastErrorInfo(&ErrorInfo);
		str = tr("DeviceName:%1").arg(strDeviceName)+"\n"+\
			tr("ErrorCode:%2").arg(ErrorInfo.nErrorCode)+"\n"+\
			tr("ErrorDescription:%3").arg(ErrorInfo.strErrorDescription)+"\n"+\
			tr("ErrorRemark:%4\n").arg(ErrorInfo.strErrorRemark)+"\n"+\
			tr("ErrorPosition:%5\n").arg(iErrorPosition);
		QMessageBox::information(this,tr("Error"),str);
		QString strError;
		strError = QString("camera%1initial error,ErrorPosition%2").arg(index).arg(iErrorPosition);
		pMainFrm->Logfile.write(strError,OperationLog);

		m_sRealCamInfo[index].m_strErrorInfo = str;
	}
	int iRealCameraSN = index;
	if (90 == m_sRealCamInfo[iRealCameraSN].m_iImageRoAngle || 270 == m_sRealCamInfo[iRealCameraSN].m_iImageRoAngle )
	{
		int iTemp = m_sRealCamInfo[iRealCameraSN].m_iImageHeight;
		m_sRealCamInfo[iRealCameraSN].m_iImageHeight = m_sRealCamInfo[iRealCameraSN].m_iImageWidth;
		m_sRealCamInfo[iRealCameraSN].m_iImageWidth = iTemp;
	}
}

//初始化相机（设置曝光时间和触发方式）
void GlasswareDetectSystem::InitCam()
{
	for(int i = 0; i < m_sSystemInfo.iRealCamCount; i++)
	{
		m_mutexmGrab.lock();
		if(m_sRealCamInfo[i].m_iGrabType == 1)
		{
			if(m_sRealCamInfo[i].m_iTrigger == 1)
			{
				((CDHGrabberAVT*)m_sRealCamInfo[i].m_pGrabber)->AVTSetTriggerParam(AVTTriggerOn);
				m_sRealCamInfo[i].m_bGrabIsTrigger = true;
			}
			else if(m_sRealCamInfo[i].m_iTrigger == 0)
			{
				((CDHGrabberAVT*)m_sRealCamInfo[i].m_pGrabber)->AVTSetTriggerParam(AVTTriggerOff);
				m_sRealCamInfo[i].m_bGrabIsTrigger = false;
			}
			((CDHGrabberAVT*)m_sRealCamInfo[i].m_pGrabber)->AVTSetParamPro(AVTShutter,m_sRealCamInfo[i].m_iShuter);
		}
		if(m_sRealCamInfo[i].m_iGrabType == 2)
		{
			//m_sRealCamInfo[i].m_bGrabIsTrigger = true;
			//((CDHGrabberBasler*)m_sRealCamInfo[i].m_pGrabber)->BalserSetParam(Type_Basler_ExposureTimeAbs,m_sRealCamInfo[i].m_iShuter);
		}
		if(m_sRealCamInfo[i].m_iGrabType == 3)
		{
			if(m_sRealCamInfo[i].m_iTrigger == 1)
			{
				((CDHGrabberBaumer*)m_sRealCamInfo[i].m_pGrabber)->BaumerSetTriggerParam(BaumerTriggerOn);
				m_sRealCamInfo[i].m_bGrabIsTrigger = true;
			}
			else if(m_sRealCamInfo[i].m_iTrigger == 0)
			{
				((CDHGrabberBaumer*)m_sRealCamInfo[i].m_pGrabber)->BaumerSetTriggerParam(BaumerTriggerOff);
				m_sRealCamInfo[i].m_bGrabIsTrigger = false;
			}
			((CDHGrabberBaumer*)m_sRealCamInfo[i].m_pGrabber)->BaumerSetParamPro(BaumerExposure,m_sRealCamInfo[i].m_iShuter);
		}
		if(m_sRealCamInfo[i].m_iGrabType == 8)
		{
			if(m_sRealCamInfo[i].m_iTrigger == 1)
			{
				((CDHGrabberMER*)m_sRealCamInfo[i].m_pGrabber)->MERSetParamInt(MERSnapMode,1);
				m_sRealCamInfo[i].m_bGrabIsTrigger = true;
			}
			else if(m_sRealCamInfo[i].m_iTrigger == 0)
			{
				((CDHGrabberMER*)m_sRealCamInfo[i].m_pGrabber)->MERSetParamInt(MERSnapMode,0);
				m_sRealCamInfo[i].m_bGrabIsTrigger = false;
			}
			((CDHGrabberMER*)m_sRealCamInfo[i].m_pGrabber)->MERSetParamInt(MERExposure,m_sRealCamInfo[i].m_iShuter);
		}		
		m_mutexmGrab.unlock();
	}
}



//初始化图像（：读取切割信息:初始化图像队列和剪切后相机参数）
void GlasswareDetectSystem::InitImage()
{
	for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
	{
		int iRealCameraSN = m_sCarvedCamInfo[i].m_iToRealCamera;

		m_sCarvedCamInfo[i].m_iResImageWidth = m_sRealCamInfo[iRealCameraSN].m_iImageWidth;
		m_sCarvedCamInfo[i].m_iResImageHeight = m_sRealCamInfo[iRealCameraSN].m_iImageHeight;
		m_sCarvedCamInfo[i].m_iImageType = m_sRealCamInfo[iRealCameraSN].m_iImageType;
		m_sCarvedCamInfo[i].m_iIOCardSN =  m_sRealCamInfo[iRealCameraSN].m_iIOCardSN;
	}

	//获取最大图像信息
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		if (i==0)
		{
			m_sSystemInfo.m_iMaxCameraImageWidth     = m_sRealCamInfo[i].m_iImageWidth;
			m_sSystemInfo.m_iMaxCameraImageHeight    = m_sRealCamInfo[i].m_iImageHeight;
			m_sSystemInfo.m_iMaxCameraImageSize      = m_sRealCamInfo[i].m_iImageSize;
			m_sSystemInfo.m_iMaxCameraImagePixelSize = (m_sRealCamInfo[i].m_iImageBitCount+7)/8;
		}
		else
		{
			if (m_sRealCamInfo[i].m_iImageWidth > m_sSystemInfo.m_iMaxCameraImageWidth)
			{
				m_sSystemInfo.m_iMaxCameraImageWidth = m_sRealCamInfo[i].m_iImageWidth;
			}				
			if (m_sRealCamInfo[i].m_iImageHeight > m_sSystemInfo.m_iMaxCameraImageHeight)
			{
				m_sSystemInfo.m_iMaxCameraImageHeight = m_sRealCamInfo[i].m_iImageHeight;
			}				
			if (((m_sRealCamInfo[i].m_iImageBitCount+7)/8) > m_sSystemInfo.m_iMaxCameraImagePixelSize)
			{
				m_sSystemInfo.m_iMaxCameraImagePixelSize = ((m_sRealCamInfo[i].m_iImageBitCount+7)/8);
			}			
		}
		m_sSystemInfo.m_iMaxCameraImageSize = m_sSystemInfo.m_iMaxCameraImageWidth*m_sSystemInfo.m_iMaxCameraImageHeight;
	}

	// 	QSettings iniset(m_sConfigInfo.m_strConfigPath,QSettings::IniFormat);
	//读取切割信息,因为要用到初始化以后的相机信息，所以不能提前读取。
	ReadCorveConfig();
	int iCarvedCamNum = 0;
	QString strSession;
	for(int i = 0 ; i < m_sSystemInfo.iRealCamCount; i++)
	{
		//分配原始图像空间：每真实相机1个，剪切图像使用
		if (m_sRealCamInfo[i].m_pRealImage!=NULL)
		{
			delete m_sRealCamInfo[i].m_pRealImage;
			m_sRealCamInfo[i].m_pRealImage = NULL;
		}
		
		m_sRealCamInfo[i].m_pRealImage=new QImage(m_sRealCamInfo[i].m_iImageWidth,m_sRealCamInfo[i].m_iImageHeight,m_sRealCamInfo[i].m_iImageBitCount);// 用于实时显示

		if (8 == m_sRealCamInfo[i].m_iImageBitCount)
		{
			m_sRealCamInfo[i].m_pRealImage->setColorTable(m_vcolorTable);
		}

		memset(m_sRealCamInfo[i].m_pRealImage->bits(),0, m_sRealCamInfo[i].m_pRealImage->byteCount());

		for(int j = 0; j < m_sRealCamInfo[i].m_sCorves.i_ImageCount; j++)
		{
			iCarvedCamNum = m_sRealCamInfo[i].m_sCorves.i_GrabSN[j];

			m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount = m_sRealCamInfo[i].m_iImageBitCount;   //图像位数从相机处继承[8/7/2013 nanjc]
			m_sCarvedCamInfo[iCarvedCamNum].m_iImageRoAngle = m_sRealCamInfo[i].m_iImageRoAngle;
			// 错误统计用类
			m_sRunningInfo.m_cErrorTypeInfo[iCarvedCamNum].m_iErrorTypeCount = m_sErrorInfo.m_iErrorTypeCount;

			//实时显示用, 预分配QImage空间，每切出相机一个
			if (m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage!=NULL)
			{
				delete m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage;
				m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage = NULL;
			}
			if (90 == m_sCarvedCamInfo[iCarvedCamNum].m_iImageRoAngle || 270 ==m_sCarvedCamInfo[iCarvedCamNum].m_iImageRoAngle)
			{
				m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage=new QImage(m_sCarvedCamInfo[iCarvedCamNum].m_iImageHeight,m_sCarvedCamInfo[iCarvedCamNum].m_iImageWidth,m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount);// 用于实时显示
			}
			else
			{
				m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage=new QImage(m_sCarvedCamInfo[iCarvedCamNum].m_iImageWidth,m_sCarvedCamInfo[iCarvedCamNum].m_iImageHeight,m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount);// 用于实时显示
			}
			m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage->setColorTable(m_vcolorTable);
			//开始采集前补一张黑图
			BYTE* pByte = m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage->bits();
			int iLength = m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage->byteCount();
			memset((pByte),0,(iLength));
			//分配图像剪切内存区域,大小等于真实相机大小
			if (m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp!=NULL)
			{
				delete m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp; 
				m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp = NULL;
			}
			m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp = new BYTE[m_sRealCamInfo[i].m_iImageWidth*m_sRealCamInfo[i].m_iImageHeight];
			//分配元素链表中图像的内存，每剪切出来的相机10个。
			m_queue[iCarvedCamNum].mGrabLocker.lock();
			m_queue[iCarvedCamNum].InitQueue(m_sCarvedCamInfo[iCarvedCamNum].m_iImageWidth, m_sCarvedCamInfo[iCarvedCamNum].m_iImageHeight,m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount, 30, true);
			m_queue[iCarvedCamNum].mGrabLocker.unlock();
			for (int k = 0; k < 256;k++)
			{
				delete []m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_AlgImageLocInfos.sXldPoint.nColsAry;
				delete []m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_AlgImageLocInfos.sXldPoint.nRowsAry;

				m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_iHaveInfo = 0;
				m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_AlgImageLocInfos.sXldPoint.nRowsAry = new int[4*BOTTLEXLD_POINTNUM];
				m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_AlgImageLocInfos.sXldPoint.nColsAry = new int[4*BOTTLEXLD_POINTNUM];
				memset(m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_AlgImageLocInfos.sXldPoint.nRowsAry,0, 4*BOTTLEXLD_POINTNUM);
				memset(m_sCarvedCamInfo[iCarvedCamNum].sImageLocInfo[k].m_AlgImageLocInfos.sXldPoint.nColsAry,0, 4*BOTTLEXLD_POINTNUM);
// 				memset
			}
		}
	}
	SetCarvedCamInfo();
	//初始化缺陷图像列表
	m_ErrorList.initErrorList(m_sSystemInfo.m_iMaxCameraImageWidth,m_sSystemInfo.m_iMaxCameraImageHeight,m_sSystemInfo.m_iMaxCameraImagePixelSize*8,ERROR_IMAGE_COUNT,true);
}
//设置剪切后相机的参数
void GlasswareDetectSystem::SetCarvedCamInfo()
{
	for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
	{
		int iRealCameraSN = m_sCarvedCamInfo[i].m_iToRealCamera;
		m_sCarvedCamInfo[i].m_iResImageWidth = m_sRealCamInfo[iRealCameraSN].m_iImageWidth;
		m_sCarvedCamInfo[i].m_iResImageHeight = m_sRealCamInfo[iRealCameraSN].m_iImageHeight;
		m_sCarvedCamInfo[i].m_iImageType = m_sRealCamInfo[iRealCameraSN].m_iImageType;
		m_sCarvedCamInfo[i].m_iIOCardSN =  m_sRealCamInfo[iRealCameraSN].m_iIOCardSN;
		m_sCarvedCamInfo[i].m_iShuter = m_sRealCamInfo[iRealCameraSN].m_iShuter;
		m_sCarvedCamInfo[i].m_iTrigger = m_sRealCamInfo[iRealCameraSN].m_iTrigger;
		m_sCarvedCamInfo[i].m_iGrabPosition = m_sRealCamInfo[iRealCameraSN].m_iGrabPosition;
		if(i==11)
		{
			m_sCarvedCamInfo[i].m_iIOCardSN=0;
		}
	}
	
	SetCombineInfo();
}
//设置图像综合参数
void GlasswareDetectSystem::SetCombineInfo()
{
	//初始化结果综合参数
	for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
	{
		if (m_sCarvedCamInfo[i].m_iIOCardSN == 0)
		{
			m_cCombine.SetCombineCamera(i,true);
			m_sSystemInfo.IOCardiCamCount[0]++;
		}
		else
		{
			m_cCombine.SetCombineCamera(i,false);
		}
		if (2 == m_sSystemInfo.iIOCardCount)
		{
				if (m_sCarvedCamInfo[i].m_iIOCardSN == 1)
			{
				m_cCombine1.SetCombineCamera(i,true);
				m_sSystemInfo.IOCardiCamCount[1]++;
			}
			else
			{
				m_cCombine1.SetCombineCamera(i,false);
			}
		}
	}
	m_cCombine.Inital(m_sSystemInfo.IOCardiCamCount[0]);
	if (2 == m_sSystemInfo.iIOCardCount)
	{
		m_cCombine1.Inital(m_sSystemInfo.IOCardiCamCount[1]);
	}
}
//初始化IO卡
void GlasswareDetectSystem::InitIOCard()
{
	if (1 != m_sSystemInfo.iIOCardCount && (m_sSystemInfo.m_iSystemType == 0 || m_sSystemInfo.m_iSystemType == 1 || m_sSystemInfo.m_iSystemType == 4))
	{
		QMessageBox::information(this,tr("Error"),tr("IOCard Number is not correct!system type:%1,IOCardNumber:%2").arg(m_sSystemInfo.m_iSystemType).arg(m_sSystemInfo.iIOCardCount));
		pMainFrm->Logfile.write(tr("IOCard Number is not correct!system type:%1,IOCardNumber:%2").arg(m_sSystemInfo.m_iSystemType).arg(m_sSystemInfo.iIOCardCount),AbnormityLog);
//		m_sSystemInfo.m_bIsIOCardOK = FALSE;
	}
	else if (2 != m_sSystemInfo.iIOCardCount && (m_sSystemInfo.m_iSystemType == 2 || m_sSystemInfo.m_iSystemType == 3 || m_sSystemInfo.m_iSystemType == 5 || m_sSystemInfo.m_iSystemType == 6))
	{
		QMessageBox::information(this,tr("Error"),tr("IOCard Number is not correct!system type:%1,IOCardNumber:%2").arg(m_sSystemInfo.m_iSystemType).arg(m_sSystemInfo.iIOCardCount));
		pMainFrm->Logfile.write(tr("IOCard Number is not correct!system type:%1,IOCardNumber:%2").arg(m_sSystemInfo.m_iSystemType).arg(m_sSystemInfo.iIOCardCount),AbnormityLog);
//		m_sSystemInfo.m_bIsIOCardOK = FALSE;
	}
	if (m_sSystemInfo.m_bIsIOCardOK)
	{
		//		CIOCard *pIOCard;
		for (int i=0;i<m_sSystemInfo.iIOCardCount;i++)
		{
			if(i ==0)
			{
				m_sSystemInfo.m_sConfigIOCardInfo[i].iCardID = i;
				m_sSystemInfo.m_sConfigIOCardInfo[i].strCardInitFile = QString("./PIO24B_reg_init.txt");
				m_sSystemInfo.m_sConfigIOCardInfo[i].strCardName = QString("PIO24B");
			}
			else
			{
				m_sSystemInfo.m_sConfigIOCardInfo[i].iCardID = i;
				m_sSystemInfo.m_sConfigIOCardInfo[i].strCardInitFile = QString("./PIO24B_reg_init%1.txt").arg(i);
				m_sSystemInfo.m_sConfigIOCardInfo[i].strCardName = QString("PIO24B").arg(i);
			}
			m_vIOCard[i] = new CIOCard(m_sSystemInfo.m_sConfigIOCardInfo[i],i);
			connect(m_vIOCard[i],SIGNAL(emitMessageBoxMainThread(s_MSGBoxInfo)),this,SLOT(slots_MessageBoxMainThread(s_MSGBoxInfo)));
			s_IOCardErrorInfo sIOCardErrorInfo = m_vIOCard[i]->InitIOCard();
			Sleep(200);
			if (!sIOCardErrorInfo.bResult)
			{
				m_sSystemInfo.m_bIsIOCardOK = false;
				pMainFrm->Logfile.write(tr("Error in init IOCard"),AbnormityLog);
			}
			//m_vIOCard[i] = pIOCard;
		}
	}
}
//初始化算法
void GlasswareDetectSystem::InitCheckSet()
{
	//算法初始化，模板调入等 [8/4/2010 GZ]
	s_Status  sReturnStatus;
	s_AlgInitParam   sAlgInitParam;	
	//	QSettings iniAlgSet(m_sConfigInfo.m_strConfigPath,QSettings::IniFormat);
	if(m_sSystemInfo.m_iMaxCameraImageWidth>m_sSystemInfo.m_iMaxCameraImageHeight)
	{
		sReturnStatus = init_bottle_module(m_sSystemInfo.m_iMaxCameraImageWidth,m_sSystemInfo.m_iMaxCameraImageWidth,1);
	}
	else
	{
		sReturnStatus = init_bottle_module(m_sSystemInfo.m_iMaxCameraImageHeight,m_sSystemInfo.m_iMaxCameraImageHeight,1);
	}
	if (sReturnStatus.nErrorID != RETURN_OK)
	{
		pMainFrm->Logfile.write(tr("----load model error----"),AbnormityLog);
		return;
	}	
	for (int i=0;i<m_sSystemInfo.iCamCount;i++)
	{
		sAlgInitParam.nCamIndex=i;
		sAlgInitParam.nModelType = m_sRealCamInfo[i].m_iImageType;  //检测类型
		sAlgInitParam.nWidth = m_sRealCamInfo[i].m_iImageWidth; 
		sAlgInitParam.nHeight =  m_sRealCamInfo[i].m_iImageHeight;
		memset(sAlgInitParam.chCurrentPath,0,MAX_PATH);

		strcpy_s(sAlgInitParam.chCurrentPath,m_sConfigInfo.m_sAlgFilePath.toLocal8Bit()); 
		memset(sAlgInitParam.chModelName,0,MAX_PATH); //模板名称
		strcpy_s(sAlgInitParam.chModelName,m_sSystemInfo.m_strModelName.toLocal8Bit()); 
		sReturnStatus = m_cBottleCheck[i].init(sAlgInitParam);

		if (sReturnStatus.nErrorID != RETURN_OK && sReturnStatus.nErrorID != 1)
		{
			pMainFrm->Logfile.write(tr("----camera%1 load model error----").arg(i),AbnormityLog);
			return;
		}
		if (sReturnStatus.nErrorID == 1) //模板为空
		{
			//模板为空
			m_sSystemInfo.m_bLoadModel =  FALSE;  //如果模板为空，则不能检测 
		}
		else
		{
			m_sSystemInfo.m_bLoadModel =  TRUE;  //成功载入上一次的模板
		}
		// 旋转类 [12/10/2010]
		sAlgInitParam.nModelType = 99;  //检测类型
		memset(sAlgInitParam.chModelName,0,MAX_PATH); //模板名称
		m_cBottleRotate[i].init(sAlgInitParam);
		sAlgInitParam.nModelType = 98;  //检测类型
		m_cBottleStress[i].init(sAlgInitParam);
	}
	// 算法初始化，模板调入等 [8/4/2010 GZ]
	//////////////////////////////////////////////////////////////////////////
	if (CherkerAry.pCheckerlist != NULL)
	{
		delete[] CherkerAry.pCheckerlist;
	}
	CherkerAry.iValidNum = m_sSystemInfo.iCamCount;
	CherkerAry.pCheckerlist = new s_CheckerList[CherkerAry.iValidNum];


	//////////////////////////////////////////////////////////////////////////

}
//开启相机采集
void GlasswareDetectSystem::StartCamGrab()
{ 
	for (int i = 0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		m_sRealCamInfo[i].m_pGrabber->StartGrab();
		m_sRealCamInfo[i].m_bGrabIsStart=TRUE;
	}
	//屏蔽系统键 [4/20/2011 lly]
	//OnDisableTaskKeys(TRUE);
}
//开启检测线程
void GlasswareDetectSystem::StartDetectThread()
{
	m_bIsThreadDead = FALSE;
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		pdetthread[i]->start();
	}
}
void GlasswareDetectSystem::initDetectThread()
{
	m_bIsThreadDead = FALSE;
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		m_sRealCamInfo[i].m_hReceiveLocInfo = CreateEvent(NULL, FALSE, TRUE, NULL);  
		m_sRealCamInfo[i].m_hSendLocInfo = CreateEvent(NULL, FALSE, FALSE, NULL);  
	}
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		pdetthread[i] = new DetectThread(this);
		pdetthread[i]->ThreadNumber = i;
	}
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	nCPUNumber = SystemInfo.dwNumberOfProcessors;
}
//初始化界面
void GlasswareDetectSystem::initInterface()
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog  | Qt::WindowSystemMenuHint);//去掉标题栏
	QDesktopWidget* desktopWidget = QApplication::desktop();
	QRect screenRect = desktopWidget->screenGeometry();
	setMinimumSize(screenRect.width(),screenRect.height());

	QIcon icon;
	icon.addFile(QString::fromUtf8(":/sys/icon"), QSize(), QIcon::Normal, QIcon::Off);
	setWindowIcon(icon);

	statked_widget = new QStackedWidget();
	statked_widget->setObjectName("mainStacked");
	title_widget = new WidgetTitle(this);
	widget_carveSetting = new WidgetCarveSetting;
	widget_Management = new WidgetManagement;
	test_widget = new WidgetTest(this);
	test_widget->slots_intoWidget();
	widgetHelp = new widget_Help;

	widgetInfoContainer = new QWidget;
	info_widget = new widget_info(widgetInfoContainer);
	widget_UserManege = new UserManegeWidget(widgetInfoContainer);
	QHBoxLayout *layoutWidgetInfoContainer = new QHBoxLayout(widgetInfoContainer);
	layoutWidgetInfoContainer->addWidget(widget_UserManege);
	layoutWidgetInfoContainer->addWidget(info_widget);
	widget_UserManege->setVisible(false);

	widget_count = new Widget_Count(this);
	widget_Warning = new WidgetWarning(this);
	connect(test_widget,SIGNAL(signals_sendAlarm(int, QString)),widget_Warning,SLOT(slots_ShowWarning(int, QString)));
	if (m_sSystemInfo.m_bIsUsePLC)
	{
		widget_PLC = new Widget_PLC(this);
	}
	widget_alg = new QWidget(this);
	widget_alg->setObjectName("widget_alg");
	widget_Debug = new Widget_Debug(this);


	QPalette palette;
	palette.setBrush(QPalette::Window, QBrush(Qt::white));
	statked_widget->setPalette(palette);
	statked_widget->setAutoFillBackground(true);
	statked_widget->addWidget(widgetInfoContainer);
	statked_widget->addWidget(widget_carveSetting);
	statked_widget->addWidget(widget_Management);
	statked_widget->addWidget(test_widget);
	statked_widget->addWidget(widget_count);
	if (m_sSystemInfo.m_bIsUsePLC)
	{
		statked_widget->addWidget(widget_PLC);
	}
	statked_widget->addWidget(widget_alg);
	statked_widget->addWidget(widget_Debug);
	statked_widget->addWidget(widgetHelp);

	/*作为测试之后
	widget_testWidget = new Widget_TestWighet();
	statked_widget->addWidget(widget_testWidget);*/

	//状态栏
	stateBar = new QWidget(this);
	stateBar->setFixedHeight(40);
	QGridLayout* gridLayoutStatusLight = new QGridLayout;
	for (int i = 0;i<pMainFrm->m_sSystemInfo.iCamCount;i++)
	{
		CameraStatusLabel *cameraStatus = new CameraStatusLabel(stateBar);
		cameraStatus->setObjectName("toolButtonCamera");
		cameraStatus->setAlignment(Qt::AlignCenter);
		cameraStatus->setText(QString::number(i+1));

		cameraStatus_list.append(cameraStatus);
		//hLayoutButton->addWidget(cameraStatus);
		if (0 == pMainFrm->m_sSystemInfo.m_iImageStyle)
		{
			gridLayoutStatusLight->addWidget(cameraStatus,i%2,i/2);
		}
		else if (1 == pMainFrm->m_sSystemInfo.m_iImageStyle)
		{
			if (i < (pMainFrm->m_sSystemInfo.iCamCount+1)/2)
			{
				gridLayoutStatusLight->addWidget(cameraStatus,0,i);
			}
			else
			{
				gridLayoutStatusLight->addWidget(cameraStatus ,1,i - (pMainFrm->m_sSystemInfo.iCamCount+1)/2);
			}
		}
	}
	//检查相机状态
	for (int i = 0; i < pMainFrm->m_sSystemInfo.iCamCount; i++)
	{
		int iRealCameraSN = pMainFrm->m_sCarvedCamInfo[i].m_iToRealCamera;
		CameraStatusLabel *cameraStatus = pMainFrm->cameraStatus_list.at(i);
		if (!pMainFrm->m_sRealCamInfo[iRealCameraSN].m_bCameraInitSuccess)
		{
			cameraStatus->SetCameraStatus(1);
		}
		else
		{
			cameraStatus->SetCameraStatus(0);
		}
	}

	labelCoder = new QLabel(stateBar);
	labelCoder->setText(tr("Signal Status"));
	//labelPLC = new QLabel(stateBar);
	//labelPLC->setText(tr("PLC Status"));
	//labelPLC->setMinimumSize(300,stateBar->geometry().height());
	//labelPLC->setAlignment(Qt::AlignCenter);
	if (!m_sSystemInfo.m_bIsUsePLC)
	{
		//labelPLC->setVisible(false);
	}

	labelSpeed = new QLabel(stateBar);
	labelSpeed->setText(tr("PLC Status"));
	labelSpeed->setMinimumSize(300,stateBar->geometry().height());
	labelSpeed->setAlignment(Qt::AlignCenter);

	labelUnknowMoldNumber = new QLabel(stateBar);
	labelUnknowMoldNumber->setText(tr("PLC Status"));
	labelUnknowMoldNumber->setMinimumSize(300,stateBar->geometry().height());
	labelUnknowMoldNumber->setAlignment(Qt::AlignCenter);

	QFont fontCoder;
	fontCoder.setPixelSize(28);
	//labelPLC->setFont(fontCoder);
	labelSpeed->setFont(fontCoder);
	labelUnknowMoldNumber->setFont(fontCoder);
	fontCoder.setPixelSize(28);
	labelCoder->setFont(fontCoder);
	timerUpdateCoder = new QTimer(this);
	timerUpdateCoder->setInterval(1000);
	timerUpdateCoder->start();  


	QHBoxLayout* hLayoutStateBar = new QHBoxLayout(stateBar);
	hLayoutStateBar->addLayout(gridLayoutStatusLight);
	hLayoutStateBar->addStretch();
	//hLayoutStateBar->addWidget(labelPLC);
	hLayoutStateBar->addWidget(labelSpeed);
	hLayoutStateBar->addWidget(labelUnknowMoldNumber);
	hLayoutStateBar->addStretch();
	hLayoutStateBar->addWidget(labelCoder);
	hLayoutStateBar->setSpacing(3);
	hLayoutStateBar->setContentsMargins(10, 0, 10, 0);

	QHBoxLayout *center_layout = new QHBoxLayout();
	center_layout->addWidget(statked_widget);
	//center_layout->addWidget(label1);
	center_layout->setSpacing(0);
	center_layout->setContentsMargins(5,5,5,5);
	//center_layout->setContentsMargins(0,0,0,0);

	QVBoxLayout *main_layout = new QVBoxLayout();

	main_layout->addWidget(title_widget);
	main_layout->addLayout(center_layout);
	main_layout->addWidget(stateBar);
	main_layout->setSpacing(0);
	// 	main_layout->setContentsMargins(SHADOW_WIDTH, SHADOW_WIDTH, SHADOW_WIDTH, SHADOW_WIDTH);
	main_layout->setContentsMargins(0, 0, 0, 0);

	setLayout(main_layout);

	connect(title_widget, SIGNAL(showMin()), this, SLOT(showMinimized()));
	connect(title_widget, SIGNAL(closeWidget()), this, SLOT(slots_OnExit()));
	connect(title_widget, SIGNAL(turnPage(int)), this, SLOT(slots_turnPage(int)));
	connect(this,SIGNAL(signals_intoCountWidget()),widget_count,SLOT(slots_intoWidget()));	

	connect(this,SIGNAL(signals_intoInfoWidget()),info_widget,SLOT(slots_intoWidget()));	
	connect(this,SIGNAL(signals_intoCarveSettingWidget()),widget_carveSetting,SLOT(slots_intoWidget()));	
	connect(this,SIGNAL(signals_intoManagementWidget()),widget_Management,SLOT(slots_intoWidget()));	
	connect(this,SIGNAL(signals_intoTestWidget()),test_widget,SLOT(slots_intoWidget()));	
// 	connect(this,SIGNAL(signals_intoPLCWidget()),widget_PLC,SLOT(slots_intoWidget()));	
	connect(this,SIGNAL(signals_clear()),widget_count,SLOT(slots_ClearCountInfo()));	
	connect(this,SIGNAL(signals_intoDebugWidget()),widget_Debug,SLOT(slots_intoWidget()));	
	connect(this,SIGNAL(signals_intoPLCWidget()),widget_PLC,SLOT(slots_intoWidget()));	
	connect(this,SIGNAL(signals_ShowWarning(int , QString )),widget_Warning,SLOT(slots_ShowWarning(int , QString )));	
	connect(this,SIGNAL(signals_HideWarning(int)),widget_Warning,SLOT(slots_HideWarning(int)));	
	connect(info_widget, SIGNAL(signals_updateInfo()), widget_count, SLOT(slots_updateInfo()));
	connect(timerUpdateCoder, SIGNAL(timeout()), this, SLOT(slots_UpdateCoderNumber()));   
	connect(widget_PLC, SIGNAL(signals_sendPLCStatus(int)), this, SLOT(slots_ShowPLCStatus(int)));   

 	for (int i = 0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		connect(pMainFrm->pdetthread[i], SIGNAL(signals_upDateCamera(int,int)), this, SLOT(slots_updateCameraState(int,int)));
	}   
	connect(pMainFrm->widget_carveSetting->image_widget, SIGNAL(signals_SetCameraStatus(int,int)), this, SLOT(slots_SetCameraStatus(int,int)));
	connect(info_widget,SIGNAL(signals_ShowWarning(int , QString )),widget_Warning,SLOT(slots_ShowWarning(int , QString )));	
	connect(info_widget,SIGNAL(signals_HideWarning(int)),widget_Warning,SLOT(slots_HideWarning(int)));	
	
	connect(widget_Warning, SIGNAL(signals_PauseAlert()), pMainFrm->info_widget, SLOT(slots_PauseAlert()));

	m_eLastMainPage = InfoPage;
	iLastPage = 1;
	title_widget->turnPage("1");
	skin.fill(QColor(90,90,90,120));

// 	QString path = "E:\\new glasswaredetect\\runD\\ModelInfo";
// 	fsWatcher.addPath(path);
// 	connect(&fsWatcher, SIGNAL(directoryChanged(QString)), this,SLOT(directoryChanged(QString)));
// 	connect(&fsWatcher, SIGNAL(fileChanged(QString)), this,SLOT(fileChanged(QString)));
	if (m_sRunningInfo.m_iPermission == 0)
	{
		title_widget->button_list.at(4)->setEnabled(false);
		title_widget->button_list.at(5)->setEnabled(false);
		title_widget->button_list.at(6)->setEnabled(false);
		title_widget->button_list.at(12)->setEnabled(false);
		title_widget->button_list.at(13)->setEnabled(false);
		title_widget->button_list.at(14)->setVisible(false);

		pMainFrm->test_widget->ui.widget_IOCardTest->setVisible(false);
		pMainFrm->widget_carveSetting->image_widget->buttonShowCarve->setVisible(false);
	}
}
void GlasswareDetectSystem::ShowAlelertStatus(int iStatus,int maxNumber,int CostTime,QString rate)
{
	if (iLastStatus == iStatus)
	{
		return;
	}
	else
	{
		iLastStatus = iStatus;
	}
	if(test_widget->kickOutTimer->isActive())
	{
		if (iStatus<m_vstrPLCInfoType.size())
		{
			emit signals_HideWarning(2);
			emit signals_ShowWarning(8,m_vstrPLCInfoType.at(iStatus)+rate);
		}
		else
		{
			emit signals_HideWarning(2);
			emit signals_ShowWarning(8,m_sErrorInfo.m_vstrErrorType.at(iStatus-m_vstrPLCInfoType.size()-1)
				+QString::fromLocal8Bit("报警"));
		}
	}
}
void GlasswareDetectSystem::slots_ShowPLCStatus(int iStatus)
{
	if (iLastStatus == iStatus)
	{
		return;
	}
	else
	{
		iLastStatus = iStatus;
	}
	if (iStatus<m_vstrPLCInfoType.size())
	{
		//报警框信息
		if (0 >= iStatus)
		{
			//labelPLC->setStyleSheet("color:black;");
			emit signals_HideWarning(2);
		}
		else
		{
			//labelPLC->setStyleSheet("color:red;");
			emit signals_ShowWarning(2,m_vstrPLCInfoType.at(iStatus));

		}
		//状态栏信息
		if (iStatus >= 0 )
		{
			//labelPLC->setText(m_vstrPLCInfoType.at(iStatus));
		}
		else if (-1 == iStatus)
		{
			//labelPLC->setText(tr("Manual"));
		}
		else if (-2 == iStatus)
		{
			//labelPLC->setText(tr("TestMode"));
		}
		if (0 == iStatus)
		{
			info_widget->slots_PauseAlert();
		}
	}
}
void GlasswareDetectSystem::slots_UpdateCoderNumber()
{
	int nSensorCounter = 0,nCodeNum=0,nSignNum=0,nSignNum1=0;
	if (m_sSystemInfo.m_bIsIOCardOK)
	{
		m_vIOCard[0]->m_mutexmIOCard.lock();
		nCodeNum = m_vIOCard[0]->ReadCounter(13);

		if(m_sSystemInfo.m_iSystemType == 0)
		{
			nSensorCounter = m_vIOCard[0]->ReadCounter(0);
		}else if(m_sSystemInfo.m_iSystemType == 2)
		{
			nSignNum = m_vIOCard[0]->ReadCounter(3);
		}else
		{
			nSignNum = m_vIOCard[0]->ReadCounter(4);
		}
		m_vIOCard[0]->m_mutexmIOCard.unlock();
		//药玻瓶口瓶底剔废
		
		if ((nSignNum - m_sRunningInfo.m_kickoutNumber > 0) && (nSignNum - m_sRunningInfo.m_kickoutNumber < 50))
		{
			m_sRunningInfo.m_failureNumFromIOcard = m_sRunningInfo.m_failureNumFromIOcard + nSignNum - m_sRunningInfo.m_kickoutNumber;
		}
		m_sRunningInfo.m_kickoutNumber = nSignNum;
	}

	QString strValue,strCounter,strEncoder,strTime;
	strCounter = QString(tr("Sensor Counter")+":%1").arg(nSensorCounter);
	strEncoder = QString(tr("Coder Number")+":%1").arg(nCodeNum);
	strTime = QTime::currentTime().toString() + sVersion;
	switch (pMainFrm->m_sRunningInfo.m_iKickMode)
	{
	case 0:
		labelCoder->setText(tr("Reject All") + "  " + strEncoder + " "+ strTime);
		break;
	case 1:
		labelCoder->setText(tr("Reject interval") + "  " + strEncoder + " "+ strTime);
		break;
	case 2:
		labelCoder->setText(tr("Pass All") + "  " + strEncoder + " "+ strTime);
		break;
	case 3:
	default:
		labelCoder->setText(strEncoder + " "+ strTime);
		break;

	}
	if(surplusDays>0)
	{
		labelSpeed->setText(QString::fromLocal8Bit("剩余使用天数：%1   ").arg(surplusDays)+tr("Speed:") + m_sRunningInfo.strSpeed);
	}else{
		labelSpeed->setText(tr("Speed:") + m_sRunningInfo.strSpeed);
	}
	//计算模点失败率并显示
	
	if (m_sRunningInfo.nModelReadFailureNumber !=0)
	{
		double num = 0.00;
		if(m_sRunningInfo.m_checkedNum != 0)
		{
			num = ((double)m_sRunningInfo.nModelReadFailureNumber / m_sRunningInfo.m_checkedNum) * 100;
			labelUnknowMoldNumber->setText(QString::fromLocal8Bit("读模率: %1%").arg(QString::number(num,10, 2)));
		}
	}
}
void GlasswareDetectSystem::slots_updateCameraState(int nCam,int mode)
{
	cameraStatus_list.at(nCam)->BlinkCameraStatus(mode);
}
void GlasswareDetectSystem::slots_SetCameraStatus(int nCam,int mode)
{
	cameraStatus_list.at(nCam)->SetCameraStatus(mode);

}
//裁剪
void GlasswareDetectSystem::CarveImage(uchar* pRes,uchar* pTar,int iResWidth,int iResHeight,int iTarX,int iTarY,int iTarWidth,int iTarHeight)
{
	try
	{
		uchar* pTemp = pTar;
		uchar* pTempRes = pRes+iResWidth*(iTarY)+iTarX;
		for(int i = 0; i < iTarHeight; i++)
		{
			memcpy(pTemp,pTempRes,iTarWidth);
			pTemp += iTarWidth;
			pTempRes += iResWidth;
		}
		int i=1;
	}
	catch(...)
	{
		pMainFrm->Logfile.write(tr("Error in image carve "),AbnormityLog);
	}
}
//开启IO卡线程
void GlasswareDetectSystem::AlertFunction(int i,int maxNumber,int temp_interval)
{
	pMainFrm->ShowAlelertStatus(i,maxNumber,temp_interval);
}

void GlasswareDetectSystem::slots_turnPage(int current_page, int iPara)
{
	//info_widget->tLastOperaTime;
	if (iLastPage == current_page)
	{
		return;
	}
	if ((current_page != 11) && (current_page != 12) && (current_page != 13))
	{
		if (!leaveWidget())
		{
			return;
		}
	}
	switch (current_page)
	{
	case 0:
		//		image_widget->adaptView();
		//		statked_widget->setCurrentWidget(image_widget);

		m_eLastMainPage = m_eCurrentMainPage;
		m_eCurrentMainPage = ImagePage;
		iLastPage = 0;

		break;
	case 1:
		m_eCurrentMainPage = InfoPage;
		emit signals_intoInfoWidget();
		statked_widget->setCurrentWidget(widgetInfoContainer);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 1;

		pMainFrm->Logfile.write(tr("Into information page"),OperationLog,0);

		break;
	case 2:
		m_eCurrentMainPage = CountPage;
		emit signals_intoCountWidget();
		statked_widget->setCurrentWidget(widget_count);
		//widget_count->slots_intoWidget();
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 2;
		pMainFrm->Logfile.write(tr("Into count page"),OperationLog,0);

		break;

	case 3:
		m_eCurrentMainPage = CarveSettingPage;
		emit signals_intoCarveSettingWidget();
		statked_widget->setCurrentWidget(widget_carveSetting);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 3;
		pMainFrm->Logfile.write(tr("Into Image page"),OperationLog,0);

		break;
	case 4:
		m_eCurrentMainPage = ManagementSettingPage;
		emit signals_intoManagementWidget();
		statked_widget->setCurrentWidget(widget_Management);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 4;
		pMainFrm->Logfile.write(tr("Into manegement page"),OperationLog,0);

		break;
	case 5:
		m_eCurrentMainPage = TestPage;
		emit signals_intoTestWidget();
		statked_widget->setCurrentWidget(test_widget);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 5;
		pMainFrm->Logfile.write(tr("Into system set page"),OperationLog,0);

		break;
	case 6:
		m_eCurrentMainPage = AlgPage;
		//if (widget_alg != NULL)
		//{
		//	delete widget_alg;
		//	widget_alg = new QWidget(this);
		//}
		statked_widget->setCurrentWidget(widget_alg);
		ShowCheckSet(iPara);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 6;
		break;
	case 7:
		m_eCurrentMainPage = PLCPage;
		emit signals_intoPLCWidget();
		statked_widget->setCurrentWidget(widget_PLC);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 7;
		pMainFrm->Logfile.write(tr("Into PLC page"),OperationLog,0);
		break;
	case 8:
		break;
	case 9:
		break;
	case 10:
		m_eCurrentMainPage = HelpPage;
		statked_widget->setCurrentWidget(widgetHelp);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 10;
		pMainFrm->Logfile.write(tr("Into Help page"),OperationLog,0);
		break;
	case 11:
		emit signals_clear();
		break;
	case 12:
		slots_OnBtnStar();
		break;
	case 13:
		slots_OnExit();
		break;
	case 14:
		m_eCurrentMainPage = DeBugPage;
		emit signals_intoDebugWidget();
		statked_widget->setCurrentWidget(widget_Debug);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 14;
		pMainFrm->Logfile.write(tr("In to debug page"),OperationLog,0);

		break;
	case 15:
		m_eCurrentMainPage = TestWidget2;
		statked_widget->setCurrentWidget(widget_testWidget);
		m_eLastMainPage = m_eCurrentMainPage;
		iLastPage = 15;
		pMainFrm->Logfile.write(tr("In to TestWidget2 page"),OperationLog,0);
		break;
	}
}
bool GlasswareDetectSystem::leaveWidget()
{
	switch(m_eLastMainPage)
	{
	case ImagePage:
		//if (!image_widget.leaveWidget())
		//{
		//	return false;
		//}

		break;
	case InfoPage:
		if (!info_widget->leaveWidget())
		{
			return false;
		}

		break;
	case CarveSettingPage:
		if (!widget_carveSetting->leaveWidget())
		{
			return false;
		}

		break;
	case ManagementSettingPage:
		if (!widget_Management->leaveWidget())
		{
			return false;
		}
		break;
	case TestPage:
		if (!test_widget->leaveWidget())
		{
			return false;
		}
		break;
	case CountPage:
		if (!widget_count->leaveWidget())
		{
			return false;
		}
		break;
	case PLCPage:
		if (!widget_PLC->leaveWidget())
		{
			return false;
		}
		break;
	case AlgPage:
		s_Status  sReturnStatus;
		sReturnStatus = m_cBottleModel.CloseModelDlg();

		if (0 != sReturnStatus.nErrorID)
		{
//   			QString str = QString(QLatin1String(sReturnStatus.chErrorInfo));
//   			QMessageBox::information(this,tr("Tips"),str);
			return false;
		}
		break;
	case DeBugPage:
		if (!widget_Debug->leaveWidget())
		{
			return false;
		}
		break;
	}
	return true;
}

void GlasswareDetectSystem::slots_OnBtnStar()
{
	if (m_sSystemInfo.m_bIsTest)
	{
		QMessageBox::information(this,tr("Infomation"),tr("Please Stop Test First!"));
		return;
	}
	ToolButton *TBtn = title_widget->button_list.at(12);

	if (!m_sRunningInfo.m_bCheck )//开始检测
	{
		//测试图像应力检测配置是否正确
		for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
		{
			if (2 == m_sCarvedCamInfo[i].m_iStress)
			{
				int iNormalCamera = m_sCarvedCamInfo[i].m_iToNormalCamera;
				if (iNormalCamera>m_sSystemInfo.iCamCount)
				{
					QMessageBox::information(this,tr("Error"),tr("Stress camera%1 Error,camera%2 is not a available camera!").arg(i+1).arg(iNormalCamera+1));
					return;
				}
				if (m_sCarvedCamInfo[iNormalCamera].m_iStress > 1)
				{
					QMessageBox::information(this,tr("Error"),tr("Stress camera%1 Error,camera%2 is not a normal camera!").arg(i+1).arg(iNormalCamera+1));
					return;
				}
			}
			if (0 == m_sCarvedCamInfo[i].m_iStress)
			{
				int iStressCamera = m_sCarvedCamInfo[i].m_iToStressCamera;
				if (iStressCamera>m_sSystemInfo.iCamCount)
				{
					QMessageBox::information(this,tr("Error"),tr("Stress camera%1 Error,camera%2 is not a available camera!").arg(i+1).arg(iStressCamera+1));
					return;
				}

				if (m_sCarvedCamInfo[iStressCamera].m_iStress < 1)
				{
					QMessageBox::information(this,tr("Error"),tr("camera%1 Error,camera%2 is not a Stress camera!").arg(i+1).arg(iStressCamera+1));
					return;
				}
			}
		}

		//图像综合清零
		m_cCombine.m_MutexCombin.lock();
		m_cCombine.RemovAllResult();
		m_cCombine.RemovAllError();
		m_cCombine.m_MutexCombin.unlock();

		m_cCombine1.m_MutexCombin.lock();
		m_cCombine1.RemovAllResult();
		m_cCombine1.RemovAllError();
		m_cCombine1.m_MutexCombin.unlock();

		if (m_sSystemInfo.m_bLoadModel)
		{
			//// 使能接口卡
			if (m_sSystemInfo.m_bIsIOCardOK)
			{
				for (int i = 0; i< m_sSystemInfo.iIOCardCount;i++)
				{
					pMainFrm->Logfile.write(QString(tr("OpenIOCard%1")).arg(i),OperationLog,0);
					m_vIOCard[i]->enable(true);
				}
			}
			for (int i = 0;i<=m_sSystemInfo.iRealCamCount;i++)
			{
				m_sRealCamInfo[i].m_iImageIdxLast = 0;
			}
			pMainFrm->Logfile.write(tr("Start Check"),OperationLog);
			for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
			{
				s_SystemInfoforAlg sSystemInfoforAlg;
				sSystemInfoforAlg.bIsChecking = true;
				m_cBottleCheck[i].setsSystemInfo(sSystemInfoforAlg);
			}

			m_sRunningInfo.m_bCheck = true;
		}
		else
		{
			QMessageBox::information(this,tr("Error"),tr("No Model,Please Load Model!"));
			return;
		}
		QPixmap pixmap(":/toolWidget/stop");
		TBtn->setText(tr("Stop detect"));
		TBtn->setIcon(pixmap);
		TBtn->bStatus = true;
// 		if (m_sSystemInfo.m_bIsStopNeedPermission && 0 == m_sRunningInfo.m_iPermission )
// 		{
// 			TBtn->setDisabled(true);
// 		}
		if (!sPermission.iStartStop)
		{
			TBtn->setDisabled(true);
		}
		else
		{
			TBtn->setDisabled(false);
		}
	}

	else if (m_sRunningInfo.m_bCheck)//停止检测
	{
		if (m_sSystemInfo.m_bIsIOCardOK)
		{
			for (int i = 0; i< m_sSystemInfo.iIOCardCount;i++)
			{
				pMainFrm->Logfile.write(QString(tr("CloseIOCard%1")).arg(i),OperationLog,0);
				m_vIOCard[i]->enable(false);

			}
		}
		// 停止算法检测 
		m_sRunningInfo.m_bCheck = false;
		pMainFrm->Logfile.write(tr("Stop Check"),OperationLog);

		QPixmap pixmap(":/toolWidget/start");
		TBtn->setText(tr("Start detect"));
		TBtn->setIcon(pixmap);

		for (int i = 0;i<m_sSystemInfo.iCamCount;i++)
		{
			s_SystemInfoforAlg sSystemInfoforAlg;
			sSystemInfoforAlg.bIsChecking = false;
			m_cBottleCheck[i].setsSystemInfo(sSystemInfoforAlg);
		}
		TBtn->bStatus = false;
	}

}

void GlasswareDetectSystem::paintEvent(QPaintEvent *event)
{
	QWidget::paintEvent(event);
	QPainter painter(this);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::lightGray);
	painter.drawPixmap(QRect(0, 0, this->width(), this->height()), QPixmap(skin));
	//painter.drawPixmap(QRect(0, 0, this->width(), this->height()), QPixmap(skin_name));
}
//弹出提示信息对话框
void GlasswareDetectSystem::slots_MessageBoxMainThread(s_MSGBoxInfo msgbox)
{
	QMessageBox::information(this,msgbox.strMsgtitle,msgbox.strMsgInfo);	
}
//释放IO卡
void  GlasswareDetectSystem::ReleaseIOCard()
{
	pMainFrm->Logfile.write(tr("CloseIOCard"),OperationLog);

	for (int i=0;i<m_sSystemInfo.iIOCardCount;i++)
	{
		if (m_sSystemInfo.m_bIsIOCardOK)
		{
			m_vIOCard[i]->CloseIOCard();
		}
		delete m_vIOCard[i];
	}
}
// 关闭相机 [11/11/2010 zhaodt]
void GlasswareDetectSystem::CloseCam()
{
	pMainFrm->Logfile.write(tr("CloseCam"),OperationLog);

	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		if (m_sRealCamInfo[i].m_bCameraInitSuccess && m_sRealCamInfo[i].m_bGrabIsStart) 
		{
			m_sRealCamInfo[i].m_pGrabber->StopGrab();
			m_sRealCamInfo[i].m_bGrabIsStart=FALSE;// 是否开始采集状态
		}
	}	
	Sleep(1000);
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		if (m_sRealCamInfo[i].m_bCameraInitSuccess)
		{
		}

		if (m_sRealCamInfo[i].m_pGrabber!=NULL)
		{
			m_sRealCamInfo[i].m_pGrabber->Close();
			//qDebug("---相机-%d--Close--\n",i);
			//delete m_sRealCamInfo[i].m_pGrabber;
		}
	}
}
//释放图像资源
void GlasswareDetectSystem::ReleaseImage()
{
	pMainFrm->Logfile.write(tr("ReleaseImage"),OperationLog);

	for(int i = 0 ; i < m_sSystemInfo.iRealCamCount; i++)
	{
		delete m_sRealCamInfo[i].m_pRealImage;
		m_sRealCamInfo[i].m_pRealImage = NULL;
	}
	for(int i = 0 ; i < m_sSystemInfo.iCamCount; i++)
	{
		delete m_sCarvedCamInfo[i].m_pActiveImage;
		m_sCarvedCamInfo[i].m_pActiveImage = NULL;

		delete[] m_sCarvedCamInfo[i].m_pGrabTemp;
		m_sCarvedCamInfo[i].m_pGrabTemp = NULL;
		m_queue[i].releaseMemory();
		if (m_detectElement[i].bIsImageNormalCompelet)
		{
			delete m_detectElement[i].ImageNormal->myImage;
		}
		if (m_detectElement[i].bIsImageStressCompelet)
		{
			delete m_detectElement[i].ImageStress->myImage;
		}
	}
}
//释放所有资源
void GlasswareDetectSystem::ReleaseAll()
{
	pMainFrm->Logfile.write(tr("Stop&kill check thread!"),OperationLog);

	//杀死检测线程
	if (!m_bIsThreadDead)
	{
		m_bIsThreadDead = TRUE;
	}
	if (!m_bIsIOCardThreadDead)
	{
		m_bIsIOCardThreadDead = TRUE;
	}

	//释放算法参数
	for(int i = 0; i < m_sSystemInfo.iCamCount; i++)
	{
		s_Status sReturnStatus = m_cBottleCheck[i].Free();

		// 		delete []m_sCarvedCamInfo[i].m_AlgImageLocInfos.sXldPoint.nRowsAry;
		// 		delete []m_sCarvedCamInfo[i].m_AlgImageLocInfos.sXldPoint.nColsAry;
		for (int j = 0; j < 256;j++)
		{
			delete []m_sCarvedCamInfo[i].sImageLocInfo[j].m_AlgImageLocInfos.sXldPoint.nRowsAry;
			delete []m_sCarvedCamInfo[i].sImageLocInfo[j].m_AlgImageLocInfos.sXldPoint.nColsAry;
		}

	}

	if (CherkerAry.pCheckerlist != NULL)
	{
		delete[] CherkerAry.pCheckerlist;
	}
	ReleaseImage();
}

void GlasswareDetectSystem::directoryChanged(QString path)
{
	QMessageBox::information(NULL, tr("Directory change"), path);
}

void GlasswareDetectSystem::fileChanged(QString path)
{
	QFile qss("E:/new glasswaredetect/GlasswareDetectSystem-writing/GlasswareDetectSystem/Resources/360safe.qss");
	qss.open(QFile::ReadOnly);
	qApp->setStyleSheet(qss.readAll());
	qss.close();
	QMessageBox::information(NULL, tr("qss change,Reload"), path);
}
//功能：动态切换系统语言
bool GlasswareDetectSystem::changeLanguage(int nLangIdx)
{
	QSettings sysSet("daheng","GlassDetectSystem");
	static QTranslator *translator = NULL, *qtDlgCN = NULL;
	bool bRtn = true;
	if (nLangIdx == 0)//中文
	{
		translator = new QTranslator;
		qtDlgCN = new QTranslator;
		if (translator->load("glasswaredetectsystem_zh.qm"))
		{
			qApp->installTranslator(translator);
			//中文成功后，加载Qt对话框标准翻译文件，20141202
			if (qtDlgCN->load("glasswaredetectsystem_zh.qm"))
			{
				qApp->installTranslator(qtDlgCN);
			}
			//保存设置
			sysSet.setValue("nLangIdx",nLangIdx);
//			nCurLang = nLangIdx;
		}
		else
		{
			QMessageBox::information(this,tr("Information"),tr("Load Language pack [glasswaredetectsystem_zh.qm] fail!"));
			//保存设置
			sysSet.setValue("nLangIdx",1);
			bRtn = false;
		}
	}
// 	if (nLangIdx == 1)//英文
// 	{
// //		nCurLang = nLangIdx;
// 		if (translator != NULL)
// 		{
// 			qApp->removeTranslator(translator);
// 			delete translator;
// 			translator = NULL;
// //			translateUi();
// 			//保存设置
// 			sysSet.setValue("nLangIdx",nLangIdx);
// 		}			
// 	}
	return bRtn;
}

void GlasswareDetectSystem::ShowCheckSet(int nCamIdx,int signalNumber)
{
	if (nCamIdx >= m_sSystemInfo.iCamCount)
	{
		return;
	}
	try
	{
		s_AlgModelPara  sAlgModelPara;	
		QImage tempIamge;

		if(widget_carveSetting->image_widget->bIsShowErrorImage[nCamIdx]&&pMainFrm->m_SavePicture[nCamIdx].pThat!=NULL)
		{
			tempIamge=pMainFrm->m_SavePicture[nCamIdx].m_Picture;
			sAlgModelPara.sImgLocInfo = widget_carveSetting->image_widget->sAlgImageLocInfo[nCamIdx];

		}else{
			pMainFrm->m_queue[nCamIdx].mGrabLocker.lock();
			if(pMainFrm->m_queue[nCamIdx].listGrab.size()==0)
			{
				pMainFrm->m_queue[nCamIdx].mGrabLocker.unlock();
				return;
			}
			CGrabElement *pElement = pMainFrm->m_queue[nCamIdx].listGrab.last();
			tempIamge = (*pElement->myImage);
			sAlgModelPara.sImgLocInfo = pElement->sImgLocInfo;

			pMainFrm->m_queue[nCamIdx].mGrabLocker.unlock();
		}
		

		m_cBottleModel.CloseModelDlg();
		sAlgModelPara.sImgPara.nChannel = 1;
		sAlgModelPara.sImgPara.nHeight = tempIamge.height();
		sAlgModelPara.sImgPara.nWidth = tempIamge.width();
		sAlgModelPara.sImgPara.pcData = (char*)tempIamge.bits();
		
		if (sAlgModelPara.sImgPara.nHeight != pMainFrm->m_sCarvedCamInfo[nCamIdx].m_iImageHeight)
		{
			return;
		}
		if (sAlgModelPara.sImgPara.nWidth != pMainFrm->m_sCarvedCamInfo[nCamIdx].m_iImageWidth)
		{
			return;
		}		
		
		for (int i=0;i<m_sSystemInfo.iCamCount;i++)
		{
			CherkerAry.pCheckerlist[i].nID = i;
			CherkerAry.pCheckerlist[i].pChecker = &m_cBottleCheck[i];
		}	
		int widthd = widget_alg->geometry().width();
		int heightd	= widget_alg->geometry().height();
		if (widthd < 150 || heightd < 150)
		{
			return;
		}	
		s_Status  sReturnStatus = m_cBottleModel.SetModelDlg(sAlgModelPara,&m_cBottleCheck[nCamIdx],CherkerAry,widget_alg);
		if (sReturnStatus.nErrorID != RETURN_OK)
		{
			return;
		}
		statked_widget->setCurrentWidget(widget_alg);
		m_eCurrentMainPage = AlgPage;
		m_eLastMainPage = AlgPage;
		iLastPage = 6;
	}
	catch (...)
	{
	}
	pMainFrm->Logfile.write(tr("In to Alg Page")+tr("CamraNo:%1").arg(nCamIdx),OperationLog,0);
	return;	
}
void GlasswareDetectSystem::slots_OnExit(bool ifyanz)
{
	if (ifyanz || QMessageBox::Yes == QMessageBox::question(this,tr("Exit"),
		tr("Are you sure to exit?"),
		QMessageBox::Yes | QMessageBox::No))	
	{
		if (m_sSystemInfo.m_bIsTest)
		{
			QMessageBox::information(this,tr("Infomation"),tr("Please Stop Test First!"));
			return;
		}
		if (m_sRunningInfo.m_bCheck )//开始检测
		{
			QMessageBox::information(this,tr("Infomation"),tr("Please Stop Detection First!"));
			return;		
		}
		//保存当前数据
		emit widget_count->widgetCountSet->ui.pushButton_save;


		ToolButton *TBtn = title_widget->button_list.at(12);
		pMainFrm->Logfile.write(tr("Close ModelDlg!"),OperationLog);
		s_Status  sReturnStatus = m_cBottleModel.CloseModelDlg();
		if (sReturnStatus.nErrorID != RETURN_OK)
		{
			pMainFrm->Logfile.write(tr("Error in Close ModelDlg--OnExit"),AbnormityLog);
			return;
		}
		//关闭相机	
		CloseCam();
		info_widget->timerCurve->stop();
		QSettings iniset(m_sConfigInfo.m_strDataPath,QSettings::IniFormat);
		iniset.setIniCodec(QTextCodec::codecForName("GBK"));
		iniset.setValue("/system/failureNum",m_sRunningInfo.m_failureNumFromIOcard);
		iniset.setValue("/system/checkedNum",m_sRunningInfo.m_checkedNum);
		widget_count->slots_SaveCountInfo();
		//释放接口卡
		ReleaseIOCard();
		if (m_sRunningInfo.m_bCheck)//停止检测
		{
			//停止算法检测 
			m_sRunningInfo.m_bCheck = false;
			pMainFrm->Logfile.write(tr("Stop Check"),OperationLog);
			QPixmap pixmap(":/toolWidget/start");
			TBtn->setText(tr("Start detect"));
			TBtn->setIcon(pixmap);
			TBtn->mouse_press = false;
		}
		pMainFrm->Logfile.write(tr("Exit system!"),OperationLog);
		closeWidget();
	}
}
void GlasswareDetectSystem::writeLogText(QString string,e_SaveLogType eSaveLogType)
{
	emit signals_writeLogText(string, eSaveLogType);
}
int GlasswareDetectSystem::ReadImageSignal(int nImageNum)
{
	if(0 == pMainFrm->m_sSystemInfo.m_iSystemType)//瓶身
	{
		int Position = pMainFrm->m_sRealCamInfo[nImageNum].m_iGrabPosition-1;

		switch(Position) 
		{
		case 0:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(28);
			break;
		case 1:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(29);
			break;
		case 2:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(30);
			break;
		case 3:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(31);
			break;
		case 4:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(32);
			break;
		case 5:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(33);
			break;
		default:
			pMainFrm->Logfile.write(tr("NO Position%1").arg(Position),AbnormityLog);
			return 0;
		}
	}
	else if (2 == pMainFrm->m_sSystemInfo.m_iSystemType || 6 == pMainFrm->m_sSystemInfo.m_iSystemType)//一体机
	{
		int Position = pMainFrm->m_sRealCamInfo[nImageNum].m_iGrabPosition-1;
		switch(Position) {
		case 0:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(28);
			break;
		case 1:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(34);
			break;
		case 2:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(32);
			break;
		case 3:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(28);
			break;
		case 4:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(29);
			break;
		case 8:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(30);
			break;
		case 5:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(29);
			break;
		case 6:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(30);
			break;
		case 7:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(33);
			break;
		default:
			pMainFrm->Logfile.write(tr("NO Position%1").arg(Position),AbnormityLog);
			return 0;
		}
	}
	else if (3 == pMainFrm->m_sSystemInfo.m_iSystemType)//一体机
	{
		int Position = pMainFrm->m_sRealCamInfo[nImageNum].m_iGrabPosition-1;
		switch(Position) {
		case 0:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(28);
			break;
		case 1:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(34);
			break;
		case 2:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(28);
			break;
		case 3:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(29);
			break;
		case 4:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(29);
			break;
		case 5:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(30);
			break;
		default:
			pMainFrm->Logfile.write(tr("NO Position%1").arg(Position),AbnormityLog);
			return 0;
		}
	}

	else if (4 == pMainFrm->m_sSystemInfo.m_iSystemType)
	{
		int Position = pMainFrm->m_sRealCamInfo[nImageNum].m_iGrabPosition-1;
		switch(Position) {
		case 0:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(28);
			break;
		case 1:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(34);
			break;
		case 2:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(32);
			break;
		case 3:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(29);
			break;
		case 4:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(30);
			break;
		case 5:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(33);
			break;
		default:
			pMainFrm->Logfile.write(tr("NO Position%1").arg(Position),AbnormityLog);
			return 0;
		}
	}
	else if (5 == pMainFrm->m_sSystemInfo.m_iSystemType)//药玻一体机
	{
		int Position = pMainFrm->m_sRealCamInfo[nImageNum].m_iGrabPosition-1;
		switch(Position) {
		case 0:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(28);
			break;
		case 1:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(29);
			break;
		case 2:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(30);
			break;
		case 3:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(31);
			break;
		case 4:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(32);
			break;
		case 5:
			return pMainFrm->m_vIOCard[0]->ReadImageSignal(33);
			break;
		case 6:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(28);
			break;
		case 7:
			return pMainFrm->m_vIOCard[1]->ReadImageSignal(29);
			break;
		default:
			pMainFrm->Logfile.write(tr("NO Position%1").arg(Position),AbnormityLog);
			return 0;
		}	
	}
	else{
		pMainFrm->Logfile.write(tr("NO system type%1").arg(pMainFrm->m_sSystemInfo.m_iSystemType),AbnormityLog);
		return 0;
	}
	return 1;
}

void GlasswareDetectSystem::InitCamImage(int iCameraNo)
{
	// 	pMainFrm->Logfile.write(tr("获取最大图像信息！"),OperationLog);
	//获取最大图像信息
	for (int i=0;i<m_sSystemInfo.iRealCamCount;i++)
	{
		if (i==0)
		{
			m_sSystemInfo.m_iMaxCameraImageWidth     = m_sRealCamInfo[i].m_iImageWidth;
			m_sSystemInfo.m_iMaxCameraImageHeight    = m_sRealCamInfo[i].m_iImageHeight;
			m_sSystemInfo.m_iMaxCameraImageSize      = m_sRealCamInfo[i].m_iImageSize;
			m_sSystemInfo.m_iMaxCameraImagePixelSize = (m_sRealCamInfo[i].m_iImageBitCount+7)/8;
		}
		else
		{
			if (m_sRealCamInfo[i].m_iImageWidth > m_sSystemInfo.m_iMaxCameraImageWidth)
			{
				m_sSystemInfo.m_iMaxCameraImageWidth = m_sRealCamInfo[i].m_iImageWidth;
			}				
			if (m_sRealCamInfo[i].m_iImageHeight > m_sSystemInfo.m_iMaxCameraImageHeight)
			{
				m_sSystemInfo.m_iMaxCameraImageHeight = m_sRealCamInfo[i].m_iImageHeight;
			}				
			if (((m_sRealCamInfo[i].m_iImageBitCount+7)/8) > m_sSystemInfo.m_iMaxCameraImagePixelSize)
			{
				m_sSystemInfo.m_iMaxCameraImagePixelSize = ((m_sRealCamInfo[i].m_iImageBitCount+7)/8);
			}			
		}
		m_sSystemInfo.m_iMaxCameraImageSize = m_sSystemInfo.m_iMaxCameraImageWidth*m_sSystemInfo.m_iMaxCameraImageHeight;
	}

	// 	QSettings iniset(m_sConfigInfo.m_strConfigPath,QSettings::IniFormat);
	//读取切割信息,因为要用到初始化以后的相机信息，所以不能提前读取。
	// 	pMainFrm->Logfile.write(tr("读取切割信息！"),OperationLog);
	ReadCorveConfig();

	int iCarvedCamNum = 0;
	QString strSession;
	for(int i = 0 ; i < m_sSystemInfo.iRealCamCount; i++)
	{
		for(int j = 0; j < m_sRealCamInfo[i].m_sCorves.i_ImageCount; j++)
		{
			iCarvedCamNum = m_sRealCamInfo[i].m_sCorves.i_GrabSN[j];
			if (iCameraNo ==iCarvedCamNum)
			{
				m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount = m_sRealCamInfo[i].m_iImageBitCount;   //图像位数从相机处继承[8/7/2013 nanjc]
				m_sCarvedCamInfo[iCarvedCamNum].m_iImageRoAngle = m_sRealCamInfo[i].m_iImageRoAngle;
				// 错误统计用类
				m_sRunningInfo.m_cErrorTypeInfo[iCarvedCamNum].m_iErrorTypeCount = m_sErrorInfo.m_iErrorTypeCount;

				//实时显示用, 预分配QImage空间，每切出相机一个
				if (m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage!=NULL)
				{
					delete m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage;
					m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage = NULL;
				}
				if (90 == m_sCarvedCamInfo[iCarvedCamNum].m_iImageRoAngle || 270 ==m_sCarvedCamInfo[iCarvedCamNum].m_iImageRoAngle)
				{
					m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage=new QImage(m_sCarvedCamInfo[iCarvedCamNum].m_iImageHeight,m_sCarvedCamInfo[iCarvedCamNum].m_iImageWidth,m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount);// 用于实时显示
				}
				else
				{
					m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage=new QImage(m_sCarvedCamInfo[iCarvedCamNum].m_iImageWidth,m_sCarvedCamInfo[iCarvedCamNum].m_iImageHeight,m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount);// 用于实时显示
				}
				m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage->setColorTable(m_vcolorTable);
				// 			//开始采集前补一张黑图
				BYTE* pByte = m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage->bits();
				int iLength = m_sCarvedCamInfo[iCarvedCamNum].m_pActiveImage->byteCount();
				memset((pByte),0,(iLength));
				//分配图像剪切内存区域,大小等于真实相机大小
				if (m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp!=NULL)
				{
					delete m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp; 
					m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp = NULL;
				}
				m_sCarvedCamInfo[iCarvedCamNum].m_pGrabTemp = new BYTE[m_sRealCamInfo[i].m_iImageWidth*m_sRealCamInfo[i].m_iImageHeight];
				//分配元素链表中图像的内存，每剪切出来的相机10个。
				// 			pMainFrm->Logfile.write(tr("分配元素链表中图像的内存！%1").arg(iCarvedCamNum),OperationLog);
				m_queue[iCarvedCamNum].mDetectLocker.lock();
				m_queue[iCarvedCamNum].mGrabLocker.lock();
				m_queue[iCarvedCamNum].InitQueue(m_sCarvedCamInfo[iCarvedCamNum].m_iImageWidth, m_sCarvedCamInfo[iCarvedCamNum].m_iImageHeight,m_sCarvedCamInfo[iCarvedCamNum].m_iImageBitCount, 30, true);
				m_queue[iCarvedCamNum].mGrabLocker.unlock();
				m_queue[iCarvedCamNum].mDetectLocker.unlock();

			}
		}
	}
	SetCarvedCamInfo();
	//初始化缺陷图像列表
	// 	pMainFrm->Logfile.write(tr("初始化缺陷图像列表！"),OperationLog);
//	m_ErrorList.initErrorList(m_sSystemInfo.m_iMaxCameraImageWidth,m_sSystemInfo.m_iMaxCameraImageHeight,m_sSystemInfo.m_iMaxCameraImagePixelSize*8,ERROR_IMAGE_COUNT,true);
}
bool GlasswareDetectSystem::RoAngle(uchar* pRes,uchar* pTar,int iResWidth,int iResHeight,int iAngle)
{
	int iTarWidth;
	int iTarHeight;
	if(pRes == NULL || iResWidth == 0 || iResHeight == 0)
	{
		return FALSE;
	}
	//	BYTE* pImgBuff = new BYTE[iResWidth*iResHeight*8];
	if (iAngle == 90)
	{
		iTarWidth = iResHeight;
		iTarHeight = iResWidth;
		for (int i=0;i<iResHeight;i++)
		{
			for (int j=0;j<iResWidth;j++) 
			{
				*(pTar+j*iTarWidth+(iTarWidth-i-1)) = *(pRes+i*iResWidth+j);
			}
		}
	}
	if (iAngle == 270)
	{	
		iTarWidth = iResHeight;
		iTarHeight = iResWidth;
		for (int i=0;i<iResHeight;i++)
		{
			for (int j=0;j<iResWidth;j++) 
			{
				*(pTar+(iTarHeight-j-1)*iTarWidth+i) = *(pRes+i*iResWidth+j);
			}
		}
	}
	if (iAngle == 180)
	{
		iTarWidth = iResWidth;
		iTarHeight = iResHeight;
		for (int i=0;i<iResHeight;i++)
		{
			for (int j=0;j<iResWidth;j++) 
			{
				*(pTar+(iTarHeight-i-1)*iTarWidth+(iTarWidth-j-1))=*(pRes+i*iResWidth+j);
			}
		}
	}
	return TRUE;
}
void GlasswareDetectSystem::mouseMoveEvent(QMouseEvent *event)
{
	time(&info_widget->tLastOperaTime);
}
//鼠标点击事件
void GlasswareDetectSystem::mousePressEvent(QMouseEvent *event)
{  
	time(&info_widget->tLastOperaTime);
}

void GlasswareDetectSystem::InitLastData()
{
	QSettings LoadLastData(SaveDataPath,QSettings::IniFormat);
	LoadLastData.setIniCodec(QTextCodec::codecForName("GBK"));
	QString strSession;
	for (int j = 1;j<=pMainFrm->m_sErrorInfo.m_iErrorTypeCount;j++)
	{
		for (int i = 0;i<pMainFrm->m_sSystemInfo.iCamCount;i++)
		{
			strSession = QString("DefaultTypeCount/EveryRow%1").arg(i);
			int xRowTemp=LoadLastData.value(strSession,0).toInt();

			strSession = QString("DefaultTypeCount/EveryLine%1").arg(j);
			int yLineTemp=LoadLastData.value(strSession,0).toInt();

			strSession = QString("DefaultTypeCount/EveryNumber%1%2").arg(xRowTemp).arg(yLineTemp);
			m_sRunningInfo.m_cErrorTypeInfo[xRowTemp].iErrorCountByType[yLineTemp]=LoadLastData.value(strSession,0).toInt();
			m_sRunningInfo.m_iErrorTypeCount[j]+=m_sRunningInfo.m_cErrorTypeInfo[xRowTemp].iErrorCountByType[yLineTemp];
			m_sRunningInfo.m_iErrorCamCount[i]+=m_sRunningInfo.m_cErrorTypeInfo[xRowTemp].iErrorCountByType[yLineTemp];
		}
	}
	strSession=QString("HeadCount/Checknumber");
	m_sRunningInfo.m_checkedNum=LoadLastData.value(strSession,0).toInt();

	strSession=QString("HeadCount/Failurenumber");
	m_sRunningInfo.m_failureNumFromIOcard=LoadLastData.value(strSession,0).toInt();

	strSession=QString("HeadCount/minrate");
	MinRate=LoadLastData.value(strSession,0).toDouble();

	strSession=QString("HeadCount/maxrate");
	MaxRate=LoadLastData.value(strSession,0).toDouble();
	
}
#ifdef JIAMI_INITIA
void GlasswareDetectSystem::MonitorLicense()
{
	QString  g_UidChar = "06a6914a-d863-43e1-800e-7e2eece22fd7";
	ver_code uucode;
	m_ProgramLicense.GetVerCode(&uucode);
	QString strCode = QString("%1-%2-%3-%4%5-%6%7%8%9%10%11")
		.arg(uucode.Data1,8,16,QChar('0')).arg(uucode.Data2,4,16,QChar('0')).arg(uucode.Data3,4,16,QChar('0'))
		.arg((int)uucode.Data4[0],2,16,QChar('0')).arg((int)uucode.Data4[1],2,16,QChar('0'))
		.arg((int)uucode.Data4[2],2,16,QChar('0')).arg((int)uucode.Data4[3],2,16,QChar('0'))
		.arg((int)uucode.Data4[4],2,16,QChar('0')).arg((int)uucode.Data4[5],2,16,QChar('0'))
		.arg((int)uucode.Data4[6],2,16,QChar('0')).arg((int)uucode.Data4[7],2,16,QChar('0'));
	if (g_UidChar == strCode)
	{
		//验证License
		s_KeyVerfResult res = m_ProgramLicense.CheckLicenseValid(true);
		if (res.nError <= 0)
		{
			//int m_nLicenseDays = res.nDays;
			int m_nLicenseDays = m_ProgramLicense.ReadHardwareID("getexpdate");
			if (m_nLicenseDays<=10 && m_nLicenseDays>0)
			{
				//弹出提示框
				showAllert();
			}
			//更新剩余时间
			surplusDays = m_nLicenseDays;
		}else{
			m_sSystemInfo.m_bIsTest = false;
			m_sRunningInfo.m_bCheck = true;
			slots_OnBtnStar();
			slots_OnExit(true);
		}
	}else{
		QMessageBox::information(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("加密验证失败,即将退出程序!"));
		m_sSystemInfo.m_bIsTest = false;
		m_sRunningInfo.m_bCheck = true;
		slots_OnBtnStar();
		slots_OnExit(true);
	}
}
void GlasswareDetectSystem::showAllert()
{
	pushLicense* m_tempLicense=new pushLicense; 
	m_tempLicense->slots_ShowWarning(0,QString::fromLocal8Bit("设备使用授权即将到期\n请联系商务人员！"));
	m_tempLicense->MaxNumber->start();
}
bool GlasswareDetectSystem::CheckLicense()
{
	QString  g_UidChar = "06a6914a-d863-43e1-800e-7e2eece22fd7";
	ver_code uucode;
	m_ProgramLicense.GetVerCode(&uucode);
	QString strCode = QString("%1-%2-%3-%4%5-%6%7%8%9%10%11")
		.arg(uucode.Data1,8,16,QChar('0')).arg(uucode.Data2,4,16,QChar('0')).arg(uucode.Data3,4,16,QChar('0'))
		.arg((int)uucode.Data4[0],2,16,QChar('0')).arg((int)uucode.Data4[1],2,16,QChar('0'))
		.arg((int)uucode.Data4[2],2,16,QChar('0')).arg((int)uucode.Data4[3],2,16,QChar('0'))
		.arg((int)uucode.Data4[4],2,16,QChar('0')).arg((int)uucode.Data4[5],2,16,QChar('0'))
		.arg((int)uucode.Data4[6],2,16,QChar('0')).arg((int)uucode.Data4[7],2,16,QChar('0'));
	if (g_UidChar == strCode)
	{
		//验证License
		s_KeyVerfResult res = m_ProgramLicense.CheckLicenseValid(true);
		if (res.nError <= 0)//未超时
		{
			int nDogValue = (int)m_ProgramLicense.ReadDog();
			if (nDogValue == 0)
			{
				//读取加密狗参数异常 代码：22
				QMessageBox::information(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("License过期或加密狗异常！错误代码：22"));
				return false;
			}
			int m_nLicenseDays = m_ProgramLicense.ReadHardwareID("getexpdate");
			//m_nLicenseDays = res.nDays;
			surplusDays = m_nLicenseDays;
			if (m_nLicenseDays<=10 && m_nLicenseDays>0)
			{
				showAllert();
			}
			//传递窗口句柄
			m_ProgramLicense.SetMainWnd((HWND)this->winId());
			return true; 
		}
		else
		{
			QMessageBox::information(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("License过期或加密狗异常！错误代码：%1").arg(res.nError));
			return false;
		}
	}
	else
	{
		QMessageBox::information(this,QString::fromLocal8Bit("错误"),QString::fromLocal8Bit("加密验证失败！"));
		return false;
	}
	return true;
}
#else
void GlasswareDetectSystem::MonitorLicense()
{
}
bool GlasswareDetectSystem::CheckLicense()
{
	return TRUE;
}
#endif // JIAMI_INITIA


