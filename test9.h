#ifndef TEST9_H
#define TEST9_H

#include <QObject>
#include <QTreeView>

#include <QtDebug>

class FFFF : public QObject
{
    Q_OBJECT
public:
    FFFF(QObject *p = nullptr);
    virtual ~FFFF();

public slots:
    void f();
};

class CTV : public QTreeView
{
    Q_OBJECT
public:
    CTV(QWidget *p = nullptr);

public slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
};

#endif
