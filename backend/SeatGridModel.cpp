#include "SeatGridModel.h"

SeatGridModel::SeatGridModel(QObject* parent) : QAbstractListModel(parent) {}

int SeatGridModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_rows.size();
}

QVariant SeatGridModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const SeatRow& s = m_rows.at(index.row());
    switch (role) {
        case SeatNumberRole: return s.seatNumber;
        case RowRole: return s.row;
        case LetterRole: return s.letter;
        case CabinRole: return s.cabin;
        case BookedRole: return s.booked;
        case MineRole: return s.mine;
        case PriceRole: return s.price;
        default: return {};
    }
}

QHash<int, QByteArray> SeatGridModel::roleNames() const {
    return {
        { SeatNumberRole, "seatNumber" },
        { RowRole, "seatRow" },
        { LetterRole, "letter" },
        { CabinRole, "cabin" },
        { BookedRole, "booked" },
        { MineRole, "mine" },
        { PriceRole, "price" }
    };
}

void SeatGridModel::setRows(QList<SeatRow> rows) {
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}

void SeatGridModel::clear() {
    beginResetModel();
    m_rows.clear();
    endResetModel();
}
