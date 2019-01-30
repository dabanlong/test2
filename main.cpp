#include "test9.h"

#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QApplication>
#include <QBoxLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMap>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTreeView>
#include <QtConcurrent>

#include <QtDebug>
#include <functional>
#include <iostream>

class MyFuckingLineEdit : public QLineEdit
{
public:
    MyFuckingLineEdit(QWidget *parent = 0) : QLineEdit(parent)
    {
    }

    void showEvent(QShowEvent *)
    {
        deselect();
    }
};

class TreeItem
{
public:
    explicit TreeItem(const QVector<QVariant> &data, TreeItem *parent = 0);
    ~TreeItem();

    TreeItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    TreeItem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);

private:
    QList<TreeItem *> childItems;
    QVector<QVariant> itemData;
    TreeItem *parentItem;
};

TreeItem::TreeItem(const QVector<QVariant> &data, TreeItem *parent)
{
    parentItem = parent;
    itemData = data;
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

TreeItem *TreeItem::child(int number)
{
    return childItems.value(number);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::childNumber() const
{
    if(parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem *>(this));

    return 0;
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    return itemData.value(column);
}

bool TreeItem::insertChildren(int position, int count, int columns)
{
    if(position < 0 || position > childItems.size())
        return false;

    for(int row = 0; row < count; ++row)
    {
        QVector<QVariant> data(columns);
        TreeItem *item = new TreeItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool TreeItem::insertColumns(int position, int columns)
{
    if(position < 0 || position > itemData.size())
        return false;

    for(int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    foreach(TreeItem *child, childItems)
        child->insertColumns(position, columns);

    return true;
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

bool TreeItem::removeChildren(int position, int count)
{
    if(position < 0 || position + count > childItems.size())
        return false;

    for(int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool TreeItem::removeColumns(int position, int columns)
{
    if(position < 0 || position + columns > itemData.size())
        return false;

    for(int column = 0; column < columns; ++column)
        itemData.remove(position);

    foreach(TreeItem *child, childItems)
        child->removeColumns(position, columns);

    return true;
}

bool TreeItem::setData(int column, const QVariant &value)
{
    if(column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

class TTMDL : public QStyledItemDelegate
{
public:
    TTMDL(QObject *p = nullptr) : QStyledItemDelegate(p){}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        QLineEdit *le = new MyFuckingLineEdit(parent);
        le->deselect();
        le->clearFocus();
        // please clear the fucking focus and de the fucking select, fuck
        return le;
    }

    void setEditorData(QWidget *editor, const QModelIndex &idx) const override
    {
        qDebug() << "void setEditorData(QWidget *editor, const QModelIndex &idx) const override"
                 << editor << idx << idx.data().toString();
        if(QLineEdit *btn = qobject_cast<QLineEdit*>(editor))
        {
            btn->setText(idx.data().toString());
            btn->deselect();
            btn->clearFocus();
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        qDebug() << "void setModelData" << model << index;
        QStyledItemDelegate::setModelData(editor, model, index);
        return;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override
    {
        return QSize(30, 30);
    }

};

class TreeModel : public QAbstractItemModel
{
public:
    TreeModel(const QStringList &headers, const QString &data,
              QObject *parent = 0);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole) override;

    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex()) override;
    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

private:
    void setupModelData(const QStringList &lines, TreeItem *parent);
    TreeItem *getItem(const QModelIndex &index) const;

    TreeItem *rootItem;
};

TreeModel::TreeModel(const QStringList &headers, const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    foreach(QString header, headers)
        rootData << header;

    rootItem = new TreeItem(rootData);
    setupModelData(data.split(QString("\n")), rootItem);
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

int TreeModel::columnCount(const QModelIndex & /* parent */) const
{
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if(index.isValid())
    {
        TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
        if(item)
            return item;
    }
    return rootItem;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);

    TreeItem *childItem = parentItem->child(row);
    if(childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

bool TreeModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if(!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem->parent();

    if(parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool TreeModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if(rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    qDebug()<<"bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)";
    if(role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if(result)
        emit dataChanged(index, index);

    //qDebug()<<"FUCK: "<<TreeModel::index(0, 0, createIndex(0, 0, rootItem));
    //emit dataChanged(TreeModel::index(0, 0),
                     //TreeModel::index(6, 0)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(0, 0), index.sibling(5, 1)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(1, 1), index.sibling(1, 1)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(2, 1), index.sibling(2, 1)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(3, 1), index.sibling(3, 1)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(4, 1), index.sibling(4, 1)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(0, 0), index.sibling(0, 0)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(1, 0), index.sibling(1, 0)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(2, 0), index.sibling(2, 0)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(3, 0), index.sibling(3, 0)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //emit dataChanged(index.sibling(4, 0), index.sibling(4, 0)[>, {Qt::DisplayRole, Qt::EditRole}<]);
    //qDebug()<<"FUCK: "<<TreeModel::index(0, 0).data().toString();
    //qDebug()<<"FUCK: "<<TreeModel::index(1, 0).data().toString();
    //qDebug()<<"FUCK: "<<TreeModel::index(2, 0).data().toString();
    //qDebug()<<"FUCK: "<<TreeModel::index(3, 0).data().toString();
    //qDebug()<<"FUCK: "<<TreeModel::index(4, 0).data().toString();
    //qDebug()<<"FUCK: "<<TreeModel::index(5, 0).data().toString();
    //qDebug()<<"FUCK2: "<<TreeModel::index(0, 1).data().toString();
    //qDebug()<<"FUCK2: "<<TreeModel::index(1, 1).data().toString();
    //qDebug()<<"FUCK2: "<<TreeModel::index(2, 1).data().toString();
    //qDebug()<<"FUCK2: "<<TreeModel::index(3, 1).data().toString();
    //qDebug()<<"FUCK2: "<<TreeModel::index(4, 1).data().toString();
    //qDebug()<<"FUCK2: "<<TreeModel::index(5, 1).data().toString();
    //emit dataChanged(TreeModel::index(0, 0), TreeModel::index(0, 0));
    //emit dataChanged(TreeModel::index(1, 0), TreeModel::index(1, 0));
    //emit dataChanged(TreeModel::index(2, 0), TreeModel::index(2, 0));
    //emit dataChanged(TreeModel::index(3, 0), TreeModel::index(3, 0));
    //emit dataChanged(TreeModel::index(4, 0), TreeModel::index(4, 0));
    //emit dataChanged(TreeModel::index(5, 0), TreeModel::index(5, 0));
    //emit dataChanged(TreeModel::index(0, 1), TreeModel::index(0, 1));
    //emit dataChanged(TreeModel::index(1, 1), TreeModel::index(1, 1));
    //emit dataChanged(TreeModel::index(2, 1), TreeModel::index(2, 1));
    //emit dataChanged(TreeModel::index(3, 1), TreeModel::index(3, 1));
    //emit dataChanged(TreeModel::index(4, 1), TreeModel::index(4, 1));
    //emit dataChanged(TreeModel::index(5, 1), TreeModel::index(5, 1));

    return result;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if(role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if(result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

void TreeModel::setupModelData(const QStringList &lines, TreeItem *parent)
{
    QList<TreeItem *> parents;
    QList<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while(number < lines.count())
    {
        int position = 0;
        while(position < lines[number].length())
        {
            if(lines[number].at(position) != ' ')
                break;
            ++position;
        }

        QString lineData = lines[number].mid(position).trimmed();

        if(!lineData.isEmpty())
        {
            // Read the column data from the rest of the line.
            QStringList columnStrings = lineData.split("\t", QString::SkipEmptyParts);
            QVector<QVariant> columnData;
            for(int column = 0; column < columnStrings.count(); ++column)
                columnData << columnStrings[column];

            if(position > indentations.last())
            {
                // The last child of the current parent is now the new parent
                // unless the current parent has no children.

                if(parents.last()->childCount() > 0)
                {
                    parents << parents.last()->child(parents.last()->childCount() - 1);
                    indentations << position;
                }
            }
            else
            {
                while(position < indentations.last() && parents.count() > 0)
                {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            // Append a new item to the current parent's list of children.
            TreeItem *parent = parents.last();
            parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());
            for(int column = 0; column < columnData.size(); ++column)
                parent->child(parent->childCount() - 1)->setData(column, columnData[column]);
        }

        ++number;
    }
}

class TTM : public QAbstractTableModel
{
public:
    TTM(QObject *parent) : QAbstractTableModel(parent)
    {
        fucker.resize(5);
        for(int i=0; i<5; ++i)
        {
            fucker[i].resize(10);
        }
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return 5;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return 10;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
            return QVariant();
        if(role == Qt::DisplayRole)
        {
            return fucker[index.row()][index.column()];
        }
        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)
    {
        qDebug() << "bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)"
                 << index << value << role;
        fucker[index.row()][index.column()] = value.toInt();
        //emit dataChanged(index(row, column), index(row, column));
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if(!index.isValid())
            return 0;

        Qt::ItemFlags itemflag = QAbstractItemModel::flags(index);
        itemflag |= Qt::ItemIsEditable;
        return itemflag;
    }

    void setValue(int r, int c, int v)
    {
        qDebug() << "setValue" << r << c << v;
        fucker[r][c]++;
    }

private:
    std::vector< std::vector<int> > fucker;
};

void iterate(const QModelIndex &index, const QAbstractItemModel *model,
             const std::function<void(const QModelIndex &, int)> &fun,
             int depth = 0)
{
    if(index.isValid())
        fun(index, depth);
    if(!model->hasChildren(index))
        return;
    auto rows = model->rowCount(index);
    auto cols = model->columnCount(index);
    for(int i = 0; i < rows; ++i)
        for(int j = 0; j < cols; ++j)
            iterate(model->index(i, j, index), model, fun, depth + 1);
}

void iterate(QModelIndex parent)
{
    for(int i=0; i<parent.model()->rowCount(); ++i)
    {
        for(int j=0; j<parent.model()->columnCount(); ++j)
        {
            QModelIndex child = parent.model()->index(i, j, parent);
            if(child.isValid())
            {
                qDebug()<<"CHILD: "<<i<<j<<child.data().toString();
                if(parent.model()->hasChildren(child))
                    iterate(child);
            }
        }
    }
}

void extractStringsFromModel(QAbstractItemView *view, QAbstractItemModel *model, const QModelIndex &parent)
{
    int rowCount = model->rowCount(parent);
    int columnCount = model->columnCount(parent);

    for(int i = 0; i < rowCount; ++i)
    {
        for(int j = 0; j < columnCount; ++j)
        {
            QModelIndex idx = model->index(i, j, parent);

            if(idx.isValid())
            {
                std::cerr<<idx.data().toString().toStdString()<<"    ";
                view->openPersistentEditor(idx);
            }
        }
        std::cerr<<"\n";
    }
}

void internalRecreateEditor(QAbstractItemView *view,
                            QAbstractItemModel *setupModel,
                            const QModelIndex &parent)
{
    qDebug()<<"void internalRecreateEditor(QAbstractItemView *view,";
    int rowCount = setupModel->rowCount(parent);
    for(int i = 0; i < rowCount; ++i)
    {
        QModelIndex idx = setupModel->index(i, 0, parent);
        if(idx.isValid())
        {
                qDebug()<<"RRR start";
            if(!setupModel->hasChildren(idx))
                view->openPersistentEditor(idx.sibling(i, 1));
            internalRecreateEditor(view, setupModel, idx);
                qDebug()<<"RRR end";
        }
    }
}

QWidget* testTreeViewSignal()
{
    //QTableView view;
    //TTM ttm(&view);
    //view.setModel(&ttm);

    QTreeView *view = new CTV;
    QStringList headers;
    headers << QString("Title") << QString("Description");
    QFile file("/home/ubuntu/default.txt");
    file.open(QIODevice::ReadOnly);
    TreeModel *tm = new TreeModel(headers, file.readAll());
    view->setModel(tm);

    view->setItemDelegate(new TTMDL(view));

    QWidget *w = new QWidget;
    QVBoxLayout *ml = new QVBoxLayout(w);
    QPushButton *btn = new QPushButton("TEST", w);

    QSlider *sl = new QSlider(Qt::Horizontal);
    sl->setMinimum(10);
    sl->setMaximum(210);
    sl->setValue(150);
    QObject::connect(sl, &QAbstractSlider::sliderMoved,
                     [=](int value) {
                         qDebug() << "RRRRRRRR : " << value << sl->sliderPosition();
                     });

    ml->addWidget(view);
    ml->addWidget(btn);
    ml->addWidget(sl);

    srand(time(NULL));
    //QObject::connect(&tm, &QAbstractItemModel::dataChanged,
    //[&](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    //{
    //qDebug()<<"RRR: "<<topLeft<<bottomRight<<roles;
    //});
    //QObject::connect(&tm, &QAbstractItemModel::dataChanged,
    //&view, &QAbstractItemView::dataChanged);
    //QObject::connect(&tm, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)),
    //&view, SLOT(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
    QObject::connect(btn, &QAbstractButton::clicked,
                     [&]() {
                         //for(int i=0; i<5000; ++i)
                         //{
                         //int v = rand() % 1000;
                         //int r = rand()%ttm.rowCount();
                         //int c = rand()%ttm.columnCount();
                         //ttm.setValue(r, c, v);
                         //}
                         //emit ttm.dataChanged(ttm.index(r, c), ttm.index(r, c));
                         //emit ttm.dataChanged(ttm.index(1, 2), ttm.index(1, 2));
                         //emit ttm.dataChanged(ttm.index(2, 3), ttm.index(2, 3));
                         //emit ttm.dataChanged(ttm.index(3, 4), ttm.index(3, 4));

                         emit tm->dataChanged(tm->index(0, 0), tm->index(-1, 0));
                         emit tm->dataChanged(tm->index(1, 0), tm->index(1, 0));
                         emit tm->dataChanged(tm->index(2, 0), tm->index(2, 0));
                         emit tm->dataChanged(tm->index(3, 0), tm->index(3, 0));
                         emit tm->dataChanged(tm->index(4, 0), tm->index(4, 0));
                         emit tm->dataChanged(tm->index(5, 0), tm->index(5, 0));
                         emit tm->dataChanged(tm->index(0, 1), tm->index(0, 1));
                         emit tm->dataChanged(tm->index(1, 1), tm->index(1, 1));
                         emit tm->dataChanged(tm->index(2, 1), tm->index(2, 1));
                         emit tm->dataChanged(tm->index(3, 1), tm->index(3, 1));
                         emit tm->dataChanged(tm->index(4, 1), tm->index(4, 1));
                         emit tm->dataChanged(tm->index(5, 1), tm->index(5, 1));
                     });
    //for(int i=0; i<ttm.rowCount(); ++i)
    //{
    //for(int j=0; j<ttm.columnCount(); ++j)
    //{
    //QModelIndex idx = ttm.index(i, j);
    //view.openPersistentEditor(idx);
    //}
    //}

    //extractStringsFromModel(view, view->model(), view->rootIndex());
    view->setFocusPolicy(Qt::NoFocus);
    internalRecreateEditor(view, view->model(), view->rootIndex());
    view->setFocusPolicy(Qt::NoFocus);
    view->clearFocus();

    //QSlider sl;
    //valueSl->setMinimum(0);
    //valueSl->setMaximum((fltNode->GetMax() - fltNode->GetMin()) / inc);
    //valueSl->setValue((fltNode->GetValue() - fltNode->GetMin()) / inc);

    return w;
}

class FUCKWidget : public QWidget
{
public:
    FUCKWidget(QWidget *p) : QWidget(p) {}
    ~FUCKWidget(){qDebug()<<"RRRRRRRRRRRR";}
};

QWidget* testMdiWindow()
{
    QMainWindow *w = new QMainWindow;
    QMdiArea *ma = new QMdiArea(w);
    w->setCentralWidget(ma);
    for(int i=0; i<10; ++i)
    {
        //QPushButton *btn = new QPushButton(QString("Btn%1").arg(i), w);
        QWidget *fw = new FUCKWidget(w);
        QMdiSubWindow *msw = ma->addSubWindow(fw);
        //msw->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(msw, &QMdiSubWindow::windowStateChanged,
                         [=](Qt::WindowStates oldState, Qt::WindowStates newState) {
                             qDebug() << msw << oldState << newState;
                         });
        QObject::connect(msw, &QMdiSubWindow::aboutToActivate,
                         [=]() {
                             qDebug() << msw << "fuck";
                         });
    }
    return w;
}


struct Version {
    const char *str;
    int major;
    int minor;
};

static struct Version versions[] = {
    { "1.0", 1, 0 },
    { "1.1", 1, 1 },
    { "1.2", 1, 2 },
    { "1.3", 1, 3 },
    { "1.4", 1, 4 },
    { "1.5", 1, 5 },
    { "2.0", 2, 0 },
    { "2.1", 2, 1 },
    { "3.0", 3, 0 },
    { "3.1", 3, 1 },
    { "3.2", 3, 2 },
    { "3.3", 3, 3 },
    { "4.0", 4, 0 },
    { "4.1", 4, 1 },
    { "4.2", 4, 2 },
    { "4.3", 4, 3 },
    { "4.4", 4, 4 },
    { "4.5", 4, 5 }
};

static QVariantList get_TZ_list(bool fuck)
{
    QVariantList retList;
    QMap<int, QMap<QString, QTimeZone> > tzList;
    auto availTzList = QTimeZone::availableTimeZoneIds();
    for(auto i:availTzList)
    {
        QTimeZone tz(i);
        int offset = tz.standardTimeOffset(QDateTime::currentDateTimeUtc());
        if(fuck)
        {
            if(tz.hasTransitions())
            {
                auto &tmp_map = tzList[offset];
                QString id = QString::fromLocal8Bit(tz.id());
                tmp_map[id.split("/").last()] = tz;
            }
        }
        else
        {
            auto &tmp_map = tzList[offset];
            QString id = QString::fromLocal8Bit(tz.id());
            tmp_map[id.split("/").last()] = tz;
        }
    }
    for(auto i:tzList)
    {
        for(auto j:i)
        {
            QString name;
            int offset = j.standardTimeOffset(QDateTime::currentDateTimeUtc());
            name = QString::fromLocal8Bit(j.id()).split("/").last();
            name = QString("(UTC%1) %4")
                   .arg(!offset?"":QString("%1%2:%3")
                        .arg(offset>0?"+":"-")
                        .arg(qAbs(offset/3600))
                        .arg(qAbs(offset%3600)/60, 2, 10, QChar('0')))
                   .arg(name);
            retList.push_back(name);
        }
    }
    return retList;
}

int do_magic(int n)
{
    if(n==42)
        return 42;
    return n*2;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qDebug()<<"AAAAAAAAAAAAAAAAAA"<<do_magic(2);
    QTimer::singleShot(10000, &app, &QApplication::quit);
    return app.exec();
}
