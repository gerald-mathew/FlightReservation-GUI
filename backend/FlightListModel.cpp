#include "FlightListModel.h"

FlightListModel::FlightListModel(QObject* parent) : QAbstractListModel(parent) {}

int FlightListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_rows.size();
}

QVariant FlightListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const FlightRow& r = m_rows.at(index.row());
    switch (role) {
        case IdRole: return r.id;
        case AirlineRole: return r.airline;
        case OriginRole: return r.origin;
        case OriginNameRole: return r.originName;
        case DestinationRole: return r.destination;
        case DestinationNameRole: return r.destinationName;
        case DepartureRole: return r.departure;
        case ArrivalRole: return r.arrival;
        case PriceRole: return r.price;
        case AvailableSeatsRole: return r.availableSeats;
        case TotalSeatsRole: return r.totalSeats;
        case StatusRole: return r.status;
        default: return {};
    }
}

QHash<int, QByteArray> FlightListModel::roleNames() const {
    return {
        { IdRole, "flightId" },
        { AirlineRole, "airline" },
        { OriginRole, "origin" },
        { OriginNameRole, "originName" },
        { DestinationRole, "destination" },
        { DestinationNameRole, "destinationName" },
        { DepartureRole, "departure" },
        { ArrivalRole, "arrival" },
        { PriceRole, "price" },
        { AvailableSeatsRole, "availableSeats" },
        { TotalSeatsRole, "totalSeats" },
        { StatusRole, "status" }
    };
}

void FlightListModel::setRows(QList<FlightRow> rows) {
    beginResetModel();
    m_rows = std::move(rows);
    endResetModel();
}
