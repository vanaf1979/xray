#include "OCIOTransform.h"
#include <stdexcept>
#include <algorithm>

// Color space constants
const std::string OCIOTransform::SRGB_COLORSPACE = "sRGB Encoded Rec.709 (sRGB)";
const std::string OCIOTransform::ACESCG_COLORSPACE = "ACEScg";

// Constructor that loads the OCIO config file
OCIOTransform::OCIOTransform(const std::string& configPath) {
    loadConfig(configPath);
}

// Default constructor - config must be loaded separately
OCIOTransform::OCIOTransform() : config(nullptr) {}

// Load OCIO configuration from file
void OCIOTransform::loadConfig(const std::string& configPath) {
    try {
        config = OCIO::Config::CreateFromFile("../src/colormanagement/aces.ocio");
        if (!config) {
            throw std::runtime_error("Failed to load OCIO config from: " + configPath);
        }
    } catch (const OCIO::Exception& e) {
        throw std::runtime_error("OCIO Exception: " + std::string(e.what()));
    }
}

// Transform 8-bit sRGB Mat to 32-bit float ACEScg Mat (0-1 range)
cv::Mat OCIOTransform::sRGBToACEScg(const cv::Mat& srgbMat) {
    if (!isValidSRGBMat(srgbMat)) {
        throw std::invalid_argument("Input Mat must be 8-bit 3-channel RGB");
    }

    return performTransform(srgbMat, SRGB_COLORSPACE, ACESCG_COLORSPACE, true);
}

// Transform 32-bit float ACEScg Mat (0-1 range) to 8-bit sRGB Mat for QImage
cv::Mat OCIOTransform::ACEScgToSRGB(const cv::Mat& acescgMat) {
    if (!isValidACEScgMat(acescgMat)) {
        throw std::invalid_argument("Input Mat must be 32-bit float 3-channel ACEScg");
    }

    // Transform from ACEScg to sRGB in float format first
    cv::Mat floatSRGB = performTransform(acescgMat, ACESCG_COLORSPACE, SRGB_COLORSPACE, true);

    // Convert to 8-bit for QImage compatibility
    return floatTo8Bit(floatSRGB);
}

// Internal transformation method
cv::Mat OCIOTransform::performTransform(const cv::Mat& inputMat,
                                       const std::string& inputColorSpace,
                                       const std::string& outputColorSpace,
                                       bool maintainFloatFormat) {
    if (!config) {
        throw std::runtime_error("OCIO config not loaded. Call loadConfig() first.");
    }

    if (inputMat.empty()) {
        throw std::invalid_argument("Input Mat is empty");
    }

    if (inputMat.channels() != 3) {
        throw std::invalid_argument("Input Mat must have 3 channels (RGB)");
    }

    // Convert to 32-bit float [0,1] range if needed
    cv::Mat floatMat;
    if (inputMat.type() == CV_32FC3) {
        floatMat = inputMat.clone();
    } else if (inputMat.type() == CV_8UC3 || inputMat.type() == CV_32FC1) {
        inputMat.convertTo(floatMat, CV_32FC3, 1.0/255.0);
    } else {
        throw std::invalid_argument("Input Mat must be either CV_8UC3 or CV_32FC3");
    }

    try {
        // Create OCIO processor for the transformation
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(
            inputColorSpace.c_str(),
            outputColorSpace.c_str()
        );

        if (!processor) {
            throw std::runtime_error("Failed to create OCIO processor");
        }

        // Get CPU processor for actual transformation
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();

        // Create output mat
        cv::Mat outputMat = floatMat.clone();

        // Apply transformation pixel by pixel
        int rows = outputMat.rows;
        int cols = outputMat.cols;

        if (outputMat.isContinuous()) {
            cols *= rows;
            rows = 1;
        }

        for (int i = 0; i < rows; ++i) {
            float* ptr = outputMat.ptr<float>(i);
            for (int j = 0; j < cols; ++j) {
                // Process RGB pixel (3 channels)
                float rgb[3] = {ptr[j*3], ptr[j*3+1], ptr[j*3+2]};
                cpuProcessor->applyRGB(rgb);
                ptr[j*3] = rgb[0];
                ptr[j*3+1] = rgb[1];
                ptr[j*3+2] = rgb[2];
            }
        }

        return outputMat;

    } catch (const OCIO::Exception& e) {
        throw std::runtime_error("OCIO transformation failed: " + std::string(e.what()));
    }
}

// Generic transform method (original functionality preserved)
cv::Mat OCIOTransform::transform(const cv::Mat& inputMat,
                               const std::string& inputColorSpace,
                               const std::string& outputColorSpace) {
    return performTransform(inputMat, inputColorSpace, outputColorSpace, true);
}

// Convert 32-bit float Mat to 8-bit for QImage creation
cv::Mat OCIOTransform::floatTo8Bit(const cv::Mat& floatMat) {
    if (floatMat.type() != CV_32FC3) {
        throw std::invalid_argument("Input Mat must be CV_32FC3");
    }

    cv::Mat eightBitMat;

    // Clamp values to [0,1] range before conversion
    cv::Mat clampedMat;
    cv::max(floatMat, 0.0, clampedMat);
    cv::min(clampedMat, 1.0, clampedMat);

    // Convert to 8-bit [0,255] range
    clampedMat.convertTo(eightBitMat, CV_8UC3, 255.0);

    return eightBitMat;
}

// Convert 8-bit Mat to 32-bit float (0-1 range)
cv::Mat OCIOTransform::eightBitToFloat(const cv::Mat& eightBitMat) {
    if (eightBitMat.type() != CV_8UC3) {
        throw std::invalid_argument("Input Mat must be CV_8UC3");
    }

    cv::Mat floatMat;
    eightBitMat.convertTo(floatMat, CV_32FC3, 1.0/255.0);

    return floatMat;
}

// Utility method to check if config is loaded
bool OCIOTransform::isConfigLoaded() const {
    return config != nullptr;
}

// Get available color spaces from the loaded config
std::vector<std::string> OCIOTransform::getAvailableColorSpaces() const {
    std::vector<std::string> colorSpaces;

    if (!config) {
        return colorSpaces;
    }

    for (int i = 0; i < config->getNumColorSpaces(); ++i) {
        colorSpaces.push_back(config->getColorSpaceNameByIndex(i));
    }

    return colorSpaces;
}

// Get the default display transform
std::string OCIOTransform::getDefaultDisplay() const {
    if (!config) {
        return "";
    }
    return config->getDefaultDisplay();
}

// Get available displays
std::vector<std::string> OCIOTransform::getDisplays() const {
    std::vector<std::string> displays;

    if (!config) {
        return displays;
    }

    for (int i = 0; i < config->getNumDisplays(); ++i) {
        displays.push_back(config->getDisplay(i));
    }

    return displays;
}

// Validate Mat for sRGB operations
bool OCIOTransform::isValidSRGBMat(const cv::Mat& mat) const {
    return !mat.empty() && mat.channels() == 3 && mat.type() == CV_8UC3;
}

// Validate Mat for ACEScg operations
bool OCIOTransform::isValidACEScgMat(const cv::Mat& mat) const {
    return !mat.empty() && mat.channels() == 3 && mat.type() == CV_32FC3;
}