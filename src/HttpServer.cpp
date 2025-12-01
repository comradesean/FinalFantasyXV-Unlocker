/**
 * @file HttpServer.cpp
 * @brief Minimal HTTP server for Twitch Prime URL spoofing
 *
 * This server intercepts FFXV's Twitch Prime authentication by:
 * 1. Serving a fake OAuth2 authorize endpoint
 * 2. Providing a fake goods/entitlement API response
 * 3. Serving cached blog/promotional pages from embedded resources
 *
 * The game's Twitch URLs are patched to point to localhost:443, and this
 * server responds with the appropriate content to simulate a successful
 * Twitch Prime linkage.
 *
 * All static files are served from Qt embedded resources (:/wwwroot).
 */

#include "HttpServer.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {
    // MIME type mapping for common web file extensions
    const QMap<QString, QString> MIME_TYPES = {
        {"html", "text/html"},
        {"htm",  "text/html"},
        {"css",  "text/css"},
        {"js",   "application/javascript"},
        {"json", "application/json"},
        {"png",  "image/png"},
        {"jpg",  "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif",  "image/gif"},
        {"svg",  "image/svg+xml"},
        {"ico",  "image/x-icon"},
        {"woff", "font/woff"},
        {"woff2","font/woff2"},
        {"ttf",  "font/ttf"},
        {"eot",  "application/vnd.ms-fontobject"}
    };
}

// ============================================================================
// Construction / Destruction
// ============================================================================

HttpServer::HttpServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);

    // Serve from Qt embedded resources
    m_webRoot = ":/wwwroot";
}

HttpServer::~HttpServer()
{
    stop();
}

// ============================================================================
// Server Control
// ============================================================================

bool HttpServer::start(quint16 port)
{
    if (m_server->isListening()) {
        stop();
    }

    m_port = port;

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit errorOccurred("Failed to start server: " + m_server->errorString());
        return false;
    }

    emit serverStarted(port);
    return true;
}

void HttpServer::stop()
{
    if (m_server->isListening()) {
        m_server->close();
        emit serverStopped();
    }
}

bool HttpServer::isRunning() const
{
    return m_server->isListening();
}

quint16 HttpServer::port() const
{
    return m_port;
}

void HttpServer::setWebRoot(const QString& path)
{
    m_webRoot = path;
}

QString HttpServer::webRoot() const
{
    return m_webRoot;
}

// ============================================================================
// Connection Handling
// ============================================================================

void HttpServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);
    }
}

void HttpServer::onReadyRead()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray request = socket->readAll();
    handleRequest(socket, request);
}

void HttpServer::onDisconnected()
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        socket->deleteLater();
    }
}

// ============================================================================
// Request Routing
// ============================================================================

void HttpServer::handleRequest(QTcpSocket* socket, const QByteArray& request)
{
    QString requestStr = QString::fromUtf8(request);
    QStringList lines = requestStr.split("\r\n");

    if (lines.isEmpty()) {
        sendResponse(socket, 400, "Bad Request", "Invalid request");
        return;
    }

    // Parse: "GET /path HTTP/1.1"
    QStringList requestLine = lines[0].split(' ');
    if (requestLine.size() < 2) {
        sendResponse(socket, 400, "Bad Request", "Invalid request line");
        return;
    }

    QString method = requestLine[0];
    QString fullPath = requestLine[1];

    // Separate path and query string
    QString path = fullPath;
    QString queryString;
    int queryIndex = fullPath.indexOf('?');
    if (queryIndex != -1) {
        path = fullPath.left(queryIndex);
        queryString = fullPath.mid(queryIndex + 1);
    }

    QMap<QString, QString> headers = parseHeaders(lines.mid(1));
    emit requestReceived(method, path);

    // Route to appropriate handler
    if (path == "/kraken/oauth2/authorize" && method == "GET") {
        handleOAuth2Authorize(socket, parseQueryString(queryString), headers);
    }
    else if (path == "/login" && method == "GET") {
        handleLogin(socket);
    }
    else if (path == "/twitch-prime-members-get-your-own-kooky-chocobo-more-in-final-fantasy-xv-windows-edition-87d04c6ae217" && method == "GET") {
        handleBlog(socket);
    }
    else if (path == "/kraken/commerce/user/goods" && method == "POST") {
        handleGoodsRequest(socket);
    }
    else {
        handleStaticFile(socket, path);
    }
}

// ============================================================================
// API Endpoint Handlers
// ============================================================================

/**
 * @brief Handles OAuth2 authorization redirect
 *
 * FFXV expects to be redirected to Twitch's OAuth2 flow. We intercept this
 * and redirect to our local login page which will simulate a successful auth.
 */
