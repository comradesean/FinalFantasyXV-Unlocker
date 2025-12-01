#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QMap>
#include <QDir>
#include <functional>

class HttpServer : public QObject {
    Q_OBJECT

public:
    explicit HttpServer(QObject* parent = nullptr);
    ~HttpServer();

    // Server control
    bool start(quint16 port = 443);
    void stop();
    bool isRunning() const;
    quint16 port() const;

    // Configuration
    void setWebRoot(const QString& path);
    QString webRoot() const;

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void requestReceived(const QString& method, const QString& path);
    void errorOccurred(const QString& error);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer* m_server = nullptr;
    QString m_webRoot;
    quint16 m_port = 443;

    // HTTP handling
    void handleRequest(QTcpSocket* socket, const QByteArray& request);
    void sendResponse(QTcpSocket* socket, int statusCode, const QString& statusText,
                      const QByteArray& body, const QString& contentType = "text/html");
    void sendFile(QTcpSocket* socket, const QString& filePath);
    void sendRedirect(QTcpSocket* socket, const QString& location);

    // Route handlers
    void handleOAuth2Authorize(QTcpSocket* socket, const QMap<QString, QString>& params,
                                const QMap<QString, QString>& headers);
    void handleLogin(QTcpSocket* socket);
    void handleBlog(QTcpSocket* socket);
    void handleGoodsRequest(QTcpSocket* socket);
    void handleStaticFile(QTcpSocket* socket, const QString& path);

    // Utility
    QString getMimeType(const QString& filePath);
    QMap<QString, QString> parseQueryString(const QString& query);
    QMap<QString, QString> parseHeaders(const QStringList& headerLines);
    QString urlDecode(const QString& input);
};
