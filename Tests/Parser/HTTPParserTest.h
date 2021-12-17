#pragma once

#include "BaseTestTemplate.h"
#include "HTTPParser.h"

static char *const HTTP_LINE_SEPARATOR = "\r\n\r\n";

static const char *testRequest =
        "POST /cgi-bin/process.cgi HTTP/1.1\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\n"
        "Host: www.example.com\n"
        "Content-Type: text/xml; charset=utf-8\n"
        "Content-Length: 12345\n"
        "Accept-Language: en-us\n"
        "Accept-Encoding: gzip, deflate\n"
        "Connection: Keep-Alive\n\n"
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<string xmlns=\"http://clearforest.com/\">string</string>";

static const char *testResponse =
        "HTTP/1.1 400 Bad Request\r\n"
        "Server: awselb/2.0\r\n"
        "Date: Mon, 30 Aug 2021 16:23:25 GMT\r\n"
        "Content-Type: text/html\r\n"
        "Transfer-Encoding: gzip\r\n"
        "Connection: close\r\n\r\n"
        "<html>\r\n"
        "<head><title>400 Bad Request</title></head>\r\n"
        "<body>\r\n"
        "<center><h1>400 Bad Request</h1></center>\r\n"
        "</body>\r\n"
        "</html>\r\n";

static HTTPParser *parser = NULL;

static void *httpParserSetup(const MunitParameter params[], void *userData) {
    char *httpDataBuffer = malloc(2500);
    assert_not_null(httpDataBuffer);
    memset(httpDataBuffer, 0, 2500);
    parser = getHttpParserInstance();
    assert_not_null(parser);
    return httpDataBuffer;
}

// REQUEST
static MunitResult minimalHttpRequestOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET / HTTP/1.0\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);

    assert_int(parser->method, ==, HTTP_GET);
    assert_string_equal(parser->messageBody, "");
    assert_string_equal(parser->httpVersion, "1.0");
    return MUNIT_OK;
}

static MunitResult multilineSpacesHttpOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET   /   HTTP/1.0\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);

    assert_int(parser->method, ==, HTTP_GET);
    assert_string_equal(parser->messageBody, "");
    assert_string_equal(parser->httpVersion, "1.0");
    assert_string_equal(parser->uriPath, "/");
    return MUNIT_OK;
}

static MunitResult parseEmptyValueHeaderHttpOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET /hoge HTTP/1.1\r\nHost: example.com\r\nCookie: \r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_int(parser->method, ==, HTTP_GET);
    assert_string_equal(parser->httpVersion, "1.1");

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 2);
    assert_string_equal(hashMapGet(headers, "Host"), "example.com");
    assert_string_equal(hashMapGet(headers, "Cookie"), "");
    return MUNIT_OK;
}

static MunitResult parseMultiByteValueHeaderHttpOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET /hoge HTTP/1.1\r\nHost: example.com\r\nUser-Agent: \343\201\262\343/1.0\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_int(parser->method, ==, HTTP_GET);
    assert_string_equal(parser->httpVersion, "1.1");

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 2);
    assert_string_equal(hashMapGet(headers, "Host"), "example.com");
    assert_string_equal(hashMapGet(headers, "User-Agent"), "\343\201\262\343/1.0");
    return MUNIT_OK;
}

static MunitResult parseMultiLineValueHeaderHttpOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET / HTTP/1.0\r\nfoo: \r\nfoo: b\r\n  \tc\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_int(parser->method, ==, HTTP_GET);
    assert_string_equal(parser->httpVersion, "1.0");

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 1);
    assert_string_equal(hashMapGet(headers, "foo"), "b");
    return MUNIT_OK;
}

static MunitResult parseHeaderWithTrailingSpaceHttpOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET / HTTP/1.0\r\nfoo : ab\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_int(parser->method, ==, HTTP_GET);
    assert_string_equal(parser->httpVersion, "1.0");

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 1);
    assert_string_equal(hashMapGet(headers, "foo"), "ab");
    return MUNIT_OK;
}

