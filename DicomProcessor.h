#ifndef DICOMPROCESSOR_H
#define DICOMPROCESSOR_H

#include <QString>
#include <QImage>
#include <QMap>
#include <opencv2/opencv.hpp>
#include <dcmtk/dcmdata/dctk.h>

class DicomProcessor {
public:
    DicomProcessor();
    ~DicomProcessor();
    static QImage processDicom(const QString& fileName, QString& infoMsg);
    static QImage processMonochromeDicom(const QString& fileName);
    static QImage processColorDicom(const QString& fileName);
    static QImage invertImageColors(const QImage& image);
    static bool saveDicom(const cv::Mat& image, const QString& fileName, const QMap<QString, QString>& tags);
    static QMap<QString, QString> extractAllTags(DcmDataset* dataset);
};

#endif // DICOMPROCESSOR_H