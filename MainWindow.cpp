#include "MainWindow.h"
#include "ImageProcessor.h"
#include "DicomProcessor.h"
#include "TiffProcessor.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QAction>
#include <QScreen>
#include <QGuiApplication>
#include <QSplitter>
#include <iostream>
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmimgle/dcmimage.h>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    // Устанавливаем фиксированный размер окна
    setFixedSize(800, 600);

    // Располагаем окно в центре экрана
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    // Создаем QGraphicsScene и QGraphicsView
    QGraphicsScene* scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);
    setCentralWidget(view);

    currentPixmapItem = nullptr;

    // Настраиваем QGraphicsView
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    // Устанавливаем размер окна по умолчанию 
    resize(1200, 800);

    // Создаем действия для увеличения и уменьшения
    QAction* zoomInAction = new QAction(tr("Увеличить"), this);
    zoomInAction->setIcon(QIcon(":/icons/zoom-in.png"));
    QAction* zoomOutAction = new QAction(tr("Уменьшить"), this);
    zoomOutAction->setIcon(QIcon(":/icons/zoom-out.png"));

    tagsWidget = new DicomTagsWidget(this);

    // Виджет для добавления тегов
    QLabel* tagKeyLabel = new QLabel(tr("Ключ тега:"), this);
    QLabel* tagValueLabel = new QLabel(tr("Значение тега:"), this);
    editTagKey = new QLineEdit(this);
    editTagValue = new QLineEdit(this);
    addTagButton = new QPushButton(tr("Добавить тег"), this);

    // Лейаут для виджета добавления тегов
    QHBoxLayout* tagLayout = new QHBoxLayout;
    tagLayout->addWidget(tagKeyLabel);
    tagLayout->addWidget(editTagKey);
    tagLayout->addWidget(tagValueLabel);
    tagLayout->addWidget(editTagValue);
    tagLayout->addWidget(addTagButton);

    // Виджет-контейнер для виджета добавления тегов
    QWidget* tagWidget = new QWidget;
    tagWidget->setLayout(tagLayout);

    // Вертикальный лейаут для объединения таблицы тегов и панели добавления тегов
    QVBoxLayout* tagsLayout = new QVBoxLayout;
    tagsLayout->addWidget(tagsWidget);
    tagsLayout->addWidget(tagWidget);

    // Виджет-контейнер для объединенного лейаута
    QWidget* tagsContainer = new QWidget;
    tagsContainer->setLayout(tagsLayout);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(view);
    splitter->addWidget(tagsContainer);

    QList<int> sizes;
    sizes << 800 << 200; // Значения размеров могут быть изменены в соответствии с вашими предпочтениями
    splitter->setSizes(sizes);

    setCentralWidget(splitter);

    // Создаем подписи для слайдеров
    QLabel* labelContrast = new QLabel(tr("Контрастность:"), this);
    QLabel* labelBrightness = new QLabel(tr("Яркость:"), this);

    // Создание слайдера для контраста
    sliderContrast = new QSlider(Qt::Horizontal, this);
    sliderContrast->setRange(0, 200); // Примерный диапазон, можно настроить
    sliderContrast->setValue(100); // Начальное значение (нейтральный контраст)

    // Создание слайдера для яркости
    sliderBrightness = new QSlider(Qt::Horizontal, this);
    sliderBrightness->setRange(-100, 100); // Примерный диапазон, можно настроить
    sliderBrightness->setValue(0); // Начальное значение (нейтральная яркость)

    // В конструкторе MainWindow после создания слайдеров
    connect(sliderContrast, &QSlider::valueChanged, this, &MainWindow::adjustImage);
    connect(sliderBrightness, &QSlider::valueChanged, this, &MainWindow::adjustImage);

    // Добавляем подписи и слайдеры в лейаут
    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(labelContrast);
    layout->addWidget(sliderContrast);
    layout->addWidget(labelBrightness);
    layout->addWidget(sliderBrightness);

    // Создаем виджет и устанавливаем лейаут с подписями и слайдерами
    QWidget* container = new QWidget;
    container->setLayout(layout);

    // Добавление слайдеров в лейаут или тулбар
    QToolBar* adjustToolBar = addToolBar(tr("Регулировка"));
    adjustToolBar->addWidget(container);

    // Подключаем действия к слотам
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    connect(this, &MainWindow::resizeEvent, this, &MainWindow::fitInView);

    // Меню "Файл"
    QMenu* fileMenu = menuBar()->addMenu(tr("&Файл"));
    QAction* openAction = fileMenu->addAction(tr("&Открыть"), this, &MainWindow::openFile);
    QAction* saveAction = fileMenu->addAction(tr("&Сохранить"), this, &MainWindow::saveFile);
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(tr("Вы&ход"), this, &MainWindow::close);

    // Меню "Вид"
    QMenu* viewMenu = menuBar()->addMenu(tr("&Вид"));

    // Меню "Помощь"
    QMenu* helpMenu = menuBar()->addMenu(tr("&Помощь"));
    QAction* aboutAction = helpMenu->addAction(tr("&О программе"), this, &MainWindow::about);

    // Панель инструментов
    QToolBar* toolBar = addToolBar(tr("Инструменты"));
    toolBar->addAction(zoomInAction);
    toolBar->addAction(zoomOutAction);

    // Строка состояния
    statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->showMessage(tr("Готово"));

    // Подключаем кнопку добавления тега к слоту addTag
    connect(addTagButton, &QPushButton::clicked, this, &MainWindow::addTag);
}

