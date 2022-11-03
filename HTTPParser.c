#include "HTTPParser.h"

#define HTTP_METHOD_MAX_LENGTH 7
#define HTTP_CONSTANT_NAME_WITH_SLASH "HTTP/"
#define TRANSFER_ENCODING_HEADER_NAME "Transfer-Encoding: "
#define CONTENT_TYPE_HEADER_NAME "Content-Length: "
#define CONTENT_LENGTH_VALUE_BUFFER_SIZE 20
#define HTTP_HEADERS_END_DELIMITER_LENGTH 5
#define HTTP_HEADERS_MAP_INITIAL_CAPACITY 16
#define HTTP_QUERY_PARAM_MAP_INITIAL_CAPACITY 8
#define HTTP_URI_ROOT_PATH_START "/"
#define HTTP_STATUS_CODE_MESSAGE_MAX_LENGTH 50

static const char *const HTTP_SUPPORTED_VERSIONS_ARRAY[] = {"1.0", "1.1"};

static void parseHttpVersion(const char *dataBuffer, HTTPParser *httpParser);
static void parseHttpContentLength(const char *dataBuffer, HTTPParser *httpParser);
static void parseHttpMethod(const char *dataBuffer, HTTPParser *httpParser);
static void parseUriPath(const char *dataBuffer, HTTPParser *httpParser);
static void parseHttpStatusCode(const char *dataBuffer, HTTPParser *httpParser);
static void parseHttpTransferEncoding(const char *dataBuffer, HTTPParser *httpParser);
static void parseHttpMessageBody(const char *dataBuffer, HTTPParser *httpParser);
static bool isMessageBodyNeedToBeSkipped(HTTPParser *httpParser);
static inline char *resolveHttpLineSeparator(const char *dataBuffer);
static bool isHttpHeaderKeyValid(const char *headerKey);
static bool isHttpHeaderValueValid(const char *headerValue);


HTTPParser *getHttpParserInstance() {
    HTTPParser *httpParser = malloc(sizeof(struct HTTPParser));
    if (httpParser != NULL) {
        httpParser->headers = NULL;
        httpParser->queryParameters = NULL;
    }
    return httpParser;
}

void parseHttpBuffer(char *httpDataBuffer, HTTPParser *httpParser, HTTPParserType httpType) {
    httpParser->contentLength = 0;
    httpParser->method = HTTP_NO_METHOD;
    httpParser->statusCode = HTTP_NO_STATUS;
    httpParser->messageBody = NULL;
    httpParser->httpType = httpType;
    httpParser->parserStatus = HTTP_PARSE_OK;

    memset(httpParser->httpVersion, 0, HTTP_VERSION_LENGTH);
    memset(httpParser->uriPath, 0, HTTP_REQUEST_URI_PATH_LENGTH);
    memset(httpParser->transferEncodingTypes, 0, HTTP_TRANSFER_ENCODING_TYPES_LENGTH);

    if (isStringBlank(httpDataBuffer)) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_EMPTY_DATA;
        return;
    } else if (isStringEmpty(resolveHttpLineSeparator(httpDataBuffer))) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_LINE_SEPARATORS;
        return;
    }
    parseHttpVersion(httpDataBuffer, httpParser);
    parseHttpContentLength(httpDataBuffer, httpParser);

    if (httpType == HTTP_REQUEST) {
        parseHttpMethod(httpDataBuffer, httpParser);
        parseUriPath(httpDataBuffer, httpParser);
    } else {
        parseHttpStatusCode(httpDataBuffer, httpParser);
    }
    parseHttpTransferEncoding(httpDataBuffer, httpParser);
    parseHttpMessageBody(httpDataBuffer, httpParser);
}

