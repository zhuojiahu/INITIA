//gsoap ns service name: add
//gsoap ns service namespace: http://localhost/add.wsdl
//gsoap ns service location: http://localhost
//gsoap ns service executable: add.cgi
//gsoap ns service encoding: encoded
//gsoap ns schema namespace: urn:add

     typedef char * xsd__string;
     typedef long   xsd__int; 

	 //struct ns__CameraInfo
	 //{
		// char* sCameraLabel;//�������
		// int nCheckCount;//���������
		// int nKickCount;//����߷ϸ���
	 //};

	 //typedef struct S_ArrayOfCameraInfo
	 //{
		// struct ns__CameraInfo **__ptr;
		// int __size;
	 //}ArrayOfCameraInfo;  

	 //typedef struct S_CameraInfo
	 //{
		// int nCameraCount;//�������
		// ArrayOfCameraInfo sCameraInfoArray;
	 //}CameraInfo;
	 //typedef CameraInfo xsd_CameraInfo;

	 //struct ns__ErrorTypeInfo
	 //{
		// char* sErrorTypeName;//ÿ��ȱ����������
		// int nErrorTypeClass;//ÿ��ȱ�����ͷ��ࣨƿ�ڡ�ƿ�ס�ƿ��
		// int nErrorTypeKickCount;//ÿ��ȱ�������߷ϸ���
	 //};
	 //typedef struct S_ArrayOfErrorTypeInfo
	 //{
		// struct ns__ErrorTypeInfo **__ptr;
		// int __size;
	 //}ArrayOfErrorTypeInfo;  

	 //typedef struct S_ErrorTypeInfo
	 //{
		// int nErrorTypeCount;//ȱ����������
		// ArrayOfErrorTypeInfo sErrorTypeInfoArray;
	 //}ErrorTypeInfo;
	 //typedef ErrorTypeInfo xsd_ErrorTypeInfo;


  //   struct ns__StationInfo
	 //{
		// char* sStationLabel;//��λ����
		// int nStationCheckCount;//��λ������
  //       int nStationKickCount;//��λ�߷ϸ���
	 //};

	 //typedef struct S_ArrayOfStationInfo
	 //{
		// struct ns__StationInfo **__ptr;
		// int __size;
	 //}ArrayOfStationInfo;  

	 //typedef struct S_StationInfo
	 //{
		// int nStationCount;
		// ArrayOfStationInfo sStationInfoArray;
	 //}StationInfo;
	 //typedef StationInfo xsd_StationInfo;

	 //struct ns__Detectinfo{
  //       xsd__int nMouldNumber;//ģ�����
		// xsd__string sMouldID;//ģ�ߺ�
		// xsd__int nCheckCount;//�������
		// xsd__int nKickCount;//�߷�����
  //       xsd_CameraInfo sCameraInfo;//�����Ϣ
		// xsd_ErrorTypeInfo sErrorTypeInfo;//ȱ��������Ϣ
  //       xsd_StationInfo sStationInfo;//��λ��Ϣ
		// xsd__int nReserveRes1;
		// xsd__int nReserveRes2;
		// xsd__int nReserveRes3;
	 //};
 
 int ns__getInfo( int nOrder, int nReserveReq, struct ns__Detectinfo * buf_out );
 
