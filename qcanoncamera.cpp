#include "dcrimage.h"
#include "qcanoncamera.h"


EdsError EDSCALLBACK handleObjectEvent( EdsObjectEvent event,
									   EdsBaseRef object,
									   EdsVoid * context)
{
	// do something
	QCanonCamera *qcanoncam = (QCanonCamera *) context;

	switch(event)
	{
		//case kEdsObjectEvent_DirItemCreated:
		case kEdsObjectEvent_DirItemRequestTransfer:
			qcanoncam->downloadImage(object);
			break;
		default:
			break;
	}
	

	{
		EdsRelease(object);
	}

return 0;
}
EdsError EDSCALLBACK handlePropertyEvent (EdsPropertyEvent evt, EdsPropertyID prop, EdsUInt32 value, EdsVoid * context)
{
	QCanonCamera *qcanoncam = (QCanonCamera *)context;
	if(value == 0) return 0;
	switch(evt)
	{
		case kEdsPropertyEvent_PropertyChanged:
			switch(prop)
			{
				case kEdsPropID_Av:
						qcanoncam->notifypropertychanged(QCameraProperties::Aperture, QVariant(UINT(value)));
					break;
				case kEdsPropID_Tv:
						qcanoncam->notifypropertychanged(QCameraProperties::ExposureTimes, QVariant(UINT(value)));
					break;
				case kEdsPropID_ISOSpeed:
					qcanoncam->notifypropertychanged(QCameraProperties::Iso, QVariant(UINT(value)));
					break;
				case kEdsPropID_ImageQuality:
					qcanoncam->notifypropertychanged(QCameraProperties::ResolutionMode, QVariant(UINT(value)));
					break;
				case kEdsPropID_Evf_OutputDevice:
					qcanoncam->_liveViewReady = true;
					break;
			}
			break;
	}
	return 0;
}


EdsError EDSCALLBACK handleStateEvent( EdsObjectEvent event, EdsUInt32 value, EdsVoid * context)
{
	QCanonCamera *qcanoncam = (QCanonCamera *) context;
	
	switch(event)
	{
		case kEdsStateEvent_Shutdown:
			delete qcanoncam;
			break;
		default:
			break;
	}
return 0;
}

QCanonCamera::QCanonCamera()
{

}

QCanonCamera::QCanonCamera(int index,EdsCameraRef * camera)
{
	setupisotable();
	setuplegacytable();
	setupaperturetable();
	setupshuttertable();
	setupptptable();
	setupwhitebalancetable();

	EdsError err = EDS_ERR_OK;
	EdsDeviceInfo info;
	_index = index;
	_camera = camera;
	_imagecount = 0;
	_imagedestdir = QDir::currentPath();
	_inbulbmode = false;
	_liveViewReady = false;
	_properties = 0;
	
	err = EdsGetDeviceInfo(*_camera, &info);
	if(err == EDS_ERR_OK){
		_model = QString(info.szDeviceDescription);
		if(info.deviceSubType > 0)
		{
			_hasBulbMode = true;
		}
		else
			_hasBulbMode = false;

		EdsDataType dataType;
		EdsUInt32 dataSize;
		EdsUInt32 value;
		err = EdsGetPropertySize(*_camera, kEdsPropID_BodyIDEx, 0 , &dataType, &dataSize);
		if(err == EDS_ERR_OK)
		{
			err = EdsGetPropertyData(*_camera, kEdsPropID_BodyIDEx, 0 , dataSize, &value); //gets current
		}	
		_serialnumber = QString("%1").arg(value);

	}
}

QCanonCamera::~QCanonCamera()
{
	QCDisconnect();
}

void QCanonCamera::lockUI(){
	EdsError err = EdsSendStatusCommand( *_camera, kEdsCameraStatusCommand_UILock, 0);
	if(err == EDS_ERR_OK)
	{
		_uilocked = true;
	}
}
void QCanonCamera::unlockUI()
{
	EdsError err = EdsSendStatusCommand( *_camera, kEdsCameraStatusCommand_UILock, 0);
	if(err == EDS_ERR_OK)
	{
		_uilocked = false;
	}

}

QCameraProperty *QCanonCamera::getCameraProperty(QCameraProperties::QCameraPropertyTypes prop)
{

	return _properties->getCameraProperty(prop);
}


QCameraProperties *QCanonCamera::getCameraProperties()
{
	if(_properties)
		delete _properties;
	_properties = getallproperties();

	return _properties;
}

