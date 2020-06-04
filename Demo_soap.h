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
		// char* sCameraLabel;//相机名称
		// int nCheckCount;//相机检测个数
		// int nKickCount;//相机踢废个数
	 //};

	 //typedef struct S_ArrayOfCameraInfo
	 //{
		// struct ns__CameraInfo **__ptr;
		// int __size;
	 //}ArrayOfCameraInfo;  

	 //typedef struct S_CameraInfo
	 //{
		// int nCameraCount;//相机个数
		// ArrayOfCameraInfo sCameraInfoArray;
	 //}CameraInfo;
	 //typedef CameraInfo xsd_CameraInfo;

	 //struct ns__ErrorTypeInfo
	 //{
		// char* sErrorTypeName;//每个缺陷类型名称
		// int nErrorTypeClass;//每个缺陷类型分类（瓶口、瓶底、瓶身）
		// int nErrorTypeKickCount;//每个缺陷类型踢废个数
	 //};
	 //typedef struct S_ArrayOfErrorTypeInfo
	 //{
		// struct ns__ErrorTypeInfo **__ptr;
		// int __size;
	 //}ArrayOfErrorTypeInfo;  

	 //typedef struct S_ErrorTypeInfo
	 //{
		// int nErrorTypeCount;//缺陷类型总数
		// ArrayOfErrorTypeInfo sErrorTypeInfoArray;
	 //}ErrorTypeInfo;
	 //typedef ErrorTypeInfo xsd_ErrorTypeInfo;


  //   struct ns__StationInfo
	 //{
		// char* sStationLabel;//工位名称
		// int nStationCheckCount;//工位检测个数
  //       int nStationKickCount;//工位踢废个数
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
  //       xsd__int nMouldNumber;//模具序号
		// xsd__string sMouldID;//模具号
		// xsd__int nCheckCount;//检测总数
		// xsd__int nKickCount;//踢废总数
  //       xsd_CameraInfo sCameraInfo;//相机信息
		// xsd_ErrorTypeInfo sErrorTypeInfo;//缺陷类型信息
  //       xsd_StationInfo sStationInfo;//工位信息
		// xsd__int nReserveRes1;
		// xsd__int nReserveRes2;
		// xsd__int nReserveRes3;
	 //};
 
 int ns__getInfo( int nOrder, int nReserveReq, struct ns__Detectinfo * buf_out );
 
