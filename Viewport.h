//
// Created by Stephan Nijman on 24/07/2025.
//

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QBrush>
#include <QColor>
#include <QMenu>
#include <QPoint>
#include <QPointF>
#include <QList>
#include <QString>
#include <QGraphicsItem>
#include <OpenColorIO/OpenColorIO.h>
#include "Image.h"
#include "ColorManager.h"

namespace OCIO = OCIO_NAMESPACE;

class Viewport : public QGraphicsView {
    Q_OBJECT

    public:
        Viewport(QWidget *parent = nullptr);
        OCIO::ConstConfigRcPtr ocio_config;
        Image* image;
        ColorManager* color_manager;
        QGraphicsPixmapItem* createPixmapItem(const Image::ChannelData& channelData);
        QGraphicsPixmapItem* displayChannel(const QString& channelBaseName,
                                             const QString& component,
                                             const QString& inputColorSpace,
                                             const QString& outputColorSpace);

    protected slots:
        void showContextMenu(const QPoint &pos);
};

#endif //VIEWPORT_H
