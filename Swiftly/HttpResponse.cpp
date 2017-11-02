#include "HttpResponse.h"
#include "TcpSocket.h"


HttpResponse::HttpResponse(TcpSocket *_socket)
    :QObject(),
      buffer(),
      socket(_socket),
      header(),
      statusCode(200),
      cookies(),
      sessionId(),
      hasFinished(false)
{
}

HttpResponse::HttpResponse(const HttpResponse &in)
    :QObject(),
      buffer(in.buffer),
      socket(in.socket),
      header(in.header),
      statusCode(in.statusCode),
      cookies(in.cookies),
      sessionId(in.sessionId),
      hasFinished(in.hasFinished)
{

}

void HttpResponse::operator=(const HttpResponse &in)
{
    buffer = in.buffer;
    socket = in.socket;
    header = in.header;
    statusCode = in.statusCode;
    sessionId = in.sessionId;
    cookies = in.cookies;
    hasFinished = in.hasFinished;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addCookie(const QString &key, const QVariant &value)
{
    cookies.insert(key, value);
}

HttpResponse& HttpResponse::operator<<(const QByteArray &in)
{
    buffer.append(in);
    return *this;
}

HttpResponse& HttpResponse::operator<<(const char *in)
{
    buffer.append(in);
    return *this;
}

HttpResponse& HttpResponse::operator<<(const QString &in)
{
    buffer.append(in);
    return *this;
}

void HttpResponse::write(const char* in,const size_t size)
{
    buffer.append(in,size);
}

void HttpResponse::finish(const QString &typeOverride )
{
    if (!hasFinished)
    {
        unsigned int bufferSize = buffer.size();

        QString headerString;

        switch (statusCode)
        {
        case 200:
            headerString = "HTTP/1.1 " % QString::number(statusCode) % " Ok\r\n"
                           "Content-Length: " % QString::number(bufferSize) % "\r\n"
                           "Content-Type: " % typeOverride % "; charset=\"utf-8\"\r\n";
            break;
        case 404:
            headerString = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Type: text/html; charset=\"utf-8\"\r\n";
            break;
        case 302:
            headerString = "HTTP/1.1 302 Found\r\n"
                           "Connection:keep-alive\r\n";
            break;
        case 301:
            headerString = "HTTP/1.1 301 Moved Permanently\r\n"
                           "Connection:keep-alive\r\n";
            break;
        case 400:
            headerString = "HTTP/1.1 400 Bad Request\r\n";
            break;
        case 415:
            headerString = "HTTP/1.1 415 Unsupported Media Type\r\n";
            break;
        case 422:
            headerString = "HTTP/1.1 422 Unprocessable Entity";
            break;
        default:
            qDebug() << "unimplemented http status code";
        }

        headerString = headerString % header.toString();

        QString cookieString("Set-Cookie: ");

        if (!sessionId.isEmpty())
        {
            cookieString.append("ssid=").append(sessionId);
            for(QMap<QString, QVariant>::Iterator iter = cookies.begin(); iter != cookies.end(); ++iter)
            {
                cookieString.append("&").append(iter.key()).append("=").append(iter.value().toString());
            }
        }
        else if(cookies.count() > 0)
        {
            QMap<QString, QVariant>::Iterator iter = cookies.begin();

            cookieString.append(iter.key()).append("=").append(iter.value().toString());

            for(++iter; iter != cookies.end(); ++iter)
            {
                cookieString.append("&").append(iter.key()).append("=").append(iter.value().toString());
            }
        }

        if ((!sessionId.isEmpty()) || (cookies.count() > 0))
        {
            headerString = headerString % cookieString % "; Path=/\r\n";
        }

        headerString = headerString % "\r\n";

        socket->write(headerString.toUtf8());
        socket->write(buffer);
        hasFinished = true;
    }
}

void HttpResponse::redirectTo(const QString &url)
{
    setStatusCode(302);
    setHeader("Location", url);
    finish();
}
