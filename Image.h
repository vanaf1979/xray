#ifndef IMAGE_H
#define IMAGE_H

#include <QObject>
#include <QDebug>
#include <QList>
#include <vector>
#include <memory>
#include <OpenImageIO/imagebuf.h>
// #include <OpenImageIO/imagespec.h>
#include <OpenImageIO/imageio.h>

using namespace OIIO;

class Image : public QObject {
    Q_OBJECT
    public:
        explicit Image(const char* filename);
        ~Image();
        std::unique_ptr<ImageInput> inp;
        QList<QString> getlayers();
        std::unique_ptr<OIIO::ImageBuf> getChannelDataForOCIO(
            const std::string& strippedChannelName,
            const std::string& channelSelector
        ) const;
};

#endif //IMAGE_H
