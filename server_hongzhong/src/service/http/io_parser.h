

#ifndef __WS_PARSER_H_
#define __WS_PARSER_H_
////////////////////////////////////////////////////////////////////////////////
#include "io_header.h"
////////////////////////////////////////////////////////////////////////////////
namespace http{
////////////////////////////////////////////////////////////////////////////////
inline bool is_char(char c){
    return c >= 0 && c <= 127;
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_ctl(char c){
    return (c >= 0 && c <= 31) || (c == 127);
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_digit(char c){
    return c >= '0' && c <= '9';
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_tspecial(char c){
    switch (c){
    case '(': case ')': case '<': case '>':  case '@':
    case ',': case ';': case ':': case '\\': case '"':
    case '/': case '[': case ']': case '?':  case '=':
    case '{': case '}': case ' ': case '\t':
        return true;
    }
    return false;
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_echo_char(char c){
   return (is_char(c) && !is_ctl(c));
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_word_char(char c){
    return (is_echo_char(c) && !is_tspecial(c));
}
////////////////////////////////////////////////////////////////////////////////
inline bool is_letter_char(char c){
    return (is_word_char(c) && !is_digit(c));
}
////////////////////////////////////////////////////////////////////////////////
class uri_parser{
public:
    enum result_type {good, bad, indeterminate};
    template <typename _Iter>
    inline std::tuple<result_type, _Iter> parse(
        uri&  arg,
        _Iter begin,
        _Iter end,
        int   limit = 0)
    {
        result_type result = indeterminate;
        while (begin <= end){
            result = consume(arg, *begin++, limit);
            if (result == good || result == bad){
                reset();
                break;
            }
        }
        return std::make_tuple(result, begin);
    }
    inline uri_parser(){reset();}
    inline void reset(){m_state = parameter_url_start;}
private:
    enum{
        parameter_url_start,
        parameter_url,
        parameter_first,
        parameter_name_start,
        parameter_name,
        parameter_value_start,
        parameter_value
    } m_state;
private:
    inline result_type consume(uri& avg, char input, int limit){
        if (input == '\0'){
            if (avg.url.empty())
                avg.url = "/";
            return good;
        }
        switch (m_state){
        case parameter_url_start:
            avg.clear();
            if (!is_echo_char(input)){
                return bad;
            } else if (input == '?'){
                m_state = parameter_name_start;
                return indeterminate;
            } else {
                avg.url.push_back(input);
                m_state = parameter_url;
                return indeterminate;
            }
        case parameter_url:
            if (!is_echo_char(input)){
                return bad;
            } else if (input == '?'){
                m_state = parameter_name_start;
                return indeterminate;
            } else {
                avg.url.push_back(input);
                return indeterminate;
            }
        case parameter_name_start:
            if (!is_word_char(input)){
                return bad;
            } else {
                m_state = parameter_name;
                avg.params.push_back(header());
                avg.params.back().name.push_back(input);
                return indeterminate;
            }
        case parameter_name:
            if (input == '='){
                m_state = parameter_value_start;
                return indeterminate;
            } else if (!is_word_char(input)){
                return bad;
            } else {
                avg.params.back().name.push_back(input);
                return indeterminate;
            }
        case parameter_value_start:
            if (!is_echo_char(input)){
                return bad;
            } else {
                m_state = parameter_value;
                avg.params.back().value.push_back(input);
                return indeterminate;
            }
        case parameter_value:
            if (input == '&'){
                m_state = parameter_name_start;
                return indeterminate;
            } else if (!is_echo_char(input)){
                return bad;
            } else {
                avg.params.back().value.push_back(input);
                return indeterminate;
            }
        }
        return bad;
    }
};
////////////////////////////////////////////////////////////////////////////////
class wsp_parser{
public:
    enum result_type {good, bad, indeterminate};
    template <typename _Iter>
    inline std::tuple<result_type, _Iter> parse(
        request &req,
        _Iter    begin,
        _Iter    end,
        int      limit = 0)
    {
        result_type result = indeterminate;
        while (begin != end){
            result = consume(req.wsp, *begin++, limit);
            if (result == good || result == bad){
                reset();
                break;
            }
        }
        if (result == good){
            char *p = (char*)req.wsp.data();
            for (int i = 0; i < req.wsp.size(); i++){
                p[i] ^= req.wsp.mask[i % 4];
            }
        }
        return std::make_tuple(result, begin);
    }
    inline void reset(){
        m_readed = 0;
        m_state = packet_head_1;
    }
    inline static const char* pack(
        std::string &result,
        const char  *begin,
        const char  *end,
        int          type = 1,
        unsigned int mask = 0)
    {
        int size = (int)(end - begin);
        unsigned char byte = 0;
        int eof = 0, ext = 0;
        int have_mask = mask ? 1 : 0;
        int base = (size < 126) ? 125 : 0xffff;

        int rest = size;
        int len = (rest > base) ? base : rest;
        eof = (len >= size) ? 1 : 0;

        byte = (unsigned char)(eof << 7);
        byte |= (ext << 6);
        byte |= (type & 0x0f);
        result.clear();
        result.append((char*)&byte, 1);

        byte = (have_mask << 7);
        if (base == 125){
            byte |= (unsigned char)len;
            result.append((char*)&byte, 1);
        } else {
            byte |= 126;
            result.append(((char*)&byte), 1);
            result.append(((char*)&len) + 1, 1);
            result.append(((char*)&len), 1);
        }
        if (have_mask){
            result.append((char*)&mask, 4);
            unsigned char *p = (unsigned char*)&mask;
            for (int i = 0; i < len; i++){
                unsigned char c = begin[i] ^ p[i % 4];
                result.append((char*)&c, 1);
            }
        }
        if (len > 0 )
            result.append((char*)begin, len);
        return (begin + len);
    }
    inline wsp_parser(){reset();}
private:
    enum step{
        packet_head_1,
        packet_head_2,
        packet_length_1,
        packet_length_2,
        packet_length_3,
        packet_length_4,
        packet_length_5,
        packet_length_6,
        packet_length_7,
        packet_length_8,
        packet_mask_1,
        packet_mask_2,
        packet_mask_3,
        packet_mask_4,
        packet_content
    } m_state;
    int m_readed;
private:
    inline int size(packet &req) const{
        return (req.h2 & 0x7f);
    }
    inline step next(packet &req) const{
        int bytes = size(req);
        if (bytes < 126){
            step next = packet_mask_1;
            if (!req.is_mask())
                next = packet_content;
            return next;
        }
        int n = (bytes == 126) ? 6 : 0;
        return (step)(packet_length_1 + n);
    }
    inline result_type consume(packet &req, char input, int limit){
        switch (m_state){
        case packet_head_1:
            req.clear();
            req.h1 = input;
            m_state = packet_head_2;
            return indeterminate;
        case packet_head_2:
            req.h2 = input;
            if (!size(req) && !req.is_mask()){
                return good;
            }
            m_state = next(req);
            return indeterminate;
        case packet_length_1:
            req.length[7] = input;
            m_state = packet_length_2;
            return indeterminate;
        case packet_length_2:
            req.length[6] = input;
            m_state = packet_length_3;
            return indeterminate;
        case packet_length_3:
            req.length[5] = input;
            m_state = packet_length_4;
            return indeterminate;
        case packet_length_4:
            req.length[4] = input;
            m_state = packet_length_5;
            return indeterminate;
        case packet_length_5:
            req.length[3] = input;
            m_state = packet_length_6;
            return indeterminate;
        case packet_length_6:
            req.length[2] = input;
            m_state = packet_length_7;
            return indeterminate;
        case packet_length_7:
            req.length[1] = input;
            m_state = packet_length_8;
            return indeterminate;
        case packet_length_8:
            req.length[0] = input;
            if (req.is_mask()){
                m_state = packet_mask_1;
            } else {
                m_state = packet_content;
            }
            return indeterminate;
        case packet_mask_1:
            req.mask[0] = input;
            m_state = packet_mask_2;
            return indeterminate;
        case packet_mask_2:
            req.mask[1] = input;
            m_state = packet_mask_3;
            return indeterminate;
        case packet_mask_3:
            req.mask[2] = input;
            m_state = packet_mask_4;
            return indeterminate;
        case packet_mask_4:
            req.mask[3] = input;
            if (!size(req)){
                return good;
            }
            m_state = packet_content;
            return indeterminate;
        case packet_content:
            m_readed++;
            req.queue->write(&input, 1);
            if (limit && req.queue->used() > limit)
                return bad;
            if (req.is_eof() && m_readed == req.size())
                return good;
            return indeterminate;
        }
        return bad;
    }
};
////////////////////////////////////////////////////////////////////////////////
class request_parser{
public:
    enum result_type{good, bad, indeterminate};
    template <typename _Iter>
    inline std::tuple<result_type, _Iter> parse(
        request& req,
        _Iter    begin,
        _Iter    end,
        int      limit = 0)
    {
        result_type result = indeterminate;
        while (begin != end){
            result = consume(req, *begin++, limit);
            if (result == good || result == bad){
                reset();
                break;
            }
        }
        if (result == good){
            uri_parser parser;
            uri_parser::result_type r;
            std::tie(r, std::ignore) = parser.parse(
                req.uri,
                req.url.c_str(),
                req.url.c_str() + req.url.size(),
                0
                );
            if (result != uri_parser::good){
                req.uri.clear();
            }
        }
        return std::make_tuple(result, begin);
    }
    inline request_parser(){reset();}
    inline void reset(){m_state = method_start;}
private:
    enum{
        method_start,
        method,
        url,
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } m_state;
private:
    inline result_type consume(request& req, char input, int limit){
        switch (m_state){
        case method_start:
            req.clear();
            if (!is_letter_char(input)){
                return bad;
            } else {
                char upper = toupper(input);
                req.method.push_back(upper);
                m_state = method;
                return indeterminate;
            }
        case method:
            if (input == ' '){
                m_state = url;
                return indeterminate;
            } else if (!is_letter_char(input)){
                return bad;
            } else {
                char upper = toupper(input);
                req.method.push_back(upper);
                if (limit && (int)req.method.size() > limit)
                    return bad;
                return indeterminate;
            }
        case url:
            if (input == ' '){
                req.url = url::decode(req.url);
                m_state = http_version_h;
                return indeterminate;
            } else if (!is_echo_char(input)){
                return bad;
            } else {
                req.url.push_back(input);
                if (limit && (int)req.url.size() > limit)
                    return bad;
                return indeterminate;
            }
        case http_version_h:
            if (toupper(input) == 'H'){
                m_state = http_version_t_1;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_t_1:
            if (toupper(input) == 'T'){
                m_state = http_version_t_2;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_t_2:
            if (toupper(input) == 'T'){
                m_state = http_version_p;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_p:
            if (toupper(input) == 'P'){
                m_state = http_version_slash;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_slash:
            if (input == '/'){
                m_state = http_version_major_start;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_major_start:
            if (is_digit(input)){
                req.http_version_major = req.http_version_major * 10 + input - '0';
                m_state = http_version_major;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_major:
            if (input == '.'){
                m_state = http_version_minor_start;
                return indeterminate;
            } else if (is_digit(input)) {
                req.http_version_major = req.http_version_major * 10 + input - '0';
                if (req.http_version_major > 100)
                    return bad;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_minor_start:
            if (is_digit(input)){
                req.http_version_minor = req.http_version_minor * 10 + input - '0';
                m_state = http_version_minor;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_minor:
            if (input == '\r'){
                m_state = expecting_newline_1;
                return indeterminate;
            } else if (is_digit(input)) {
                req.http_version_minor = req.http_version_minor * 10 + input - '0';
                if (req.http_version_minor > 100)
                    return bad;
                return indeterminate;
            } else {
                return bad;
            }
        case expecting_newline_1:
            if (input == '\n'){
                m_state = header_line_start;
                return indeterminate;
            } else {
                return bad;
            }
        case header_line_start:
            if (input == '\r'){
                m_state = expecting_newline_3;
                return indeterminate;
            } else if (req.headers.size() && (input == ' ' || input == '\t')){
                m_state = header_lws;
                return indeterminate;
            } else if (!is_word_char(input)){
                return bad;
            } else {
                req.headers.push_back(header());
                req.headers.back().name.push_back(input);
                m_state = header_name;
                return indeterminate;
            }
        case header_lws:
            if (input == '\r'){
                m_state = expecting_newline_2;
                return indeterminate;
            } else if (input == ' ' || input == '\t'){
                return indeterminate;
            } else if (!is_echo_char(input)){
                return bad;
            } else {
                m_state = header_value;
                req.headers.back().value.push_back(input);
                return indeterminate;
            }
        case header_name:
            if (input == ':'){
                m_state = space_before_header_value;
                return indeterminate;
            } else if (!is_word_char(input)){
                return bad;
            } else {
                req.headers.back().name.push_back(input);
                if (limit && (int)req.headers.back().name.size() > limit)
                    return bad;
                return indeterminate;
            }
        case space_before_header_value:
            if (input == ' '){
                m_state = header_value;
                return indeterminate;
            } else {
                return bad;
            }
        case header_value:
            if (input == '\r'){
                m_state = expecting_newline_2;
                return indeterminate;
            } else if (!is_echo_char(input)){
                return bad;
            } else {
                req.headers.back().value.push_back(input);
                if (limit && (int)req.headers.back().value.size() > limit)
                    return bad;
                return indeterminate;
            }
        case expecting_newline_2:
            if (input == '\n'){
                m_state = header_line_start;
                return indeterminate;
            } else {
                return bad;
            }
        case expecting_newline_3:{
                return (input == '\n') ? good : bad;
            }
        }
        return bad;
    }
};
////////////////////////////////////////////////////////////////////////////////
class response_parser{
public:
    enum result_type {good, bad, indeterminate};
    template <typename _Iter>
    inline std::tuple<result_type, _Iter> parse(
        response& res, _Iter begin, _Iter end){
        while (begin != end)
        {
            result_type result = consume(res, *begin++);
            if (result == good || result == bad){
                reset();
                return std::make_tuple(result, begin);
            }
        }
        return std::make_tuple(indeterminate, begin);
    }
    inline response_parser(){reset();}
    inline void reset(){m_state = http_version_h;}
private:
    enum state{
        http_version_h,
        http_version_t_1,
        http_version_t_2,
        http_version_p,
        http_version_slash,
        http_version_major_start,
        http_version_major,
        http_version_minor_start,
        http_version_minor,
        status_start,
        status,
        result,
        expecting_newline_1,
        header_line_start,
        header_lws,
        header_name,
        space_before_header_value,
        header_value,
        expecting_newline_2,
        expecting_newline_3
    } m_state;
    size_t m_offset;
private:
    inline result_type consume(response& res, char input){
        switch (m_state)
        {
        case http_version_h:
            res.clear();
            if (!is_char(input) || is_ctl(input) || is_tspecial(input)){
                return bad;
            } else if (input == 'H' || input == 'h'){
                m_state = http_version_t_1;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_t_1:
            if (input == 'T' || input == 't'){
                m_state = http_version_t_2;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_t_2:
            if (input == 'T' || input == 't'){
                m_state = http_version_p;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_p:
            if (input == 'P' || input == 'p'){
                m_state = http_version_slash;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_slash:
            if (input == '/'){
                m_state = http_version_major_start;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_major_start:
            if (is_digit(input)){
                res.http_version_major = res.http_version_major * 10 + input - '0';
                m_state = http_version_major;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_major:
            if (input == '.'){
                m_state = http_version_minor_start;
                return indeterminate;
            } else if (is_digit(input)) {
                res.http_version_major = res.http_version_major * 10 + input - '0';
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_minor_start:
            if (is_digit(input)){
                res.http_version_minor = res.http_version_minor * 10 + input - '0';
                m_state = http_version_minor;
                return indeterminate;
            } else {
                return bad;
            }
        case http_version_minor:
            if (input == ' '){
                m_state = status_start;
                return indeterminate;
            } else if (is_digit(input)) {
                res.http_version_minor = res.http_version_minor * 10 + input - '0';
                return indeterminate;
            } else {
                return bad;
            }
        case status_start:
            if (!is_digit(input)){
                return bad;
            } else {
                m_state = status;
                res.status.push_back(input);
                return indeterminate;
            }
        case status:
            if (input == ' '){
                m_state = result;
                return indeterminate;
            } else if (!is_digit(input)){
                return bad;
            } else {
                res.status.push_back(input);
                return indeterminate;
            }
        case result:
            if (input == '\r'){
                m_state = expecting_newline_1;
                return indeterminate;
            } else if (is_char(input)){
                res.result.push_back(input);
                return indeterminate;
            } else {
                return bad;
            }
        case expecting_newline_1:
            if (input == '\n'){
                m_state = header_line_start;
                return indeterminate;
            } else {
                return bad;
            }
        case header_line_start:
            if (input == '\r'){
                m_state = expecting_newline_3;
                return indeterminate;
            } else if (!res.headers.empty() && (input == ' ' || input == '\t')){
                m_state = header_lws;
                return indeterminate;
            } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)){
                return bad;
            } else {
                res.headers.push_back(header());
                res.headers.back().name.push_back(input);
                m_state = header_name;
                return indeterminate;
            }
        case header_lws:
            if (input == '\r'){
                m_state = expecting_newline_2;
                return indeterminate;
            } else if (input == ' ' || input == '\t'){
                return indeterminate;
            } else if (is_ctl(input)){
                return bad;
            } else {
                m_state = header_value;
                res.headers.back().value.push_back(input);
                return indeterminate;
            }
        case header_name:
            if (input == ':'){
                m_state = space_before_header_value;
                return indeterminate;
            } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)){
                return bad;
            } else {
                res.headers.back().name.push_back(input);
                if (res.headers.back().name.size() > 128)
                    return bad;
                return indeterminate;
            }
        case space_before_header_value:
            if (input == ' '){
                m_state = header_value;
                return indeterminate;
            } else {
                return bad;
            }
        case header_value:
            if (input == '\r'){
                m_state = expecting_newline_2;
                return indeterminate;
            } else if (is_ctl(input)){
                return bad;
            } else {
                res.headers.back().value.push_back(input);
                if (res.headers.back().value.size() > 512)
                    return bad;
                return indeterminate;
            }
        case expecting_newline_2:
            if (input == '\n'){
                m_state = header_line_start;
                return indeterminate;
            } else {
                return bad;
            }
        case expecting_newline_3:
            return (input == '\n') ? good : bad;
        default: return bad;
        }
    }
};
////////////////////////////////////////////////////////////////////////////////
} // namespace http
////////////////////////////////////////////////////////////////////////////////
#endif //__WS_PARSER_H_