QCameraProperties *QCanonCamera::getallproperties()
{
	QCameraProperties *props = new QCameraProperties();

	QCameraProperty *camProp = new QCameraProperty("Resolutions");

	EdsError err = EDS_ERR_OK;
	EdsDataType dataType;
	EdsUInt32 dataSize;
	EdsUInt32 *value = new EdsUInt32();
	EdsPropertyDesc *propDesc;


////Image Quality

	err = EdsGetPropertySize(*_camera, kEdsPropID_ImageQuality, 0 , &dataType, &dataSize);
	if(err == EDS_ERR_OK)
	{
		err = EdsGetPropertyData(*_camera, kEdsPropID_ImageQuality, 0 , dataSize, value); //gets current
	}

	camProp->setCurrentValue(QVariant((UINT)*value));

	propDesc = new EdsPropertyDesc();
	err = EdsGetPropertyDesc(*_camera, kEdsPropID_ImageQuality, propDesc); //gets available Resolutions
	if(err == EDS_ERR_OK)
	{
		for(int i=0; i < propDesc->numElements; i++)
		{
			camProp->appendValue(_ptpImageQualTable.value(propDesc->propDesc[i]), propDesc->propDesc[i]);
		}
		props->addProperty(QCameraProperties::ResolutionMode,camProp);

	}
	else //legacy
	{
		foreach(UINT key, _legacyImageQualTable.keys())
		{
			camProp->appendValue(_legacyImageQualTable.value(key), QVariant(key));
		}
		props->addProperty(QCameraProperties::ResolutionMode, camProp);
	}
	
///ISOs
	propDesc = new EdsPropertyDesc();
	value = new EdsUInt32();
	camProp = new QCameraProperty("ISOs");
	err = EdsGetPropertySize(*_camera, kEdsPropID_ISOSpeed, 0 , &dataType, &dataSize);
	if(err == EDS_ERR_OK)
	{
		err = EdsGetPropertyData(*_camera, kEdsPropID_ISOSpeed, 0 , dataSize, value); //gets current
	}

	camProp->setCurrentValue(QVariant((UINT)*value));


	err = EdsGetPropertyDesc(*_camera, kEdsPropID_ISOSpeed, propDesc); //gets available ISOs
	if(err == EDS_ERR_OK)
	{
		for(int i=0; i < propDesc->numElements; i++)
		{
			camProp->appendValue(_isotable.value(propDesc->propDesc[i]), propDesc->propDesc[i]);
		}
		props->addProperty(QCameraProperties::Iso,camProp);
	}


//Exposure Times/Shutter Speed
	propDesc = new EdsPropertyDesc();
	value = new EdsUInt32();
	camProp = new QCameraProperty("Exposures");
	err = EdsGetPropertySize(*_camera, kEdsPropID_Tv, 0 , &dataType, &dataSize);
	if(err == EDS_ERR_OK)
	{
		err = EdsGetPropertyData(*_camera, kEdsPropID_Tv, 0 , dataSize, value); //gets current
	}

	camProp->setCurrentValue(QVariant((UINT)*value));

	err = EdsGetPropertyDesc(*_camera, kEdsPropID_Tv, propDesc); //gets available Shutter speeds
	if(err == EDS_ERR_OK)
	{
		for(int i=0; i < propDesc->numElements; i++)
		{
			camProp->appendValue(_shuttertable.value(propDesc->propDesc[i]), propDesc->propDesc[i]);
		}
		props->addProperty(QCameraProperties::ExposureTimes,camProp);
	}

///Aperture
	propDesc = new EdsPropertyDesc();
	value = new EdsUInt32();
	camProp = new QCameraProperty("FNumbers");
	err = EdsGetPropertySize(*_camera, kEdsPropID_Av, 0 , &dataType, &dataSize);
	if(err == EDS_ERR_OK)
	{
		err = EdsGetPropertyData(*_camera, kEdsPropID_Av, 0 , dataSize, value); //gets current
	}

	camProp->setCurrentValue(QVariant((UINT)*value));

	err = EdsGetPropertyDesc(*_camera, kEdsPropID_Av, propDesc); //gets available Apertures
	if(err == EDS_ERR_OK)
	{
		for(int i=0; i < propDesc->numElements; i++)
		{
			camProp->appendValue(_aperturetable.value(propDesc->propDesc[i]), propDesc->propDesc[i]);
		}
		props->addProperty(QCameraProperties::Aperture,camProp);
	}

///Whitebalance
	propDesc = new EdsPropertyDesc();
	EdsInt32 *ivalue = new EdsInt32();
	camProp = new QCameraProperty("WhiteBalance");
	err = EdsGetPropertySize(*_camera, kEdsPropID_WhiteBalance, 0 , &dataType, &dataSize);
	if(err == EDS_ERR_OK)
	{
		err = EdsGetPropertyData(*_camera, kEdsPropID_WhiteBalance, 0 , dataSize, ivalue); //gets current
	}

	camProp->setCurrentValue(QVariant((int)*value));

	err = EdsGetPropertyDesc(*_camera, kEdsPropID_WhiteBalance, propDesc); //gets available WhiteBalance
	if(err == EDS_ERR_OK)
	{
		for(int i=0; i < propDesc->numElements; i++)
		{
			camProp->appendValue(_whitebalanceTable.value(propDesc->propDesc[i]), propDesc->propDesc[i]);
		}
		props->addProperty(QCameraProperties::WhiteBalance,camProp);
	}

	return props;
}


