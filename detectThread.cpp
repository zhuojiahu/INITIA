#include "DetectThread.h"
#include <QMessageBox>
#include "glasswaredetectsystem.h"
#include <math.h>
#include <QMessageBox>
extern GlasswareDetectSystem *pMainFrm;
DetectThread::DetectThread(QObject *parent)
	: QThread(parent),tempOri()
{
	m_bStopThread = false;
	ThreadNumber = -1;
	dirSaveImagePath = new QDir;
	iMaxErrorType = 0;
	iMaxErrorArea = 0;
	iErrorType = 0;
	for (int i = 0; i<pMainFrm->m_sSystemInfo.iCamCount; i++)
	{
		isShowImage[i] = true;
	}
	for (int i=0;i<256;i++)
	{
		pMainFrm->bIsGetSample[i] = false;
	}
}

DetectThread::~DetectThread()
{

}

void DetectThread::run()
{
	while (!pMainFrm->m_bIsThreadDead)
	{
		if(m_bStopThread || (2 == pMainFrm->m_sCarvedCamInfo[ThreadNumber].m_iStress))
		{
			break;
		}
		if (pMainFrm->m_queue[ThreadNumber].listDetect.length()>0)
		{
			pMainFrm->m_queue[ThreadNumber].mDetectLocker.lock();
			CDetectElement DetectElement = pMainFrm->m_queue[ThreadNumber].listDetect.first();
			pMainFrm->m_queue[ThreadNumber].listDetect.removeFirst();
			pMainFrm->m_queue[ThreadNumber].mDetectLocker.unlock();
			iCamera = DetectElement.iCameraNormal;
			CountRuningData(iCamera);
			DetectNormal(DetectElement.ImageNormal);

			if (pMainFrm->m_queue[iCamera].InitID == DetectElement.ImageNormal->initID)
			{ 
				pMainFrm->m_queue[iCamera].mGrabLocker.lock();
				pMainFrm->m_queue[iCamera].listGrab.push_back(DetectElement.ImageNormal);
				pMainFrm->m_queue[iCamera].mGrabLocker.unlock();
			}
			else
			{
				//pMainFrm->Logfile.write(tr("delete a element!"),AbnormityLog);
				delete DetectElement.ImageNormal->myImage;
				DetectElement.ImageNormal = NULL;
				delete DetectElement.ImageNormal;
			}

			if (0 == DetectElement.iType)
			{
				iStressCamera = DetectElement.iCameraStress;
				//Sleep(2);
				DetectStress(DetectElement.ImageStress);
				if (pMainFrm->m_queue[iStressCamera].InitID == DetectElement.ImageStress->initID)
				{
					pMainFrm->m_queue[iStressCamera].mGrabLocker.lock();
					pMainFrm->m_queue[iStressCamera].listGrab.push_back(DetectElement.ImageStress);
					pMainFrm->m_queue[iStressCamera].mGrabLocker.unlock();
				}
				else
				{
					//pMainFrm->Logfile.write(tr("delete a element!"),AbnormityLog);
					delete DetectElement.ImageStress->myImage;
					DetectElement.ImageStress->myImage = NULL;
					delete DetectElement.ImageStress;
				}
			}
		}
		Sleep(1);
	}
	delete dirSaveImagePath;
}
void DetectThread::DetectNormal(CGrabElement *pElement)
{
	checkTimecost.StartSpeedTest();

	int iRealCamera = pMainFrm->m_sCarvedCamInfo[iCamera].m_iToRealCamera;
	bCheckResult[iCamera] = false;
	iErrorType = 0;
	iMaxErrorType = 0;
	iMaxErrorArea = 0;
	pElement->cErrorRectList.clear();
	try
	{
		rotateImage(pElement);
		if (pMainFrm->m_sRunningInfo.m_bCheck)
		{
			try
			{
				checkImage(pElement, 1);
				bool bOK = getCheckResult(pElement);
				if (!bOK)
				{
					return;
				}
			}
			catch (...)
			{
			}
		}
	}
	catch(...)
	{
	}
	try
	{
		kickOutBad(pElement->nSignalNo);
		saveImage(pElement);
		//将错误图像加入错误链表
		if (bCheckResult[iCamera])
		{
			addErrorImageList(pElement);
		}
	}
	catch (...)
	{
	}
	checkTimecost.StopSpeedTest();
	pElement->dCostTime = checkTimecost.dfTime;

	//刷新图像和状态
	if (pMainFrm->m_queue[iCamera].InitID == pElement->initID)
	{
		if(sReturnStatus.nErrorID != 10)
		{
			upDateState(pElement->myImage,pElement->nSignalNo,pElement->dCostTime, pElement->nMouldID, pElement->cErrorRectList, pElement->initID);
		}
	}
	pElement = NULL;
}
void DetectThread::DetectStress(CGrabElement *pElement)
{
	checkTimecost.StartSpeedTest();
	iCamera = iStressCamera;

	int iRealCamera = pMainFrm->m_sCarvedCamInfo[iCamera].m_iToRealCamera;
	bCheckResult[iCamera] = false;
	iErrorType = 0;
	iMaxErrorType = 0;
	iMaxErrorArea = 0;
	pElement->cErrorRectList.clear();

	try
	{
		rotateImage(pElement);
		if (0 == pMainFrm->m_sCarvedCamInfo[iCamera].m_iImageType)
		{
			sAlgCInp.fParam = pMainFrm->m_sSystemInfo.fPressScale;
		}
		else if (2 == pMainFrm->m_sCarvedCamInfo[iCamera].m_iImageType)
		{
			sAlgCInp.fParam = pMainFrm->m_sSystemInfo.fBasePressScale;
		}

		pMainFrm->m_cBottleStress[iCamera].Check(sAlgCInp, &pAlgCheckResult);//应力增强

		if (pMainFrm->m_sRunningInfo.m_bCheck)
		{
			try
			{
				checkImage(pElement, 2);
				bool bOK = getCheckResult(pElement);
				if (!bOK)
				{
					return;
				}
			}
			catch (...)
			{
			}
		}
	}
	catch(...)
	{
	}
	try
	{
		kickOutBad(pElement->nSignalNo);
		//保存图像
		saveImage(pElement);
		//将错误图像加入错误链表
		if (bCheckResult[iCamera])
		{
			addErrorImageList(pElement);
		}
	}
	catch (...)
	{
	}
	checkTimecost.StopSpeedTest();
	pElement->dCostTime = checkTimecost.dfTime;
	//刷新图像和状态
	if (pMainFrm->m_queue[iCamera].InitID == pElement->initID)
	{
		if(sReturnStatus.nErrorID != 10)
		{
			upDateState(pElement->myImage,pElement->nSignalNo,pElement->dCostTime, pElement->nMouldID, pElement->cErrorRectList, pElement->initID);
		}
	}
	pElement = NULL;
}

