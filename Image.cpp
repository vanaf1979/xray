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


std::unique_ptr<OIIO::ImageBuf> Image::getChannelDataForOCIO(
    const std::string& strippedChannelName,
    const std::string& channelSelector
) const {
    // 1. Validate the input image buffer.
    if (!inp) {
        std::cerr << "ERROR: Image buffer not initialized. Cannot extract channel data." << std::endl;
        return nullptr; // Return null if the image is not loaded.
    }

    const OIIO::ImageSpec& spec = this->inp->spec(); // Get the specification of the input image.
    int width = spec.width;
    int height = spec.height;
    int output_nchannels = 4; // Output will always be RGBA (4 channels).
    OIIO::TypeDesc output_format = OIIO::TypeDesc::FLOAT; // OCIO prefers float data.

    // 2. Create a new ImageSpec for the output buffer.
    OIIO::ImageSpec output_spec(width, height, output_nchannels, output_format);
    output_spec.channelnames.clear(); // Clear default channel names.
    output_spec.channelnames.push_back("R");
    output_spec.channelnames.push_back("G");
    output_spec.channelnames.push_back("B");
    output_spec.channelnames.push_back("A");

    // 3. Create the output OIIO::ImageBuf.
    auto output_buf = std::make_unique<OIIO::ImageBuf>(output_spec);
    if (output_buf->has_error()) {
        std::cerr << "ERROR: Could not create output ImageBuf: " << output_buf->geterror() << std::endl;
        return nullptr; // Return null if buffer creation fails.
    }

    // 4. Initialize the output buffer to black (0,0,0) and opaque alpha (1.0).
    // This ensures a clean slate and handles cases where channels are missing.
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float pixel[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Default to black, opaque.
            output_buf->setpixel(x, y, pixel);
        }
    }

    // Helper lambda to find the index of a channel in the input ImageBuf's spec.
    auto get_input_channel_index = [&](const std::string& full_channel_name) -> int {
        for (int i = 0; i < spec.nchannels; ++i) {
            if (spec.channelnames[i] == full_channel_name) {
                return i;
            }
        }
        return -1; // Channel not found.
    };

    // --- Core Logic: Handle 'all' vs. single channel selection ---
    if (channelSelector == "all") {
        // Case 1: Combine all components of the strippedChannelName (e.g., diffuse.r, diffuse.g, diffuse.b, diffuse.a).
        std::vector<std::string> matching_channels;
        for (const auto& channel_name : spec.channelnames) {
            // Check if the channel name starts with the strippedChannelName.
            // Example: "diffuse.r" starts with "diffuse".
            if (channel_name.rfind(strippedChannelName, 0) == 0) {
                matching_channels.push_back(channel_name);
            }
        }

        if (matching_channels.empty()) {
            std::cerr << "WARNING: No channels found starting with '" << strippedChannelName << "'." << std::endl;
            // Return the initialized black opaque image if no matching channels are found.
            return output_buf;
        }

        // Iterate through all pixels to copy data.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float current_output_pixel[4];
                output_buf->getpixel(x, y, current_output_pixel); // Get current (initialized) pixel values.

                for (const auto& full_channel_name : matching_channels) {
                    int input_channel_idx = get_input_channel_index(full_channel_name);
                    if (input_channel_idx == -1) continue; // Should not happen if matching_channels is correct.

                    float input_val;
                    if (this->inp->getpixel(x, y, input_channel_idx, &input_val)) { // 'inp' is OIIO::ImageBuf, so .getpixel() is correct
                        // Determine which output channel (R, G, B, A) this input channel maps to.
                        int output_channel_idx = -1;

                        // Check for common suffixes (.r, .g, .b, .a, .x, .y, .z)
                        if (full_channel_name.length() > strippedChannelName.length() &&
                            full_channel_name[strippedChannelName.length()] == '.') {
                            char suffix = full_channel_name.back(); // Get the character after the last dot.
                            if (suffix == 'r' || suffix == 'x') output_channel_idx = 0; // R
                            else if (suffix == 'g' || suffix == 'y') output_channel_idx = 1; // G
                            else if (suffix == 'b' || suffix == 'z') output_channel_idx = 2; // B
                            else if (suffix == 'a') output_channel_idx = 3; // A
                        }
                        // Special handling for single channels that are themselves the stripped name (e.g., "Z").
                        else if (full_channel_name == strippedChannelName && spec.nchannels == 1 && spec.channelnames[0] == strippedChannelName) {
                            output_channel_idx = 0; // If it's a single channel named "Z", map it to R.
                                                    // This will be replicated to G and B later for grayscale visualization.
                        }

                        if (output_channel_idx != -1) {
                            current_output_pixel[output_channel_idx] = input_val;
                        }
                    }
                }
                output_buf->setpixel(x, y, current_output_pixel); // Set the updated pixel.
            }
        }
    } else {
        // Case 2: Extract a specific component (r, g, b, or a) and visualize it as RGBA.
        std::string target_full_channel_name = strippedChannelName + "." + channelSelector;
        int input_channel_idx = get_input_channel_index(target_full_channel_name);

        // Handle cases where the strippedChannelName itself IS the channel (e.g., "Z" channel).
        // If the specific component (e.g., "Z.r") is not found, but the stripped name ("Z") exists
        // as a channel, use that. This covers "x, y, z channels etc."
        if (input_channel_idx == -1 && strippedChannelName == target_full_channel_name) {
             input_channel_idx = get_input_channel_index(strippedChannelName);
        }


        if (input_channel_idx == -1) {
            std::cerr << "WARNING: Channel '" << target_full_channel_name << "' or '" << strippedChannelName << "' not found." << std::endl;
            // Return the initialized black opaque image if the specific channel is not found.
            return output_buf;
        }

        // Replicate the single channel data across R, G, B for visualization.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float val;
                if (this->inp.getpixel(x, y, input_channel_idx, &val)) { // 'inp' is OIIO::ImageBuf, so .getpixel() is correct
                    float current_output_pixel[4];
                    output_buf->getpixel(x, y, current_output_pixel);

                    // Replicate the value to R, G, B channels.
                    current_output_pixel[0] = val; // R
                    current_output_pixel[1] = val; // G
                    current_output_pixel[2] = val; // B

                    // Handle Alpha:
                    // If 'a' was explicitly selected, put its value into the output alpha channel.
                    if (channelSelector == "a") {
                        current_output_pixel[3] = val;
                    } else {
                        // If 'r', 'g', or 'b' was selected, try to find a corresponding alpha channel
                        // for the group (e.g., "diffuse.a" if "diffuse.r" was selected).
                        int alpha_idx = get_input_channel_index(strippedChannelName + ".a");
                        if (alpha_idx != -1) {
                            float alpha_val;
                            if (inp.getpixel(x, y, alpha_idx, &alpha_val)) { // 'inp' is OIIO::ImageBuf, so .getpixel() is correct
                                current_output_pixel[3] = alpha_val;
                            }
                        }
                        // Otherwise, alpha remains at its default initialized value (1.0).
                    }
                    output_buf->setpixel(x, y, current_output_pixel);
                }
            }
        }
    }

    return output_buf; // Return the prepared ImageBuf.
}


Image::~Image() {

}
