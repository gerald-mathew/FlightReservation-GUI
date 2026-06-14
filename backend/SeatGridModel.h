#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

struct SeatRow {
    QString seatNumber;
    int row = 0;
    QString letter;
    QString cabin;   // Economy / Business / FirstClass
    bool booked = false;
    bool mine = false;
    double price = 0.0;
};

class SeatGridModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        SeatNumberRole = Qt::UserRole + 1,
        RowRole,
        LetterRole,
        CabinRole,
        BookedRole,
        MineRole,
        PriceRole
    };

    explicit SeatGridModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QList<SeatRow> rows);
    void clear();

private:
    QList<SeatRow> m_rows;
};