MainWindow::~MainWindow()
{
    // Деструктор (если необходим)
}

void MainWindow::addTag() {
    QString key = editTagKey->text().trimmed();
    QString value = editTagValue->text().trimmed();

    if (!key.isEmpty() && !value.isEmpty()) {
        QMap<QString, QString> tags = tagsWidget->getTags();
        tags.insert(key, value);
        tagsWidget->setTags(tags);

        // Очистка полей ввода после добавления тега
        editTagKey->clear();
        editTagValue->clear();
    }
    else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Ключ и значение тега не могут быть пустыми"));
    }
}


QMap<QString, QString> MainWindow::extractTags(const QString& infoMsg) {
    QMap<QString, QString> tagsMap;
    QStringList lines = infoMsg.split("\n"); // Разделение информации на строки

    foreach(const QString & line, lines) {
        if (line.trimmed().isEmpty()) continue; // Пропускаем пустые строки

        // Разделяем строку на ключ и значение по первому вхождению двоеточия
        int splitIndex = line.indexOf(":");
        if (splitIndex == -1) continue; // Если двоеточие не найдено, пропускаем строку

        QString key = line.left(splitIndex).trimmed();
        QString value = line.mid(splitIndex + 1).trimmed();

        if (!key.isEmpty() && !value.isEmpty()) {
            tagsMap.insert(key, value); // Добавляем пару ключ-значение в карту
        }
    }

    return tagsMap;
}


void MainWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть файл"), "", tr("Изображения (*.png *.jpg *.bmp *.tiff *.tif *.raw *.dcm)"));
    if (fileName.isEmpty()) {
        return;
    }

    QPixmap pixmap;
    int imageWidth = 0;
    int imageHeight = 0;
    int bitDepth = 0;
    int dpiX = 0;
    int dpiY = 0;

    try {
        if (fileName.endsWith(".raw", Qt::CaseInsensitive)) {
            QMap<QString, QString> tags;
            cv::Mat cvImage = ImageProcessor::readImageFromRawFile(fileName.toStdString(), tags);
            tagsWidget->setTags(tags.isEmpty() ? QMap<QString, QString>() : tags);
            if (!cvImage.empty()) {
                QImage qImage = QImage(cvImage.data, cvImage.cols, cvImage.rows, static_cast<int>(cvImage.step), QImage::Format_Grayscale16);
                pixmap = QPixmap::fromImage(qImage);
                currentImage = cvImage;
                imageWidth = cvImage.cols;
                imageHeight = cvImage.rows;
                bitDepth = cvImage.elemSize() * 8;
            }
            else {
                throw std::runtime_error("Не удалось открыть изображение .raw");
            }
        }
        else if (fileName.endsWith(".dcm", Qt::CaseInsensitive)) {
            QString infoMsg;
            QImage qImage = DicomProcessor::processDicom(fileName, infoMsg);
            QMap<QString, QString> tags = extractTags(infoMsg);
            tagsWidget->setTags(tags.isEmpty() ? QMap<QString, QString>() : tags);
            if (!qImage.isNull()) {
                pixmap = QPixmap::fromImage(qImage);
                currentImage = ImageProcessor::QImageToCvMat(qImage);
                dpiX = dpiY = 96; // Assuming default DPI for DICOM images
                imageWidth = qImage.width();
                imageHeight = qImage.height();
                bitDepth = qImage.depth();
            }
            else {
                throw std::runtime_error("Не удалось открыть DICOM изображение");
            }
        }
        else if (fileName.endsWith(".tiff", Qt::CaseInsensitive) || fileName.endsWith(".tif", Qt::CaseInsensitive)) {
            cv::Mat cvImage;
            QMap<QString, QString> tags;
            if (TiffProcessor::loadTiffWithTags(fileName, cvImage, tags)) {
                QImage qImage = QImage(cvImage.data, cvImage.cols, cvImage.rows, static_cast<int>(cvImage.step), cvImage.channels() == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888);
                pixmap = QPixmap::fromImage(qImage);
                currentImage = cvImage;
                imageWidth = cvImage.cols;
                imageHeight = cvImage.rows;
                bitDepth = cvImage.elemSize() * 8;
                dpiX = qImage.dotsPerMeterX() * 0.0254; // Convert from dots per meter to DPI
                dpiY = qImage.dotsPerMeterY() * 0.0254; // Convert from dots per meter to DPI
                tagsWidget->setTags(tags.isEmpty() ? QMap<QString, QString>() : tags);
            }
            else {
                throw std::runtime_error("Не удалось открыть изображение .tiff");
            }
        }
        else {
            QImage image;
            if (image.load(fileName)) {
                pixmap = QPixmap::fromImage(image);
                currentImage = ImageProcessor::QImageToCvMat(image);
                imageWidth = image.width();
                imageHeight = image.height();
                bitDepth = image.depth();
                dpiX = image.dotsPerMeterX() * 0.0254; // Convert from dots per meter to DPI
                dpiY = image.dotsPerMeterY() * 0.0254; // Convert from dots per meter to DPI
                tagsWidget->setTags(QMap<QString, QString>());
            }
            else {
                throw std::runtime_error("Не удалось открыть изображение");
            }
        }

        view->scene()->clear();
        view->scene()->addItem(new QGraphicsPixmapItem(pixmap));
        currentPixmapItem = view->scene()->addPixmap(pixmap);
        view->scene()->setSceneRect(pixmap.rect());
        fitInView();

        QString statusMessage = tr("Файл открыт: %1x%2, Глубина цвета: %3 бит, DPI: %4").arg(imageWidth).arg(imageHeight).arg(bitDepth).arg(dpiX);
        statusLabel->setText(statusMessage); // Обновление текста QLabel в статусной строке

        // Обновление заголовка окна с именем файла
        QFileInfo fileInfo(fileName);
        setWindowTitle(tr("Просмотр изображения - %1").arg(fileInfo.fileName()));
    }
    catch (const std::exception& ex) {
        QMessageBox::warning(this, tr("Ошибка"), tr(ex.what()));
    }
}

