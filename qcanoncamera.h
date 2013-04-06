#ifndef QCANONCAMERA_H
#define QCANONCAMERA_H
#include <EDSDK.h>
#include "qcanoncamera_global.h"

#include <QTimer>
#include <QImage>

#include <qcamera.h>

class  QCanonCamera: public QCamera
{
   Q_OBJECT
public:
	QCanonCamera();
	~QCanonCamera();


	int QCConnect();
	int QCDisconnect();

	void capture(int seconds=0);
	void setbulbmode(bool bulb);
	void startbulbexposure(){};
	void stopbulbexposure(){};
	QImage getImage(){return QImage();}
	QString identifier() {return QString("%1 (%2)").arg(_model).arg(_serialnumber); }
	QString model() { return _model;}
	void setSelected() {}
	QCameraProperties *getCameraProperties();
	QCameraProperty *getCameraProperty(QCameraProperties::QCameraPropertyTypes prop);
	void setCameraProperty(QCameraProperties::QCameraPropertyTypes prop, QVariant value);

	void setImageDirectory(QString dir){_imagedestdir = dir;}
	void setImageFilePrefix(QString imagenameprefix){_nameprefix = imagenameprefix;}

	void toggleLiveView(bool onoff){}
	
	int batteryLevel();

	bool hasBulbMode(){return _hasBulbMode; }
	virtual bool canSetBulbMode(){return _hasBulbMode; }
	virtual bool hasLiveView(){return true;}
	virtual bool canStreamLiveView() {return true; }
	void initializeLiveView();
	QPixmap *getLiveViewImage();
	void endLiveView();

	virtual void lockUI();
	virtual void unlockUI();

	friend class QCanonInterface;
	friend EdsError EDSCALLBACK handleObjectEvent( EdsObjectEvent event,
									   EdsBaseRef object,
									   EdsVoid * context);
	friend EdsError EDSCALLBACK handlePropertyEvent (EdsPropertyEvent event,
										EdsPropertyID property, EdsUInt32 value,
										EdsVoid * context);
protected:
	QCanonCamera(int index, EdsCameraRef *camera);
	virtual void notifypropertychanged(QCameraProperties::QCameraPropertyTypes prop, QVariant value){emit camera_property_changed(prop, value);};

private slots:
	void on_bulbtimer_timeout();

private:
	int _index;
	EdsCameraRef *_camera;
	QVariant _oldShutter;
	QString _model;
	QString _serialnumber;
	bool _inbulbmode;
	QTimer *bulbtimer;
	QString _imagedestdir;
	QString _nameprefix;
	int _imagecount;
	QString _darkframe;
	bool _usedarkframe;
	bool _hasBulbMode;
	bool _uilocked;
	bool _liveViewReady;
	QCameraProperties *_properties;


	QMap<UINT, QString> _isotable;
	void setupisotable();

	QMap<USHORT, QString> _shuttertable;
	void setupshuttertable();

	QMap<USHORT, QString> _aperturetable;
	void setupaperturetable();

	QMap<UINT,QString> _legacyImageQualTable;
	void setuplegacytable();

	QMap<UINT,QString> _ptpImageQualTable;
	void setupptptable();

	QMap<int, QString> _whitebalanceTable;
	void setupwhitebalancetable();

	void downloadImage(EdsDirectoryItemRef directoryItem);
	QCameraProperties *getallproperties();

};

#endif // QCANONCAMERA_H
