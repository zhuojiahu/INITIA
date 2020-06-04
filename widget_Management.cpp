#include <QDir>
#include <QStringList>
#include <QTextCodec>
#include <QInputDialog>
#include <QFileDialog>
#include <QPushButton>

//#include <QFileDialog>
#include "widget_Management.h"
#include "widget_MyCellWidget.h"
#include "glasswaredetectsystem.h"

extern GlasswareDetectSystem *pMainFrm;

MyCameraPoButton::MyCameraPoButton(QWidget *parent)
	: QPushButton(parent)

{
	connect(this,SIGNAL(clicked()),this,SLOT(slots_showCameraPo()));

}
MyCameraPoButton::~MyCameraPoButton()
{

}
void MyCameraPoButton::slots_showCameraPo()
{
	emit signals_showCameraPo(iNumber);
}

WidgetManagement::WidgetManagement(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.tableCate->setFont(QFont("΢���ź�",16,3));
	ui.tableCate->setColumnCount(3);
	QStringList strHoriHeader;
	strHoriHeader<<tr("Image")<<tr("Category Info")<<tr("Camera Position");
	ui.tableCate->setHorizontalHeaderLabels(strHoriHeader);
	ui.tableCate->setEditTriggers(QAbstractItemView::NoEditTriggers);//���ɱ༭
	ui.tableCate->setSelectionBehavior(QAbstractItemView::SelectRows);//����ѡ��
	ui.tableCate->setSelectionMode(QAbstractItemView::SingleSelection);//��Ŀ��ѡ��
	ui.tableCate->verticalHeader()->setVisible(false);
//	ui.tableCate->setColumnWidth(3,150);
	UpdateTable();
	connect(ui.tableCate,SIGNAL(cellClicked(int,int)),this,SLOT(slots_cellClicked(int,int)));
	connect(ui.tableCate,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(slots_load()));
	connect(ui.btnNewCate,SIGNAL(clicked()),this,SLOT(slots_new()));
	connect(ui.btnLoadCate,SIGNAL(clicked()),this,SLOT(slots_load()));
	connect(ui.btnBackupCate,SIGNAL(clicked()),this,SLOT(slots_backup()));
	connect(ui.btnDeleteCate,SIGNAL(clicked()),this,SLOT(slots_delete()));
	connect(ui.btnExport,SIGNAL(clicked()),this,SLOT(slots_export()));
	connect(ui.btnImport,SIGNAL(clicked()),this,SLOT(slots_import()));

	connect(this, SIGNAL(signals_clearTable()), pMainFrm->widget_carveSetting->errorList_widget, SLOT(slots_clearTable()));
	ui.widget_Managment->setVisible(true);
	ui.widget_CameraPosition->setVisible(false);
	layoutCameraPo = new QGridLayout();
	ui.widget_CameraPosition->ui.widget_CameraPosition->setLayout(layoutCameraPo);

	initCameraPositionWidget();
}
WidgetManagement::~WidgetManagement()
{

}
void WidgetManagement::slots_intoWidget()
{
	iSelectRow = iCurModelRow;
	ui.widget_Managment->setVisible(true);
	ui.widget_CameraPosition->setVisible(false);

}
bool WidgetManagement::leaveWidget()
{
	return true;
}

