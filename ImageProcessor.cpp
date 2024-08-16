#include "ImageProcessor.h"
#include <fstream>
#include <iostream>
#include <QJsonDocument>
#include <QJsonObject>

namespace ImageProcessor {

    bool endsWith(const std::string& str, const std::string& suffix) {
        if (str.size() < suffix.size()) return false;
        return std::mismatch(suffix.rbegin(), suffix.rend(), str.rbegin()).first == suffix.rend();
    };

    cv::Mat readImageFromRawFile(const std::string& imagePath, QMap<QString, QString>& tags) {
        std::ifstream file(imagePath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл: " + imagePath);
        }

        uint16_t width, height;
        file.read(reinterpret_cast<char*>(&height), sizeof(height));
        file.read(reinterpret_cast<char*>(&width), sizeof(width));

        cv::Mat image(height, width, CV_16UC1);
        file.read(reinterpret_cast<char*>(image.data), width * height * sizeof(uint16_t));

        // Проверка на конец файла
        if (file.eof()) {
            throw std::runtime_error("Файл поврежден или имеет неверный формат: " + imagePath);
        }

        // Позиционирование на конец файла для чтения длины JSON
        file.seekg(-static_cast<int>(sizeof(uint32_t)), std::ios::end);
        uint32_t jsonLength;
        file.read(reinterpret_cast<char*>(&jsonLength), sizeof(jsonLength));

        // Проверка корректности длины JSON
        std::streamoff fileSize = file.tellg();
        if (jsonLength <= 0 || jsonLength > fileSize - sizeof(uint32_t) - width * height * sizeof(uint16_t) - 2 * sizeof(uint16_t)) {
            // Некорректная длина JSON, возвращаем изображение без тегов
            file.close();
            return image;
        }

        // Чтение JSON строки
        file.seekg(-static_cast<int>(sizeof(uint32_t) + jsonLength), std::ios::end);
        std::vector<char> jsonString(jsonLength);
        file.read(jsonString.data(), jsonLength);

        // Проверка на конец файла после чтения JSON
        if (file.eof()) {
            throw std::runtime_error("Файл поврежден или имеет неверный формат: " + imagePath);
        }

        QJsonDocument doc = QJsonDocument::fromJson(QByteArray(jsonString.data(), jsonLength));
        if (doc.isNull() || !doc.isObject()) {
            // Ошибка парсинга JSON, возвращаем изображение без тегов
            file.close();
            return image;
        }

        QJsonObject json = doc.object();
        for (auto it = json.begin(); it != json.end(); ++it) {
            tags.insert(it.key(), it.value().toString());
        }

        file.close();

        double minVal, maxVal;
        cv::minMaxLoc(image, &minVal, &maxVal);
        if (minVal < 0 || maxVal > 65535) {
            throw std::runtime_error("Ошибка: значения пикселей выходят за пределы 16-битного диапазона.");
        }

        return image;
    }

    bool saveImageToRawFormat(const cv::Mat& image, const std::string& filePath, const QMap<QString, QString>& tags) {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        uint16_t width = static_cast<uint16_t>(image.cols);
        uint16_t height = static_cast<uint16_t>(image.rows);

        file.write(reinterpret_cast<const char*>(&height), sizeof(height));
        file.write(reinterpret_cast<const char*>(&width), sizeof(width));
        file.write(reinterpret_cast<const char*>(image.data), width * height * sizeof(uint16_t));

        // Подготовка JSON строки с тегами
        QJsonObject json;
        for (auto it = tags.begin(); it != tags.end(); ++it) {
            json.insert(it.key(), it.value());
        }
        QJsonDocument doc(json);
        std::string jsonString = doc.toJson(QJsonDocument::Compact).toStdString();
        uint32_t jsonLength = static_cast<uint32_t>(jsonString.size());

        // Запись JSON строки
        file.write(jsonString.c_str(), jsonLength);

        // Запись длины JSON строки в самом конце файла
        file.write(reinterpret_cast<const char*>(&jsonLength), sizeof(jsonLength));

        file.close();
        return true;
    }

    // Преобразование QImage в cv::Mat
    cv::Mat QImageToCvMat(const QImage& inImage, bool inCloneImageData) {
        switch (inImage.format()) {
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied: {
            cv::Mat mat(inImage.height(), inImage.width(), CV_8UC4, const_cast<uchar*>(inImage.bits()), static_cast<size_t>(inImage.bytesPerLine()));
            return (inCloneImageData ? mat.clone() : mat);
        }
        case QImage::Format_RGB32: {
            if (!inCloneImageData) {
                std::cerr << "QImageToCvMat() - Conversion requires cloning because we use a temporary QImage" << std::endl;
            }
            QImage swapped = inImage.rgbSwapped();
            return cv::Mat(swapped.height(), swapped.width(), CV_8UC3, const_cast<uchar*>(swapped.bits()), static_cast<size_t>(swapped.bytesPerLine())).clone();
        }
        case QImage::Format_RGB888: {
            if (!inCloneImageData) {
                std::cerr << "QImageToCvMat() - Conversion requires cloning because we use a temporary QImage" << std::endl;
            }
            QImage swapped = inImage.rgbSwapped();
            return cv::Mat(swapped.height(), swapped.width(), CV_8UC3, const_cast<uchar*>(swapped.bits()), static_cast<size_t>(swapped.bytesPerLine())).clone();
        }
        case QImage::Format_Indexed8:
        case QImage::Format_Grayscale8: {
            cv::Mat mat(inImage.height(), inImage.width(), CV_8UC1, const_cast<uchar*>(inImage.bits()), static_cast<size_t>(inImage.bytesPerLine()));
            return (inCloneImageData ? mat.clone() : mat);
        }
        default:
            std::cerr << "QImageToCvMat() - QImage format not handled in switch: " << inImage.format() << std::endl;
            break;
        }

        return cv::Mat();
    }

    cv::Mat convertTo16BitGrayscale(const QImage& qImage) {
        cv::Mat image;

        // Преобразование QImage в cv::Mat
        switch (qImage.format()) {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32:
        case QImage::Format_ARGB32_Premultiplied:
            image = cv::Mat(qImage.height(), qImage.width(), CV_8UC4, const_cast<uchar*>(qImage.bits()), qImage.bytesPerLine()).clone();
            cv::cvtColor(image, image, cv::COLOR_BGRA2BGR);
            break;
        case QImage::Format_RGB888:
            image = cv::Mat(qImage.height(), qImage.width(), CV_8UC3, const_cast<uchar*>(qImage.bits()), qImage.bytesPerLine()).clone();
            cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
            break;
        case QImage::Format_Grayscale8:
            image = cv::Mat(qImage.height(), qImage.width(), CV_8UC1, const_cast<uchar*>(qImage.bits()), qImage.bytesPerLine()).clone();
            break;
        default:
            throw std::runtime_error("Неподдерживаемый формат изображения");
        }

        // Преобразование в 16-битный серый формат
        cv::Mat image16Bit;
        if (image.type() == CV_8UC1) {
            // Если изображение уже в градациях серого, просто конвертируем его в 16-бит
            image.convertTo(image16Bit, CV_16U, 65535.0 / 255.0);
        }
        else {
            // Если изображение в цвете, сначала переводим в градации серого, затем в 16-бит
            cv::Mat imageGray;
            cv::cvtColor(image, imageGray, cv::COLOR_BGR2GRAY);
            imageGray.convertTo(image16Bit, CV_16U, 65535.0 / 255.0);
        }

        return image16Bit;
    }

} // namespace ImageProcessor