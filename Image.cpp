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
        qDebug() << channel_name;
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


Image::ChannelData Image::getChannelDataForOCIO(const QString& channelBaseName, const QString& component) {
    const ImageSpec& spec = this->inp->spec();
    ChannelData result;

    result.width = spec.width;
    result.height = spec.height;

    // Find all channels that match the base name
    std::vector<int> matching_channel_indices;
    std::vector<std::string> matching_channel_names;

    if (channelBaseName == "default") {
        // Handle default channels (R, G, B, A) for PNG, PSD, etc.
        std::vector<std::string> defaultChannels = {"R", "G", "B", "A"};

        for (const std::string& defaultChannel : defaultChannels) {
            for (int i = 0; i < spec.channelnames.size(); ++i) {
                if (spec.channelnames[i] == defaultChannel) {
                    matching_channel_indices.push_back(i);
                    matching_channel_names.push_back(spec.channelnames[i]);
                    break; // Found this channel, move to next
                }
            }
        }
    } else {
        // Handle named channels with dot notation (diffuse.r, specular.g, etc.)
        for (int i = 0; i < spec.channelnames.size(); ++i) {
            const std::string& channel_name = spec.channelnames[i];
            QString qchannel_name = QString::fromStdString(channel_name);

            // Extract base name by removing suffix after last dot
            QString base_name = qchannel_name;
            int pos = qchannel_name.lastIndexOf('.');
            if (pos != -1) {
                base_name = qchannel_name.left(pos);
            }

            if (base_name == channelBaseName) {
                matching_channel_indices.push_back(i);
                matching_channel_names.push_back(channel_name);
            }
        }
    }

    if (matching_channel_indices.empty()) {
        qDebug() << "No channels found for base name:" << channelBaseName;
        return result;
    }

    // Read the entire image data - fix the read_image call
    std::vector<float> image_data(spec.width * spec.height * spec.nchannels);
    if (!this->inp->read_image(0, 0, 0, spec.nchannels, TypeDesc::FLOAT, image_data.data())) {
        qDebug() << "Failed to read image data";
        return result;
    }

    if (component == "all") {
        // Return all matching channels (typically RGBA)
        result.channels = matching_channel_indices.size();
        result.data.resize(spec.width * spec.height * result.channels);
        result.channel_names = matching_channel_names;

        // Extract and interleave the matching channels
        for (int y = 0; y < spec.height; ++y) {
            for (int x = 0; x < spec.width; ++x) {
                int pixel_idx = y * spec.width + x;
                for (int c = 0; c < matching_channel_indices.size(); ++c) {
                    int src_idx = pixel_idx * spec.nchannels + matching_channel_indices[c];
                    int dst_idx = pixel_idx * result.channels + c;
                    result.data[dst_idx] = image_data[src_idx];
                }
            }
        }
    } else {
        // Return single component (r, g, b, a, etc.) but prepare for viewport display
        // We'll create a 4-channel RGBA image where the requested component
        // is replicated across RGB and alpha is set to 1.0

        // Find the specific component channel
        int target_channel_idx = -1;

        if (channelBaseName == "default") {
            // For default channels, look for the component directly (R, G, B, A)
            std::string target_channel = component.toUpper().toStdString();

            for (int i = 0; i < matching_channel_indices.size(); ++i) {
                if (matching_channel_names[i] == target_channel) {
                    target_channel_idx = matching_channel_indices[i];
                    break;
                }
            }
        } else {
            // For named channels, look for the dot notation (channelname.r, etc.)
            std::string target_suffix = "." + component.toStdString();

            for (int i = 0; i < matching_channel_indices.size(); ++i) {
                if (matching_channel_names[i].ends_with(target_suffix)) {
                    target_channel_idx = matching_channel_indices[i];
                    break;
                }
            }
        }

        if (target_channel_idx == -1) {
            qDebug() << "Component" << component << "not found for channel" << channelBaseName;
            return result;
        }

        result.channels = 4; // Always RGBA for viewport display
        result.data.resize(spec.width * spec.height * 4);
        result.channel_names = {"R", "G", "B", "A"}; // Generic names for single component display

        // Fill the result data
        for (int y = 0; y < spec.height; ++y) {
            for (int x = 0; x < spec.width; ++x) {
                int pixel_idx = y * spec.width + x;
                int src_idx = pixel_idx * spec.nchannels + target_channel_idx;
                int dst_base_idx = pixel_idx * 4;

                float value = image_data[src_idx];

                // Replicate the component value across RGB channels
                result.data[dst_base_idx + 0] = value; // R
                result.data[dst_base_idx + 1] = value; // G
                result.data[dst_base_idx + 2] = value; // B
                result.data[dst_base_idx + 3] = 1.0f;  // A (full opacity)
            }
        }
    }

    return result;
}


Image::~Image() {

}
