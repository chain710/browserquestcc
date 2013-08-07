#include "websocket_codec.h"
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

//////////////////////////////////////////////////////////////////////////
uint64_t ntoh64(uint64_t input)
{
    uint64_t rval;
    uint8_t *data = (uint8_t *)&rval;

    data[0] = input >> 56;
    data[1] = input >> 48;
    data[2] = input >> 40;
    data[3] = input >> 32;
    data[4] = input >> 24;
    data[5] = input >> 16;
    data[6] = input >> 8;
    data[7] = input >> 0;

    return rval;
}

uint64_t hton64(uint64_t input)
{
    return (ntoh64(input));
}
//////////////////////////////////////////////////////////////////////////

void parse_ws_fixed_head(const char* data, ws_frame_fixed_head_t* out)
{
    // len(data) >= 2 && NULL != out
    const unsigned char* p = (const unsigned char* )data;
    out->fin_ = p[0] >> 7;
    out->rsv_ = (p[0] >> 4) & 0x07;
    out->opcode_ = p[0] & 0x0f;
    out->has_mask_ = p[1] >> 7;
    out->payload_len_ = p[1] & 0x7f;
}

bool is_client_ws_frame(const char* data, size_t size)
{
    if (size < WS_FIXED_HEAD_LEN)
    {
        return false;
    }

    ws_frame_fixed_head_t head;
    parse_ws_fixed_head(data, &head);
    return 0 == head.rsv_ && 1 == head.has_mask_;
}

uint64_t get_ws_frame_len( const char* data, size_t size )
{
    ws_frame_fixed_head_t head;
    memset(&head, 0, sizeof(head));
    if (size < WS_FIXED_HEAD_LEN) return 0;
    parse_ws_fixed_head(data, &head);
    uint64_t payload_len = 0;
    int offset = WS_FIXED_HEAD_LEN;
    if (head.payload_len_ < 126)
    {
        payload_len = head.payload_len_;
    }
    else if (126 == head.payload_len_)
    {
        // ushort
        unsigned short* len = (unsigned short*)&data[offset];
        if (size < offset+sizeof(unsigned short)) return 0;
        payload_len = ntohs(*len);
        offset += sizeof(unsigned short);
    }
    else
    {
        // 127 uint64
        uint64_t *len = (uint64_t *)&data[offset];
        if (size < offset+sizeof(uint64_t)) return 0;
        payload_len = ntoh64(*len);
        offset += sizeof(uint64_t);
    }

    if (head.has_mask_)
    {
        offset += WS_MASK_LEN;
    }

    if (size < offset + payload_len) return 0;
    return offset + payload_len;
}

ws_frame_t::ws_frame_t():
    payload_len_(0),
    payload_(NULL)
{
    memset(&head_, 0, sizeof(head_));
    memset(mask_, 0, sizeof(mask_));
    head_.fin_ = 1;
}

ws_frame_t::~ws_frame_t()
{
    if (payload_)
    {
        delete []payload_;
        payload_ = NULL;
    }
    
}

int ws_frame_t::from_bytes( const void* data, size_t data_len, unsigned int flag )
{
    const char* p = (const char*)data;
    if (data_len < WS_FIXED_HEAD_LEN) return -1;
    parse_ws_fixed_head(p, &head_);
    int offset = WS_FIXED_HEAD_LEN;
    if (head_.payload_len_ < 126)
    {
        payload_len_ = head_.payload_len_;
    }
    else if (126 == head_.payload_len_)
    {
        // ushort
        unsigned short* len = (unsigned short*)&p[offset];
        if (data_len < offset+sizeof(unsigned short)) return -offset;
        payload_len_ = ntohs(*len);
        offset += sizeof(unsigned short);
    }
    else
    {
        // 127 uint64
        uint64_t *len = (uint64_t *)&p[offset];
        if (data_len < offset+sizeof(uint64_t)) return -offset;
        payload_len_ = ntoh64(*len);
        offset += sizeof(uint64_t);
    }

    if (head_.has_mask_)
    {
        if (data_len < offset + sizeof(mask_)) return -offset;
        memcpy(mask_, &p[offset], sizeof(mask_));
        offset += sizeof(mask_);
    }

    if (data_len < offset + payload_len_) return -offset;

    if (flag & PWF_UNMASK_PAYLOAD)
    {
        const char* payload = &p[offset];
        payload_ = new char[payload_len_];
        if (NULL == payload_) return -1;
        for (uint64_t i = 0; i < payload_len_; ++i)
        {
            payload_[i] = payload[i] ^ mask_[i % sizeof(mask_)];
        }
    }

    return offset + payload_len_;
}

int ws_frame_t::pack_head( void* data, size_t data_len, unsigned int flag )
{
    if (data_len < WS_FIXED_HEAD_LEN) return -1;
    int offset = WS_FIXED_HEAD_LEN;
    unsigned char* out = (unsigned char*)data;
    out[0] = ((head_.fin_&0x01) << 7) | 
        ((head_.rsv_&0x07) << 4) | 
        (head_.opcode_&0x0f);
    if (payload_len_ < 126)
    {
        out[1] = payload_len_;
    }
    else if (payload_len_ < 65535)
    {
        out[1] = 126;
        unsigned short* plen = (unsigned short*)&out[offset];
        if (data_len < offset + sizeof(unsigned short)) return -1;
        *plen = htons((unsigned short)payload_len_);
        offset += sizeof(unsigned short);
    }
    else
    {
        out[1] = 127;
        uint64_t* plen = (uint64_t*)&out[offset];
        if (data_len < offset + sizeof(uint64_t)) return -1;
        *plen = hton64(payload_len_);
        offset += sizeof(uint64_t);
    }

    out[1] = out[1] | (head_.has_mask_&0x01 << 7);
    // TODO: mask key
    if (flag & PWF_MOVE_TO_END)
    {
        unsigned char* dst = out + data_len - offset;
        memmove(dst, data, offset);
    }

    return offset;
}
