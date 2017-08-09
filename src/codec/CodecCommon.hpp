#ifndef SRC_CODEC_CODEC_COMMON_HPP_
#define SRC_CODEC_CODEC_COMMON_HPP_
#include "google/protobuf/utility/json_util.h"

namespace thunder
{

/*
 0               1               2               3
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
 FIN：1位
                 表示这是消息的最后一帧（结束帧），一个消息由一个或多个数据帧构成。若消息由一帧构成，起始帧即结束帧。
 * */
/*
 RSV1，RSV2，RSV3：3位
                 如果未定义扩展，各位是0；如果定义了扩展，即为非0值。如果接收的帧此处非0，扩展中却没有该值的定义，那么关闭连接。
 * */
/*
 OPCODE：4位
                 解释PayloadData，如果接收到未知的opcode，接收端必须关闭连接。
 0x0表示附加数据帧
 0x1表示文本数据帧
 0x2表示二进制数据帧
 0x3-7暂时无定义，为以后的非控制帧保留
 0x8表示连接关闭
 0x9表示ping
 0xA表示pong
 0xB-F暂时无定义，为以后的控制帧保留
 * */

const uint8 WEBSOCKET_FIN                   = 0x80;
const uint8 WEBSOCKET_RSV1                  = 0x40;
const uint8 WEBSOCKET_RSV2                  = 0x20;
const uint8 WEBSOCKET_RSV3                  = 0x10;

/*
 OPCODE：4位
 解释PayloadData，如果接收到未知的opcode，接收端必须关闭连接。
 0x0表示附加数据帧
 0x1表示文本数据帧
 0x2表示二进制数据帧
 0x3-7暂时无定义，为以后的非控制帧保留
 0x8表示连接关闭
 0x9表示ping
 0xA表示pong
 0xB-F暂时无定义，为以后的控制帧保留
  |Opcode  | Meaning                             | Reference |
-+--------+-------------------------------------+-----------|
 | 0      | Continuation Frame                  | RFC 6455  |
-+--------+-------------------------------------+-----------|
 | 1      | Text Frame                          | RFC 6455  |
-+--------+-------------------------------------+-----------|
 | 2      | Binary Frame                        | RFC 6455  |
-+--------+-------------------------------------+-----------|
 | 8      | Connection Close Frame              | RFC 6455  |
-+--------+-------------------------------------+-----------|
 | 9      | Ping Frame                          | RFC 6455  |
-+--------+-------------------------------------+-----------|
 | 10     | Pong Frame                          | RFC 6455  |
-+--------+-------------------------------------+-----------|
 * */
const uint8 WEBSOCKET_OPCODE                = (0x0F);
const uint8 WEBSOCKET_FRAME_CONTINUE        = (0x0);
const uint8 WEBSOCKET_FRAME_TEXT            = (0x1);
const uint8 WEBSOCKET_FRAME_BINARY          = (0x2);
const uint8 WEBSOCKET_FRAME_CLOSE           = (0x8);
const uint8 WEBSOCKET_FRAME_PING            = (0x9);
const uint8 WEBSOCKET_FRAME_PONG            = (0xA);


/*
 * MASK & PAYLOAD_LEN
 * */
const uint8 WEBSOCKET_MASK                  = 0x80;
const uint8 WEBSOCKET_PAYLOAD_LEN           = 0x7F;
const uint8 WEBSOCKET_PAYLOAD_LEN_UINT16    = 126;
const uint8 WEBSOCKET_PAYLOAD_LEN_UINT64    = 127;

inline uint64 htonll(uint64 val) {
    return (((uint64) htonl(val)) << 32) + htonl(val >> 32);
}

inline uint64 ntohll(uint64 val) {
    return (((uint64) ntohl(val)) << 32) + ntohl(val >> 32);
}


#define STATUS_CODE(code, str) case code: return str;

inline const char * status_string(int code)
{
    switch (code)
    {
    STATUS_CODE(100, "Continue")
    STATUS_CODE(101, "Switching Protocols")
    STATUS_CODE(102, "Processing")            // RFC 2518) obsoleted by RFC 4918
    STATUS_CODE(200, "OK")
    STATUS_CODE(201, "Created")
    STATUS_CODE(202, "Accepted")
    STATUS_CODE(203, "Non-Authoritative Information")
    STATUS_CODE(204, "No Content")
    STATUS_CODE(205, "Reset Content")
    STATUS_CODE(206, "Partial Content")
    STATUS_CODE(207, "Multi-Status")               // RFC 4918
    STATUS_CODE(300, "Multiple Choices")
    STATUS_CODE(301, "Moved Permanently")
    STATUS_CODE(302, "Moved Temporarily")
    STATUS_CODE(303, "See Other")
    STATUS_CODE(304, "Not Modified")
    STATUS_CODE(305, "Use Proxy")
    STATUS_CODE(307, "Temporary Redirect")
    STATUS_CODE(400, "Bad Request")
    STATUS_CODE(401, "Unauthorized")
    STATUS_CODE(402, "Payment Required")
    STATUS_CODE(403, "Forbidden")
    STATUS_CODE(404, "Not Found")
    STATUS_CODE(405, "Method Not Allowed")
    STATUS_CODE(406, "Not Acceptable")
    STATUS_CODE(407, "Proxy Authentication Required")
    STATUS_CODE(408, "Request Time-out")
    STATUS_CODE(409, "Conflict")
    STATUS_CODE(410, "Gone")
    STATUS_CODE(411, "Length Required")
    STATUS_CODE(412, "Precondition Failed")
    STATUS_CODE(413, "Request Entity Too Large")
    STATUS_CODE(414, "Request-URI Too Large")
    STATUS_CODE(415, "Unsupported Media Type")
    STATUS_CODE(416, "Requested Range Not Satisfiable")
    STATUS_CODE(417, "Expectation Failed")
    STATUS_CODE(418, "I\"m a teapot")              // RFC 2324
    STATUS_CODE(422, "Unprocessable Entity")       // RFC 4918
    STATUS_CODE(423, "Locked")                     // RFC 4918
    STATUS_CODE(424, "Failed Dependency")          // RFC 4918
    STATUS_CODE(425, "Unordered Collection")       // RFC 4918
    STATUS_CODE(426, "Upgrade Required")           // RFC 2817
    STATUS_CODE(500, "Internal Server Error")
    STATUS_CODE(501, "Not Implemented")
    STATUS_CODE(502, "Bad Gateway")
    STATUS_CODE(503, "Service Unavailable")
    STATUS_CODE(504, "Gateway Time-out")
    STATUS_CODE(505, "HTTP Version not supported")
    STATUS_CODE(506, "Variant Also Negotiates")    // RFC 2295
    STATUS_CODE(507, "Insufficient Storage")       // RFC 4918
    STATUS_CODE(509, "Bandwidth Limit Exceeded")
    STATUS_CODE(510, "Not Extended")                // RFC 2774
    }
    return 0;
}

} /* namespace thunder */

#endif /* SRC_CODEC_CodecWebSocketJsonJSON_HPP_ */
