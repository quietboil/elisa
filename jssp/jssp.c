#include "jssp.h"
#include <stdlib.h>

static inline void jssp_context_init(jssp_context_t * context, void * result)
{
    context->result = result;
    context->path_markers = 0;
    context->path_end = UINT8_MAX;
}

static inline void jssp_context_mark_object(jssp_context_t * context)
{
    context->path_markers &= ~(1 << context->path_end);
}

static inline void jssp_context_mark_array(jssp_context_t * context)
{
    context->path_markers |= 1 << context->path_end;
}

static inline void jssp_context_update(jssp_context_t * context, uint8_t token)
{
    switch (token) {
        case JSON_OBJECT_BEGIN: {
            ++context->path_end;
            jssp_context_mark_object(context);
            break;
        }
        case JSON_ARRAY_BEGIN: {
            ++context->path_end;
            jssp_context_mark_array(context);
            context->path[context->path_end] = UINT8_MAX;
            break;
        }
        case JSON_OBJECT_END:
        case JSON_ARRAY_END: {
            --context->path_end;
            break;
        }
        default: {
            if (jssp_context_is_array(context)) {
                // update array element index
                ++context->path[context->path_end];
            }
        }
    }
}

static inline void jssp_context_update_path_tail(jssp_context_t * context, uint8_t name_id)
{
    context->path[context->path_end] = name_id;
}

static uint32_t jssp_scan_name(uint32_t state, jssp_name_scanner_cb_t next_state, const char * name_ptr, uint16_t name_length)
{
    while (state && name_length > 0) {
        state = next_state(state, *name_ptr);
        ++name_ptr;
        --name_length;
    }
    return state;
}

static uint8_t jssp_parse(jssp_t * parser, uint8_t token)
{
    data_cb_t process_data = NULL;
    while (token > JSON_CONTINUE) {
        jssp_context_update(&parser->context, token);
        if (token == JSON_MEMBER_NAME_PART) {
            uint16_t name_length;
            const char * name = jspp_text(&parser->json_parser, &name_length);
            parser->name_scanner_state = jssp_scan_name(parser->name_scanner_state, parser->name_scanner_next_state, name, name_length);
            token = JSON_CONTINUE;
            break;
        }
        if (token == JSON_MEMBER_NAME) {
            uint16_t name_length;
            const char * name = jspp_text(&parser->json_parser, &name_length);
            uint32_t name_id = jssp_scan_name(parser->name_scanner_state, parser->name_scanner_next_state, name, name_length);
            if (name_id) {
                token = name_id;
                jssp_context_update_path_tail(&parser->context, token);
            }
            parser->name_scanner_state = 1;
        }
        uint8_t next_path_state = parser->path_matcher_next_state(parser->path_matcher_state, token, &process_data);
        if (!next_path_state) {
            token = jspp_skip(&parser->json_parser);
        } else {
            parser->path_matcher_state = next_path_state;
            if (process_data) {
                uint16_t data_length;
                const char * data = jspp_text(&parser->json_parser, &data_length);
                process_data(data, data_length, token, &parser->context);
                process_data = NULL;
            }
            token = jspp_next(&parser->json_parser);
        }
    }
    return token;
}

uint8_t jssp_start(jssp_t * parser, void * result, const char * json_text, uint16_t text_length)
{
    parser->path_matcher_state = 1;
    parser->name_scanner_state = 1;
    jssp_context_init(&parser->context, result);

    uint8_t token = jspp_start(&parser->json_parser, json_text, text_length);
    return jssp_parse(parser, token);
}

uint8_t jssp_continue(jssp_t * parser, const char * json_text, uint16_t text_length)
{
    uint8_t token = jspp_continue(&parser->json_parser, json_text, text_length);
    return jssp_parse(parser, token);
}