static MunitResult parseHttpRequestOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, testRequest);
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.1");
    assert_int(parser->contentLength, ==, 12345);
    assert_int(parser->method, ==, HTTP_POST);
    assert_string_equal(parser->uriPath, "/cgi-bin/process.cgi");
    assert_string_equal(parser->messageBody, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<string xmlns=\"http://clearforest.com/\">string</string>");
    assert_string_equal(parser->transferEncodingTypes, "");

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 7);
    assert_string_equal(hashMapGet(headers, "User-Agent"), "Mozilla/4.0 (compatible; MSIE5.01; Windows NT)");
    assert_string_equal(hashMapGet(headers, "Host"), "www.example.com");
    assert_string_equal(hashMapGet(headers, "Content-Type"), "text/xml; charset=utf-8");
    assert_string_equal(hashMapGet(headers, "Content-Length"), "12345");
    assert_string_equal(hashMapGet(headers, "Accept-Language"), "en-us");
    assert_string_equal(hashMapGet(headers, "Accept-Encoding"), "gzip, deflate");
    assert_string_equal(hashMapGet(headers, "Connection"), "Keep-Alive");
    return MUNIT_OK;
}

static MunitResult parseHttpRequestQueryParamsOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "/api/user?id=1&test=1245&utm=value");
    parseHttpQueryParameters(parser, httpDataBuffer);
    assert_true(isHashMapNotEmpty(parser->queryParameters));
    assert_true(isHashMapContainsKey(parser->queryParameters, "id"));
    assert_true(isHashMapContainsKey(parser->queryParameters, "test"));
    assert_true(isHashMapContainsKey(parser->queryParameters, "utm"));

    assert_string_equal(hashMapGet(parser->queryParameters, "id"), "1");
    assert_string_equal(hashMapGet(parser->queryParameters, "test"), "1245");
    assert_string_equal(hashMapGet(parser->queryParameters, "utm"), "value");
    return MUNIT_OK;
}

static MunitResult malformedBodyHttpFail(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET / HTTP/1.0\r\n\r");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_ERROR_INVALID_LINE_SEPARATORS);
    return MUNIT_OK;
}

