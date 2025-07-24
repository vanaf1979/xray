#include "Image.h"


Image::Image(const char* filename) {
    this->inp = ImageInput::open(filename);

    if (!this->inp) {
        qDebug() << "File " << filename << " could not be opened";
        return;
    }
}


QList<QString> Image::getlayers() {

    const ImageSpec& spec = this->inp->spec();

    QList<QString> layers;
    for (const auto& channel_name : spec.channelnames) {
        QString new_layer_name = channel_name.c_str();
        int pos = channel_name.find_last_of('.');
        if (pos != std::string::npos) {
            new_layer_name.resize(pos);
        }

        if (!layers.contains(new_layer_name)) {
            layers.append(new_layer_name);
        }

        // Uncomment if we want all color channels listed as wel.
        // layers.append(channel_name.c_str());
    }

    return layers;
}


Image::~Image() {

}