void QCanonCamera::setCameraProperty(QCameraProperties::QCameraPropertyTypes prop, QVariant value)
{
	EdsError err = EDS_ERR_OK;
	switch(prop)
	{
		case QCameraProperties::ResolutionMode:
			{
			EdsUInt32 avValue = value.toUInt();
			err = EdsSetPropertyData(*_camera, kEdsPropID_ImageQuality, 0 , sizeof(avValue), &avValue);
			}
			break;
		case QCameraProperties::ImageTypes:
			break;
		case QCameraProperties::Aperture:
			{
			EdsUInt32 avValue = value.toUInt();
			err = EdsSetPropertyData(*_camera, kEdsPropID_Av, 0 , sizeof(avValue), &avValue);
			}
			break;
		case QCameraProperties::BulbMode:
			break;
		case QCameraProperties::ExposureMode:
			//SetExposureEx(_index, value.toUInt());
			break;
		case QCameraProperties::Iso:
			{
			EdsUInt32 avValue = value.toUInt();
			err = EdsSetPropertyData(*_camera, kEdsPropID_ISOSpeed, 0 , sizeof(avValue), &avValue);
			}
			break;
		case QCameraProperties::ExposureTimes:
			EdsUInt32 tvValue = value.toUInt();
			err = EdsSetPropertyData(*_camera, kEdsPropID_Tv, 0 , sizeof(tvValue), &tvValue);
			break;
	}
}

int QCanonCamera::batteryLevel(){
	EdsUInt32 *value = new EdsUInt32();
	EdsDataType dataType;
	EdsUInt32 dataSize;
	EdsError err = EDS_ERR_OK;

	err = EdsGetPropertySize(*_camera, kEdsPropID_BatteryLevel, 0 , &dataType, &dataSize);
	if(err == EDS_ERR_OK)
	{
		err = EdsGetPropertyData(*_camera, kEdsPropID_BatteryLevel, 0 , dataSize, value); //gets current
	}
	return *value;
}


void QCanonCamera::setbulbmode(bool bulb)
{
	EdsError err = EDS_ERR_OK;
	EdsUInt32 *value = new EdsUInt32();
	EdsDataType dataType;
	EdsUInt32 dataSize;

	if(!_hasBulbMode) return;

	_inbulbmode = bulb;

	if(_inbulbmode)
	{
		err = EdsGetPropertySize(*_camera, kEdsPropID_Tv, 0 , &dataType, &dataSize);
		if(err == EDS_ERR_OK)
		{
			err = EdsGetPropertyData(*_camera, kEdsPropID_Tv, 0 , dataSize, value); //gets current
		}

		_oldShutter = QVariant((UINT)*value);

		setCameraProperty(QCameraProperties::ExposureTimes,QVariant(0x0C));
	}
	else
	{
		setCameraProperty(QCameraProperties::ExposureTimes, _oldShutter);
	}
}


void QCanonCamera::capture(int seconds)
{
	EdsError err = EDS_ERR_OK;

	if(_inbulbmode && _hasBulbMode)
	{
		if(!_uilocked)
			lockUI();
		if(err == EDS_ERR_OK)
		{
			err = EdsSendCommand( *_camera, kEdsCameraCommand_BulbStart, 0);
		}
		if(err != EDS_ERR_OK && _uilocked)
		{
			unlockUI();
			setCameraProperty(QCameraProperties::ExposureTimes, _oldShutter);
		}
		else if(err == EDS_ERR_OK)
		{
			bulbtimer = new QTimer();
			connect(bulbtimer,SIGNAL(timeout()),this,SLOT(on_bulbtimer_timeout()));
			bulbtimer->start(seconds * 1000);

			emit camera_busy(true);

		}
	}
	else
	{
		err =  EdsSendCommand( *_camera, kEdsCameraCommand_TakePicture, 0 );
	}
}

void QCanonCamera::on_bulbtimer_timeout()
{
	EdsError err;
	err = EdsSendCommand(*_camera ,kEdsCameraCommand_BulbEnd, 0);
	if(err != EDS_ERR_OK)
	{
			err = EdsSendCommand( *_camera, kEdsCameraCommand_BulbStart, 0);
	}

	delete bulbtimer;

	emit camera_busy(false);
}

