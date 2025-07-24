#include "MainWindow.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyleFactory>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    this->setWindowTitle("EXRay v0.0.1");
    this->setupUi();
}


MainWindow::~MainWindow() {
}


void MainWindow::setupUi() {
    // Create the main vertical layout.
    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // Create the viewport widget and add it to the layout.
    QGraphicsView *viewport = this->setupViewport();
    main_layout->addWidget(viewport);

    // Create an empty widget, and the layout to it and set it as the central widget.
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(main_layout);
    this->setCentralWidget(centralWidget);
}


QGraphicsView *MainWindow::setupViewport() {
    Viewport *viewport = new Viewport(this);
    return viewport;
}


QMap<QString, QStringList> MainWindow::loadOcioConfigAndPopulateData() {
    QMap<QString, QStringList> ocioFamilyToColorSpaces;

    ocioFamilyToColorSpaces.clear(); // Clear previous data

    try {
        OCIO::ConstConfigRcPtr config;

        try {
            config = OCIO::Config::CreateFromFile("../colormanagement/aces.ocio");
        } catch (const OCIO::Exception &e) {
            qWarning() << "Could not load config from " << e.what();
        }

        if (!config) {
            qWarning() << "Failed to load any OpenColorIO configuration.";
        }

        // Iterate through all color spaces in the config
        for (int i = 0; i < config->getNumColorSpaces(); ++i) {
            const char *csName = config->getColorSpaceNameByIndex(i);
            OCIO::ConstColorSpaceRcPtr colorSpace = config->getColorSpace(csName);

            QString family = colorSpace->getFamily();
            if (family.isEmpty()) {
                family = "Uncategorized"; // Group color spaces without a family
            }

            // Add the color space name to the list for its family
            ocioFamilyToColorSpaces[family].append(csName);
        }

        // Sort color spaces within each family alphabetically
        for (QString &family: ocioFamilyToColorSpaces.keys()) {
            ocioFamilyToColorSpaces[family].sort();
        }

        qDebug() << "OCIO config successfully processed and data populated.";
    } catch (const OCIO::Exception &e) {
        qDebug() << "OCIO Error: ";
    } catch (const std::exception &e) {
        qDebug() << "OCIO Error: ";
    }

    return ocioFamilyToColorSpaces;
}
