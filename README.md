# HTTP Parser

[![tests](https://github.com/ximtech/HTTPParser/actions/workflows/cmake-ci.yml/badge.svg)](https://github.com/ximtech/HTTPParser/actions/workflows/cmake-ci.yml)
[![codecov](https://codecov.io/gh/ximtech/HTTPParser/branch/main/graph/badge.svg?token=VEZG1RPTTL)](https://codecov.io/gh/ximtech/HTTPParser)

HTTP Parser provides the functionality to parse both HTTP requests and responses. Written in C and was created for
working in embedded applications.

### Features

- All dependencies in one package
- Request/Response validation
- Decodes chunked encoding
- Easy to manipulate with headers and query parameters(see example)

The parser extracts the following information from HTTP messages:

- HTTP version
- Content-Length
- Request method
- Response status code
- Request URL
- Transfer-Encoding
- Message body
- Headers as key and value pairs
- URL query parameters as key and value pairs

### Add as CPM project dependency

How to add CPM to the project, check the [link](https://github.com/cpm-cmake/CPM.cmake)

```cmake
CPMAddPackage(
        NAME HTTPParser
        GITHUB_REPOSITORY ximtech/HTTPParser
        GIT_TAG origin/main)

target_link_libraries(${PROJECT_NAME} HTTPParser)
```

```cmake
add_executable(${PROJECT_NAME}.elf ${SOURCES} ${LINKER_SCRIPT})
# For Clion STM32 plugin generated Cmake use 
target_link_libraries(${PROJECT_NAME}.elf HTTPParser)
```

### Usage

***NOTE:*** Parser use split function. Character buffer will be spoiled

```c
const char *testRequest =
        "GET /cgi-bin/process.cgi?param1=val1&param2=val2 HTTP/1.1\n"
        "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\n"
        "Host: www.example.com\n"
        "Content-Type: text/xml; charset=utf-8\n"
        "Content-Length: 12345\n"
        "Accept-Language: en-us\n"
        "Accept-Encoding: gzip, deflate\n"
        "Connection: Keep-Alive\n\n";

char httpDataBuffer[1000];
strcpy(httpDataBuffer, testRequest);

HTTPParser *parser = getHttpParserInstance();
parseHttpBuffer(httpDataBuffer, parser, HTTP_REQUEST);  // Parse request
if (parser->parserStatus == HTTP_PARSE_OK) {    // Parser checks for errors
    printf("Http Version: %s\n", parser->httpVersion);
    printf("Content-Length: %ul\n", parser->contentLength);
    printf("Method: %s\n", getHttpMethodName(parser->method));
    printf("URI path: %s\n\n", parser->uriPath);
}

// Parse headers
parseHttpHeaders(parser, httpDataBuffer);
HashMap headers = parser->headers;
printf("Header count: %d\n", getHashMapSize(headers));
HashMapIterator headerIterator = getHashMapIterator(headers);
while (hashMapHasNext(&headerIterator)) {
    printf("[%s]: [%s]\n", headerIterator.key, headerIterator.value);
}

printf("\n");

// Parse URL parameters
parseHttpQueryParameters(parser, httpDataBuffer);
HashMap params = parser->queryParameters;
printf("Params count: %d\n", getHashMapSize(params));
HashMapIterator paramIterator = getHashMapIterator(params);
while (hashMapHasNext(&paramIterator)) {
    printf("[%s]: [%s]\n", paramIterator.key, paramIterator.value);
}

deleteHttpParser(parser);
```
