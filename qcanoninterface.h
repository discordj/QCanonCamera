#ifndef QCANONINTERFACE_H
#define QCANONINTERFACE_H

#include "qcanoncamera_global.h"
#include <qcamerainterface.h>
#include <QObject>
#include <QtPlugin>



#include "qcanoncamera.h"
class QCANONCAMERA_EXPORT QCanonInterface : public QCameraInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.ctphoto.QCameraInterface/0.1");
	Q_INTERFACES(QCameraInterface)

public:
	QCanonInterface();
	~QCanonInterface();
	void initialize();
	void unload();
	QList<QCamera *> getcameras();
	QString name();
	QCamera * selectedCamera();


private:
	
};

#endif // QCANONINTERFACE_H