static MunitResult parseMalformedHttpFail(const MunitParameter params[], void *httpDataBuffer) {
    const char *malformedRequests = munit_parameters_get(params, "invalidRequest");
    strcpy(httpDataBuffer, malformedRequests);
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    if (parser->parserStatus != HTTP_PARSE_ERROR_NOT_FOUND_HTTP_CONSTANT &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_HTTP_VERSION &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_LINE_SEPARATORS &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_HTTP_CONSTANT) {
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

static MunitResult parseMalformedMethodHttpFail(const MunitParameter params[], void *httpDataBuffer) {
    const char *malformedHttpMethod = munit_parameters_get(params, "invalidMethod");
    strcpy(httpDataBuffer, malformedHttpMethod);
    strcat(httpDataBuffer, HTTP_LINE_SEPARATOR);
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    if (parser->parserStatus != HTTP_PARSE_ERROR_NOT_FOUND_HTTP_METHOD &&
        parser->parserStatus != HTTP_PARSE_ERROR_NO_SUCH_HTTP_METHOD) {
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

static MunitResult parseMalformedURIHttpFail(const MunitParameter params[], void *httpDataBuffer) {
    const char *malformedHttpUri = munit_parameters_get(params, "invalidURI");
    strcpy(httpDataBuffer, malformedHttpUri);
    strcat(httpDataBuffer, HTTP_LINE_SEPARATOR);
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    if (parser->parserStatus != HTTP_PARSE_ERROR_URI_PATH_NOT_FOUND &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_URI_PATH) {
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

static MunitResult parseMalformedHeadersHttpFail(const MunitParameter params[], void *httpDataBuffer) {
    const char *malformedHttpHeader = munit_parameters_get(params, "invalidHeader");
    strcpy(httpDataBuffer, malformedHttpHeader);
    strcat(httpDataBuffer, HTTP_LINE_SEPARATOR);
    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    if (isHashMapNotEmpty(headers)) {
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}

static MunitResult parseTransferTypeAndContentLengthHttpFail(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "GET /hoge HTTP/1.1\r\nContent-Length: 12345\r\nTransfer-Encoding: chunked, gzip\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_ERROR_UNEXPECTED_CONTENT_LENGTH);
    return MUNIT_OK;
}

static MunitResult parseMalformedHttpRequestQueryParamsFail(const MunitParameter params[], void *httpDataBuffer) {
    const char *uriWithParameters = munit_parameters_get(params, "invalidURIParams");
    strcpy(httpDataBuffer, uriWithParameters);
    parseHttpQueryParameters(parser, httpDataBuffer);
    if (isHashMapNotEmpty(parser->queryParameters)) {
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}


// RESPONSE
static MunitResult minimalHttpResponseOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "HTTP/1.0 200 OK\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.0");
    assert_int(parser->statusCode, ==, HTTP_OK);

    memset(httpDataBuffer, 0, strlen(httpDataBuffer));
    strcpy(httpDataBuffer, "HTTP/1.1   200 \t  OK\r\n\r\n");  // accept multiple spaces between tokens
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.1");
    assert_int(parser->statusCode, ==, HTTP_OK);
    return MUNIT_OK;
}

static MunitResult headersHttpResponseOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "HTTP/1.1 200 OK\r\nHost: example.com\r\nCookie: \r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.1");
    assert_int(parser->statusCode, ==, HTTP_OK);

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 2);
    assert_string_equal(hashMapGet(headers, "Host"), "example.com");
    assert_string_equal(hashMapGet(headers, "Cookie"), "");
    return MUNIT_OK;
}

static MunitResult headersMultilineHttpResponseOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "HTTP/1.0 200 OK\r\nfoo: \r\nfoo: b\r\n  \tc\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.0");
    assert_int(parser->statusCode, ==, HTTP_OK);

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 1);
    assert_string_equal(hashMapGet(headers, "foo"), "b");
    return MUNIT_OK;
}

static MunitResult responseCodeHttpResponseOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, "HTTP/1.0 500 Internal Server Error\r\n\r\n");
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.0");
    assert_int(parser->statusCode, ==, HTTP_INTERNAL_SERVER_ERROR);
    return MUNIT_OK;
}

static MunitResult parseHttpResponseOk(const MunitParameter params[], void *httpDataBuffer) {
    strcpy(httpDataBuffer, testResponse);
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);
    assert_int(parser->parserStatus, ==, HTTP_PARSE_OK);
    assert_string_equal(parser->httpVersion, "1.1");
    assert_int(parser->statusCode, ==, HTTP_BAD_REQUEST);
    assert_string_equal(parser->transferEncodingTypes, "gzip");
    assert_string_equal(parser->messageBody, "<html>\r\n<head><title>400 Bad Request</title></head>\r\n<body>\r\n<center><h1>400 Bad Request</h1></center>\r\n</body>\r\n</html>\r\n");

    parseHttpHeaders(parser, httpDataBuffer);
    HashMap headers = parser->headers;
    assert_int(getHashMapSize(headers), ==, 5);
    assert_string_equal(hashMapGet(headers, "Date"), "Mon, 30 Aug 2021 16:23:25 GMT");
    assert_string_equal(hashMapGet(headers, "Server"), "awselb/2.0");
    assert_string_equal(hashMapGet(headers, "Content-Type"), "text/html");
    assert_string_equal(hashMapGet(headers, "Transfer-Encoding"), "gzip");
    assert_string_equal(hashMapGet(headers, "Connection"), "close");
    return MUNIT_OK;
}

