#include "DicomProcessor.h"
#include <QMessageBox>
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmimgle/dcmimage.h"   // for DcmImage
#include "dcmtk/dcmdata/dctk.h"        // for DcmFileFormat
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcuid.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcmetinf.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dcistrma.h>
#include <opencv2/opencv.hpp>

DicomProcessor::DicomProcessor() {}

DicomProcessor::~DicomProcessor() {}

unsigned short width, height, bitsAllocated, bitsStored, highBit;
OFString photometricInterpretation;

// Определения тегов
const DcmTagKey DCM_XRaySource = DcmTagKey(0x0018, 0x7040);
const DcmTagKey DCM_ObjectDiameter = DcmTagKey(0x0018, 0x9116);
const DcmTagKey DCM_ObjectMaterial = DcmTagKey(0x0018, 0x9117);
const DcmTagKey DCM_ObjectSeamNumber = DcmTagKey(0x0018, 0x9118);
const DcmTagKey DCM_ObjectThickness = DcmTagKey(0x0018, 0x9119);
const DcmTagKey DCM_InspectionDate = DcmTagKey(0x0018, 0x9121);
const DcmTagKey DCM_InspectionTime = DcmTagKey(0x0018, 0x9122);
const DcmTagKey DCM_InspectionLocation = DcmTagKey(0x0018, 0x9123);

QImage DicomProcessor::processMonochromeDicom(const QString& fileName) {
    QImage qImage;
    std::unique_ptr<DicomImage> dicomImage(new DicomImage(fileName.toStdString().c_str(), CIF_MayDetachPixelData));

    if (dicomImage && dicomImage->getStatus() == EIS_Normal) {
        if (bitsAllocated == 16 && bitsStored <= 12) {
            unsigned short* pixelData16 = (unsigned short*)(dicomImage->getOutputData(16));
            if (pixelData16) {
                QImage qImage16(width, height, QImage::Format_Grayscale16);
                for (int y = 0; y < height; ++y) {
                    unsigned short* line = (unsigned short*)qImage16.scanLine(y);
                    for (int x = 0; x < width; ++x) {
                        line[x] = pixelData16[y * width + x] << (16 - bitsStored);
                    }
                }
                qImage = qImage16.copy();
            }
        }
        else if (photometricInterpretation == "MONOCHROME1" || photometricInterpretation == "MONOCHROME2") {
            uchar* pixelData = (uchar*)(dicomImage->getOutputData(8));
            if (pixelData) {
                qImage = QImage(pixelData, width, height, QImage::Format_Grayscale8).copy();
            }
        }
    }
    else {
        QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось обработать DICOM изображение"));
    }

    return qImage;
}

QImage DicomProcessor::invertImageColors(const QImage& sourceImage) {
    QImage invertedImage = sourceImage.copy();
    int height = invertedImage.height();
    int width = invertedImage.width();

    for (int y = 0; y < height; ++y) {
        uchar* line = invertedImage.scanLine(y);
        for (int x = 0; x < width; ++x) {
            // Получаем указатель на текущий пиксель (3 байта на пиксель для RGB888)
            uchar* pixel = &line[x * 3];
            // Инвертируем цвета (RGB)
            pixel[0] = 255 - pixel[0]; // Red
            pixel[1] = 255 - pixel[1]; // Green
            pixel[2] = 255 - pixel[2]; // Blue
        }
    }
    return invertedImage;
}

