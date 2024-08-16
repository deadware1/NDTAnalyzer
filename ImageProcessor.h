#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <opencv2/opencv.hpp>
#include <QImage>
#include <string>

namespace ImageProcessor {

	// Проверка окончания строки
	bool endsWith(const std::string& str, const std::string& suffix);

	// Чтение изображения из файла
	cv::Mat readImageFromRawFile(const std::string& imagePath, QMap<QString, QString>& tags);

	// Преобразование QImage в cv::Mat
	cv::Mat QImageToCvMat(const QImage& inImage, bool inCloneImageData = true);

	// Сохранение изображения в техническом формате
	bool saveImageToRawFormat(const cv::Mat& image, const std::string& filePath, const QMap<QString, QString>& tags);

	// Преобразование QImage в 16-битное серое изображение
	cv::Mat convertTo16BitGrayscale(const QImage& qImage);

} // namespace ImageProcessor

#endif // IMAGEPROCESSOR_H