static MunitResult checkInvalidHttpResponse(const MunitParameter params[], void *httpDataBuffer) {
    const char *malformedHttpResponse = munit_parameters_get(params, "invalidResponse");
    strcpy(httpDataBuffer, malformedHttpResponse);
    strcat(httpDataBuffer, HTTP_LINE_SEPARATOR);
    parseHttpBuffer(httpDataBuffer, parser, HTTP_RESPONSE);

    if (parser->parserStatus != HTTP_PARSE_ERROR_NOT_FOUND_HTTP_CONSTANT &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_HTTP_VERSION &&
        parser->parserStatus != HTTP_PARSE_ERROR_NOT_SUPPORTED_HTTP_VERSION &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_LINE_SEPARATORS &&
        parser->parserStatus != HTTP_PARSE_ERROR_STATUS_CODE_NOT_FOUND &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_HTTP_STATUS_CODE &&
        parser->parserStatus != HTTP_PARSE_ERROR_STATUS_CODE_MESSAGE_NOT_FOUND &&
        parser->parserStatus != HTTP_PARSE_ERROR_INVALID_STATUS_CODE_MESSAGE) {
        return MUNIT_FAIL;
    }
    return MUNIT_OK;
}


static void httpParserTearDown(void *httpDataBuffer) {
    deleteHttpParser(parser);
    free(httpDataBuffer);
    httpDataBuffer = NULL;
    munit_assert_ptr_null(httpDataBuffer);
}

static char *malformedHttpRequests[] = {
        "GET",
        "GET ",
        "GET /",
        "GET / ",
        "GET / H",
        "GET / HTTP/1.",
        "GET / HTTP/1.0",  // no separators
        "GET / HTTP/1.1\t", // invalid separator
        "GET / HTkP/1.0",   // invalid protocol constant
        "G\0T / HTTP/1.0\r\n\r\n", // NUL in method
        NULL
};

static char *malformedMethodRequest[] = {
        " / HTTP/1.0",     // empty method
        "G\tT / HTTP/1.0", // tab in method
        "G T / HTTP/1.0",  // whitespace in method
        ":GET / HTTP/1.0", // invalid method
        "GETS / HTTP/1.0", // unknown method
        "get / HTTP/1.0", // lowercase method
        NULL
};

static char *malformedURIRequest[] = {
        "GET  HTTP/1.0", // empty request-target
        "GET /\x7fhello HTTP/1.0", // DEL in uri-path
        "GET \\ HTTP/1.0", // invalid root path
        NULL
};

static char *malformedHttpHeaders[] = {
        ":a",        // empty header name
        " :a",       // header name (space only)
        "a\0b: c",   // NUL in header name
        "ab: c\0d",  // NUL in header value
        "a\033b: c", // CTL in header name
        "ab: c\033", // CTL in header value
        "/: 1",      // invalid char in header key
        "\x7b: 1",   // disallow "{"
        NULL
};

static char *malformedHttpResponses[] = {
        "H",
        "HTTP/1.",
        "HTTP/1.1",
        "HTTP/1.1z",
        "HTTP/2.0", // unsupported version
        "HTTP/1.1 ",
        "HTTP/1.1 2",
        "HTTP/1.1 2000 OK",
        "HTTP/1.1 924 OK",  // unknown status code
        "HTTP/1.1 92 OK",  // short status code
        "HTTP/1.1 200",
        "HTTP/1.1 200X",
        "HTTP/1.1 200 ",
        "HTTP/1.1 200 O",
        "HTTP/1.1 OK",
        "HTTP/1.1 200 OK \t\b\b\b\0",  // invalid separators
        NULL
};

static char *malformedHttpUriParams[] = {
        " \t\t ",
        "/?+",
        "/test?key-val",
        "/test!key=val",
        "/test/key=val",
        "/test key=val",
        "/test? key=val",
        NULL
};

static MunitParameterEnum httpTestParameters1[] = {
        {.name = "invalidRequest", .values = malformedHttpRequests},
        END_OF_PARAMETERS
};

