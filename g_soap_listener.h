#ifndef G_SOAP_LISTENER_H
#define G_SOAP_LISTENER_H

#include <QObject>
#include "MySoap.h"

class g_soap_listener : public QObject
{
	Q_OBJECT

public:
	g_soap_listener(QObject *parent);
	~g_soap_listener();

private:
	MySoap* pMySoap;
	void init();
};

#endif // G_SOAP_LISTENER_H
