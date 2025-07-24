#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QComboBox>
#include <QLabel>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QPoint>
#include <OpenColorIO/OpenColorIO.h>
#include "Viewport.h"

namespace OCIO = OCIO_NAMESPACE;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private:
        void setupUi();
        QGraphicsView* setupViewport();
        QMap<QString, QStringList> loadOcioConfigAndPopulateData();

};
#endif // MAINWINDOW_H