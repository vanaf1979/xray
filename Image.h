#ifndef IMAGE_H
#define IMAGE_H

#include <QObject>
#include <QDebug>
#include <QList>
#include <QString>
#include <OpenImageIO/imageio.h>
using namespace OIIO;

class Image : public QObject {
    Q_OBJECT
    public:
        explicit Image(const char* filename);
        ~Image();
        std::unique_ptr<ImageInput> inp;
        QList<QString> getlayers();
};

#endif //IMAGE_H
