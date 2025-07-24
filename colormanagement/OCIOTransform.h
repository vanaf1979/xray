#pragma once

#include <opencv2/opencv.hpp>
#include <OpenColorIO/OpenColorIO.h>
#include <string>
#include <vector>

namespace OCIO = OCIO_NAMESPACE;

class OCIOTransform {
private:
    OCIO::ConstConfigRcPtr config;
    
    // Color space constants
    static const std::string SRGB_COLORSPACE;
    static const std::string ACESCG_COLORSPACE;

    // Internal transformation method
    cv::Mat performTransform(const cv::Mat& inputMat,
                           const std::string& inputColorSpace,
                           const std::string& outputColorSpace,
                           bool maintainFloatFormat = false);

public:
    // Constructor that loads the OCIO config file
    OCIOTransform(const std::string& configPath);

    // Default constructor - config must be loaded separately
    OCIOTransform();

    // Load OCIO configuration from file
    void loadConfig(const std::string& configPath);

    // Transform 8-bit sRGB Mat to 32-bit float ACEScg Mat (0-1 range)
    cv::Mat sRGBToACEScg(const cv::Mat& srgbMat);

    // Transform 32-bit float ACEScg Mat (0-1 range) to 8-bit sRGB Mat for QImage
    cv::Mat ACEScgToSRGB(const cv::Mat& acescgMat);

    // Generic transform method (original functionality preserved)
    cv::Mat transform(const cv::Mat& inputMat,
                     const std::string& inputColorSpace,
                     const std::string& outputColorSpace);

    // Convert 32-bit float Mat to 8-bit for QImage creation
    cv::Mat floatTo8Bit(const cv::Mat& floatMat);

    // Convert 8-bit Mat to 32-bit float (0-1 range)
    cv::Mat eightBitToFloat(const cv::Mat& eightBitMat);

    // Utility method to check if config is loaded
    // Utility method to check if config is loaded
    bool isConfigLoaded() const;

    // Get available color spaces from the loaded config
    std::vector<std::string> getAvailableColorSpaces() const;

    // Get the default display transform
    std::string getDefaultDisplay() const;

    // Get available displays
    std::vector<std::string> getDisplays() const;

    // Validate Mat for specific operations
    bool isValidSRGBMat(const cv::Mat& mat) const;
    bool isValidACEScgMat(const cv::Mat& mat) const;
};