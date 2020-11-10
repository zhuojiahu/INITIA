// ProgramLicense.h: interface for the CProgramLicense class.
//
//////////////////////////////////////////////////////////////////////
// #include "alg_struct.h"
// using namespace Alg;

#if !defined(AFX_PROGRAMLICENSE_H__A36FE32F_4A7E_435E_B3E1_D226AF592D80__INCLUDED_)
#define AFX_PROGRAMLICENSE_H__A36FE32F_4A7E_435E_B3E1_D226AF592D80__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef GUID ver_code;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define	_EXPORTING

#ifdef _EXPORTING
#define CLASS_DECLSPEC    __declspec(dllexport)  //�������ʱʹ��
#else
#define CLASS_DECLSPEC    __declspec(dllimport)	 //��̬��Ӧ��ʱʹ��
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CLOSE_WINDOWN           WM_USER+1000   //�رճ�����Ϣ���������й����аε����ܹ����߻ز�ʱ��
#define TIME_ERROR              WM_USER+1001   //�������й����аε����ܹ����߻ز�ʱ��, һ���Ӻ���CLOSE_WINDOWN��Ϣ
#define DATE_WARN               WM_USER+1002   //������������

#define KV_NOLIMITATION			0		//���к�������
#define KV_NOTREACHLIMITATION	-1		//δ�ﵽ���ƣ������ֹ���ڻ���Զ
#define KV_CLOSETODEADLINE		-2		//δ�ﵽ���ƣ����ֹ���ںܽ�

#define KV_NOTFINDROCKEYDLL		1		//û�ҵ����ܹ���̬��
#define KV_NOTFINDDONGLE		2		//û�ҵ����ܹ�
#define KV_OPENDONGLEFAIL		3		//�򿪼��ܹ�ʧ��
#define KV_FAILLOADKEY			4		//������Կʧ��
#define KV_UNVALIDUSERID		5		//�û�������
#define KV_INVALIDKEY			6		//�Ƿ���Կ����Կ����
#define KV_EXPIREDKEY			7		//��Կ���ڣ������ˣ�
#define KV_LICENSEFILENOTFOUND  8		//û�ҵ�License�ļ�
#define KV_INFOCHANGED			9		//ע����Ϣ������
#define KV_VCODEERROR			10		//UUID��֤ʧ��
#define KV_CALLBACKTIME			11		//��⵽�ز�
#define KV_LOADALGORITHMFAIL	12		//���ؼ��ܹ��㷨ʧ��
#define KV_ALGORITHMVERIFYFAIL	13		//���ܹ��㷨����
#define KV_LOADDONGLEDATAFAIL	14		//��ȡ���ܹ�����ʧ��
#define KV_INVALIDTEMPLICENSE	15		//�Ƿ�����ʱ��Ȩ
#define KV_WRITEDOGFAILED		16		//д����ܹ�ʧ��
#define KV_CODEDECODEFAIL		17		//���룬����ʧ��
#define KV_INVALIDDATESTRING	18		//�Ƿ��������ַ���
#define KV_INVALIDDATEFORMAT	19		//�Ƿ������ڸ�ʽ
#define KV_LOADDOGFUNCTIONFAIL	20		//���ؼ��ܹ�����ʧ��
#define KV_PERIODCHECKDOGFAIL	21		//������֤���ܹ�ʧ��
#define KV_UNKNOWPROBLEM		99		//��������

typedef struct _KeyVerfResult
{
	int nError;				//��Ӧ�ϱߵ�define�Ķ���	
	int nExpireDate;		//-1: �޴�������  0: δ����ֹ����  1: �����ֹ��������15��
	int nDays;				//-1: �޴�������  0: δ�ﵽʹ����������  1: ����ʹ���������ƻ�ʣ����15��
	int nGlobalRuntime;		//-1: �޴�������  0: δ�ﵽʹ��ʱ������  1: ����ʹ��������������30����
	int nExecutions;		//-1: �޴�������  0: δ�ﵽʹ�ô�������  1: ����ʹ�ô������ƻ�ʣ����10��
	char chErrorDetail[1024];
}s_KeyVerfResult;

class CLASS_DECLSPEC CProgramLicense  
{
public:
	
	CProgramLicense();
	virtual ~CProgramLicense();
	
	void GetVerCode(ver_code* vCode);//��֤UU��
	void SetMainWnd(const HWND hWnd);//���������ھ���������ô˺��������ھ��Ĭ��ΪAfxSetMainWnd()->m_hWnd
	s_KeyVerfResult CheckLicenseValid(BOOL bFirst);//��Ȩ��֤����ʱ��֤	
	int	ReadHardwareID(char* chHardwareID);//getexpdate���ʣ������,warn_days������Ϣ��������warn_10�뵽��10������DATE_WARN
	int ResetDog(char* chPassWord);//0:���ܹ����óɹ���1�����ܹ�����ʧ�ܣ�2�������ʽ����3:���������֤ʧ��
	DWORD ReadDog(void);//�����ܹ���0xFFFFFFFF��ȡ����

	int CheckDogTime();	//������0: Good  1: �ز�����  2: �Ƿ����ڻ��/���ܴ���  >3: ���ܹ����ʹ���
	void ShowDongleErrorCode();//������	
	void OpenDog(); //�����ã���
	
private:
	void* m_pCheck;
//	LONGLONG m_llFreq;	
};

#endif // !defined(AFX_PROGRAMLICENSE_H__A36FE32F_4A7E_435E_B3E1_D226AF592D80__INCLUDED_)