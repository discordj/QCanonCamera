#include <EDSDK.h>
#include "qcanoninterface.h"

QCanonInterface::QCanonInterface()
{

}

QCanonInterface::~QCanonInterface()
{

}

void QCanonInterface::initialize(){
	EdsError err = EDS_ERR_OK;
	EdsCameraRef camera = NULL;
	bool isSDKLoaded = false;
	// Initialize SDK
	err = EdsInitializeSDK();
	if(err == EDS_ERR_OK)
	{
		isSDKLoaded = true;
	}
}

void QCanonInterface::unload(){
}

QList<QCamera *> QCanonInterface::getcameras(){
	EdsError err = EDS_ERR_OK;
	EdsCameraListRef cameraList = NULL;
	EdsUInt32 count = 0;
	QList<QCamera *> cameras; 
	EdsCameraRef *camera = new EdsCameraRef();
	// Get camera list
	err = EdsGetCameraList(&cameraList);
	// Get number of cameras
	if(err == EDS_ERR_OK)
	{
		err = EdsGetChildCount(cameraList, &count);
		if(count == 0)
		{
			err = EDS_ERR_DEVICE_NOT_FOUND;
		}
	}



	// Get first camera retrieved
	if(err == EDS_ERR_OK)
	{
		for(int i=0; i < count; i++){
			err = EdsGetChildAtIndex(cameraList , i , camera);
			QCanonCamera *cam = new QCanonCamera(i, camera);
			cameras.append(cam);
		}
	}
	// Release camera list
	if(cameraList != NULL)
	{
		EdsRelease(cameraList);
		cameraList = NULL;
	}

	return cameras;

}

QString QCanonInterface::name(){
	return QString();
}

QCamera * QCanonInterface::selectedCamera()
{
	return (QCamera *)0;
}

//Q_EXPORT_PLUGIN2( canon, QCanonInterface )
Q_PLUGIN_METADATA(IID "com.ctphoto.QCameraInterface/0.1");