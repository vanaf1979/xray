#include "ColorManager.h"


ColorManager::ColorManager() {
    this->config = OCIO::Config::CreateFromFile("../colormanagement/aces.ocio");
}


QMap<QString, QList<QString>> ColorManager::getTransforms() {
    //QList<QString> transforms;
    QMap<QString, QList<QString>> transforms;

    for (int i = 0; i < this->config->getNumColorSpaces(); ++i) {
        OCIO::ConstColorSpaceRcPtr cs = config->getColorSpace(this->config->getColorSpaceNameByIndex(i));

        if (!transforms.contains(cs->getName())) {
            transforms[cs->getFamily()].append(cs->getName());
        }
    }

    return transforms;
}

ColorManager::~ColorManager() {

}