QImage DicomProcessor::processColorDicom(const QString& fileName) {
    QImage qImage;

    DcmFileFormat fileFormat;
    OFCondition status = fileFormat.loadFile(fileName.toStdString().c_str());

    if (status.good()) {
        DcmDataset* dataset = fileFormat.getDataset();

        // Извлекаем информацию о пикселях
        unsigned short samplesPerPixel;
        dataset->findAndGetUint16(DCM_SamplesPerPixel, samplesPerPixel);

        if (samplesPerPixel == 3) {
            // Предполагаем, что изображение в формате RGB
            DcmElement* element = nullptr;
            status = dataset->findAndGetElement(DCM_PixelData, element);
            if (status.good() && element != nullptr) {
                Uint8* pixelData = nullptr;
                element->getUint8Array(pixelData);

                if (pixelData) {
                    // Создаем QImage из пиксельных данных
                    qImage = QImage(width, height, QImage::Format_RGB888);
                    Uint8* src = pixelData; // Указатель на исходные данные пикселей
                    for (unsigned int y = 0; y < height; ++y) {
                        uchar* dest = qImage.scanLine(y);
                        for (unsigned int x = 0; x < width; ++x) {
                            // Копируем пиксельные данные
                            *dest++ = *src++; // Red
                            *dest++ = *src++; // Green
                            *dest++ = *src++; // Blue
                        }
                    }
                }
            }
            qImage = invertImageColors(qImage);
        }
        else
            //if (samplesPerPixel == 1) 
        {
            // Обработка монохромного изображения, которое было ошибочно определено как цветное
            DcmElement* element = nullptr;
            status = dataset->findAndGetElement(DCM_PixelData, element);
            if (status.good() && element != nullptr) {
                Uint16* pixelData = nullptr;
                element->getUint16Array(pixelData);

                if (pixelData) {
                    // Создаем QImage из пиксельных данных
                    qImage = QImage(width, height, QImage::Format_Grayscale16);
                    for (int y = 0; y < height; ++y) {
                        unsigned short* line = (unsigned short*)qImage.scanLine(y);
                        for (int x = 0; x < width; ++x) {
                            line[x] = pixelData[y * width + x];
                        }
                    }
                }
            }
        }

    }
    else {
        QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось обработать цветное DICOM изображение"));
    }
    qImage = qImage.convertToFormat(QImage::Format_Grayscale8);
    return qImage;
}


QMap<QString, QString> DicomProcessor::extractAllTags(DcmDataset* dataset) {
    QMap<QString, QString> tags;

    // Основные теги
    OFString value;
    if (dataset->findAndGetOFString(DCM_PatientName, value).good()) {
        tags.insert("Название объекта", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PatientID, value).good()) {
        tags.insert("ID объекта", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_StudyDate, value).good()) {
        tags.insert("Дата производства", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_Modality, value).good()) {
        tags.insert("Модальность", value.c_str());
    }

    // Дополнительные теги
    if (dataset->findAndGetOFString(DCM_Manufacturer, value).good()) {
        tags.insert("Производитель", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_StudyDescription, value).good()) {
        tags.insert("Описание исследования", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_SeriesDescription, value).good()) {
        tags.insert("Описание серии", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PatientBirthDate, value).good()) {
        tags.insert("Дата рождения объекта", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PatientSex, value).good()) {
        tags.insert("Пол объекта", value.c_str());
    }

    OFString xRaySource, detectorType;
    dataset->findAndGetOFString(DCM_XRaySource, xRaySource);
    tags.insert("Рентгеновский источник", xRaySource.c_str());
    dataset->findAndGetOFString(DCM_DetectorType, detectorType);
    tags.insert("Тип детектора", detectorType.c_str());

    Uint16 kV, mA, exposureTime;
    dataset->findAndGetUint16(DCM_KVP, kV);
    tags.insert("Напряжение (кВ)", QString::number(kV));
    dataset->findAndGetUint16(DCM_ExposureTime, exposureTime);
    tags.insert("Время экспозиции (мс)", QString::number(exposureTime));

    OFString objectDiameter, objectMaterial, objectSeamNumber, objectThickness;
    dataset->findAndGetOFString(DCM_ObjectDiameter, objectDiameter);
    tags.insert("Диаметр объекта (мм)", objectDiameter.c_str());
    dataset->findAndGetOFString(DCM_ObjectMaterial, objectMaterial);
    tags.insert("Материал объекта", objectMaterial.c_str());
    dataset->findAndGetOFString(DCM_ObjectSeamNumber, objectSeamNumber);
    tags.insert("Номер шва", objectSeamNumber.c_str());
    dataset->findAndGetOFString(DCM_ObjectThickness, objectThickness);
    tags.insert("Толщина объекта (мм)", objectThickness.c_str());

    OFString inspectionDate, inspectionTime, inspectionLocation;
    dataset->findAndGetOFString(DCM_InspectionDate, inspectionDate);
    tags.insert("Дата проведения", inspectionDate.c_str());
    dataset->findAndGetOFString(DCM_InspectionTime, inspectionTime);
    tags.insert("Время проведения", inspectionTime.c_str());
    dataset->findAndGetOFString(DCM_InspectionLocation, inspectionLocation);
    tags.insert("Место проведения", inspectionLocation.c_str());

    OFString inspector1, inspector2;
    dataset->findAndGetOFString(DCM_ReferringPhysicianName, inspector1);
    tags.insert("Дефектоскопист 1", inspector1.c_str());
    dataset->findAndGetOFString(DCM_ResponsiblePerson, inspector2);
    tags.insert("Дефектоскопист 2", inspector2.c_str());

    dataset->findAndGetUint16(DCM_Rows, height);
    tags.insert("Количество строк", QString::number(height));
    dataset->findAndGetUint16(DCM_Columns, width);
    tags.insert("Количество столбцов", QString::number(width));
    dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated);
    tags.insert("Биты на пиксель", QString::number(bitsAllocated));
    dataset->findAndGetUint16(DCM_BitsStored, bitsStored);
    tags.insert("Биты сохранены", QString::number(bitsStored));
    dataset->findAndGetUint16(DCM_HighBit, highBit);
    tags.insert("Старший бит", QString::number(highBit));
    dataset->findAndGetOFString(DCM_PhotometricInterpretation, photometricInterpretation);
    tags.insert("Фотометрическая интерпретация", photometricInterpretation.c_str());

    return tags;
}

