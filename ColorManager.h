#ifndef COLORMANAGER_H
#define COLORMANAGER_H

#include <QObject>
#include <QDebug>
#include <OpenColorIO/OpenColorIO.h>

namespace OCIO = OCIO_NAMESPACE;

class ColorManager : public QObject {
    Q_OBJECT

public:
    explicit ColorManager();
    ~ColorManager();
    OCIO::ConstConfigRcPtr config;
    QMap<QString, QList<QString>> getTransforms();
};

#endif //COLORMANAGER_H
