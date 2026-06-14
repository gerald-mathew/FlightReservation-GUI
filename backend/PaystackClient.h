#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

// Thin asynchronous wrapper around the two Paystack endpoints the app needs:
//   POST /transaction/initialize  -> authorization URL + reference
//   GET  /transaction/verify/{ref} -> payment status + amount
// Implemented with Qt Networking (no curl dependency). The secret key is read
// once from config/paystack_secret.txt, the same file the CLI build uses.
class PaystackClient : public QObject {
    Q_OBJECT
public:
    explicit PaystackClient(QObject* parent = nullptr);

    bool hasSecret() const { return !m_secret.isEmpty(); }

    // Kicks off an initialize request; results arrive via initialized()/error().
    void initialize(const QString& email, double naira);

    // Kicks off a verify request; results arrive via verified()/error().
    void verify(const QString& reference);

signals:
    void initialized(const QString& reference, const QString& authorizationUrl);
    void verified(const QString& reference, bool success, double naira, const QString& status);
    void error(const QString& message);

private:
    static QString loadSecret();

    QNetworkAccessManager m_nam;
    QString m_secret;
};