static MunitParameterEnum httpTestParameters2[] = {
        {.name = "invalidMethod", .values = malformedMethodRequest},
        END_OF_PARAMETERS
};

static MunitParameterEnum httpTestParameters3[] = {
        {.name = "invalidURI", .values = malformedURIRequest},
        END_OF_PARAMETERS
};

static MunitParameterEnum httpTestParameters4[] = {
        {.name = "invalidHeader", .values = malformedHttpHeaders},
        END_OF_PARAMETERS
};

static MunitParameterEnum httpTestParameters5[] = {
        {.name = "invalidResponse", .values = malformedHttpResponses},
        END_OF_PARAMETERS
};

static MunitParameterEnum httpTestParameters6[] = {
        {.name = "invalidURIParams", .values = malformedHttpUriParams},
        END_OF_PARAMETERS
};

static MunitTest httpParserTests[] = {
        {.name = "Test OK  parseHttpBuffer() - Request: Minimal HTTP", .test = minimalHttpRequestOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK  parseHttpBuffer() - Request: Multiline spaces HTTP", .test = multilineSpacesHttpOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Request: Header with empty value", .test = parseEmptyValueHeaderHttpOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Request: Header multi byte value", .test = parseMultiByteValueHeaderHttpOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Request: Header multi line value", .test = parseMultiLineValueHeaderHttpOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Request: Header with trailing spaces", .test = parseHeaderWithTrailingSpaceHttpOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Request: Full with headers", .test = parseHttpRequestOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Request: Query parameters", .test = parseHttpRequestQueryParamsOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},

        {.name = "Test FAIL parseHttpBuffer() - Request: Malformed body", .test = malformedBodyHttpFail, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test FAIL parseHttpBuffer() - Request: Malformed HTTPs", .test = parseMalformedHttpFail, .setup = httpParserSetup, .tear_down = httpParserTearDown, .parameters = httpTestParameters1},
        {.name = "Test FAIL parseHttpBuffer() - Request: Malformed method", .test = parseMalformedMethodHttpFail, .setup = httpParserSetup, .tear_down = httpParserTearDown, .parameters = httpTestParameters2},
        {.name = "Test FAIL parseHttpBuffer() - Request: Malformed URI", .test = parseMalformedURIHttpFail, .setup = httpParserSetup, .tear_down = httpParserTearDown, .parameters = httpTestParameters3},
        {.name = "Test FAIL parseHttpBuffer() - Request: Malformed headers", .test = parseMalformedHeadersHttpFail, .setup = httpParserSetup, .tear_down = httpParserTearDown, .parameters = httpTestParameters4},
        {.name = "Test FAIL parseHttpBuffer() - Request: Unexpected content length", .test = parseTransferTypeAndContentLengthHttpFail, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test FAIL parseHttpBuffer() - Request: Malformed URI params", .test = parseMalformedHttpRequestQueryParamsFail, .setup = httpParserSetup, .tear_down = httpParserTearDown, .parameters = httpTestParameters6},

        {.name = "Test OK parseHttpBuffer() - Response: Minimal HTTP", .test = minimalHttpResponseOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Response: Header with empty value", .test = headersHttpResponseOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Response: Header multi line value", .test = headersMultilineHttpResponseOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Response: Status code", .test = responseCodeHttpResponseOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},
        {.name = "Test OK parseHttpBuffer() - Response: Full with headers", .test = parseHttpResponseOk, .setup = httpParserSetup, .tear_down = httpParserTearDown},

        {.name = "Test FAIL parseHttpBuffer() - Response: Malformed responses", .test = checkInvalidHttpResponse, .setup = httpParserSetup, .tear_down = httpParserTearDown, .parameters = httpTestParameters5},
        END_OF_TESTS
};

static const MunitSuite httpParserTestSuite = {
        .prefix = "HTTPParser: ",
        .tests = httpParserTests,
        .suites = NULL,
        .iterations = 1,
        .options = MUNIT_SUITE_OPTION_NONE
};