int QCanonCamera::QCConnect(){
	EdsError err = EDS_ERR_OK;
	if(err == EDS_ERR_OK)
	{
		err = EdsOpenSession(*_camera);
	}
	//EdsSendStatusCommand ( *_camera, kEdsCameraStatusCommand_EnterDirectTransfer, 0);
	EdsUInt32 whereTo = kEdsSaveTo_Host;
	err = EdsSetPropertyData(*_camera, kEdsPropID_SaveTo, 0 , sizeof(whereTo), &whereTo);

	err = EdsSetObjectEventHandler(*_camera, kEdsObjectEvent_All, handleObjectEvent, this);

	err = EdsSetCameraStateEventHandler(*_camera, kEdsStateEvent_All, handleStateEvent, this);
	err = EdsSetPropertyEventHandler(*_camera, kEdsPropertyEvent_All, handlePropertyEvent, this);

	lockUI();
	getCameraProperties();

	return err;
}
int QCanonCamera::QCDisconnect(){ 
	EdsError err = EDS_ERR_OK;

	err = EdsCloseSession(*_camera);

	return err;
}


QPixmap *QCanonCamera::getLiveViewImage()
{


	EdsError err = EDS_ERR_OK;
	EdsStreamRef stream = NULL;
	EdsStreamRef filestream = NULL;
	EdsEvfImageRef evfImage = NULL;
	// Create memory stream.
	err = EdsCreateMemoryStream( 0, &stream);
	// Create EvfImageRef.
	if(err == EDS_ERR_OK)
	{
		err = EdsCreateEvfImageRef(stream, &evfImage);
	}	// Download live view image data.
	if(err == EDS_ERR_OK)
	{
		do {
		err = EdsDownloadEvfImage(*_camera, evfImage);
		}while(err == EDS_ERR_OBJECT_NOTREADY );
	}
	// Get the incidental data of the image.EDS_ERR_OBJECT_NOTREADY
	if(err == EDS_ERR_OK)
	{
		// Get the zoom ratio
		EdsUInt32 zoom;
		EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(zoom), &zoom);
		// Get the focus and zoom border position
		EdsPoint point;
		EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(point), &point);
	}
	


	unsigned char *data;
	EdsVoid *ptr;
	EdsUInt32 datalen;

	EdsGetLength(stream, &datalen);


	data = (unsigned char *)malloc(sizeof(unsigned char*) * datalen);

	EdsGetPointer(stream, (EdsVoid**)&ptr);
	memcpy(data, ptr, datalen);

	QPixmap *pix= new QPixmap();
	pix->loadFromData(data,datalen);

	free(data);
//	QImage image((const char *)data);
//
// Display image
//
// Release stream
	if(stream != NULL)
	{
		EdsRelease(stream);
		stream = NULL;
	}
// Release evfImage
	if(evfImage != NULL)
	{
		EdsRelease(evfImage);
		evfImage = NULL;
	}
	return pix;
}

