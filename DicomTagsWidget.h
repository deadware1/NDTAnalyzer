#ifndef DICOMTAGSWIDGET_H
#define DICOMTAGSWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QMap>

class DicomTagsWidget : public QWidget {
    Q_OBJECT

public:
    explicit DicomTagsWidget(QWidget* parent = nullptr);
    void setTags(const QMap<QString, QString>& tags);
    QMap<QString, QString> getTags() const;

private:
    QTableView* tableView;
    QLineEdit* filterEdit;
    QSortFilterProxyModel* proxyModel;
    QStandardItemModel* model;

private slots:
    void filterTags(const QString& text);
};

#endif // DICOMTAGSWIDGET_H