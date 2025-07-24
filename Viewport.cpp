#include "Viewport.h"

Viewport::Viewport(QWidget *parent): QGraphicsView(parent) {
    this->setStyleSheet("QGraphicsView { border: 0px; }");
    this->setRenderHint(QPainter::Antialiasing);
    this->setDragMode(QGraphicsView::NoDrag);
    this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    this->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);


    // Create the graphics scene
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->setBackgroundBrush(QBrush(QColor(10, 10, 10)));
    scene->setSceneRect(-5000, -5000, 10000, 10000);
    this->setScene(scene);

    // Create the ocio color manager.
    this->color_manager = new ColorManager();

    // Load in a new image.
    this->image = new Image("../test.exr");

    // Get the selected channel data.
    auto rgba_data = image->getChannelDataForOCIO("ViewLayer.Combined", "all");

    // Convert from input color space to ACEScg
    auto transformedData = this->color_manager->transform(rgba_data, "Linear Rec.709 (sRGB)", "ACEScg");

    // Add a gamma correction.
    transformedData = image->applyGammaCorrection(transformedData, 1.0);

    // Convert from ACEScg to the output color space
    transformedData = this->color_manager->transform(transformedData, "ACEScg", "sRGB - Display");

    // Turn the image data into a QGraphicsPixmapItem
    QGraphicsPixmapItem *item = this->createPixmapItem(transformedData);

    // Set the item position based on its width and height.
    item->setPos(item->boundingRect().width() / -2, item->boundingRect().height() / -2);

    // Add the item to the scene.
    scene->addItem(item);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QGraphicsView::customContextMenuRequested, this, &Viewport::showContextMenu);
}


void Viewport::showContextMenu(const QPoint &pos) {
    QMenu contextMenu(this);

    QPointF scenePos = this->mapToScene(pos);

    QGraphicsItem *item = this->scene()->itemAt(scenePos, this->transform());

    if (item) {
        qDebug() << "Item was rightclicked.";
    } else {
        /*
         * View layers menu.
         */
        QMenu *viewMenu = contextMenu.addMenu("View Layer");
        QList<QString> layers = this->image->getlayers();
        for (const QString &layer: layers) {
            viewMenu->addAction(layer, this, []() {
                qDebug() << "Layer selected";
            });
        }

        contextMenu.addSeparator();

        QMap<QString, QList<QString> > color_transforms = this->color_manager->getTransforms();

        /*
         * Input colorspace menu.
         */
        QMenu *inputMenu = contextMenu.addMenu("Input Colorspace");

        for (QMap<QString, QList<QString> >::const_iterator it = color_transforms.constBegin();
             it != color_transforms.constEnd(); ++it) {
            QMenu *subMenu = inputMenu->addMenu(it.key());

            for (const QString transform: it.value()) {
                subMenu->addAction(transform, this, []() {
                    qDebug() << "Click";
                });
            }
        }

        /*
         * Output colorspace menu.
         */
        QMenu *outputMenu = contextMenu.addMenu("Output Colorspace");

        for (QMap<QString, QList<QString> >::const_iterator it = color_transforms.constBegin();
             it != color_transforms.constEnd(); ++it) {
            QMenu *subMenu = outputMenu->addMenu(it.key());

            for (const QString transform: it.value()) {
                subMenu->addAction(transform, this, []() {
                    qDebug() << "Click";
                });
            }
        }
    }

    contextMenu.exec(mapToGlobal(pos));
}


QGraphicsPixmapItem *Viewport::createPixmapItem(const Image::ChannelData &channelData) {
    // Validate input data
    if (channelData.data.empty() || channelData.width <= 0 || channelData.height <= 0) {
        qDebug() << "Error: Invalid channel data for pixmap creation";
        return nullptr;
    }

    // We need at least 3 channels (RGB) for display
    if (channelData.channels < 3) {
        qDebug() << "Error: Need at least 3 channels for display";
        return nullptr;
    }

    // Create QImage - we'll use RGBA format for consistency
    QImage image(channelData.width, channelData.height, QImage::Format_RGBA8888);

    // Convert float data to 8-bit and populate QImage
    for (int y = 0; y < channelData.height; ++y) {
        for (int x = 0; x < channelData.width; ++x) {
            int pixelIdx = y * channelData.width + x;
            int dataIdx = pixelIdx * channelData.channels;

            // Get float values (0.0 to 1.0 range)
            float r = channelData.data[dataIdx + 0];
            float g = channelData.data[dataIdx + 1];
            float b = channelData.data[dataIdx + 2];
            float a = (channelData.channels >= 4) ? channelData.data[dataIdx + 3] : 1.0f;

            // Clamp values to valid range
            r = std::max(0.0f, std::min(1.0f, r));
            g = std::max(0.0f, std::min(1.0f, g));
            b = std::max(0.0f, std::min(1.0f, b));
            a = std::max(0.0f, std::min(1.0f, a));

            // Convert to 8-bit values (0-255)
            quint8 r8 = static_cast<quint8>(r * 255.0f);
            quint8 g8 = static_cast<quint8>(g * 255.0f);
            quint8 b8 = static_cast<quint8>(b * 255.0f);
            quint8 a8 = static_cast<quint8>(a * 255.0f);

            // Set pixel in QImage
            QRgb pixel = qRgba(r8, g8, b8, a8);
            image.setPixel(x, y, pixel);
        }
    }

    // Create QPixmap from QImage
    QPixmap pixmap = QPixmap::fromImage(image);

    // Create QGraphicsPixmapItem
    QGraphicsPixmapItem *pixmapItem = new QGraphicsPixmapItem(pixmap);

    // Optional: Set some useful properties
    pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    qDebug() << "Created pixmap item:" << channelData.width << "x" << channelData.height
            << "with" << channelData.channels << "channels";

    return pixmapItem;
}

// Convenience method to create and display a transformed channel
QGraphicsPixmapItem *Viewport::displayChannel(const QString &channelBaseName, const QString &component, const QString &inputColorSpace, const QString &outputColorSpace) {
    // Get the channel data from the image
    auto channelData = this->image->getChannelDataForOCIO(channelBaseName, component);

    if (channelData.data.empty()) {
        qDebug() << "No data for channel:" << channelBaseName << component;
        return nullptr;
    }

    // Transform the color space
    auto transformedData = this->color_manager->transform(channelData, inputColorSpace, outputColorSpace);

    // Create the pixmap item
    QGraphicsPixmapItem *pixmapItem = createPixmapItem(transformedData);

    return pixmapItem;
}
