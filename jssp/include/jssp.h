#ifndef __JSSP_H
#define __JSSP_H

#include <jspp.h>
#include <stdbool.h>

#define JSSP_MAX_PATH (((JSON_MAX_STACK + 3) & ~3) - 1)
#if JSSP_MAX_PATH > 31
#error "Too deep for this implementation"
#endif

/**
 * A structure that represent current parsing context, which is
 * passed to the data handlers.
 */
typedef struct _jssp_context {
    void *   result;                ///< Pointer to the `result` that was passed to the `jssp_start`
    uint32_t path_markers;          ///< A set of Object/Array markers
    uint8_t  path_end;              ///< Index of the last path element
    uint8_t  path[JSSP_MAX_PATH];   ///< Array where each element [0..path_end] represents either an object member name or an array index
} jssp_context_t;

/**
 * A helper function to determine whether the last level of the context represents an array.
 * \param  context  Pointer to the parser context.
 * \return true if the top level of the context is an array.
 */
static inline bool jssp_context_is_array(jssp_context_t * context)
{
    return context->path_markers & 1 << context->path_end;
}

/**
 * Type of the generated object member names scanner.
 * \param  state      The current state of the name scanner.
 * \param  next_char  The next/lookahead character.
 * \return The new automaton state
 */
typedef uint32_t (*jssp_name_scanner_cb_t)(uint32_t state, char next_char);

/**
 * Type of the data handling callback functions.
 * \param  data         Pointer to the beginning of the data
 * \param  data_length  Length of the data
 * \param  data_type    Type of the JSON data - either partial (`JSON_NUMBER_PART`, `JSON_STRING_PART`)
 *                      or one of the literals (`JSON_NULL`, `JSON_TRUE`, `JSON_FALSE`)
 *                      or numeric types (`JSON_INTEGER`, `JSON_DECIMAL`, `JSON_FLOATING_POINT`)
 *                      or a string (`JSON_STRING`)
 * \param  context      Pointer to the parsing context
 */
typedef void (*data_cb_t)(const char * data, uint16_t data_length, uint8_t data_type, const jssp_context_t * context);

/**
 * Type of the generated path matching automaton.
 * \param  state      The current state of the path mathing automaton.
 * \param  next_elem  The JSPP token or member name ID
 * \param[out] action Pointer to the variable with the data handling callback
 * \return The new automaton state or 0 when `next_elem` cannot be accepted in the current state.
 */
typedef uint32_t (*jssp_path_matcher_cb_t)(uint32_t state, uint8_t next_elem, data_cb_t * action);

/**
 * A structure that represents JSSP internal state.
 */
typedef struct _json_struct_parser {
    jspp_t                  json_parser;                ///< embedded JSPP
    jssp_path_matcher_cb_t  path_matcher_next_state;    ///< Pointer to the path matching automaton
    jssp_name_scanner_cb_t  name_scanner_next_state;    ///< Pointer to the name scanning automaton
    uint32_t                name_scanner_state;         ///< Current name scanner state
    uint32_t                path_matcher_state;         ///< Current path matching state
    jssp_context_t          context;                    ///< Data handling context
} jssp_t;

/**
 * A helper function that associates JSSP and the compiled JSON specification (name and path automata).
 * \param  jssp             Pointer to the JSSP structure.
 * \param  path_matcher_cb  Pointer to the path matching automaton function that calculates path matching transitions.
 * \param  name_scanner_cb  Pointer to the object memeber name scanner automaton function that calculates name scanner transitions.
 */
static inline void jssp_init(jssp_t * jssp, jssp_path_matcher_cb_t path_matcher_cb, jssp_name_scanner_cb_t name_scanner_cb)
{
    jssp->path_matcher_next_state = path_matcher_cb;
    jssp->name_scanner_next_state = name_scanner_cb;
}

/**
 * Function that starts JSON-to-Struct parsing.
 * \param  jssp         Pointer to the JSSP structure.
 * \param  result       Pointer to the memory (structure) where data processing handlers will store matched data.
 * \param  json_text    Pointer to the bufger with the text of the first (or the only) JSON fragment.
 * \param  text_length  Length of the first fragment (in bytes).
 * \return JSON token that terminated first fragment parsing. It will be either `JSON_END` if JSON paylod fit entirely
 *         into the first fragment and parsing was successful, or `JSON_CONTINUE` if more data are expected/needed to
 *         finish parsing, or an error token - `JSON_INVALID` or `JSON_TOO_DEEP`
 */
uint8_t jssp_start(jssp_t * jssp, void * result, const char * json_text, uint16_t text_length);

/**
 * Function that continues JSON-to-Struct parsing using the second and subsequent fragments.
 * \param  jssp         Pointer to the JSSP structure.
 * \param  json_text    Pointer to the bufger with the text of the JSON fragment.
 * \param  text_length  Length of the text (in bytes).
 * \return JSON token that terminated first fragment parsing. It will be either `JSON_END` if JSON paylod ended
 *         in the current and parsing was successful, or `JSON_CONTINUE` if more data are expected/needed to
 *         finish parsing, or an error token - `JSON_INVALID` or `JSON_TOO_DEEP`
 */
uint8_t jssp_continue(jssp_t * jssp, const char * json_text, uint16_t text_length);

#endif