void QCanonCamera::initializeLiveView()
{
	EdsError err = EDS_ERR_OK;
// Get the output device for the live view image
	EdsUInt32 device;
	err = EdsGetPropertyData(*_camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device );
// PC live view starts by setting the PC as the output device for the live view image.
		if(err == EDS_ERR_OK)
		{
			device |= kEdsEvfOutputDevice_PC;
			err = EdsSetPropertyData(*_camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
		}
}
void QCanonCamera::endLiveView()
{
	EdsError err = EDS_ERR_OK;
	// Get the output device for the live view image
	EdsUInt32 device;
	err = EdsGetPropertyData(*_camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device );
	// PC live view ends if the PC is disconnected from the live view image output device.
	if(err == EDS_ERR_OK)
	{
		device &= ~kEdsEvfOutputDevice_PC;
		err = EdsSetPropertyData(*_camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
	}
}

//PRIVATE
void QCanonCamera::downloadImage(EdsDirectoryItemRef directoryItem)
{
	EdsError err = EDS_ERR_OK;
	EdsStreamRef stream = NULL;
	// Get directory item information
	EdsDirectoryItemInfo dirItemInfo;
	EdsImageRef imageref;
	EdsDataType dataType;
	EdsUInt32 dataSize;
	EdsUInt32 value;
	QString filename;
	bool isRaw = false;

	err = EdsGetDirectoryItemInfo(directoryItem, & dirItemInfo);


	//err = EdsGetPropertySize(*_camera, kEdsPropID_ImageQuality, 0 , &dataType, &dataSize);
	//if(err == EDS_ERR_OK)
	//{
	//	err = EdsGetPropertyData(*_camera, kEdsPropID_ImageQuality, 0 , dataSize, &value); //gets current
	//}

	_imagecount++;
	if(dirItemInfo.format & kEdsImageType_CRW ||dirItemInfo.format & kEdsImageType_RAW ||dirItemInfo.format & kEdsImageType_CR2){
		filename = QString("%1/%2_%4.%3").arg(_imagedestdir).arg(_nameprefix).arg("cr2").arg(_imagecount,4,10,QChar('0'));
		isRaw = true;
	}
	else
		filename = QString("%1/%2_%4.%3").arg(_imagedestdir).arg(_nameprefix).arg("jpg").arg(_imagecount,4,10,QChar('0'));

	// Create file stream for transfer destination
	if(err == EDS_ERR_OK)
	{
		err = EdsCreateFileStream( filename.toLatin1().data(),
			kEdsFileCreateDisposition_CreateAlways,
			kEdsAccess_ReadWrite, &stream);
		//err = EdsCreateMemoryStream(dirItemInfo.size, &stream);
	}
	// Download image
	if(err == EDS_ERR_OK)
	{
		err = EdsDownload( directoryItem, dirItemInfo.size, stream);
	}
	// Issue notification that download is complete
	if(err == EDS_ERR_OK)
	{
		err = EdsDownloadComplete(directoryItem);
	}
	// Release stream
	if( stream != NULL)
	{
		EdsRelease(stream);
		stream = NULL;
	}

	QImage image;
	if(isRaw)
	{
		//Do raw processing
		DcRImage dcraw;

		//incase the of long shutter exposure and camera hasn't finished writing and it grabs the pic before and it turns out to be jpg
		if(dcraw.isRaw(filename)){
			//if(_usedarkframe && QFile::exists(_darkframe))
			//{
			//	QStringList args;
			//	args += "dcrawqt";
			//	args += "-T";
			//	args += "-c";
			//	args += QString("-K %1").arg(_darkframe); 

			//	dcraw.load(filename, args);
			//}
			//else
				dcraw.loadthumbnail(filename);

			//QByteArray *image =dcraw.GetImage(previewFile.absoluteFilePath());

			image = dcraw.getthumbimage(); //.loadFromData(*image);
		}
	}
	else
	{
		image.load(filename);
	}


	emit image_captured(image);

//	return err;
}

void QCanonCamera::setupwhitebalancetable(){
	_whitebalanceTable.clear();
	_whitebalanceTable.insert(0, "Auto");
	_whitebalanceTable.insert(1, "Daylight");
	_whitebalanceTable.insert(2, "Cloudy");
	_whitebalanceTable.insert(3, "Tungsten");
	_whitebalanceTable.insert(4, "FLourescent");
	_whitebalanceTable.insert(5, "Flash");
	_whitebalanceTable.insert(6, "Manual");
	_whitebalanceTable.insert(8, "Shade");
	_whitebalanceTable.insert(9, "Color Temperature");
	_whitebalanceTable.insert(10, "Custom 1");
	_whitebalanceTable.insert(11, "Custom 2");
	_whitebalanceTable.insert(12, "Custom 3");
	_whitebalanceTable.insert(15, "Manual 2");
	_whitebalanceTable.insert(16, "Manual 3");
	_whitebalanceTable.insert(18, "Manual 4");
	_whitebalanceTable.insert(19, "Manual 5");
	_whitebalanceTable.insert(20, "Custom 4");
	_whitebalanceTable.insert(21, "Custom 5");
	_whitebalanceTable.insert(-1, "Image Coords");
	_whitebalanceTable.insert(-2, "Copied");
}

void QCanonCamera::setuplegacytable()
{
	_legacyImageQualTable.clear();
	_legacyImageQualTable.insert(0x001f000f,"Large - Normal");
	_legacyImageQualTable.insert(0x051f000f,"Medium - 1");
	_legacyImageQualTable.insert(0x061f000f,"Medium - 2");
	_legacyImageQualTable.insert(0x021f000f,"Small - Normal");
	_legacyImageQualTable.insert(0x00130000,"Large - Super Fine");
	_legacyImageQualTable.insert(0x01130000,"Medium - Super Fine");
	_legacyImageQualTable.insert(0x00120000,"Large - Fine");
	_legacyImageQualTable.insert(0x01120000,"Medium - Fine");
	_legacyImageQualTable.insert(0x02130000,"Small - Super Fine");
	_legacyImageQualTable.insert(0x02120000,"Small - Normal");
	_legacyImageQualTable.insert(0x00240000,"Raw");
	_legacyImageQualTable.insert(0x002f001f,"Raw + Large - Normal");
	_legacyImageQualTable.insert(0x002f051f,"Raw + Meduim - 1");
	_legacyImageQualTable.insert(0x002f061f,"Raw + Medium - 2");
	_legacyImageQualTable.insert(0x002f021f,"Raw + Small - Normal");
	_legacyImageQualTable.insert(0x002f000f,"Raw");
	_legacyImageQualTable.insert(0x00240013,"Raw + Large - Super Fine");
	_legacyImageQualTable.insert(0x00240012,"Raw + Large - Fine");
	_legacyImageQualTable.insert(0x00240113,"Raw + Medium - Super Fine");
	_legacyImageQualTable.insert(0x00240112,"Raw + Medium - Fine");
	_legacyImageQualTable.insert(0x00240213,"Raw + Small - Super Fine");
	_legacyImageQualTable.insert(0x00240212,"Raw + Small - Fine");
}
void QCanonCamera::setupptptable()
{
	_ptpImageQualTable.clear();
	_ptpImageQualTable.insert(0x00100f0f,"Large - Normal");
	_ptpImageQualTable.insert(0x05100f0f,"Medium - 1");
	_ptpImageQualTable.insert(0x06100f0f,"Medium - 2");
	_ptpImageQualTable.insert(0x02100f0f,"Small - Normal");
	_ptpImageQualTable.insert(0x00130f0f,"Large - Super Fine");
	_ptpImageQualTable.insert(0x01130f0f,"Medium - Super Fine");
	_ptpImageQualTable.insert(0x00120f0f,"Large - Fine");
	_ptpImageQualTable.insert(0x01120f0f,"Medium - Fine");
	_ptpImageQualTable.insert(0x02130f0f,"Small - Super Fine");
	_ptpImageQualTable.insert(0x02120f0f,"Small - Normal");

	_ptpImageQualTable.insert(0x00640f0f,"Raw");
	_ptpImageQualTable.insert(0x0064ff0f,"Raw");
	_ptpImageQualTable.insert(0x00640010,"Raw + Large - Normal");
	_ptpImageQualTable.insert(0x00640510,"Raw + Meduim - 1");
	_ptpImageQualTable.insert(0x00640610,"Raw + Medium - 2");
	_ptpImageQualTable.insert(0x00640210,"Raw + Small - Normal");
//	_ptpImageQualTable.insert(0x002f000f,"Raw");
	_ptpImageQualTable.insert(0x00640013,"Raw + Large - Super Fine");
	_ptpImageQualTable.insert(0x00640012,"Raw + Large - Fine");
	_ptpImageQualTable.insert(0x00640113,"Raw + Medium - Super Fine");
	_ptpImageQualTable.insert(0x00640112,"Raw + Medium - Fine");
	_ptpImageQualTable.insert(0x00640213,"Raw + Small - Super Fine");
	_ptpImageQualTable.insert(0x00640212,"Raw + Small - Fine");

	_ptpImageQualTable.insert(0x01640f0f,"S-Raw-1 M-Raw");
	_ptpImageQualTable.insert(0x16400010,"M-Raw + Large - Normal");
	_ptpImageQualTable.insert(0x16400510,"M-Raw + Medium - 1");
	_ptpImageQualTable.insert(0x16400610,"M-Raw + Medium - 2");
	_ptpImageQualTable.insert(0x16400210,"M-Raw + Small - Normal");

	_ptpImageQualTable.insert(0x01640013,"S-Raw-1 + Large - Super Fine/M-Raw + Large Super Fine");
	_ptpImageQualTable.insert(0x01640012,"S-Raw-1 + Large - Fine/M-Raw + Large - Fine");
	_ptpImageQualTable.insert(0x01640113,"S-Raw-1 + Medium Super Fine / M-Raw + Medium Super Fine");
	_ptpImageQualTable.insert(0x01640112,"S-Raw-1 + Medium Fine / M-Raw + Medium Fine");
	_ptpImageQualTable.insert(0x01640213,"S-Raw-1 + Small Super Fine / M-Raw + Small Super Fine");
	_ptpImageQualTable.insert(0x01640212,"S-Raw-1 + Small Fine / M-Raw + Small Fine");

	_ptpImageQualTable.insert(0x02640f0f,"S-Raw / S-Raw-2");
	_ptpImageQualTable.insert(0x02640010,"S-Raw + Large Normal");
	_ptpImageQualTable.insert(0x02640510,"S-Raw + Meduim 1");
	_ptpImageQualTable.insert(0x02640610,"S-Raw + Medium 2");
	_ptpImageQualTable.insert(0x02640210,"S-Raw + Small Normal");

	_ptpImageQualTable.insert(0x02640013,"S-Raw + Large Super Fine/S-Raw-2 + Large Super Fine");
	_ptpImageQualTable.insert(0x02640012,"S-Raw + Large Fine/S-Raw-2 + Large Fine");
	_ptpImageQualTable.insert(0x02640113,"S-Raw + Medium Super Fine/S-Raw-2 + Medium Super Fine");
	_ptpImageQualTable.insert(0x02640112,"S-Raw + Medium Fine/S-Raw-2 + Medium Fine");
	_ptpImageQualTable.insert(0x02640213,"S-Raw + Large Small Fine/S-Raw-2 + Small Super Fine");
	_ptpImageQualTable.insert(0x02640212,"S-Raw + Small Fine/S-Raw-2 + Small Fine");

	 _ptpImageQualTable.insert(0x00130f0f, "Large Fine Jpeg");
    _ptpImageQualTable.insert(0x00120f0f, "Large Normal Jpeg");
    _ptpImageQualTable.insert(0x01130f0f, "Middle Fine Jpeg");
    _ptpImageQualTable.insert(0x01120f0f, "Middle Normal Jpeg");
    _ptpImageQualTable.insert(0x02130f0f, "Small Fine Jpeg");
    _ptpImageQualTable.insert(0x02120f0f, "Small Normal Jpeg");

	 _ptpImageQualTable.insert(0x0013ff0f, "Large Fine Jpeg");
    _ptpImageQualTable.insert(0x0012ff0f, "Large Normal Jpeg");
    _ptpImageQualTable.insert(0x0113ff0f, "Middle Fine Jpeg");
    _ptpImageQualTable.insert(0x0112ff0f, "Middle Normal Jpeg");
    _ptpImageQualTable.insert(0x0213ff0f, "Small Fine Jpeg");
    _ptpImageQualTable.insert(0x0212ff0f, "Small Normal Jpeg");

    _ptpImageQualTable.insert(0x00100f0f, "Large Jpeg");
    _ptpImageQualTable.insert(0x05100f0f, "Middle1 Jpeg");
    _ptpImageQualTable.insert(0x06100f0f, "Middle2 Jpeg");
    _ptpImageQualTable.insert(0x02100f0f, "Small Jpeg");
}

void QCanonCamera::setupisotable()
{
	_isotable.clear();
	_isotable.insert(0x00000028, "6");
	_isotable.insert(0x00000030, "12");
	_isotable.insert(0x00000038, "25");
	_isotable.insert(0x00000040, "50");
	_isotable.insert(0x00000048, "100");
	_isotable.insert(0x0000004b, "125");
	_isotable.insert(0x0000004d, "160");
	_isotable.insert(0x00000050, "200");
	_isotable.insert(0x00000053, "250");
	_isotable.insert(0x00000055, "320");
	_isotable.insert(0x00000058, "400");
	_isotable.insert(0x0000005b, "500");
	_isotable.insert(0x0000005d, "640");
	_isotable.insert(0x00000060, "800");
	_isotable.insert(0x00000063, "1000");
	_isotable.insert(0x00000065, "1250");
	_isotable.insert(0x00000068, "1600");
	_isotable.insert(0x00000070, "3200");
	_isotable.insert(0x00000078, "6400");
	_isotable.insert(0x00000080, "12800");
	_isotable.insert(0x00000088, "25600");
	_isotable.insert(0x00000090, "51200");
	_isotable.insert(0x00000098, "102400");
}

void QCanonCamera::setupshuttertable()
{
	_shuttertable.clear();
	_shuttertable.insert(0x0C, "Bulb");
	_shuttertable.insert(0x10, "30\"");
	_shuttertable.insert(0x13, "25\"");
	_shuttertable.insert(0x14, "20\"");
	_shuttertable.insert(0x15, "20\"");
	_shuttertable.insert(0x18, "15\"");
	_shuttertable.insert(0x1B, "13\"");
	_shuttertable.insert(0x1C, "10\"");
	_shuttertable.insert(0x01D, "10\"");
	_shuttertable.insert(0x20, "8\"");
	_shuttertable.insert(0x23, "6\"");
	_shuttertable.insert(0x24, "6\"");
	_shuttertable.insert(0x25, "5\"");
	_shuttertable.insert(0x28, "4\"");
	_shuttertable.insert(0x2B, "3.2\"");
	_shuttertable.insert(0x2C, "3\"");
	_shuttertable.insert(0x2D, "2.5\"");
	_shuttertable.insert(0x30, "2\"");
	_shuttertable.insert(0x33, "1.6\"");
	_shuttertable.insert(0x34, "1.5\"");
	_shuttertable.insert(0x35, "1.3\"");
	_shuttertable.insert(0x38, "1\"");
	_shuttertable.insert(0x3B, "0.8\"");
	_shuttertable.insert(0x3C, "0.7\"");
	_shuttertable.insert(0x3D, "0.6\"");
	_shuttertable.insert(0x40, "0.5\"");
	_shuttertable.insert(0x43, "0.4\"");
	_shuttertable.insert(0x44, "0.3\"");
	_shuttertable.insert(0x45, "0.3\"");
	_shuttertable.insert(0x48, "4");
	_shuttertable.insert(0x4B, "5");
	_shuttertable.insert(0x4C, "6");
	_shuttertable.insert(0x4D, "6");
	_shuttertable.insert(0x50, "8");
	_shuttertable.insert(0x53, "10");
	_shuttertable.insert(0x54, "10");
	_shuttertable.insert(0x55, "13");
	_shuttertable.insert(0x58, "15");
	_shuttertable.insert(0x5B, "20");
	_shuttertable.insert(0x5C, "20");
	_shuttertable.insert(0x5D, "25");
	_shuttertable.insert(0x60, "30");
	_shuttertable.insert(0x63, "40");
	_shuttertable.insert(0x64, "45");
	_shuttertable.insert(0x65, "50");
	_shuttertable.insert(0x68, "60");
	_shuttertable.insert(0x6B, "80");
	_shuttertable.insert(0x6C, "90");
	_shuttertable.insert(0x6D, "100");
	_shuttertable.insert(0x70, "125");
	_shuttertable.insert(0x73, "160");
	_shuttertable.insert(0x74, "180");
	_shuttertable.insert(0x75, "200");
	_shuttertable.insert(0x78, "250");
	_shuttertable.insert(0x7B, "320");
	_shuttertable.insert(0x7C, "350");
	_shuttertable.insert(0x7D, "400");
	_shuttertable.insert(0x80, "500");
	_shuttertable.insert(0x83, "640");
	_shuttertable.insert(0x84, "750");
	_shuttertable.insert(0x85, "800");
	_shuttertable.insert(0x88, "1000");
	_shuttertable.insert(0x8B, "1250");
	_shuttertable.insert(0x8C, "1500");
	_shuttertable.insert(0x8D, "1600");
	_shuttertable.insert(0x90, "2000");
	_shuttertable.insert(0x93, "2500");
	_shuttertable.insert(0x94, "3000");
	_shuttertable.insert(0x95, "3200");	
	_shuttertable.insert(0x98, "4000");
	_shuttertable.insert(0x9B, "5000");
	_shuttertable.insert(0x9C, "6000");
	_shuttertable.insert(0x9D, "6400");
	_shuttertable.insert(0xA0, "8000");
}

void QCanonCamera::setupaperturetable()
{
	_aperturetable.clear();
	_aperturetable.insert(0x08,"1");
	_aperturetable.insert(0x0B,"1.1");
	_aperturetable.insert(0x0C,"1.2");
	_aperturetable.insert(0x0D,"1.2");
	_aperturetable.insert(0x10,"1.4");
	_aperturetable.insert(0x13,"1.6");
	_aperturetable.insert(0x14,"1.8");
	_aperturetable.insert(0x15,"1.8");
	_aperturetable.insert(0x18,"2");
	_aperturetable.insert(0x1B,"2.2");
	_aperturetable.insert(0x1C,"2.5");
	_aperturetable.insert(0x1D,"2.5");
	_aperturetable.insert(0x20,"2.8");
	_aperturetable.insert(0x23,"3.2");
	_aperturetable.insert(0x24,"3.5");
	_aperturetable.insert(0x25,"3.5");
	_aperturetable.insert(0x28,"4");
	_aperturetable.insert(0x2B,"4.5");
	_aperturetable.insert(0x2C,"4.5");
	_aperturetable.insert(0x2D,"5.0");
	_aperturetable.insert(0x30,"5.6");
	_aperturetable.insert(0x33,"6.3");
	_aperturetable.insert(0x34,"6.7");
	_aperturetable.insert(0x35,"7.1");
	_aperturetable.insert(0x38,"8");
	_aperturetable.insert(0x3B,"9");
	_aperturetable.insert(0x3C,"9.5");
	_aperturetable.insert(0x3D,"10");
	_aperturetable.insert(0x40,"11");
	_aperturetable.insert(0x43,"13");
	_aperturetable.insert(0x44,"13");
	_aperturetable.insert(0x45,"14");
	_aperturetable.insert(0x48,"16");
	_aperturetable.insert(0x4B,"18");
	_aperturetable.insert(0x4C,"19");
	_aperturetable.insert(0x4D,"20");
	_aperturetable.insert(0x50,"22");
	_aperturetable.insert(0x53,"25");
	_aperturetable.insert(0x54,"27");
	_aperturetable.insert(0x55,"29");
	_aperturetable.insert(0x58,"32");
	_aperturetable.insert(0x5B,"36");
	_aperturetable.insert(0x5C,"38");
	_aperturetable.insert(0x5D,"40");
	_aperturetable.insert(0x60,"45");
	_aperturetable.insert(0x63,"51");
	_aperturetable.insert(0x64,"54");
	_aperturetable.insert(0x65,"57");
	_aperturetable.insert(0x68,"64");
	_aperturetable.insert(0x6B,"72");
	_aperturetable.insert(0x6C,"76");
	_aperturetable.insert(0x6D,"80");
	_aperturetable.insert(0x70,"91");
	

}




