#ifndef COLOR_MANAGER_H
#define COLOR_MANAGER_H

#include <opencv2/opencv.hpp>
#include <OpenColorIO/OpenColorIO.h> // This includes OpenColorTypes.h indirectly
#include <string>
#include <stdexcept> // For std::runtime_error
#include <memory>    // For std::shared_ptr (used by OCIO's RcPtr)

// Alias for the OpenColorIO namespace.
// OCIO_NAMESPACE is a macro defined by OpenColorIO to handle versioning.
// We keep this alias for other OCIO types, but not necessarily for pixel formats.
namespace OCIO = OCIO_NAMESPACE;

/**
 * @brief Manages OpenColorIO configurations and color space transformations.
 * This class loads an OCIO config and provides a method to transform image data
 * between specified color spaces, returning a new transformed image.
 */
class ColorManager {
public:
    /**
     * @brief Constructs a ColorManager instance and loads the specified OCIO config.
     * @param configPath The file path to the OCIO configuration file (e.g., ".../aces_1.3/config.ocio").
     * @throws std::runtime_error if the OCIO configuration cannot be loaded.
     */
    ColorManager(const std::string& configPath);

    /**
     * @brief Destructor for ColorManager.
     * OCIO smart pointers handle memory cleanup automatically.
     */
    ~ColorManager();

    /**
     * @brief Loads an OCIO configuration file.
     * This method can be called from the constructor or separately if needed.
     * @param configPath The file path to the OCIO configuration file.
     * @throws std::runtime_error if the OCIO configuration cannot be loaded.
     */
    void loadConfig(const std::string& configPath);

    /**
     * @brief Transforms an image from a source color space to a target color space.
     * The input image is not modified. A new cv::Mat is returned, which will be
     * CV_32FC3 (32-bit float, 3 channels, RGB order) in the specified outputColorSpace.
     *
     * @param inputImage The input const cv::Mat (e.g., 8-bit BGR sRGB).
     * @param inputColorSpace The name of the input image's color space (e.g., "sRGB Encoded Rec.709 (sRGB)").
     * @param outputColorSpace The name of the desired output color space (e.g., "ACEScg").
     * @return A new cv::Mat object, CV_32FC3 (RGB), in the outputColorSpace.
     * @throws std::runtime_error if the transformation fails (e.g., color spaces not found, OCIO errors).
     */
    cv::Mat transform(const cv::Mat& inputImage,
                      const std::string& inputColorSpace,
                      const std::string& outputColorSpace);

private:
    OCIO::ConstConfigRcPtr m_config; ///< Stores the loaded OCIO configuration.

    /**
     * @brief Helper function to log OpenColorIO exceptions.
     * @param context A string describing where the error occurred (e.g., "loadConfig").
     * @param e The OCIO::Exception object caught.
     */
    void logOCIOError(const std::string& context, const OCIO::Exception& e) const;
};

#endif // COLOR_MANAGER_H