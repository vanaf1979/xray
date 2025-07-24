#include "ColorManager.h"
#include <iostream> // For std::cout, std::cerr
#include <string>   // For std::string

// Constructor: Calls loadConfig directly upon instantiation
ColorManager::ColorManager(const std::string& configPath) {
    loadConfig(configPath);
}

// Destructor: No explicit cleanup needed as m_config is an OCIO smart pointer
ColorManager::~ColorManager() {
    // m_config (OCIO::ConstConfigRcPtr) is a smart pointer and will clean itself up.
}

// Loads the OCIO configuration from the specified path
void ColorManager::loadConfig(const std::string& configPath) {
    try {
        m_config = OCIO::Config::CreateFromFile(configPath.c_str());
        if (!m_config) {
            // If CreateFromFile returns null without throwing, it's a failure.
            throw std::runtime_error("Failed to load OCIO config from: " + configPath);
        }
        std::cout << "OCIO Config loaded successfully from: " << configPath << std::endl;
        std::cout << "OCIO Config Version: " << m_config->getMajorVersion() << "." << m_config->getMinorVersion() << std::endl;
    } catch (const OCIO::Exception& e) {
        logOCIOError("ColorManager initialization (loadConfig)", e);
        // Rethrow as std::runtime_error for consistent error handling in client code
        throw std::runtime_error("OCIO Initialization Error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        // Catch other standard library exceptions during file loading/string operations
        std::cerr << "Standard Library Error in ColorManager::loadConfig: " << e.what() << std::endl;
        throw; // Re-throw the original exception
    }
}

// Transforms an image from source to target color space
cv::Mat ColorManager::transform(const cv::Mat& inputImage,
                                const std::string& inputColorSpace,
                                const std::string& outputColorSpace) {
    if (inputImage.empty()) {
        throw std::runtime_error("ColorManager::transform: Input image is empty.");
    }
    if (!m_config) {
        throw std::runtime_error("ColorManager::transform: OCIO config not loaded. Call loadConfig first.");
    }

    // Create a mutable copy of the input image for in-place processing.
    // This copy will be the one returned after transformation.
    cv::Mat processedImage = inputImage.clone();

    // --- Image Pre-processing for OCIO Compatibility ---

    // Step 1: Ensure the image is 3-channel. OCIO typically works with 3 or 4 channels.
    // Grayscale (1-channel) needs to be converted to 3-channel (BGR for OpenCV's next step).
    int currentChannels = processedImage.channels();
    if (currentChannels == 1) {
        cv::cvtColor(processedImage, processedImage, cv::COLOR_GRAY2BGR);
        currentChannels = 3; // Update channels count
        std::cout << "Info: Converted 1-channel grayscale image to 3-channel BGR." << std::endl;
    } else if (currentChannels == 4) {
        // If it's 4-channel (BGRA), convert to 3-channel (BGR), discarding alpha.
        // For color transforms, alpha is usually handled separately or ignored.
        cv::cvtColor(processedImage, processedImage, cv::COLOR_BGRA2BGR);
        currentChannels = 3; // Update channels count
        std::cout << "Info: Converted 4-channel image to 3-channel BGR (alpha discarded)." << std::endl;
    }

    // Step 2: Ensure the image is floating-point (CV_32F).
    // OCIO processors often work most efficiently with float data (0.0-1.0 range).
    if (processedImage.depth() != CV_32F) {
        // Convert 8-bit (0-255) to float (0.0-1.0).
        // If it's 16-bit, convert to float and scale (1.0 / 65535.0).
        // Assuming typical 8-bit image input from cv::imread of JPEG/PNG.
        processedImage.convertTo(processedImage, CV_32F, 1.0 / 255.0);
        std::cout << "Info: Converted image to CV_32FC3 and normalized values to 0.0-1.0." << std::endl;
    }
    // Now 'processedImage' is guaranteed to be CV_32FC3.

    // Step 3: Convert from OpenCV's BGR to OCIO's expected RGB.
    // This is crucial for correct color interpretation by OCIO.
    // Only perform if it's a 3-channel image (which it should be after Step 1).
    if (currentChannels == 3) {
        cv::cvtColor(processedImage, processedImage, cv::COLOR_BGR2RGB);
        std::cout << "Info: Converted BGR to RGB for OCIO processing." << std::endl;
    }

    // Ensure the matrix data is contiguous in memory. This is vital for OCIO's direct buffer access.
    if (!processedImage.isContinuous()) {
        processedImage = processedImage.clone(); // Make it contiguous if it's not
        std::cout << "Warning: Image data was not contiguous. Cloned to ensure contiguous memory." << std::endl;
    }

    // --- OCIO Transformation ---
    try {
        // Retrieve the OCIO ColorSpace objects from the loaded configuration.
        // Using `m_config->getColorSpace()` is the correct OCIO 2.x API.
        OCIO::ConstColorSpaceRcPtr inputCS = m_config->getColorSpace(inputColorSpace.c_str());
        OCIO::ConstColorSpaceRcPtr outputCS = m_config->getColorSpace(outputColorSpace.c_str());

        if (!inputCS) {
            throw std::runtime_error("Input color space '" + inputColorSpace + "' not found in OCIO config.");
        }
        if (!outputCS) {
            throw std::runtime_error("Output color space '" + outputColorSpace + "' not found in OCIO config.");
        }

        // Get the processor object responsible for the color transformation.
        OCIO::ConstProcessorRcPtr processor = m_config->getProcessor(inputCS, outputCS);

        // Get the CPU processor. This is the object that actually applies the transform to pixel data.
        OCIO::ConstCPUProcessorRcPtr cpuProcessor = processor->getDefaultCPUProcessor();
        if (!cpuProcessor) {
            throw std::runtime_error("Failed to get OCIO CPU processor for the transformation.");
        }

        // --- Prepare OCIO::PackedImageDesc ---
        // OCIO dimensions must be long.
        long width = processedImage.cols;
        long height = processedImage.rows;
        long numChannels = processedImage.channels(); // Should be 3 (RGB)

        // Calculate strides correctly for contiguous float RGB data
        long pixelStrideBytes = numChannels * sizeof(float); // Stride between pixels (e.g., R1 to R2)
        // For a 2D image, row stride is width * pixel_stride.
        long rowStrideBytes = width * pixelStrideBytes;
        // zStride is 0 for 2D images.
        long zStrideBytes = 0; // For 2D images, zStride is 0.

        // Use the 8-argument constructor for PackedImageDesc that includes PixelFormat and strides.
        // Based on the 'OpenColorTypes.h' inclusion pattern, PIXEL_FORMAT_F32_RGB is likely
        // defined within the standard OpenColorIO namespace.
        OCIO::PackedImageDesc ocio_image_desc(
            (float*)processedImage.data,                  // Pointer to the raw image data (must be float*)
            width,                                        // Image width
            height,                                       // Image height
            numChannels,                                  // Number of channels (e.g., 3 for RGB)
            OpenColorIO::PIXEL_FORMAT_F32_RGB,            // Explicit pixel format for float RGB data
            pixelStrideBytes,                             // xStride (stride between pixels, bytes)
            rowStrideBytes,                               // yStride (stride between rows, bytes)
            zStrideBytes                                  // zStride (stride between slices, bytes - 0 for 2D)
        );

        // Apply the transformation to the image data in-place (on processedImage's data).
        cpuProcessor->apply(ocio_image_desc);

        // The 'processedImage' cv::Mat now contains the transformed data.
        // It remains CV_32FC3 and in RGB order, matching the specified output.
        return processedImage; // Return the transformed copy

    } catch (const OCIO::Exception& e) {
        logOCIOError("transform(" + inputColorSpace + " to " + outputColorSpace + ")", e);
        throw std::runtime_error("OCIO Transformation Error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        std::cerr << "Standard exception in ColorManager::transform: " << e.what() << std::endl;
        throw; // Re-throw the original exception
    }
}

// Helper function to log OCIO errors
void ColorManager::logOCIOError(const std::string& context, const OCIO::Exception& e) const {
    std::cerr << "OCIO Error in " << context << ": " << e.what() << std::endl;
}