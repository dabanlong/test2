#include "test9.h"

FFFF::FFFF(QObject *p)
    :QObject(p)
{
}

FFFF::~FFFF()
{
}

void FFFF::f()
{
    qDebug() << "RRRRRRRRRRRRR";
}

CTV::CTV(QWidget *p)
    :QTreeView(p)
{
}

void CTV::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    qDebug() << "void CTV::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)"
             << topLeft
             << bottomRight
             << roles;
    QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

