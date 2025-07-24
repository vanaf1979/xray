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
        struct ChannelData {
            std::vector<float> data;
            int width;
            int height;
            int channels;
            std::vector<std::string> channel_names;
        };
        explicit Image(const char* filename);
        ~Image();
        std::unique_ptr<ImageInput> inp;
        QList<QString> getlayers();
        ChannelData getChannelDataForOCIO(const QString& channelBaseName, const QString& component);

};

#endif //IMAGE_H