void DetectThread::WaitThreadStop()
{
	if (isRunning())
	{
		if (!m_bStopThread)
		{
			m_bStopThread = true;
		}
		wait();
	}
}
//旋转图像
void DetectThread::rotateImage(CGrabElement *pElement)
{

	sAlgCInp.sInputParam.nWidth = pElement->myImage->width();
	sAlgCInp.sInputParam.nHeight = pElement->myImage->height();
	sAlgCInp.sInputParam.pcData = (char*)pElement->myImage->bits();

	int tempWidth = pElement->myImage->width();
	int tempHeight = pElement->myImage->height();

	if(pMainFrm->m_sCarvedCamInfo[iCamera].m_iImageAngle != 0)
	{
		sAlgCInp.nParam = pMainFrm->m_sCarvedCamInfo[iCamera].m_iImageAngle;
		pMainFrm->m_cBottleRotate[iCamera].Check(sAlgCInp, &pAlgCheckResult);
	}

}
//检测
void DetectThread::checkImage(CGrabElement *pElement,int iCheckMode)
{
	sAlgCInp.sInputParam.nHeight = pElement->myImage->height();
	sAlgCInp.sInputParam.nWidth = pElement->myImage->width();
	sAlgCInp.sInputParam.nChannel = (pElement->myImage->byteCount()+7)/8;
	sAlgCInp.sInputParam.pcData = (char*)pElement->myImage->bits();

	//检测有应力检测时，需要传递坐标
	if (1 == iCheckMode)
	{
		sReturnStatus = pMainFrm->m_cBottleCheck[iCamera].Check(sAlgCInp,&pAlgCheckResult);
		//pMainFrm->m_stressStatue[iCamera].lock();
		pMainFrm->m_sCarvedCamInfo[iCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sLocOri = pAlgCheckResult->sImgLocInfo.sLocOri;
		pMainFrm->m_sCarvedCamInfo[iCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sXldPoint.nCount  = pAlgCheckResult->sImgLocInfo.sXldPoint.nCount;
		
		memcpy(pMainFrm->m_sCarvedCamInfo[iCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sXldPoint.nColsAry, \
			pAlgCheckResult->sImgLocInfo.sXldPoint.nColsAry,\
			4*BOTTLEXLD_POINTNUM);														
		memcpy(pMainFrm->m_sCarvedCamInfo[iCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sXldPoint.nRowsAry, \
			pAlgCheckResult->sImgLocInfo.sXldPoint.nRowsAry,\
			4*BOTTLEXLD_POINTNUM);
		//pMainFrm->m_stressStatue[iCamera].unlock();
		pMainFrm->m_sCarvedCamInfo[iCamera].sImageLocInfo[pElement->nSignalNo].m_iHaveInfo = 1;
	}
	else if (2 == iCheckMode)
	{
		int normalCamera = pMainFrm->m_sCarvedCamInfo[iCamera].m_iToNormalCamera;
		//pMainFrm->m_stressStatue[normalCamera].lock();
		pElement->sImgLocInfo.sLocOri = pMainFrm->m_sCarvedCamInfo[normalCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sLocOri;
		pElement->sImgLocInfo.sXldPoint.nCount = pMainFrm->m_sCarvedCamInfo[normalCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sXldPoint.nCount;
		
		memcpy(pElement->sImgLocInfo.sXldPoint.nColsAry,\
			pMainFrm->m_sCarvedCamInfo[normalCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sXldPoint.nColsAry, \
			4*BOTTLEXLD_POINTNUM);							
		memcpy(pElement->sImgLocInfo.sXldPoint.nRowsAry,\
			pMainFrm->m_sCarvedCamInfo[normalCamera].sImageLocInfo[pElement->nSignalNo].m_AlgImageLocInfos.sXldPoint.nRowsAry, \
			4*BOTTLEXLD_POINTNUM);
		//pMainFrm->m_stressStatue[normalCamera].unlock();
		if(pElement->sImgLocInfo.sLocOri.modelCol == 0 || pElement->sImgLocInfo.sLocOri.modelRow == 0)
		{
			pElement->sImgLocInfo.sLocOri = tempOri;
		}else{
			tempOri = pElement->sImgLocInfo.sLocOri;
		}
		sAlgCInp.sImgLocInfo = pElement->sImgLocInfo;
		sReturnStatus = pMainFrm->m_cBottleCheck[iCamera].Check(sAlgCInp,&pAlgCheckResult);
		pMainFrm->m_sCarvedCamInfo[ pMainFrm->m_sCarvedCamInfo[iCamera].m_iToNormalCamera].sImageLocInfo[pElement->nSignalNo].m_iHaveInfo = 0;
	}
	else
	{
		sReturnStatus = pMainFrm->m_cBottleCheck[iCamera].Check(sAlgCInp,&pAlgCheckResult);
	}
}
//获取检测结果
bool DetectThread::getCheckResult(CGrabElement *pElement)
{
	if (sReturnStatus.nErrorID != 0 && sReturnStatus.nErrorID !=10)
	{
		return false;
	}

	//获取检测模点的总数据
	//GetModelDotData(pElement);
	if (pAlgCheckResult->nSizeError >0 && pMainFrm->m_sRunningInfo.m_bIsCheck[iCamera]) //有错误并且此相机未关闭检测
	{
		//连续剔瓶统计
		bCheckResult[iCamera] = true;
		//pMainFrm->m_sCarvedCamInfo[iCamera].m_iErrorCount++;
		pElement->cErrorParaList.clear(); //先清空

		for (int j=0;j<pAlgCheckResult->nSizeError;j++) //获得算法返回错误矩形区域
		{
			s_ErrorPara  sErrorPara;
			sErrorPara = pAlgCheckResult->vErrorParaAry[j];
			if(sErrorPara.nArea > iMaxErrorArea)
			{
				iMaxErrorArea = sErrorPara.nArea;
				iMaxErrorType = sErrorPara.nErrorType;
			}
			QRect rect(sErrorPara.rRctError.left,\
				sErrorPara.rRctError.top, \
				sErrorPara.rRctError.right - sErrorPara.rRctError.left, \
				sErrorPara.rRctError.bottom - sErrorPara.rRctError.top);
			if (iMaxErrorType > pMainFrm->m_sErrorInfo.m_iErrorTypeCount)
			{
				iMaxErrorType = pMainFrm->m_sErrorInfo.m_iErrorTypeCount+1;
				sErrorPara.nErrorType = pMainFrm->m_sErrorInfo.m_iErrorTypeCount+1;
			}
			//	找不到原点不踢废
			if (1 == sErrorPara.nErrorType&&(1 == pMainFrm->m_sSystemInfo.m_iNoRejectIfNoOrigin[iCamera] || 1 == pMainFrm->m_sSystemInfo.m_NoKickIfNoFind ))
			{
				bCheckResult[iCamera] = false;
			}
			// 预处理错误不踢废
			if (2 == sErrorPara.nErrorType&&(1 == pMainFrm->m_sSystemInfo.m_iNoRejectIfROIfail[iCamera] || 1 == pMainFrm->m_sSystemInfo.m_NoKickIfROIFail ))
			{
				bCheckResult[iCamera] = false;
			}			
			if(sErrorPara.nErrorType==39)
			{
				bCheckResult[iCamera] = true;
			}
			//将算法返回错误结果放入链表
			if (bCheckResult[iCamera])
			{
				pElement->cErrorRectList.append(rect);
				pElement->cErrorParaList.append(sErrorPara);
			}

			//错误类型综合
			//找不到原点不综合
			if (1 == sErrorPara.nErrorType&&(1 == pMainFrm->m_sSystemInfo.m_iNoRejectIfNoOrigin[iCamera] || 1 == pMainFrm->m_sSystemInfo.m_NoKickIfNoFind ))
			{
				;
			}
			else if (2 == sErrorPara.nErrorType&&(1 == pMainFrm->m_sSystemInfo.m_iNoRejectIfROIfail[iCamera] || 1 == pMainFrm->m_sSystemInfo.m_NoKickIfROIFail ))
			{
				;
			}
			else
			{
				if(pMainFrm->m_sCarvedCamInfo[iCamera].m_iIOCardSN == 0)
				{
					pMainFrm->m_cCombine.m_MutexCombin.lock();
					pMainFrm->m_cCombine.AddError(pElement->nSignalNo,iCamera,sErrorPara);
					pMainFrm->m_cCombine.m_MutexCombin.unlock();
				}
				else
				{
					pMainFrm->m_cCombine1.m_MutexCombin.lock();
					pMainFrm->m_cCombine1.AddError(pElement->nSignalNo,iCamera,sErrorPara);
					pMainFrm->m_cCombine1.m_MutexCombin.unlock();
				}
			}
		}	

		iErrorType = iMaxErrorType;
		//取样
		/*if (pMainFrm->m_sSampleInfo.m_bSampleType[iErrorType])
		{
			pMainFrm->mutexSetSample.lock();
			pMainFrm->bIsGetSample[pElement->nSignalNo] = true;
			pMainFrm->mutexSetSample.unlock();
		}*/
		pElement->nCheckRet = iErrorType;
	}
	else//没有错误加入
	{
		s_ErrorPara sErrorPara;
		sErrorPara.nArea = 0;
		sErrorPara.nErrorType = 0;
		if(pMainFrm->m_sCarvedCamInfo[iCamera].m_iIOCardSN == 0)
		{
			pMainFrm->m_cCombine.m_MutexCombin.lock();
			pMainFrm->m_cCombine.AddError(pElement->nSignalNo,iCamera,sErrorPara);
			pMainFrm->m_cCombine.m_MutexCombin.unlock();
		}
		else
		{
			pMainFrm->m_cCombine1.m_MutexCombin.lock();
			pMainFrm->m_cCombine1.AddError(pElement->nSignalNo,iCamera,sErrorPara);
			pMainFrm->m_cCombine1.m_MutexCombin.unlock();
		}
	}
	return true;
}
//取样,只在接口卡1取样
void DetectThread::kickOutSample(int nSignalNo, int result)
{	
	if (true == pMainFrm->bIsGetSample[nSignalNo]&&1 == result)
	{		
		if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
		{
			s_ResultInfo sResultInfo;
			sResultInfo.tmpResult = 1;
			sResultInfo.nImgNo = nSignalNo;
			sResultInfo.nIOCardNum = 0;
			pMainFrm->m_mutexmSendResult.lock();
			pMainFrm->m_vIOCard[0]->GetSample(sResultInfo);
			pMainFrm->m_mutexmSendResult.unlock(); 

			pMainFrm->mutexSetSample.lock();
			pMainFrm->bIsGetSample[nSignalNo] = false;
			if(!pMainFrm->m_sSampleInfo.bContinuousSampling)
			{
				pMainFrm->m_sSampleInfo.m_iSampleCount--;
			}
			pMainFrm->mutexSetSample.unlock();
		}
	}
	else
	{
		s_ResultInfo sResultInfo;
		sResultInfo.tmpResult = 0;
		sResultInfo.nImgNo = nSignalNo;
		sResultInfo.nIOCardNum = 0;
		if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
		{
			pMainFrm->m_mutexmSendResult.lock();
			pMainFrm->m_vIOCard[0]->GetSample(sResultInfo);
			pMainFrm->m_mutexmSendResult.unlock();
		}
		pMainFrm->bIsGetSample[nSignalNo] = false;
	}
}

void DetectThread::kickOutBad(int nSignalNo)
{
	int tmpResult=0;
	int iKickMode = pMainFrm->m_sRunningInfo.m_iKickMode;
	int iKickMode2 = pMainFrm->m_sRunningInfo.m_iKickMode2;
	if (pMainFrm->m_sCarvedCamInfo[iCamera].m_iIOCardSN == 0)
	{
		switch (iKickMode)
		{
		case 0:			// 连续踢 
			tmpResult=1;
			break;
		case 1:			// 隔瓶踢
			tmpResult=nSignalNo%2;
			break;
		case 2:			// 持续好
			tmpResult=0;
			break;
		case 3:			// 正常踢
			tmpResult=bCheckResult[iCamera];
			break;
		default:			
			break;
		}
	}
	else if(pMainFrm->m_sCarvedCamInfo[iCamera].m_iIOCardSN == 1)
	{
		switch (iKickMode2)
		{
		case 0:			// 连续踢 
			tmpResult=1;
			break;
		case 1:			// 隔瓶踢
			tmpResult=nSignalNo%2;
			break;
		case 2:			// 持续好
			tmpResult=0;
			break;
		case 3:			// 正常踢
			tmpResult=bCheckResult[iCamera];
			break;
		default:			
			break;
		}
	}
	if (!pMainFrm->m_sRunningInfo.m_bCheck)
	{
		tmpResult=0;
	}
	if(pMainFrm->m_sCarvedCamInfo[iCamera].m_iIOCardSN == 0)
	{
		CountDefectIOCard0(nSignalNo,tmpResult);
	}
	else if(pMainFrm->m_sCarvedCamInfo[iCamera].m_iIOCardSN == 1)
	{
		CountDefectIOCard1(nSignalNo,tmpResult);
	}
}
void DetectThread::CountDefectIOCard0(int nSignalNo,int tmpResult)
{
	int comResult = -1;//综合后的结果
	pMainFrm->m_cCombine.AddResult(nSignalNo,iCamera,tmpResult);
	//pMainFrm->m_cCombine.m_MutexCombin.lock();
	if (pMainFrm->m_cCombine.ConbineResult(nSignalNo,0,comResult))//图像都拍完后结果综合
	{
		//pMainFrm->m_cCombine.m_MutexCombin.unlock();
		for	(int i = nSignalNo-pMainFrm->iLastAbnormalStep; i<nSignalNo ;i++)
		{
			if (!pMainFrm->m_cCombine.IsReject((i+256)%256))
			{
				pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[2]++;
				s_ResultInfo sResultInfo;
				sResultInfo.tmpResult = pMainFrm->m_cCombine.m_Rlts[(i+256)%256].iResult;
				sResultInfo.nImgNo = (i+256)%256;
				sResultInfo.nIOCardNum = 0;
				if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
				{
					//暂时使用无用变量作为总的综合踢废数目 by zl
					pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[2] += 1;
					pMainFrm->m_vIOCard[sResultInfo.nIOCardNum]->SendResult(sResultInfo);
				}
				pMainFrm->m_cCombine.SetReject((i+256)%256);
			}
		}

		for	(int i = nSignalNo; i<nSignalNo+pMainFrm->iLastAbnormalStep;i++)
		{
			pMainFrm->m_cCombine.SetReject(i%256,false);
		}
		//暂时使用无用变量作为总的综合过检数目 by zl
		pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[0]++;
		s_ResultInfo sResultInfo;
		sResultInfo.tmpResult = comResult;
		sResultInfo.nImgNo = nSignalNo;
		sResultInfo.nIOCardNum = 0;
		if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
		{
			pMainFrm->m_vIOCard[sResultInfo.nIOCardNum]->SendResult(sResultInfo);
		}
		pMainFrm->m_cCombine.SetReject(nSignalNo);
		//缺陷统计
		pMainFrm->m_cCombine.RemoveOneResult(nSignalNo);
		if (pMainFrm->m_sRunningInfo.m_bCheck)	
		{
			int iErrorCamera = pMainFrm->m_cCombine.ErrorCamera(nSignalNo);
			s_ErrorPara sComErrorpara = pMainFrm->m_cCombine.ConbineError(nSignalNo);
			if (pMainFrm->m_sRunningInfo.m_cErrorTypeInfo[iErrorCamera].ErrorTypeJudge(sComErrorpara.nErrorType))
			{
				pMainFrm->m_sRunningInfo.m_cErrorTypeInfo[iErrorCamera].iErrorCountByType[sComErrorpara.nErrorType]+=1;
				pMainFrm->m_sRunningInfo.m_iErrorCamCount[iErrorCamera] += 1;
				//暂时使用无用变量作为总的综合踢废数目 by zl
				pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[0] += 1;//阴同添加
				pMainFrm->m_sRunningInfo.m_iErrorTypeCount[sComErrorpara.nErrorType] +=1;
				//pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[sComErrorpara.nErrorType] +=1;//阴同添加
				//增加一个判断函数，确认是否报警
				ifSendAret(sComErrorpara.nErrorType);
			}
			else
			{
				pMainFrm->m_sRunningInfo.m_cErrorTypeInfo[iErrorCamera].iErrorCountByType[0]+=1;
				pMainFrm->m_sRunningInfo.m_iErrorTypeCount[0] +=1;
			}	
		}
		//更新统计信息
		//pMainFrm->m_cCombine.m_MutexCombin.unlock();
	}
	else
	{
		//pMainFrm->m_cCombine.m_MutexCombin.unlock();
	}
}
void DetectThread::CountDefectIOCard1(int nSignalNo,int tmpResult)
{
	int comResult = -1;//综合后的结果
	pMainFrm->m_cCombine1.AddResult(nSignalNo,iCamera,tmpResult);
	//pMainFrm->m_cCombine1.m_MutexCombin.lock();
	if (pMainFrm->m_cCombine1.ConbineResult(nSignalNo,1,comResult))//图像都拍完后结果综合
	{
		//pMainFrm->m_cCombine1.m_MutexCombin.unlock();
		for	(int i = nSignalNo-pMainFrm->iLastAbnormalStep; i<nSignalNo ;i++)
		{
			if (!pMainFrm->m_cCombine1.IsReject((i+256)%256))
			{
				pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[3]++;
				s_ResultInfo sResultInfo;
				sResultInfo.tmpResult = pMainFrm->m_cCombine1.m_Rlts[(i+256)%256].iResult;
				sResultInfo.nImgNo = (i+256)%256;
				sResultInfo.nIOCardNum = 1;
				if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
				{
					pMainFrm->m_vIOCard[sResultInfo.nIOCardNum]->SendResult(sResultInfo);
					pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[3] += 1;
				}
				pMainFrm->m_cCombine1.SetReject((i+256)%256);
			}
		}
		for	(int i = nSignalNo; i< nSignalNo + pMainFrm->iLastAbnormalStep;i++)
		{
			pMainFrm->m_cCombine1.SetReject(i%256,false);
		}	
		//暂时使用无用变量作为总的综合踢废数目 by zl
		pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[1]++;
		s_ResultInfo sResultInfo;
		sResultInfo.tmpResult = comResult;
		sResultInfo.nImgNo = nSignalNo;
		sResultInfo.nIOCardNum = 1;
		if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
		{
			pMainFrm->m_vIOCard[sResultInfo.nIOCardNum]->SendResult(sResultInfo);
		}
		pMainFrm->m_cCombine1.SetReject(nSignalNo);
		//缺陷统计
		pMainFrm->m_cCombine1.RemoveOneResult(nSignalNo);
		int iErrorCamera = pMainFrm->m_cCombine1.ErrorCamera(nSignalNo);
		s_ErrorPara sComErrorpara1 = pMainFrm->m_cCombine1.ConbineError(nSignalNo);
		if (pMainFrm->m_sRunningInfo.m_cErrorTypeInfo[iErrorCamera].ErrorTypeJudge(sComErrorpara1.nErrorType))
		{
			pMainFrm->m_sRunningInfo.m_cErrorTypeInfo[iErrorCamera].iErrorCountByType[sComErrorpara1.nErrorType]+=1;
			pMainFrm->m_sRunningInfo.m_iErrorCamCount[iErrorCamera] += 1;
			//暂时使用无用变量作为总的综合踢废数目 by zl
			pMainFrm->m_sRunningInfo.nGSoap_ErrorCamCount[1] += 1;//阴同添加
			pMainFrm->m_sRunningInfo.m_iErrorTypeCount[sComErrorpara1.nErrorType] +=1;
			//pMainFrm->m_sRunningInfo.nGSoap_ErrorTypeCount[sComErrorpara1.nErrorType] +=1;//阴同添加
			ifSendAret(sComErrorpara1.nErrorType);
		}
		else
		{
			pMainFrm->m_sRunningInfo.m_cErrorTypeInfo[iErrorCamera].iErrorCountByType[0]+=1;
			pMainFrm->m_sRunningInfo.m_iErrorTypeCount[0] +=1;
		}	
		//更新统计信息
		//pMainFrm->m_cCombine1.m_MutexCombin.unlock();
	}
	else
	{
		//pMainFrm->m_cCombine1.m_MutexCombin.unlock();
	}
}
//存图
void DetectThread::saveImage(CGrabElement *pElement)
{
	//方小海修改，当检测算法检测出黑图时，直接返回
	
	if (1 == pMainFrm->m_sSystemInfo.m_iSaveNormalErrorImageByTime)
	{
		if (bCheckResult[iCamera] && pMainFrm->m_sCarvedCamInfo[iCamera].m_iStress != 2)
		{
			QDateTime time = QDateTime::currentDateTime();
			QString strSaveImagePath = QString(pMainFrm->m_sConfigInfo.m_strAppPath + tr("SaveImageByTime\\") +\
				tr("normal image\\") + time.date().toString("yyyy-MM-dd") + tr("\\camera%1").arg(iCamera+1)) + "\\" + time.time().toString("hh");
			bool exist = dirSaveImagePath->exists(strSaveImagePath);
			if(!exist)
			{
				bool ok = dirSaveImagePath->mkpath(strSaveImagePath);
				if (!ok)
				{
					pMainFrm->writeLogText(tr("Failure in create Path!"),AbnormityLog);
				}
			}
			QString strSavePath = QString(strSaveImagePath + "/image number%1_%2%3%4.bmp").arg(pElement->nSignalNo).arg(time.time().hour()).arg(time.time().minute()).arg(time.time().second());
			pElement->myImage->mirrored().save(strSavePath);
		}
	}
	if (1 == pMainFrm->m_sSystemInfo.m_iSaveStressErrorImageByTime)
	{
		if (bCheckResult[iCamera] && pMainFrm->m_sCarvedCamInfo[iCamera].m_iStress == 2)
		{
			QDateTime time = QDateTime::currentDateTime();
			QString strSaveImagePath = QString(pMainFrm->m_sConfigInfo.m_strAppPath + tr("SaveImageByTime\\") +\
				tr("stress image\\") + time.date().toString("yyyy-MM-dd") + tr("\\camera%1").arg(iCamera+1)) + "\\" + time.time().toString("hh");
			bool exist = dirSaveImagePath->exists(strSaveImagePath);
			if(!exist)
			{
				bool ok = dirSaveImagePath->mkpath(strSaveImagePath);
				if (!ok)
				{
					pMainFrm->writeLogText(tr("Failure in create Path!"),AbnormityLog);
				}
			}
			QString strSavePath = QString(strSaveImagePath + "/image number%1_%2%3%4.bmp").arg(pElement->nSignalNo).arg(time.time().hour()).arg(time.time().minute()).arg(time.time().second());
			pElement->myImage->mirrored().save(strSavePath);
		}

	}
	if (AllImage == pMainFrm->m_sRunningInfo.m_eSaveImageType || AllImageInCount == pMainFrm->m_sRunningInfo.m_eSaveImageType)
	{
		if (0 == pMainFrm->m_sSystemInfo.m_bSaveCamera[iCamera])
		{
			return;
		}
		QTime time = QTime::currentTime();
		QString strSaveImagePath = QString(pMainFrm->m_sConfigInfo.m_strAppPath + "SaveImage/All-image/camera%1/").arg(iCamera+1);
		bool exist = dirSaveImagePath->exists(strSaveImagePath);
		if(!exist)
		{
			bool ok = dirSaveImagePath->mkpath(strSaveImagePath);
			if (!ok)
			{
				pMainFrm->writeLogText(tr("Failure in create Path!"),AbnormityLog);
			}
		}
		if (AllImage == pMainFrm->m_sRunningInfo.m_eSaveImageType)
		{
			QString strSavePath = QString(strSaveImagePath + "image number%1_%2%3%4.bmp").arg(pElement->nSignalNo).arg(time.hour()).arg(time.minute()).arg(time.second());
			pElement->myImage->mirrored().save(strSavePath);
		}
		if (AllImageInCount == pMainFrm->m_sRunningInfo.m_eSaveImageType)
		{
			pMainFrm->m_sRunningInfo.m_mutexRunningInfo.lock();
			if (pMainFrm->m_sRunningInfo.m_iSaveImgCount[iCamera] > 0)
			{
				QString strSavePath = QString(strSaveImagePath + "image number%1_%2%3%4.bmp").arg(pElement->nSignalNo).arg(time.hour()).arg(time.minute()).arg(time.second());
				pElement->myImage->mirrored().save(strSavePath);
				pMainFrm->m_sRunningInfo.m_iSaveImgCount[iCamera]--;
			}
			int itempSavemode = 0;
			for (int i = 0 ; i<pMainFrm->m_sSystemInfo.iCamCount;i++)
			{
				if (pMainFrm->m_sSystemInfo.m_bSaveCamera[i] == 1)
				{
					itempSavemode = 1;
				}
			}
			if (0 == itempSavemode)
			{
				pMainFrm->m_sRunningInfo.m_eSaveImageType = NotSave;
			}
			pMainFrm->m_sRunningInfo.m_mutexRunningInfo.unlock();
		}
	}else if (FailureImage == pMainFrm->m_sRunningInfo.m_eSaveImageType||FailureImageInCount == pMainFrm->m_sRunningInfo.m_eSaveImageType)
	{
		if (0 == pMainFrm->m_sSystemInfo.m_bSaveCamera[iCamera])
		{
			return;
		}
		if (0 == pMainFrm->m_sSystemInfo.m_bSaveErrorType[iErrorType])
		{
			return;
		}
		QTime time = QTime::currentTime();
		QString strSaveImagePath = QString(pMainFrm->m_sConfigInfo.m_strAppPath + "SaveImage/Failure-image/camera%1").arg(iCamera+1);
		bool exist = dirSaveImagePath->exists(strSaveImagePath);
		if(!exist)
		{
			bool ok = dirSaveImagePath->mkpath(strSaveImagePath);
			if (!ok)
			{
				pMainFrm->writeLogText(tr("Failure in create Path!"),AbnormityLog);
			}
		}
		if (FailureImage == pMainFrm->m_sRunningInfo.m_eSaveImageType)
		{
			QString strSavePath = QString(strSaveImagePath + "/image number%1_%2%3%4.bmp").arg(pElement->nSignalNo).arg(time.hour()).arg(time.minute()).arg(time.second());
			pElement->myImage->mirrored().save(strSavePath);
		}
		if (FailureImageInCount == pMainFrm->m_sRunningInfo.m_eSaveImageType)
		{
			pMainFrm->m_sRunningInfo.m_mutexRunningInfo.lock();
			if (pMainFrm->m_sRunningInfo.m_iSaveImgCount[iCamera] > 0)
			{
				QString strSavePath = QString(strSaveImagePath + "/image number%1_%2%3%4.bmp").arg(pElement->nSignalNo).arg(time.hour()).arg(time.minute()).arg(time.second());
				pElement->myImage->mirrored().save(strSavePath);
				pMainFrm->m_sRunningInfo.m_iSaveImgCount[iCamera]--;
			}
			if (0 == pMainFrm->m_sRunningInfo.m_iSaveImgCount[iCamera])
			{
				pMainFrm->m_sSystemInfo.m_bSaveCamera[iCamera] = 0;
			}
			int itempSavemode = 0;
			for (int i = 0 ; i<pMainFrm->m_sSystemInfo.iCamCount;i++)
			{
				if (pMainFrm->m_sSystemInfo.m_bSaveCamera[i] == 1)
				{
					itempSavemode = 1;
				}
			}
			if (0 == itempSavemode)
			{
				pMainFrm->m_sRunningInfo.m_eSaveImageType = NotSave;
			}
			pMainFrm->m_sRunningInfo.m_mutexRunningInfo.unlock();
		}
	}
}
//将缺陷图像加入错误链表
void DetectThread::addErrorImageList(CGrabElement *pElement)
{
	pMainFrm->m_ErrorList.m_mutexmErrorList.lock();
	CGrabElement *pErrorElement = pMainFrm->m_ErrorList.listError.last();
	pMainFrm->m_ErrorList.listError.removeLast();
	pErrorElement->nCamSN = pElement->nCamSN;
	pErrorElement->dCostTime = pElement->dCostTime;
	pErrorElement->nCheckRet = pElement->nCheckRet;
// 	pErrorElement->nGrabImageCount = pElement->nGrabImageCount;
// 	pErrorElement->nImgSN = pElement->nImgSN;
// 	pErrorElement->nImgType = pElement->nImgType;
	pErrorElement->nSignalNo = pElement->nSignalNo; 
	pErrorElement->cErrorRectList = pElement->cErrorRectList;
	pErrorElement->cErrorParaList = pElement->cErrorParaList;

	pErrorElement->sImgLocInfo.sLocOri = pElement->sImgLocInfo.sLocOri;
	pErrorElement->sImgLocInfo.sXldPoint.nCount = pElement->sImgLocInfo.sXldPoint.nCount;
	memcpy(pErrorElement->sImgLocInfo.sXldPoint.nColsAry,\
		pElement->sImgLocInfo.sXldPoint.nColsAry,\
		4*BOTTLEXLD_POINTNUM);							
	memcpy(pErrorElement->sImgLocInfo.sXldPoint.nRowsAry,\
		pElement->sImgLocInfo.sXldPoint.nRowsAry,\
		4*BOTTLEXLD_POINTNUM);

	pErrorElement->cErrorParaList.clear();
	for (int i =0;i<pElement->cErrorParaList.length();i++)
	{
		s_ErrorPara tempErrorPara = pElement->cErrorParaList.at(i);
		pErrorElement->cErrorParaList.append(tempErrorPara);
	}
	/****************************************************************************/
	if (pErrorElement->myImage != NULL)
	{
		delete pErrorElement->myImage;
		pErrorElement->myImage = NULL;
	}
	pErrorElement->myImage = new QImage(*pElement->myImage);

	pMainFrm->m_ErrorList.listError.push_front(pErrorElement);
	pMainFrm->m_ErrorList.m_mutexmErrorList.unlock();

	emit signals_AddErrorTableView(pElement->nCamSN,pElement->nSignalNo,pElement->cErrorParaList.first().nErrorType);

}
//更新显示状态
void DetectThread::upDateState( QImage* myImage, int signalNo,double costTime, int nMouldID, QList<QRect> listErrorRectList,int QueenID)
{
	QString camera = QString::number(this->iCamera+1);
	QString imageSN = QString::number(signalNo);
	QString time = QString::number(costTime,'f',2);
	//	QString result = QString::number(iErrorType);
	QString result = pMainFrm->m_sErrorInfo.m_vstrErrorType.at(iErrorType);
	QString mouldID = QString::number(nMouldID);

	if(0 == pMainFrm->test_widget->ifshowImage)
	{
		emit signals_updateActiveImg(iCamera,signalNo,costTime,iErrorType);//更新剪切的图像显示
		emit signals_updateImage(myImage, camera, imageSN, time, result, mouldID, listErrorRectList, QueenID);
	}else if(1 == pMainFrm->test_widget->ifshowImage)
	{
		if (bCheckResult[iCamera])
		{
			emit signals_updateActiveImg(iCamera,signalNo,costTime,iErrorType);//更新剪切的图像显示
			emit signals_updateImage(myImage, camera, imageSN, time, result, mouldID, listErrorRectList, QueenID);
		}
	}
	if (!bCheckResult[iCamera])//刷新主界面相机按钮状态
	{
		//emit signals_upDateCamera(iCamera,0 );//好图不刷新
	}
	else
	{
		emit signals_upDateCamera(iCamera,1 );
	}

	for (int i = 0;i<pMainFrm->m_sSystemInfo.iCamCount;i++)
	{
		if (pMainFrm->m_sRunningInfo.m_checkedNum != 0)
		{
			pMainFrm->m_sRunningInfo.m_iErrorCamRate[i] = 1.0*pMainFrm->m_sRunningInfo.m_iErrorCamCount[i]/pMainFrm->m_sRunningInfo.m_checkedNum * 100;
		}
		else
		{
			pMainFrm->m_sRunningInfo.m_iErrorCamRate[i] = 0;
		}
	}
	emit signals_updateCameraFailureRate();

}

void DetectThread::GetModelDotData(CGrabElement *pElement)
{
	// 获取瓶底模点，20180528， by wenfan
	pElement->nMouldID = pAlgCheckResult->nMouldID;
	if (pAlgCheckResult->nMouldID>0 && pAlgCheckResult->nMouldID < 100)
	{
		pMainFrm->m_sRunningInfo.nModelReadFailureNumber++;
	}
}

void DetectThread::CountGrabRate(int cameraNumber)
{
	static DWORD lastSpeed[CAMERA_MAX_COUNT];
	static int nSpeedCount[CAMERA_MAX_COUNT];
	if(lastSpeed[cameraNumber] == 0)
	{
		lastSpeed[cameraNumber] = timeGetTime();
	}
	else
	{
		nSpeedCount[cameraNumber]++;			
		int nCurTime = timeGetTime();
		//计算瓶子传送速度 = 个数/时间（min）：：超过1s计算一次
		if (nCurTime-lastSpeed[cameraNumber] > 1000)
		{
			int nCurSpeed = nSpeedCount[cameraNumber]*1.0 / (nCurTime-lastSpeed[cameraNumber]) * 60000;
			lastSpeed[cameraNumber] = nCurTime;
			nSpeedCount[cameraNumber] = 0;
			pMainFrm->m_sRunningInfo.strSpeed = QString::number(nCurSpeed);
			emit signals_showspeed(nCurSpeed);
		}
	}
}

void DetectThread::CountRuningData( int cameraNumber )
{
	if (cameraNumber == 0)
	{
		CountGrabRate(cameraNumber);
	}
}

void DetectThread::ifSendAret(int i)
{
	i--;
	time_t t;
	t = time(NULL);
	if(pMainFrm->test_widget->iIsTrackErrorType[i])
	{
		pMainFrm->AertNumber[i].push_back(t);
	}
	int maxNumber = pMainFrm->test_widget->iStatisMaxNumber[i];
	if(pMainFrm->AertNumber[i].count() >= maxNumber && maxNumber != 0 && maxNumber!=1)
	{
		int temp_interval = t-pMainFrm->AertNumber[i].at(0);
		if(temp_interval < pMainFrm->test_widget->iStatisMaxTime[i]*60)
		{
			//发送报警信号
			temp_interval=temp_interval/60;
			if(temp_interval == 0)
			{
				temp_interval = 1;
			}
			pMainFrm->AertNumber[i].clear();
			if (pMainFrm->m_sSystemInfo.m_bIsIOCardOK)
			{
				pMainFrm->m_vIOCard[0]->m_mutexmIOCard.lock();
				pMainFrm->m_vIOCard[0]->m_Pio24b.setCardOutput(7,1);
				pMainFrm->m_vIOCard[0]->m_mutexmIOCard.unlock();
			}
			//弹出提示框
			pMainFrm->AlertFunction(i+pMainFrm->m_vstrPLCInfoType.size()+2,maxNumber,temp_interval);
			//改变控件的颜色
			pMainFrm->test_widget->ShowAssert(i);
		}
	}
}