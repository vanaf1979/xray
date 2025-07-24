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


Image::ChannelData ColorManager::transform(const Image::ChannelData& inputData, const QString& inputColorSpace, const QString& outputColorSpace) {

    Image::ChannelData result = inputData; // Copy input data structure

    // Validate input
    if (inputData.data.empty()) {
        qDebug() << "Error: Empty input data for color transformation";
        return result;
    }

    if (inputData.channels < 3) {
        qDebug() << "Error: Need at least 3 channels (RGB) for color transformation";
        return result;
    }

    try {
        // Create the color transform
        OCIO::ConstColorSpaceRcPtr inputCS = this->config->getColorSpace(inputColorSpace.toStdString().c_str());
        OCIO::ConstColorSpaceRcPtr outputCS = this->config->getColorSpace(outputColorSpace.toStdString().c_str());

        if (!inputCS) {
            qDebug() << "Error: Input colorspace" << inputColorSpace << "not found in config";
            return result;
        }

        if (!outputCS) {
            qDebug() << "Error: Output colorspace" << outputColorSpace << "not found in config";
            return result;
        }

        // Create the processor
        OCIO::ConstProcessorRcPtr processor = this->config->getProcessor(inputCS, outputCS);
        if (!processor) {
            qDebug() << "Error: Could not create processor from" << inputColorSpace << "to" << outputColorSpace;
            return result;
        }

        // Get the CPU processor for actual transformation
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        if (!cpuProcessor) {
            qDebug() << "Error: Could not create CPU processor";
            return result;
        }

        // Copy the input data to result for transformation
        result.data = inputData.data;

        // Determine the number of pixels
        int numPixels = inputData.width * inputData.height;

        // OpenColorIO applies transformation per pixel, so we need to loop
        if (inputData.channels == 3) {
            // RGB data - transform each pixel
            for (int i = 0; i < numPixels; ++i) {
                float* pixelPtr = &result.data[i * 3];
                cpuProcessor->applyRGB(pixelPtr);
            }
        }
        else if (inputData.channels >= 4) {
            // RGBA data - transform each pixel
            for (int i = 0; i < numPixels; ++i) {
                float* pixelPtr = &result.data[i * inputData.channels];
                cpuProcessor->applyRGBA(pixelPtr);
            }
        }
        else {
            qDebug() << "Error: Unsupported number of channels:" << inputData.channels;
            return inputData; // Return original data unchanged
        }

        qDebug() << "Successfully transformed" << numPixels << "pixels from" << inputColorSpace << "to" << outputColorSpace;

    } catch (const OCIO::Exception& e) {
        qDebug() << "OCIO Error during transformation:" << e.what();
        return inputData; // Return original data unchanged
    } catch (const std::exception& e) {
        qDebug() << "Standard exception during transformation:" << e.what();
        return inputData; // Return original data unchanged
    }

    return result;
}


ColorManager::~ColorManager() {

}