void parseHttpHeaders(HTTPParser *httpParser, char *dataBuffer) {
    if (httpParser == NULL || isStringBlank(dataBuffer)) return;
    initSingletonHashMap(&httpParser->headers, HTTP_HEADERS_MAP_INITIAL_CAPACITY);
    hashMapClear(httpParser->headers);

    char *headersDelimiter = resolveHttpLineSeparator(dataBuffer);
    char headersEndDelimiter[HTTP_HEADERS_END_DELIMITER_LENGTH] = {0};
    strcpy(headersEndDelimiter, headersDelimiter);
    strcat(headersEndDelimiter, headersDelimiter);

    if (isStringNotEmpty(headersDelimiter) && isStringNotEmpty(headersEndDelimiter)) {
        char *headersStartPointer = strstr(dataBuffer, headersDelimiter); // skip HTTP constant line
        char *headersEndPointer = strstr(dataBuffer, headersEndDelimiter);
        headersStartPointer += strlen(headersDelimiter);
        headersEndPointer += strlen(headersEndDelimiter);

        uint32_t delimiterLength = strlen(headersDelimiter);
        if ((headersStartPointer + delimiterLength) < headersEndPointer) {  // check that headers exist
            char *savePointer;
            char *header = splitStringReentrant(headersStartPointer, headersDelimiter, &savePointer);
            while (header != NULL) {

                if (strchr(header, ':') != NULL) {
                    char *headerValue = strstr(header, ": ");
                    headerValue = headerValue != NULL ? headerValue + 2 : headerValue; // strlen(": ")
                    char *headerKey = strtok(header, ": ");
                    if (isHttpHeaderKeyValid(headerKey) && isHttpHeaderValueValid(headerValue)) {
                        hashMapPut(httpParser->headers, headerKey, headerValue);
                    }
                }

                if (savePointer + delimiterLength + 1 >= headersEndPointer) {
                    break;
                }
                header = splitStringReentrant(NULL, headersDelimiter, &savePointer);
            }
        }
    }
}

void parseHttpQueryParameters(HTTPParser *httpParser, char *url) {
    if (httpParser == NULL || isStringBlank(url)) return;
    initSingletonHashMap(&httpParser->queryParameters, HTTP_QUERY_PARAM_MAP_INITIAL_CAPACITY);
    hashMapClear(httpParser->queryParameters);

    char *parametersStartPointer = strstr(url, "?");
    if (parametersStartPointer != NULL) {
        char *parametersEndPointer = strchr(parametersStartPointer, ' ');
        if (parametersEndPointer != NULL) {
            *parametersEndPointer = '\0';
        }
        parametersStartPointer++; // skip "?"

        char *nextParameter;
        char *parameter = splitStringReentrant(parametersStartPointer, "&", &nextParameter);
        while (parameter != NULL) {
            if (strchr(parameter, '=') != NULL) {
                char *argValue = strstr(parameter, "=");
                char *argKey = strtok(parameter, "=");
                hashMapPut(httpParser->queryParameters, argKey, ++argValue);    // skip "="
            }
            parameter = splitStringReentrant(NULL, "&", &nextParameter);
        }
    }
}

void deleteHttpParser(HTTPParser *httpParser) {
    if (httpParser != NULL) {
        hashMapDelete(httpParser->headers);
        hashMapDelete(httpParser->queryParameters);
        httpParser->headers = NULL;
        httpParser->queryParameters = NULL;
        free(httpParser);
    }
}

static bool isHttpVersionSupported(const char *httpVersion) {
    for (uint32_t i = 0; i < sizeof(HTTP_SUPPORTED_VERSIONS_ARRAY) / sizeof(char *); i++) {
        if (strcmp(HTTP_SUPPORTED_VERSIONS_ARRAY[i], httpVersion) == 0) {
            return true;
        }
    }
    return false;
}

static void parseHttpVersion(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK) return;
    const char *httpVersionPointer = strstr(dataBuffer, HTTP_CONSTANT_NAME_WITH_SLASH);
    if (httpVersionPointer == NULL) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_NOT_FOUND_HTTP_CONSTANT;
        return;
    }

    bool isAtLineStart = (dataBuffer - httpVersionPointer) == 0;
    if (httpParser->httpType == HTTP_RESPONSE && !isAtLineStart) {  // check that HTTP constant at response beginning
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_CONSTANT;
        return;
    }
    httpVersionPointer += strlen(HTTP_CONSTANT_NAME_WITH_SLASH);

    if (!isdigit(*httpVersionPointer)) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_VERSION;
        return;
    }
    httpParser->httpVersion[0] = *httpVersionPointer;
    httpVersionPointer++;

    if (*httpVersionPointer != '.') {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_VERSION;
        return;
    }
    httpParser->httpVersion[1] = *httpVersionPointer;
    httpVersionPointer++;

    if (!isdigit(*httpVersionPointer)) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_VERSION;
        return;
    }
    httpParser->httpVersion[2] = *httpVersionPointer;

    if (!isHttpVersionSupported(httpParser->httpVersion)) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_NOT_SUPPORTED_HTTP_VERSION;
    }
}

