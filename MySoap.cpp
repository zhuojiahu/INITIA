#include "MySoap.h"
#include <QMutex>
#include <QMessageBox>
#include "add.nsmap"
#include "Demo_soap.h"

struct soap m_soap;//soap服务器对象
QMutex csSoapData;//SOAP数据访问互斥
MySoap* pContainer;//容器指针

MySoap::MySoap(void)
{
	SOAP_FMAC1 void SOAP_FMAC2 soap_set_recv_logfile(struct soap*, const char*);
	SOAP_FMAC1 void SOAP_FMAC2 soap_set_sent_logfile(struct soap*, const char*);
}

MySoap::MySoap(int nPort,SOAPCALLBACK pCallBackFunc)
{
	m_bIsOpen = false;
	pContainer = this;
	m_nSoapPort = nPort;
	m_pCallBackFunc = pCallBackFunc;
}

bool MySoap::OpenSoap()
{
	if(InitSoapServer())
	{
		//开启SOAPServer线程
		m_SoapServerThread.m_bExit = true;
		m_SoapServerThread.start(QThread::NormalPriority);
		m_bIsOpen = true;
	}
	else
	{
		return false;
	}
	return true;
}

bool MySoap::CloseSoap()
{
	if (m_SoapServerThread.isRunning())
	{
		m_SoapServerThread.DetectThreadClosed();
		m_bIsOpen = false;
	}
	return true;
}

MySoap::~MySoap(void)
{
	CloseSoap();
}

bool MySoap::IsOpen()
{
	return m_bIsOpen;
}

bool MySoap::InitSoapServer()
{
	int m; /* masterockets */
	soap_init(&m_soap);
	setlocale(LC_ALL, "");
	soap_set_mode((soap*)&m_soap,SOAP_C_MBSTRING);
	QString sPort = QString("%1").arg(m_nSoapPort);
	QByteArray ba = sPort.toLatin1();
	char *cstr = ba.data();
	m = soap_bind(&m_soap, NULL, atoi(cstr), 100);
	if (m < 0)
	{
		return false;
	}
	return true;
}

void MySoap::SoapServerThread::run()
{
	while (m_bExit)
	{
		int s;
		s = soap_accept(&m_soap); 
		if (s < 0)
		{ 
			soap_print_fault(&m_soap, stderr);
			soap_destroy(&m_soap);
			soap_end(&m_soap);	
			return;
		}
		soap_serve(&m_soap);//该句说明该server的服务
		soap_destroy(&m_soap);
		soap_end(&m_soap);	
	}
}

void MySoap::SoapServerThread::DetectThreadClosed()
{
	if (isRunning())
	{
		if (m_bExit)
		{
			m_bExit = false;
			soap_destroy(&m_soap);
			soap_end(&m_soap);
			soap_done(&m_soap);
		}
		wait();
	}
}

int ns__getInfo(struct soap *m_soap,  int nOrder, int nReserveReq, ns__Detectinfo * buf_out)
{
	s_DetectInfo sInfo;

	pContainer->m_pCallBackFunc(&sInfo);

	csSoapData.lock();
	QByteArray ba;
	QList<int> listErrorCount;
	switch (nOrder)
	{
	case 0://获取数据命令
		buf_out->nMouldNumber = sInfo.nMouldNumber;
		ba = sInfo.sMouldID.toLatin1();
		buf_out->sMouldID = soap_strdup(m_soap,ba.data());
		buf_out->nCheckCount = sInfo.nCheckCount;
		buf_out->nKickCount = sInfo.nKickCount;
		//buf_out->sCameraInfo.nCameraCount = sInfo.nCameraCount;
		buf_out->sCameraInfo.nCameraCount = sInfo.nCameraCount;
		buf_out->sCameraInfo.sCameraInfoArray.__size = sInfo.nCameraCount;
		buf_out->sCameraInfo.sCameraInfoArray.__ptr = (struct ns__CameraInfo**)soap_malloc(m_soap,sInfo.nCameraCount*sizeof(struct ns__CameraInfo));  
		for (int i=0;i<sInfo.nCameraCount;i++)
		{
			buf_out->sCameraInfo.sCameraInfoArray.__ptr[i] = (struct ns__CameraInfo*)soap_malloc(m_soap,sizeof(struct ns__CameraInfo));  
			buf_out->sCameraInfo.sCameraInfoArray.__ptr[i]->nCheckCount = sInfo.nCameraCheckCount[i];  
			buf_out->sCameraInfo.sCameraInfoArray.__ptr[i]->nKickCount= sInfo.nCameraKickCount[i];  
			ba = QString("Camera%1").arg(i+1).toLatin1();
			buf_out->sCameraInfo.sCameraInfoArray.__ptr[i]->sCameraLabel = soap_strdup(m_soap,ba.data());
		}
		buf_out->sErrorTypeInfo.nErrorTypeCount = sInfo.nErrorTypeCount;
		buf_out->sErrorTypeInfo.sErrorTypeInfoArray.__size = sInfo.nErrorTypeCount;
		buf_out->sErrorTypeInfo.sErrorTypeInfoArray.__ptr = (struct ns__ErrorTypeInfo**)soap_malloc(m_soap,sInfo.nErrorTypeCount*sizeof(struct ns__ErrorTypeInfo));  
		for (int i=0;i<sInfo.nErrorTypeCount;i++)
		{
			buf_out->sErrorTypeInfo.sErrorTypeInfoArray.__ptr[i] = (struct ns__ErrorTypeInfo*)soap_malloc(m_soap,sizeof(struct ns__ErrorTypeInfo));  
			ba = sInfo.sErrorTypeName[i].toLatin1();
			buf_out->sErrorTypeInfo.sErrorTypeInfoArray.__ptr[i]->sErrorTypeName = soap_strdup(m_soap,ba.data());
			buf_out->sErrorTypeInfo.sErrorTypeInfoArray.__ptr[i]->nErrorTypeClass = sInfo.nErrorTypeClass[i];
			buf_out->sErrorTypeInfo.sErrorTypeInfoArray.__ptr[i]->nErrorTypeKickCount = sInfo.nErrorTypeKickCount[i];
		}
		buf_out->sStationInfo.nStationCount = sInfo.nStationCount;
		buf_out->sStationInfo.sStationInfoArray.__size = sInfo.nStationCount;
		buf_out->sStationInfo.sStationInfoArray.__ptr = (struct ns__StationInfo**)soap_malloc(m_soap,sInfo.nStationCount*sizeof(struct ns__StationInfo));  
		for(int i=0;i<sInfo.nStationCount;i++)
		{
			buf_out->sStationInfo.sStationInfoArray.__ptr[i] = (struct ns__StationInfo*)soap_malloc(m_soap,sizeof(struct ns__StationInfo));  
			buf_out->sStationInfo.sStationInfoArray.__ptr[i]->nStationCheckCount = sInfo.nStationCheckCount[i];  
			buf_out->sStationInfo.sStationInfoArray.__ptr[i]->nStationKickCount= sInfo.nStationKickCount[i];  
			ba = sInfo.sStationName[i].toLatin1();
			buf_out->sStationInfo.sStationInfoArray.__ptr[i]->sStationLabel = soap_strdup(m_soap,ba.data());
		}
		break;
	case 1://其他命令
		break;
	}
	csSoapData.unlock();
	return 0;
}