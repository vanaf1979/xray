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

    this->ocio_config = OCIO::Config::CreateFromFile("../colormanagement/aces.ocio");
    this->iamge = new Image("../test.exr");
    this->color_manager = new ColorManager();

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
        QList<QString> layers = this->iamge->getlayers();
        for (const QString& layer : layers) {
            viewMenu->addAction(layer, this, []() {
                qDebug() << "Layer selected";
            });
        }

        contextMenu.addSeparator();

        QMap<QString, QList<QString>> color_transforms = this->color_manager->getTransforms();

        /*
         * Input colorspace menu.
         */
        QMenu *inputMenu = contextMenu.addMenu("Input Colorspace");

        for (QMap<QString, QList<QString>>::const_iterator it = color_transforms.constBegin(); it != color_transforms.constEnd(); ++it)
        {
            QMenu *subMenu = inputMenu->addMenu(it.key());

            for (const QString transform : it.value()) {
                subMenu->addAction(transform, this, []() {
                    qDebug() << "Click";
                });
            }
        }

        /*
         * Output colorspace menu.
         */
        QMenu *outputMenu = contextMenu.addMenu("Output Colorspace");

        for (QMap<QString, QList<QString>>::const_iterator it = color_transforms.constBegin(); it != color_transforms.constEnd(); ++it)
        {
            QMenu *subMenu = outputMenu->addMenu(it.key());

            for (const QString transform : it.value()) {
                subMenu->addAction(transform, this, []() {
                    qDebug() << "Click";
                });
            }
        }

    }

    contextMenu.exec(mapToGlobal(pos));
}