static void parseHttpContentLength(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK) return;
    const char *httpContentLengthPointer = strstr(dataBuffer, CONTENT_TYPE_HEADER_NAME);
    if (httpContentLengthPointer != NULL) {
        httpContentLengthPointer += strlen(CONTENT_TYPE_HEADER_NAME);
        char buffer[CONTENT_LENGTH_VALUE_BUFFER_SIZE] = {[0 ... CONTENT_LENGTH_VALUE_BUFFER_SIZE - 1] = 0};
        for (uint8_t i = 0; isdigit(*httpContentLengthPointer); i++) {
            buffer[i] = *httpContentLengthPointer;
            httpContentLengthPointer++;
        }

        if (isStringNotEmpty(buffer)) {
            httpParser->contentLength = strtoul(buffer, NULL, 10);
        }
    }
}

static void parseHttpMethod(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK) return;
    const char *httpMethodPointer = dataBuffer;
    char httpMethodBuffer[HTTP_METHOD_MAX_LENGTH + 1] = {[0 ... HTTP_METHOD_MAX_LENGTH] = 0};
    for (uint8_t i = 0; !isspace(*httpMethodPointer) && i < HTTP_METHOD_MAX_LENGTH; i++) {
        httpMethodBuffer[i] = *httpMethodPointer;
        httpMethodPointer++;
    }

    if (isStringBlank(httpMethodBuffer)) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_NOT_FOUND_HTTP_METHOD;
        return;
    }

    HTTPMethod httpMethod = getHttpMethodByName(httpMethodBuffer);
    if (httpMethod == HTTP_NO_METHOD) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_NO_SUCH_HTTP_METHOD;
        return;
    }
    httpParser->method = httpMethod;
}

static void parseUriPath(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK) return;
    const char *targetRequestStartPointer = strstr(dataBuffer, HTTP_URI_ROOT_PATH_START);
    const char *targetRequestEndPointer = strstr(dataBuffer, HTTP_CONSTANT_NAME_WITH_SLASH);

    if (targetRequestStartPointer > targetRequestEndPointer) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_URI_PATH_NOT_FOUND;
        return;
    } else if ((targetRequestEndPointer - targetRequestStartPointer) + 1 > HTTP_REQUEST_URI_PATH_LENGTH) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_URI_PATH_TOO_LONG;
        return;
    }

    for (uint32_t i = 0; !isspace(*targetRequestStartPointer) && *targetRequestStartPointer != '?'; i++) {
        if (iscntrl(*targetRequestStartPointer)) {
            httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_URI_PATH;
            return;
        }
        httpParser->uriPath[i] = *targetRequestStartPointer;
        targetRequestStartPointer++;
    }
}

static void parseHttpStatusCode(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK) return;

    const char *httpStatusCodePointer = strstr(dataBuffer, HTTP_CONSTANT_NAME_WITH_SLASH);
    if (httpStatusCodePointer == NULL) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_STATUS_CODE_NOT_FOUND;
        return;
    }
    httpStatusCodePointer += strlen(HTTP_CONSTANT_NAME_WITH_SLASH) + strlen(httpParser->httpVersion);   // skip "HTTP/1.1"
    while (isspace(*httpStatusCodePointer)) {
        httpStatusCodePointer++;
    }

    char httpStatusBuffer[HTTP_STATUS_LENGTH + 1] = {[0 ... HTTP_STATUS_LENGTH] = 0};
    for (uint8_t i = 0; i < HTTP_STATUS_LENGTH; i++) {
        if (isdigit(*httpStatusCodePointer)) {
            httpStatusBuffer[i] = *httpStatusCodePointer;
            httpStatusCodePointer++;
        } else {
            httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_STATUS_CODE;
            return;
        }
    }

    bool isUnexpectedCharAfterCode = *httpStatusCodePointer != ' ';
    if (isUnexpectedCharAfterCode) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_STATUS_CODE;
        return;
    }

    uint32_t statusCode = strtoul(httpStatusBuffer, NULL, 10);
    if (statusCode == 0 || statusCode >= HTTP_STATUS_CODE_MAX_VALUE) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_HTTP_STATUS_CODE;
        return;
    }
    httpParser->statusCode = statusCode;

    const char *httpStatusMessageStartPointer = httpStatusCodePointer;
    while (*httpStatusMessageStartPointer != '\0' && isspace(*httpStatusMessageStartPointer)) {
        httpStatusMessageStartPointer++;
    }

    const char *httpStatusMessageEndPointer = httpStatusMessageStartPointer;
    while (*httpStatusMessageEndPointer != '\0' && *httpStatusMessageEndPointer != '\r' && *httpStatusMessageEndPointer != '\n') {
        httpStatusMessageEndPointer++;
    }

    uint16_t messageLength = httpStatusMessageEndPointer - httpStatusMessageStartPointer;
    if (messageLength == 0) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_STATUS_CODE_MESSAGE_NOT_FOUND;
        return;
    }
    char statusCodeMessageBuffer[HTTP_STATUS_CODE_MESSAGE_MAX_LENGTH] = {0};
    strncpy(statusCodeMessageBuffer, httpStatusMessageStartPointer, messageLength);
    const char *statusCodeMeaning = getHttpStatusCodeMeaning(httpParser->statusCode);

    if (isStringNotEquals(statusCodeMeaning, trimString(statusCodeMessageBuffer))) {
        httpParser->parserStatus = HTTP_PARSE_ERROR_INVALID_STATUS_CODE_MESSAGE;
        return;
    }
}

