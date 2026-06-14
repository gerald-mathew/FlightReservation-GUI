#include "PaystackClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

#include <cmath>

namespace {
constexpr const char* kBaseUrl = "https://api.paystack.co";

long long nairaToKobo(double naira) {
    return static_cast<long long>(std::llround(naira * 100.0));
}

double koboToNaira(long long kobo) {
    return static_cast<double>(kobo) / 100.0;
}
} // namespace

PaystackClient::PaystackClient(QObject* parent)
    : QObject(parent), m_secret(loadSecret()) {}

QString PaystackClient::loadSecret() {
    // Mirror the CLI candidate paths (core/payment.cpp loadSecretKey), then also
    // look next to the executable so it works regardless of the launch directory.
    QStringList candidates{
        QStringLiteral("config/paystack_secret.txt"),
        QStringLiteral("../config/paystack_secret.txt"),
        QStringLiteral("../../config/paystack_secret.txt"),
        QDir(QCoreApplication::applicationDirPath()).filePath("config/paystack_secret.txt"),
    };
    for (const QString& path : candidates) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        QTextStream in(&file);
        const QString key = in.readLine().trimmed();
        if (!key.isEmpty())
            return key;
    }
    return {};
}

void PaystackClient::initialize(const QString& email, double naira) {
    if (m_secret.isEmpty()) {
        emit error(QStringLiteral("Paystack secret key not configured (config/paystack_secret.txt)."));
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("email")] = email;
    payload[QStringLiteral("amount")] = static_cast<double>(nairaToKobo(naira));

    QNetworkRequest req(QUrl(QStringLiteral("%1/transaction/initialize").arg(kBaseUrl)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_secret.toUtf8());

    QNetworkReply* reply = m_nam.post(req, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError && body.isEmpty()) {
            emit error(QStringLiteral("Network error: %1").arg(reply->errorString()));
            return;
        }
        const QJsonObject root = QJsonDocument::fromJson(body).object();
        if (!root.value(QStringLiteral("status")).toBool()) {
            emit error(root.value(QStringLiteral("message")).toString(
                QStringLiteral("Paystack initialization failed.")));
            return;
        }
        const QJsonObject data = root.value(QStringLiteral("data")).toObject();
        const QString url = data.value(QStringLiteral("authorization_url")).toString();
        const QString ref = data.value(QStringLiteral("reference")).toString();
        if (url.isEmpty() || ref.isEmpty()) {
            emit error(QStringLiteral("Paystack returned an incomplete response."));
            return;
        }
        emit initialized(ref, url);
    });
}

void PaystackClient::verify(const QString& reference) {
    if (m_secret.isEmpty()) {
        emit error(QStringLiteral("Paystack secret key not configured (config/paystack_secret.txt)."));
        return;
    }

    QNetworkRequest req(QUrl(QStringLiteral("%1/transaction/verify/%2").arg(kBaseUrl, reference)));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + m_secret.toUtf8());

    QNetworkReply* reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, reference]() {
        reply->deleteLater();
        const QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError && body.isEmpty()) {
            emit error(QStringLiteral("Network error: %1").arg(reply->errorString()));
            return;
        }
        const QJsonObject root = QJsonDocument::fromJson(body).object();
        if (!root.value(QStringLiteral("status")).toBool()) {
            emit error(root.value(QStringLiteral("message")).toString(
                QStringLiteral("Paystack verification failed.")));
            return;
        }
        const QJsonObject data = root.value(QStringLiteral("data")).toObject();
        const QString status = data.value(QStringLiteral("status")).toString();
        const bool success = (status == QStringLiteral("success"));
        const long long kobo = static_cast<long long>(data.value(QStringLiteral("amount")).toDouble());
        emit verified(reference, success, koboToNaira(kobo), status);
    });
}