void WidgetManagement::UpdateTable()
{
	Sleep(200);//�ȴ��ı��������
	ui.tableCate->clearContents();//�����
	ui.tableCate->setRowCount(0);

	QString strModelName = pMainFrm->m_sSystemInfo.m_strModelName;//.section('\\',-1);//��ǰ��Ʒ����
	QDir dir(pMainFrm->m_sConfigInfo.m_strAppPath+"ModelInfo/");
	dirList = dir.entryInfoList(QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);

	for (int i=0;i<dirList.size();++i)
	{
		int iRowNo = ui.tableCate->rowCount();
		ui.tableCate->insertRow(iRowNo);
		QFileInfo dirInfo = dirList.at(i);
		QString t123 = dirInfo.absoluteFilePath();
		QFileInfo modelInfo(dirInfo.absoluteFilePath()+"/ModelPara.ini");
		QFileInfo imageInfo(dirInfo.absoluteFilePath()+"/modle.bmp");

		WidgetMyCellWidget *cellWidget = new WidgetMyCellWidget(this);
		QWidget *widgetButton = new QWidget(this);

		MyCameraPoButton *buttonCameraPo = new MyCameraPoButton(widgetButton);
 		buttonCameraPo->iNumber = i;
		buttonCameraPo->setFixedSize(100,60);
		buttonCameraPo->setText(tr("Position"));

		QFont font1;
		font1.setPointSize(12);
		buttonCameraPo->setFont(font1);

		connect(cellWidget->action_selectImage,SIGNAL(triggered()),this,SLOT(slots_action_selectImage()));
		connect(buttonCameraPo,SIGNAL(signals_showCameraPo(int)),this,SLOT(slots_showCameraPo(int)));
		if (imageInfo.exists())
		{
			QPixmap pixmap;

			pixmap.load(dirInfo.absoluteFilePath()+"/modle.bmp");
			cellWidget->addImg(pixmap);
		}
		else
		{
			cellWidget->addText(dirInfo.completeBaseName());
		}
		ui.tableCate->setCellWidget(i,0,cellWidget);
		ui.tableCate->setCellWidget(i,1,widgetButton);
		ui.tableCate->setItem(i,2,new QTableWidgetItem(tr("Name:")+dirInfo.completeBaseName()+"\n"+tr("Last modified:")+dirInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
		QString str1 = dirInfo.completeBaseName();
		if (strModelName == dirInfo.completeBaseName())
		{
			iCurModelRow = i;
			ui.tableCate->item(i,2)->setTextColor(QColor(255,0,0));
		}
	}
	ui.tableCate->setColumnWidth(0,100);
	ui.tableCate->verticalHeader()->setDefaultSectionSize(80); //�����и�
	//ui.tableCate->resizeColumnsToContents();//���д�С������ƥ��
	//ui.tableCate->resizeRowsToContents();
	ui.tableCate->horizontalHeader()->setStretchLastSection(true);//���Զ�������
	iSelectRow = -1;

}

void WidgetManagement::slots_cellClicked(int row,int col)
{
	iSelectRow = row;
}

void WidgetManagement::slots_new()
{
	bool ok;
	QString strModelPath;
//	QString	strSrcPath;
	QDir dir;
// 	strSrcPath = ui.leditCateName->text();
	QString strModelName = QInputDialog::getText(this, tr("Input a new name"),tr("New name:"), QLineEdit::Normal,"", &ok);
	if (!ok)
	{
		return;
	}
	
	strModelPath = pMainFrm->m_sConfigInfo.m_strAppPath+"ModelInfo/"+strModelName;//+ui.leditCateName->text();
	if ("" == strModelName)
	{
		QMessageBox::information(this,tr("Information"),tr("Please input model name."));
	} 
	else if (dir.exists(strModelPath))
	{
		QMessageBox::information(this,tr("Information"),tr("Model name already exist."));
		return;
	}
	dir.mkpath(strModelPath);

	QString strSrcPath = pMainFrm->m_sConfigInfo.m_strAppPath + "ModelInfo/" + pMainFrm->m_sSystemInfo.m_strModelName;
	BackupCate(strModelPath,strSrcPath);

	UpdateTable();
// 	ui.tableCate->setCurrentCell(0,0);
	iSelectRow = -1;
	pMainFrm->Logfile.write((tr("New model:[%1]").arg(strModelName.section("/",-1))),OperationLog);

}
void WidgetManagement::slots_load(bool bCurModel)
{

	if (pMainFrm->m_sRunningInfo.m_bCheck)
	{
		QMessageBox::information(this,tr("Information"),tr("Please stop detection!"));
		return;
	}
	if (iSelectRow == iCurModelRow && !bCurModel)
	{
		QMessageBox::information(this,tr("Information"),tr("Current product model is already loaded!"));
		return;
	}
	if ((iSelectRow > dirList.size()-1)||iSelectRow < 0)
	{
		QMessageBox::information(this,tr("Information"),tr("Product model not selected!"));
		return;
	}
	QString strDirPath = dirList.at(iSelectRow).absoluteFilePath();

	if (QMessageBox::No == QMessageBox::question(this,tr("Load"),
		tr("Are you sure to load:<font color=red>[%1]</font>?").arg(strDirPath.section("/",-1)),
		QMessageBox::Yes | QMessageBox::No))
	{
		return;
	}

	for (int iRealCameraSN = 0; iRealCameraSN < pMainFrm->m_sSystemInfo.iRealCamCount;iRealCameraSN++)
	{
		pMainFrm->m_sRealCamInfo[iRealCameraSN].m_bGrabIsStart = FALSE;
	}
	//pMainFrm->m_sSystemInfo.m_strModelName = (strDirPath.section("/",-1)).toLocal8Bit();
	pMainFrm->m_sSystemInfo.m_strModelName = strDirPath.section("/",-1);
	//���ؼ������ú��ع�ʱ��
	pMainFrm->m_sConfigInfo.m_strGrabInfoPath = pMainFrm->m_sConfigInfo.m_strAppPath + "ModelInfo/" + pMainFrm->m_sSystemInfo.m_strModelName + "/GrabInfo.ini";
	//�����ع�ʱ��
	
	//pMainFrm->ReadCorveConfig();
	pMainFrm->InitImage();
	pMainFrm->InitCheckSet();
	//����ģ��
	//s_Status  sReturnStatus;
	//s_AlgInitParam   sAlgInitParam;	

	////��ʼ���㷨
	//for (int i=0;i<pMainFrm->m_sSystemInfo.iCamCount;i++)
	//{
	//	sAlgInitParam.nCamIndex=i;// �㷨�¼Ӳ��� [5/24/2010 GZ]
	//	sAlgInitParam.nModelType = pMainFrm->m_sRealCamInfo[i].m_iImageType;  //�������
	//	sAlgInitParam.nWidth = pMainFrm->m_sRealCamInfo[i].m_iImageWidth; 
	//	sAlgInitParam.nHeight =  pMainFrm->m_sRealCamInfo[i].m_iImageHeight;
	//	memset(sAlgInitParam.chCurrentPath,0,MAX_PATH);

	//	strcpy_s(sAlgInitParam.chCurrentPath,pMainFrm->m_sConfigInfo.m_sAlgFilePath.toLocal8Bit()); 
	//	memset(sAlgInitParam.chModelName,0,MAX_PATH); //ģ������
	//	strcpy_s(sAlgInitParam.chModelName,(strDirPath.section("/",-1)).toLocal8Bit()); 
	//	sReturnStatus = pMainFrm->m_cBottleCheck[i].init(sAlgInitParam);

	//	if (sReturnStatus.nErrorID != RETURN_OK && sReturnStatus.nErrorID != 1)
	//	{
	//		pMainFrm->Logfile.write(tr("----Camera%1 load model failur----").arg(i),AbnormityLog);
	//	}		
	//	if (sReturnStatus.nErrorID == 1) //ģ��Ϊ��
	//	{
	//		//ģ��Ϊ��
	//		pMainFrm->m_sSystemInfo.m_bLoadModel =  FALSE;  //���ģ��Ϊ�գ����ܼ�� 
	//	}
	//	else
	//	{
	//		pMainFrm->m_sSystemInfo.m_bLoadModel =  TRUE;  //�ɹ�������һ�ε�ģ��
	//	}
	//	// ��ת�� [12/10/2010]
	//	sAlgInitParam.nModelType = 99;  //�������
	//	memset(sAlgInitParam.chModelName,0,MAX_PATH); //ģ������
	//	pMainFrm->m_cBottleRotate[i].init(sAlgInitParam);
	//	sAlgInitParam.nModelType = 98;  //�������
	//	pMainFrm->m_cBottleStress[i].init(sAlgInitParam);
	//}
	pMainFrm->m_sSystemInfo.LastModelName = (QString)(strDirPath.section("/",-1));
	pMainFrm->info_widget->labelCateName->setText(pMainFrm->m_sSystemInfo.LastModelName);

	//����
	UpdateTable();
	iSelectRow = -1;
//	pMainFrm->m_sSystemInfo.m_strModelName = pMainFrm->m_sConfigInfo.m_strAppPath+"ModelInfo/"+strDirPath.section("/",-1);
	pMainFrm->Logfile.write(QString(tr("Loading the template:")+strDirPath.section("/",-1)),OperationLog);

	SaveModelNeme(strDirPath);

	emit signals_clearTable();

	for (int iRealCameraSN = 0; iRealCameraSN < pMainFrm->m_sSystemInfo.iRealCamCount;iRealCameraSN++)
	{
		pMainFrm->m_sRealCamInfo[iRealCameraSN].m_bGrabIsStart = TRUE;
	}
}
void WidgetManagement::SaveModelNeme(QString strDirPath)
{
	//д�����ļ�
	QSettings modelInfoSet(pMainFrm->m_sConfigInfo.m_strConfigPath,QSettings::IniFormat);
	modelInfoSet.setIniCodec(QTextCodec::codecForName("GBK"));
	//modelInfoSet.beginGroup("system");
	//modelInfoSet.setValue(QString("LastModelName"),(QString)(strDirPath.section("/",-1)));
	//modelInfoSet.endGroup();
	QString str = strDirPath.section("/",-1);
	modelInfoSet.setValue("/system/LastModelName",str);	//��ȡ�ϴ�ʹ��ģ��
}
void WidgetManagement::slots_backup()
{
 	bool ok;
 	if ((iSelectRow > dirList.size()-1)||iSelectRow < 0)
 	{
 		QMessageBox::information(this,tr("Information"),tr("Product model not selected!"));
 		return;
 	}

 	QString strName = QInputDialog::getText(this, 
											tr("Input a new name"),
											tr("New name:"), 
											QLineEdit::Normal,
											dirList.at(iSelectRow).baseName() + " - Backup", 
											&ok);

 	if (!ok || strName.isEmpty())
 		return;

 	QString strSrcPath = dirList.at(iSelectRow).absoluteFilePath();
 	QString strDestPath = pMainFrm->m_sConfigInfo.m_strAppPath+"ModelInfo/"+strName;
 	QDir dir;
	if (dir.exists(strDestPath))
	{
		QMessageBox::information(this,tr("Information"),tr("Model name already exist."));
		return;
	}
 	if (strName == strSrcPath.section("/",-1))
 	{
 		QMessageBox::information(this,tr("Information"),tr("Cannot input same name!"));
 		return;
 	}
	BackupCate(strDestPath,strSrcPath);
 	//ˢ�±���
 	UpdateTable();
// 	ui.tableCate->setCurrentCell(0,0);
 	iSelectRow = -1;
 	//��־
	pMainFrm->Logfile.write((tr("Backup model:[%1]").arg(strSrcPath.section("/",-1))),OperationLog);

}
void WidgetManagement::slots_delete()
{
 	if (iSelectRow == iCurModelRow)
 	{
 		QMessageBox::information(this,tr("Information"),tr("Current product model is loaded, can not delete!"));
 		return;
 	}
 	if ((iSelectRow > dirList.size()-1)||iSelectRow < 0)
 	{
 		QMessageBox::information(this,tr("Information"),tr("Product model not selected!"));
 		return;
 	}
 	QString strDirPath = dirList.at(iSelectRow).absoluteFilePath();
 	if (QMessageBox::No==QMessageBox::question(this,tr("Delete"),
 		tr("Are you sure to delete:<font color=red>[%1]</font>?").arg(strDirPath.section("/",-1)),
 		QMessageBox::Yes | QMessageBox::No))
 	{
 		return;
 	}	
	DeleteCate(strDirPath);
 	UpdateTable();
 	iSelectRow = -1;
 	//��־
	pMainFrm->Logfile.write((tr("Delete model:[%1]").arg(strDirPath.section("/",-1))),OperationLog);

}
void WidgetManagement::slots_action_selectImage()
{
	QString strDirPath = dirList.at(iSelectRow).absoluteFilePath();
	QString strImagePath = strDirPath + "/Modle.bmp";

	QDir dir;
	if (dir.exists(strImagePath))
	{
		if (QMessageBox::No==QMessageBox::question(this,tr("Information"),
			tr("Image already exist.Are you sure to change it?"),
			QMessageBox::Yes | QMessageBox::No))
		{
			return;
		}	
	}

	QString filename;  
	filename = QFileDialog::getOpenFileName(this, tr("Select a image:"),"", 
		tr("Images (*.png *.bmp *.jpg *.tif *.GIF)")); //ѡ��·��  
	if(filename.isEmpty())  
	{  
		return;  
	}
	QImage *imageSrc = new QImage(filename);
	QImage imageMin = imageSrc->scaledToHeight(80);
	if (imageMin.width()>100)
	{
		imageMin = imageMin.scaledToWidth(100);
	}
	delete imageSrc;
	imageMin.save(strImagePath);

	UpdateTable();
//	ui.tableCate->setCurrentCell(0,0);
	iSelectRow = -1;
	//��־
	pMainFrm->Logfile.write((tr("change image:[%1]").arg(strDirPath.section("/",-1))),OperationLog);

}
void WidgetManagement::DeleteCate(QString strDirPath)
{
	QDir dir(strDirPath);
	QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
	//ɾ���ļ����ڵ��ļ�
	for (int i=0;i<fileList.size();++i)
	{
		QFileInfo fileInfo = fileList.at(i);
		if (!dir.remove(fileInfo.absoluteFilePath()))
		{
			QMessageBox::information(this,tr("Information"),tr("Deleting [%1] fail!").arg(fileInfo.completeBaseName()));
			return;
		}
	}
	QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);
	//�ݹ����ɾ���ļ����ڵ��ļ�
	for (int i=0;i<dirList.size();++i)
	{
		DeleteCate(dirList.at(i).absoluteFilePath());
	}

	//ɾ�����ļ���
	if (!dir.rmdir(strDirPath))
	{
		QMessageBox::information(this,tr("Information"),tr("Deleting [%1] fail!").arg(strDirPath.section('/',-1)));
		return;
	}

}
void WidgetManagement::BackupCate(QString strDestPath,QString strSrcPath)
{
	QDir dir;
	if (!dir.exists(strDestPath))
		dir.mkpath(strDestPath);
	dir.setPath(strSrcPath);
	QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
	//�����ļ����ڵ��ļ�
	for (int i=0;i<fileList.size();++i)
	{
		QFileInfo fileInfo = fileList.at(i);
		if (!QFile::copy(fileInfo.absoluteFilePath(),strDestPath+"\\"+fileInfo.completeBaseName()+"."+fileInfo.completeSuffix()))
		{
			QMessageBox::information(this,tr("Information"),tr("Copying [%1] fail!").arg(fileInfo.completeBaseName()));
			return;
		}
	}
	QFileInfoList dirList = dir.entryInfoList(QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);
	for (int i=0;i<dirList.size();++i)
	{
		QString strDeepDestPath = strDestPath + "/" + dirList.at(i).completeBaseName();
		QString strDeepSrcPath = strSrcPath + "/" + dirList.at(i).completeBaseName();
		BackupCate(strDeepDestPath,strDeepSrcPath);
	}
}
void WidgetManagement::slots_export()
{
	QString pathName = QFileDialog::getExistingDirectory(this, tr("Export Cart"),".\\ModelInfo");
	
	if ((iSelectRow > dirList.size()-1)||iSelectRow < 0)
	{
		QMessageBox::information(this,tr("Information"),tr("Product model not selected!"));
		return;
	}

	QString strSrcPath = dirList.at(iSelectRow).absoluteFilePath();
	QString strDestPath = pathName +"\\" + dirList.at(iSelectRow).baseName() + " - Backup";
	QDir dir;
	if (dir.exists(strDestPath))
	{
		QMessageBox::information(this,tr("Information"),tr("Model already Backup."));
		return;
	}
	BackupCate(strDestPath,strSrcPath);
	//ˢ�±���
	UpdateTable();
	// 	ui.tableCate->setCurrentCell(0,0);
	iSelectRow = -1;
	//��־
	pMainFrm->Logfile.write((tr("Export model:[%1]").arg(strSrcPath.section("/",-1))),OperationLog);

}
void WidgetManagement::slots_import()
{
	QString pathName = QFileDialog::getExistingDirectory(this, tr("Import Cart"),".\\ModelInfo");

	QString strSrcPath = pathName;
	QString strDestPath = pMainFrm->m_sConfigInfo.m_strAppPath + "ModelInfo/" + pathName.section("\\",-1);
	QDir dir;
	if (dir.exists(strDestPath))
	{
		QMessageBox::information(this,tr("Information"),tr("Model already exist."));
		return;
	}
	// 	if (strName == strSrcPath.section("/",-1))
	// 	{
	// 		QMessageBox::information(this,tr("Information"),tr("Cannot input same name!"));
	// 		return;
	// 	}
	BackupCate(strDestPath,strSrcPath);
	//ˢ�±���
	UpdateTable();
	// 	ui.tableCate->setCurrentCell(0,0);
	iSelectRow = -1;
	//��־
	pMainFrm->Logfile.write((tr("Import model:[%1]").arg(strSrcPath.section("/",-1))),OperationLog);
}
//���λ��widget
void WidgetManagement::slots_showCameraPo(int iRow)
{
	ui.widget_CameraPosition->setVisible(true);
	ui.widget_Managment->setVisible(false);
	iSelectRow = iRow;
	intoCameraPositionWidget();
}
void WidgetManagement::initCameraPositionWidget()
{
	for (int i = 0 ;i<pMainFrm->m_sSystemInfo.iRealCamCount;i++)
	{
		QLabel *labelCameraNo = new QLabel();
		listLabelCameraNo.append(labelCameraNo);
		labelCameraNo->setText(tr("CameraNo:%1").arg(i+1));
		QLabel labelCameraHeight;
		labelCameraHeight.setText(tr("Height:"));
		QLabel labelCameraAngle;
		labelCameraAngle.setText(tr("Angle:"));

		QDoubleSpinBox *dSpinBoxCameraHeight = new QDoubleSpinBox();
		listdSpinBoxCameraHeight.append(dSpinBoxCameraHeight);
		QDoubleSpinBox *dSpinBoxCameraAngle = new QDoubleSpinBox();
		listdSpinBoxCameraAngle.append(dSpinBoxCameraAngle);

		layoutCameraPo->addWidget(labelCameraNo,i,0);
		layoutCameraPo->addWidget(&labelCameraHeight,i,1);
		layoutCameraPo->addWidget(dSpinBoxCameraHeight,i,2);
		layoutCameraPo->addWidget(&labelCameraAngle,i,3);
		layoutCameraPo->addWidget(dSpinBoxCameraAngle,i,4);

	}
	connect(ui.widget_CameraPosition->ui.pushButton_ok,SIGNAL(clicked()),this, SLOT(slots_CameraPositionWidgetOK()));
	connect(ui.widget_CameraPosition->ui.pushButton_Cancel,SIGNAL(clicked()),this, SLOT(slots_CameraPositionWidgetCancel()));
}
void WidgetManagement::intoCameraPositionWidget()
{
	if ((iSelectRow > dirList.size()-1)||iSelectRow < 0)
	{
		QMessageBox::information(this,tr("Information"),tr("Product model not selected!"));
		return;
	}
	QString strDirPath = dirList.at(iSelectRow).absoluteFilePath();
	QString strType = (strDirPath.section("/",-1)).toLocal8Bit();
	strDirPath +=  "/CameraPosition.ini";
	QSettings setCameraPo(strDirPath,QSettings::IniFormat);
	ui.widget_CameraPosition->ui.label_Cate->setText(strType);
	for (int i = 0 ;i<pMainFrm->m_sSystemInfo.iRealCamCount;i++)
	{
		double dTempHeight = 1.0*setCameraPo.value(QString("/height/Camera%1").arg(i+1),0).toInt()/100;
		double dTempAngle = 1.0*setCameraPo.value(QString("/angle/Camera%1").arg(i+1),0).toInt()/100;
		listdSpinBoxCameraHeight.at(i)->setValue(dTempHeight);
		listdSpinBoxCameraAngle.at(i)->setValue(dTempAngle);
	}

}

void WidgetManagement::slots_CameraPositionWidgetOK()
{
 	QString strDirPath = dirList.at(iSelectRow).absoluteFilePath()+"/CameraPosition.ini";

	QSettings setCameraPo(strDirPath,QSettings::IniFormat);

	for (int i = 0 ;i<pMainFrm->m_sSystemInfo.iRealCamCount;i++)
	{
		double dTempHeight = listdSpinBoxCameraHeight.at(i)->value();
		setCameraPo.setValue(QString("/height/Camera%1").arg(i+1),dTempHeight*100);
		double dTempAngle = listdSpinBoxCameraAngle.at(i)->value();;
		setCameraPo.setValue(QString("/angle/Camera%1").arg(i+1),dTempAngle*100);
		
		listdSpinBoxCameraAngle.at(i)->setValue(dTempAngle);
	}
	slots_CameraPositionWidgetCancel();

}
void WidgetManagement::slots_CameraPositionWidgetCancel()
{
	ui.widget_CameraPosition->setVisible(false);
	ui.widget_Managment->setVisible(true);
// 	listdSpinBoxCameraHeight.clear();
// 	listdSpinBoxCameraAngle.clear();
}