void MainWindow::saveFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл"), "", tr("Изображения (*.png *.jpg *.bmp *.raw *.dcm *.tiff)"));
    if (!fileName.isEmpty()) {
        QGraphicsPixmapItem* item = qgraphicsitem_cast<QGraphicsPixmapItem*>(view->scene()->items().first());
        if (item) {
            QPixmap pixmap = item->pixmap();
            if (fileName.endsWith(".raw", Qt::CaseInsensitive)) {
                // Получаем изображение из QGraphicsPixmapItem
                QImage qImage = item->pixmap().toImage();
                cv::Mat image = ImageProcessor::convertTo16BitGrayscale(qImage);

                // Сохраняем изображение в формате .raw
                QMap<QString, QString> tags = tagsWidget->getTags();
                if (ImageProcessor::saveImageToRawFormat(image, fileName.toStdString(), tags)) {
                    statusBar()->showMessage(tr("Файл сохранен"), 2000);
                }
                else {
                    QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение в формате .raw"));
                }
            }
            else if (fileName.endsWith(".dcm", Qt::CaseInsensitive)) {
                // Получаем изображение из QGraphicsPixmapItem
                QImage qImage = item->pixmap().toImage();
                cv::Mat image = ImageProcessor::convertTo16BitGrayscale(qImage);

                // Сохраняем изображение в формате DICOM
                QMap<QString, QString> tags = tagsWidget->getTags(); // Предполагается, что tagsWidget имеет метод getTags()
                if (DicomProcessor::saveDicom(image, fileName, tags)) {
                    statusBar()->showMessage(tr("Файл сохранен"), 2000);
                }
                else {
                    QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение в формате DICOM"));
                }
            }
            else if (fileName.endsWith(".tiff", Qt::CaseInsensitive)) {
                // Сохранение в TIFF формате
                QImage qImage = item->pixmap().toImage();
                cv::Mat image = ImageProcessor::convertTo16BitGrayscale(qImage);
                QMap<QString, QString> tags = tagsWidget->getTags();
                if (TiffProcessor::saveTiffWithTags(image, fileName, tags)) {
                    statusBar()->showMessage(tr("Файл сохранен"), 2000);
                }
                else {
                    QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение в формате TIFF"));
                }
            }
            else {
                // Сохраняем изображение в других поддерживаемых форматах
                if (pixmap.save(fileName)) {
                    statusBar()->showMessage(tr("Файл сохранен"), 2000);
                }
                else {
                    QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение"));
                }
            }
        }
        else {
            QMessageBox::warning(this, tr("Ошибка"), tr("Изображение не открыто"));
        }
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("О программе"), tr("Программа-аналог X-Vizor"));
}

void MainWindow::zoomIn() {
    view->scale(1.1, 1.1);
}

void MainWindow::zoomOut() {
    view->scale(0.9, 0.9);
}

void MainWindow::fitInView() {
    view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::adjustImage() {
    double contrastValue = sliderContrast->value() / 50.0;
    double brightnessValue = static_cast<double>(sliderBrightness->value());

    cv::Mat adjustedImage;
    if (currentImage.depth() == CV_8U) {
        currentImage.convertTo(adjustedImage, CV_8U, contrastValue, brightnessValue);
    }
    else if (currentImage.depth() == CV_16U) {
        currentImage.convertTo(adjustedImage, CV_16U, contrastValue, brightnessValue);
    }

    QImage qImage;
    if (adjustedImage.depth() == CV_8U) {
        qImage = QImage((uchar*)adjustedImage.data, adjustedImage.cols, adjustedImage.rows, adjustedImage.step,
            adjustedImage.channels() == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888).copy();
    }
    else if (adjustedImage.depth() == CV_16U) {
        qImage = QImage((uchar*)adjustedImage.data, adjustedImage.cols, adjustedImage.rows, adjustedImage.step,
            QImage::Format_Grayscale16).copy();
    }

    QPixmap pixmap = QPixmap::fromImage(qImage);
    currentPixmapItem->setPixmap(pixmap);
    view->scene()->update();
}