static void parseHttpTransferEncoding(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK) return;
    const char *httpTransferEncodingPointer = strstr(dataBuffer, TRANSFER_ENCODING_HEADER_NAME);
    if (httpTransferEncodingPointer != NULL) {
        httpTransferEncodingPointer += strlen(TRANSFER_ENCODING_HEADER_NAME);
        for (uint8_t i = 0; *httpTransferEncodingPointer != '\r' && i < HTTP_TRANSFER_ENCODING_TYPES_LENGTH; i++) {
            httpParser->transferEncodingTypes[i] = *httpTransferEncodingPointer;
            httpTransferEncodingPointer++;
        }

        if (containsString(httpParser->transferEncodingTypes, "chunked") && httpParser->contentLength > 0) {
            httpParser->parserStatus = HTTP_PARSE_ERROR_UNEXPECTED_CONTENT_LENGTH;// Cannot use chunked encoding and a content-length header together per the HTTP specification
        }
    }
}

static void parseHttpMessageBody(const char *dataBuffer, HTTPParser *httpParser) {
    if (httpParser->parserStatus != HTTP_PARSE_OK || isMessageBodyNeedToBeSkipped(httpParser)) return;
    char *messageBodyPointer = strstr(dataBuffer, "\r\n\r\n");
    if (messageBodyPointer != NULL) {
        messageBodyPointer += strlen("\r\n\r\n");
        httpParser->messageBody = messageBodyPointer;
        return;
    }

    messageBodyPointer = strstr(dataBuffer, "\n\n");
    if (messageBodyPointer != NULL) {
        messageBodyPointer += strlen("\n\n");
        httpParser->messageBody = messageBodyPointer;
        return;
    }
    httpParser->parserStatus = HTTP_PARSE_ERROR_MALFORMED_MESSAGE_BODY;
}

static bool isMessageBodyNeedToBeSkipped(HTTPParser *httpParser) {
    HTTPStatus statusCode = httpParser->statusCode;
    return httpParser->httpType == HTTP_RESPONSE && (statusCode < HTTP_OK || statusCode == HTTP_NO_CONTENT || statusCode == HTTP_NOT_MODIFIED);
}

static inline char *resolveHttpLineSeparator(const char *dataBuffer) {
    if (containsString(dataBuffer, "\r\n\r\n")) {
        return "\r\n";
    } else if (containsString(dataBuffer, "\n\n")) {
        return "\n";
    }
    return "";
}

static bool isHttpHeaderKeyValid(const char *headerKey) {
    if (isStringBlank(headerKey)) return false;
    while (*headerKey != '\0') {
        if (iscntrl(*headerKey) || *headerKey == '/' || *headerKey == '{') {
            return false;
        }
        headerKey++;
    }
    return true;
}

static bool isHttpHeaderValueValid(const char *headerValue) {
    if (isStringBlank(headerValue)) return true;
    while (*headerValue != '\0') {
        if (iscntrl(*headerValue)) {
            return false;
        }
        headerValue++;
    }
    return true;
}
