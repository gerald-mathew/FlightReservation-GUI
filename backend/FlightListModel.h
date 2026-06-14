#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

struct FlightRow {
    QString id;
    QString airline;
    QString origin;
    QString originName;
    QString destination;
    QString destinationName;
    QString departure;
    QString arrival;
    double price = 0.0;
    int availableSeats = 0;
    int totalSeats = 0;
    QString status;
};

class FlightListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        AirlineRole,
        OriginRole,
        OriginNameRole,
        DestinationRole,
        DestinationNameRole,
        DepartureRole,
        ArrivalRole,
        PriceRole,
        AvailableSeatsRole,
        TotalSeatsRole,
        StatusRole
    };

    explicit FlightListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRows(QList<FlightRow> rows);

private:
    QList<FlightRow> m_rows;
};
