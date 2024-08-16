#include "DicomTagsWidget.h"
#include <QHeaderView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QMessageBox>

DicomTagsWidget::DicomTagsWidget(QWidget* parent) : QWidget(parent) {
    tableView = new QTableView(this);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setSortingEnabled(true);
    tableView->horizontalHeader()->setStretchLastSection(true);

    model = new QStandardItemModel(this);
    model->setColumnCount(2);
    model->setHorizontalHeaderLabels(QStringList() << "Tag" << "Value");

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterKeyColumn(-1); // Filter all columns

    tableView->setModel(proxyModel);

    filterEdit = new QLineEdit(this);
    connect(filterEdit, &QLineEdit::textChanged, this, &DicomTagsWidget::filterTags);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(filterEdit);
    layout->addWidget(tableView);
    setLayout(layout);
}

void DicomTagsWidget::filterTags(const QString& text) {
    proxyModel->setFilterRegularExpression(QRegularExpression(text, QRegularExpression::CaseInsensitiveOption));
}

void DicomTagsWidget::setTags(const QMap<QString, QString>& tags) {
    model->setRowCount(tags.size());
    int row = 0;
    for (auto it = tags.begin(); it != tags.end(); ++it, ++row) {
        model->setItem(row, 0, new QStandardItem(it.key()));
        model->setItem(row, 1, new QStandardItem(it.value()));
    }
    if (tags.isEmpty()) {
        model->clear();
    }
}

QMap<QString, QString> DicomTagsWidget::getTags() const {
    QMap<QString, QString> tags;
    for (int row = 0; row < model->rowCount(); ++row) {
        QString key = model->item(row, 0)->text();
        QString value = model->item(row, 1)->text();
        tags.insert(key, value);
    }
    return tags;
}