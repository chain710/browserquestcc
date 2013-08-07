#ifndef _WEBSOCKET_CODEC_H_
#define _WEBSOCKET_CODEC_H_

// https://tools.ietf.org/html/rfc6455

#include <inttypes.h>
#include <stddef.h>

#define WS_MASK_LEN (4)
#define WS_FIXED_HEAD_LEN (2)

enum ws_frame_opcode_t
{
    WFOP_FRAGMENT = 0,
    WFOP_TEXT = 1,
    WFOP_BINARY = 2,
    /*
    The Close frame MAY contain a body (the "Application data" portion of the frame) that indicates a reason for closing.
    no app data in body, so we can simply ignore them
     */
    WFOP_CLOSE = 8,
    WFOP_PING = 9,
    WFOP_PONG = 10,
};

#pragma pack(1)
struct ws_frame_fixed_head_t {
    /*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
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
    */

    // CANNOT USE MEMCPY: The order of allocation of bit-fields within a unit (high-order to low-order or low-order to high-order) is implementation-defined.
    unsigned opcode_:4; // see ws_frame_opcode_t
    unsigned rsv_:3;    // should be zero
    unsigned fin_:1;    // 0=more frames follow, 1=final frame of this msg
    unsigned payload_len_:7;
    unsigned has_mask_:1;   // 0=not masked, 1=masked
};
#pragma pack(0)

// check if websocket frame
bool is_client_ws_frame(const char* data, size_t size);
// get websocket frame length, 0 means insufficient length,>0 means frame length
uint64_t get_ws_frame_len( const char* data, size_t size );

class ws_frame_t
{
public:
    enum codec_flag_t
    {
        PWF_UNMASK_PAYLOAD = 0x01,
        PWF_MOVE_TO_END = 0x02,
    };

    ws_frame_t();
    ~ws_frame_t();

    // return unserialized len, <0 if error
    int from_bytes(const void* data, size_t data_len, unsigned int flag);
    // return packed-head len, <0 if error
    int pack_head(void* data, size_t data_len, unsigned int flag);
    const char* get_payload() const { return payload_; }
    char* get_payload() { return payload_; }
    uint64_t get_payload_len() { return payload_len_; }
    void set_opcode(int opcode) { head_.opcode_ = opcode; }
    void set_payload(uint64_t payload_len, char* payload) { payload_len_ = payload_len; payload_ = payload; }
    int get_opcode() const { return (unsigned char)head_.opcode_; }
    bool is_fin() const { return 1 == head_.fin_; }
private:
    // deny copy-cons & assignment for now
    ws_frame_t(ws_frame_t& c) {}
    ws_frame_t& operator = (const ws_frame_t& c) { return *this; }

    ws_frame_fixed_head_t head_;
    uint64_t payload_len_;
    char mask_[WS_MASK_LEN];
    char* payload_;
};

#endif // !_WEBSOCKET_CODEC_H_