void HttpServer::handleOAuth2Authorize(QTcpSocket* socket,
                                        const QMap<QString, QString>& params,
                                        const QMap<QString, QString>& headers)
{
    if (!params.contains("client_id") || !params.contains("response_type")) {
        sendResponse(socket, 400, "Bad Request", "Missing required parameters");
        return;
    }

    QString clientId = params["client_id"];

    // Build redirect URL with original params encoded
    QStringList paramList;
    for (auto it = params.begin(); it != params.end(); ++it) {
        paramList << it.key() + "=" + it.value();
    }
    QString redirectParams = QUrl::toPercentEncoding(paramList.join("&"));

    QString loginUrl = QString("http://localhost:%1/login?client_id=%2&redirect_params=%3")
                           .arg(m_port)
                           .arg(clientId)
                           .arg(redirectParams);

    // curl/API clients get HTML link, browsers get redirect
    QString userAgent = headers.value("User-Agent", "");
    QString accept = headers.value("Accept", "");

    if (userAgent.contains("curl") || accept.contains("application/json")) {
        QString html = QString("<a href=\"%1\">Found</a>").arg(loginUrl.toHtmlEscaped());
        sendResponse(socket, 200, "OK", html.toUtf8(), "text/html");
    } else {
        sendRedirect(socket, loginUrl);
    }
}

void HttpServer::handleLogin(QTcpSocket* socket)
{
    sendFile(socket, m_webRoot + "/login.html");
}

void HttpServer::handleBlog(QTcpSocket* socket)
{
    sendFile(socket, m_webRoot + "/twitch-prime-members-get-your-own-kooky-chocobo-more-in-final-fantasy-xv-windows-edition-87d04c6ae217.html");
}

/**
 * @brief Returns fake Twitch Prime goods/entitlements
 *
 * FFXV queries this endpoint to check which Twitch Prime items the user owns.
 * We return all three SKUs to unlock all Twitch Prime content.
 */
void HttpServer::handleGoodsRequest(QTcpSocket* socket)
{
    QJsonObject response;
    QJsonArray goods;

    // All three Twitch Prime item SKUs
    QJsonObject item1; item1["sku"] = "FFXV_TP_001"; goods.append(item1);
    QJsonObject item2; item2["sku"] = "FFXV_TP_002"; goods.append(item2);
    QJsonObject item3; item3["sku"] = "FFXV_TP_003"; goods.append(item3);

    response["goods"] = goods;

    QJsonDocument doc(response);
    sendResponse(socket, 200, "OK", doc.toJson(QJsonDocument::Compact), "application/json");
}

// ============================================================================
// Static File Serving
// ============================================================================

void HttpServer::handleStaticFile(QTcpSocket* socket, const QString& path)
{
    QString filePath = m_webRoot + path;

    // Directory requests default to index.html
    if (filePath.endsWith('/')) {
        filePath += "index.html";
    }

    // Prevent directory traversal attacks
    if (path.contains("..")) {
        sendResponse(socket, 403, "Forbidden", "Access denied");
        return;
    }

    sendFile(socket, filePath);
}

void HttpServer::sendFile(QTcpSocket* socket, const QString& filePath)
{
    QFile file(filePath);

    if (!file.exists()) {
        sendResponse(socket, 404, "Not Found", "File not found: " + filePath.toUtf8());
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        sendResponse(socket, 500, "Internal Server Error", "Cannot read file");
        return;
    }

    QByteArray content = file.readAll();
    file.close();

    QString mimeType = getMimeType(filePath);
    sendResponse(socket, 200, "OK", content, mimeType);
}

// ============================================================================
// Response Helpers
// ============================================================================

void HttpServer::sendResponse(QTcpSocket* socket, int statusCode, const QString& statusText,
                               const QByteArray& body, const QString& contentType)
{
    QString response = QString("HTTP/1.1 %1 %2\r\n"
                               "Content-Type: %3\r\n"
                               "Content-Length: %4\r\n"
                               "Connection: close\r\n"
                               "\r\n")
                           .arg(statusCode)
                           .arg(statusText)
                           .arg(contentType)
                           .arg(body.size());

    socket->write(response.toUtf8());
    socket->write(body);
    socket->flush();
    socket->disconnectFromHost();
}

void HttpServer::sendRedirect(QTcpSocket* socket, const QString& location)
{
    QString response = QString("HTTP/1.1 302 Found\r\n"
                               "Location: %1\r\n"
                               "Connection: close\r\n"
                               "\r\n")
                           .arg(location);

    socket->write(response.toUtf8());
    socket->flush();
    socket->disconnectFromHost();
}

// ============================================================================
// Parsing Helpers
// ============================================================================

QString HttpServer::getMimeType(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();
    return MIME_TYPES.value(ext, "application/octet-stream");
}

QMap<QString, QString> HttpServer::parseQueryString(const QString& query)
{
    QMap<QString, QString> params;

    QStringList pairs = query.split('&', Qt::SkipEmptyParts);
    for (const QString& pair : pairs) {
        int eqIndex = pair.indexOf('=');
        if (eqIndex != -1) {
            QString key = urlDecode(pair.left(eqIndex));
            QString value = urlDecode(pair.mid(eqIndex + 1));
            params[key] = value;
        } else {
            params[urlDecode(pair)] = "";
        }
    }

    return params;
}

QMap<QString, QString> HttpServer::parseHeaders(const QStringList& headerLines)
{
    QMap<QString, QString> headers;

    for (const QString& line : headerLines) {
        if (line.isEmpty()) break;  // Empty line marks end of headers

        int colonIndex = line.indexOf(':');
        if (colonIndex != -1) {
            QString key = line.left(colonIndex).trimmed();
            QString value = line.mid(colonIndex + 1).trimmed();
            headers[key] = value;
        }
    }

    return headers;
}

QString HttpServer::urlDecode(const QString& input)
{
    return QUrl::fromPercentEncoding(input.toUtf8());
}
