#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "HTTPMethod.h"
#include "HTTPStatus.h"
#include "StringUtils.h"
#include "HashMap.h"

#define HTTP_VERSION_LENGTH 4
#define HTTP_REQUEST_URI_PATH_LENGTH 80
#define HTTP_TRANSFER_ENCODING_TYPES_LENGTH 35

typedef enum HTTPParserType {
    HTTP_REQUEST,
    HTTP_RESPONSE
} HTTPParserType;

typedef enum HTTPParserStatus {
    HTTP_PARSE_OK,
    HTTP_PARSE_ERROR_EMPTY_DATA,
    HTTP_PARSE_ERROR_INVALID_LINE_SEPARATORS,
    HTTP_PARSE_ERROR_NOT_FOUND_HTTP_CONSTANT,
    HTTP_PARSE_ERROR_INVALID_HTTP_CONSTANT,
    HTTP_PARSE_ERROR_INVALID_HTTP_VERSION,
    HTTP_PARSE_ERROR_NOT_SUPPORTED_HTTP_VERSION,    // Only 1.0 and 1.1 supported
    HTTP_PARSE_ERROR_NOT_FOUND_HTTP_METHOD,
    HTTP_PARSE_ERROR_NO_SUCH_HTTP_METHOD,
    HTTP_PARSE_ERROR_URI_PATH_NOT_FOUND,
    HTTP_PARSE_ERROR_URI_PATH_TOO_LONG,
    HTTP_PARSE_ERROR_INVALID_URI_PATH,
    HTTP_PARSE_ERROR_STATUS_CODE_NOT_FOUND,
    HTTP_PARSE_ERROR_INVALID_HTTP_STATUS_CODE,
    HTTP_PARSE_ERROR_STATUS_CODE_MESSAGE_NOT_FOUND,
    HTTP_PARSE_ERROR_INVALID_STATUS_CODE_MESSAGE,
    HTTP_PARSE_ERROR_UNEXPECTED_CONTENT_LENGTH,
    HTTP_PARSE_ERROR_MALFORMED_MESSAGE_BODY
} HTTPParserStatus;

typedef struct HTTPParser {
    char httpVersion[HTTP_VERSION_LENGTH];
    uint32_t contentLength;
    HTTPMethod method;
    HTTPStatus statusCode;
    char uriPath[HTTP_REQUEST_URI_PATH_LENGTH];
    char transferEncodingTypes[HTTP_TRANSFER_ENCODING_TYPES_LENGTH];
    char *messageBody;
    HTTPParserType httpType;
    HashMap headers;
    HashMap queryParameters;
    HTTPParserStatus parserStatus;
} HTTPParser;


HTTPParser *getHttpParserInstance();
void parseHttpBuffer(char *httpDataBuffer, HTTPParser *httpParser, HTTPParserType httpType);
void parseHttpHeaders(HTTPParser *httpParser, char *dataBuffer);
void parseHttpQueryParameters(HTTPParser *httpParser, char *url);
void deleteHttpParser(HTTPParser *httpParser);