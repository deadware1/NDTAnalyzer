#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QResizeEvent>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <opencv2/opencv.hpp>
#include "DicomTagsWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openFile();
    void saveFile();
    void about();
    void zoomIn();
    void zoomOut();
    void fitInView();
    void adjustImage();
    void addTag(); // Слот для добавления тега

private:
    cv::Mat currentImage; // Храните текущее изображение как поле класса для изменений
    QGraphicsPixmapItem* currentPixmapItem; // Храните текущий элемент сцены для обновления изображения

    QGraphicsView* view;
    QSlider* sliderContrast; // Добавленный слайдер для контраста
    QSlider* sliderBrightness; // Добавленный слайдер для яркости
    QLineEdit* editTagKey; // Поле для ввода ключа тега
    QLineEdit* editTagValue; // Поле для ввода значения тега
    QPushButton* addTagButton; // Кнопка для добавления тега
    DicomTagsWidget* tagsWidget;
    QLabel* statusLabel; // Добавленный QLabel для отображения информации в статусной строке
    QMap<QString, QString> extractTags(const QString& infoMsg);
};

#endif // MAINWINDOW_H