QImage DicomProcessor::processDicom(const QString& fileName, QString& infoMsg) {
    DcmFileFormat fileFormat;

    OFCondition status = fileFormat.loadFile(fileName.toStdString().c_str());
    QImage qImage;

    if (status.good()) {
        DcmDataset* dataset = fileFormat.getDataset();

        QMap<QString, QString> tags = extractAllTags(dataset);

        infoMsg = "Информация о DICOM файле:\n";
        for (auto it = tags.begin(); it != tags.end(); ++it) {
            infoMsg += QString("%1: %2\n").arg(it.key()).arg(it.value());
        }

        E_TransferSyntax xfer = fileFormat.getDataset()->getOriginalXfer();
        DicomImage* dicomImage = new DicomImage(fileName.toStdString().c_str(), xfer);
        if (dicomImage != nullptr) {
            if (dicomImage->isMonochrome()) {
                qImage = processMonochromeDicom(fileName);
            }
            else {
                qImage = processColorDicom(fileName);
            }
        }
        else {
            infoMsg = QObject::tr("Ошибка: Не удалось создать объект DicomImage");
        }
        delete dicomImage;
    }
    else {
        infoMsg = QObject::tr("Ошибка: Не удалось загрузить DICONDE файл: ") + QString::fromStdString(status.text());
    }

    return qImage;
}

bool DicomProcessor::saveDicom(const cv::Mat& image, const QString& fileName, const QMap<QString, QString>& tags) {
    DcmFileFormat fileFormat;
    DcmDataset* dataset = fileFormat.getDataset();

    // Установка основных DICOM тегов
    //dataset->putAndInsertString(DCM_PatientName, "Anonymous");
   // dataset->putAndInsertString(DCM_PatientID, "12345");
    dataset->putAndInsertString(DCM_SOPClassUID, UID_SecondaryCaptureImageStorage);

    // Установка пользовательских тегов
    dataset->putAndInsertString(DCM_PatientName, tags["Название объекта"].toStdString().c_str());
    dataset->putAndInsertString(DCM_PatientID, tags["ID объекта"].toStdString().c_str());
    dataset->putAndInsertString(DCM_StudyDate, tags["Дата производства"].toStdString().c_str());
    dataset->putAndInsertString(DCM_Modality, tags["Модальность"].toStdString().c_str());
    dataset->putAndInsertString(DCM_Manufacturer, tags["Производитель"].toStdString().c_str());
    dataset->putAndInsertString(DCM_StudyDescription, tags["Описание исследования"].toStdString().c_str());
    dataset->putAndInsertString(DCM_SeriesDescription, tags["Описание серии"].toStdString().c_str());
    dataset->putAndInsertString(DCM_PatientBirthDate, tags["Дата рождения объекта"].toStdString().c_str());
    dataset->putAndInsertString(DCM_PatientSex, tags["Пол объекта"].toStdString().c_str());
    dataset->putAndInsertString(DCM_XRaySource, tags["Рентгеновский источник"].toStdString().c_str());
    dataset->putAndInsertString(DCM_DetectorType, tags["Тип детектора"].toStdString().c_str());
    dataset->putAndInsertUint16(DCM_KVP, tags["Напряжение (кВ)"].toInt());
    dataset->putAndInsertUint16(DCM_ExposureTime, tags["Время экспозиции (мс)"].toInt());
    dataset->putAndInsertString(DCM_ObjectDiameter, tags["Диаметр объекта (мм)"].toStdString().c_str());
    dataset->putAndInsertString(DCM_ObjectMaterial, tags["Материал объекта"].toStdString().c_str());
    dataset->putAndInsertString(DCM_ObjectSeamNumber, tags["Номер шва"].toStdString().c_str());
    dataset->putAndInsertString(DCM_ObjectThickness, tags["Толщина объекта (мм)"].toStdString().c_str());
    dataset->putAndInsertString(DCM_InspectionDate, tags["Дата проведения"].toStdString().c_str());
    dataset->putAndInsertString(DCM_InspectionTime, tags["Время проведения"].toStdString().c_str());
    dataset->putAndInsertString(DCM_InspectionLocation, tags["Место проведения"].toStdString().c_str());
    dataset->putAndInsertString(DCM_ReferringPhysicianName, tags["Дефектоскопист 1"].toStdString().c_str());
    dataset->putAndInsertString(DCM_ResponsiblePerson, tags["Дефектоскопист 2"].toStdString().c_str());
    dataset->putAndInsertUint16(DCM_Rows, tags["Количество строк"].toInt());
    dataset->putAndInsertUint16(DCM_Columns, tags["Количество столбцов"].toInt());
    dataset->putAndInsertUint16(DCM_BitsAllocated, tags["Биты на пиксель"].toInt());
    dataset->putAndInsertUint16(DCM_BitsStored, tags["Биты сохранены"].toInt());
    dataset->putAndInsertUint16(DCM_HighBit, tags["Старший бит"].toInt());
    dataset->putAndInsertString(DCM_PhotometricInterpretation, tags["Фотометрическая интерпретация"].toStdString().c_str());

    // Установка размера изображения
    dataset->putAndInsertUint16(DCM_Rows, image.rows);
    dataset->putAndInsertUint16(DCM_Columns, image.cols);
    dataset->putAndInsertUint16(DCM_BitsAllocated, image.elemSize1() * 8); // Установка BitsAllocated в зависимости от размера элемента
    dataset->putAndInsertUint16(DCM_BitsStored, image.elemSize1() * 8); // Установка BitsStored в зависимости от размера элемента
    dataset->putAndInsertUint16(DCM_HighBit, (image.elemSize1() * 8) - 1); // Установка HighBit
    dataset->putAndInsertUint16(DCM_PixelRepresentation, 0); // 0 для unsigned данных

    // Установка данных пикселей
    dataset->putAndInsertUint16Array(DCM_PixelData, (Uint16*)image.data, image.total() * image.channels());

    // Сохранение файла
    OFCondition status = fileFormat.saveFile(fileName.toStdString().c_str(), EXS_LittleEndianExplicit);
    if (status.good()) {
        return true;
    }
    else {
        return false;
    }
}