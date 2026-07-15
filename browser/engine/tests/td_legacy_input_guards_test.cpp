/*
 * Copyright 2026 The Vanilla Conquer Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * The guarded functions live in the global-state legacy engine and cannot be
 * invoked by the asset-free adapter test binary without constructing a retail
 * scenario. This source-structure test keeps narrow trust and WebTD rendering
 * checks adjacent to the legacy code they protect. A future semantic rewrite
 * must update this test explicitly and therefore receive review.
 */

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::string ReadFile(const char* path)
{
    std::ifstream input(path, std::ios::in | std::ios::binary);
    assert(input.good());
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

size_t MatchingBrace(const std::string& source, size_t opening)
{
    enum State
    {
        NORMAL,
        LINE_COMMENT,
        BLOCK_COMMENT,
        STRING_LITERAL,
        CHARACTER_LITERAL
    };

    State state = NORMAL;
    bool escaped = false;
    unsigned int depth = 0u;
    for (size_t index = opening; index < source.size(); ++index) {
        const char current = source[index];
        const char next = index + 1u < source.size() ? source[index + 1u] : '\0';
        if (state == LINE_COMMENT) {
            if (current == '\n') {
                state = NORMAL;
            }
            continue;
        }
        if (state == BLOCK_COMMENT) {
            if (current == '*' && next == '/') {
                state = NORMAL;
                ++index;
            }
            continue;
        }
        if (state == STRING_LITERAL || state == CHARACTER_LITERAL) {
            if (escaped) {
                escaped = false;
            } else if (current == '\\') {
                escaped = true;
            } else if ((state == STRING_LITERAL && current == '"')
                       || (state == CHARACTER_LITERAL && current == '\'')) {
                state = NORMAL;
            }
            continue;
        }
        if (current == '/' && next == '/') {
            state = LINE_COMMENT;
            ++index;
        } else if (current == '/' && next == '*') {
            state = BLOCK_COMMENT;
            ++index;
        } else if (current == '"') {
            state = STRING_LITERAL;
        } else if (current == '\'') {
            state = CHARACTER_LITERAL;
        } else if (current == '{') {
            ++depth;
        } else if (current == '}') {
            assert(depth != 0u);
            --depth;
            if (depth == 0u) {
                return index;
            }
        }
    }
    assert(false && "legacy function has no matching closing brace");
    return std::string::npos;
}

std::string FunctionBody(const std::string& source, const char* signature)
{
    const size_t signature_position = source.find(signature);
    assert(signature_position != std::string::npos);
    assert(source.find(signature, signature_position + 1u) == std::string::npos);
    const size_t opening = source.find('{', signature_position);
    assert(opening != std::string::npos);
    const size_t closing = MatchingBrace(source, opening);
    return source.substr(opening, closing - opening + 1u);
}

bool IdentifierStart(char value)
{
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || value == '_';
}

bool IdentifierPart(char value)
{
    return IdentifierStart(value) || (value >= '0' && value <= '9');
}

std::vector<std::string> Tokens(const std::string& source)
{
    std::vector<std::string> tokens;
    for (size_t index = 0u; index < source.size();) {
        const char current = source[index];
        const char next = index + 1u < source.size() ? source[index + 1u] : '\0';
        if (current == '/' && next == '/') {
            index += 2u;
            while (index < source.size() && source[index] != '\n') {
                ++index;
            }
        } else if (current == '/' && next == '*') {
            index += 2u;
            while (index + 1u < source.size() && !(source[index] == '*' && source[index + 1u] == '/')) {
                ++index;
            }
            assert(index + 1u < source.size());
            index += 2u;
        } else if (IdentifierStart(current)) {
            const size_t start = index++;
            while (index < source.size() && IdentifierPart(source[index])) {
                ++index;
            }
            tokens.push_back(source.substr(start, index - start));
        } else if (current >= '0' && current <= '9') {
            const size_t start = index++;
            while (index < source.size()
                   && ((source[index] >= '0' && source[index] <= '9') || IdentifierStart(source[index]))) {
                ++index;
            }
            tokens.push_back(source.substr(start, index - start));
        } else if ((current == '>' && next == '=') || (current == '<' && next == '=')
                   || (current == '=' && next == '=') || (current == '!' && next == '=')
                   || (current == '&' && next == '&') || (current == '|' && next == '|')
                   || (current == '<' && next == '<') || (current == '>' && next == '>')
                   || (current == '-' && next == '>')) {
            tokens.push_back(source.substr(index, 2u));
            index += 2u;
        } else if (current == '"' || current == '\'') {
            const char terminator = current;
            const size_t start = index++;
            bool escaped = false;
            while (index < source.size()) {
                const char value = source[index++];
                if (escaped) {
                    escaped = false;
                } else if (value == '\\') {
                    escaped = true;
                } else if (value == terminator) {
                    break;
                }
            }
            tokens.push_back(source.substr(start, index - start));
        } else if (current == ' ' || current == '\t' || current == '\r' || current == '\n') {
            ++index;
        } else {
            tokens.push_back(source.substr(index, 1u));
            ++index;
        }
    }
    return tokens;
}

size_t FindSequence(const std::vector<std::string>& tokens,
                    const char* const* expected,
                    size_t expected_count,
                    size_t start)
{
    for (size_t offset = start; offset + expected_count <= tokens.size(); ++offset) {
        bool matches = true;
        for (size_t index = 0u; index < expected_count; ++index) {
            if (tokens[offset + index] != expected[index]) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return offset;
        }
    }
    return std::string::npos;
}

template <size_t Count>
size_t RequireSequence(const std::vector<std::string>& tokens,
                       const char* const (&expected)[Count],
                       size_t start = 0u)
{
    const size_t position = FindSequence(tokens, expected, Count, start);
    assert(position != std::string::npos);
    return position;
}

size_t CountToken(const std::vector<std::string>& tokens, const char* value)
{
    size_t count = 0u;
    for (std::vector<std::string>::const_iterator token = tokens.begin(); token != tokens.end(); ++token) {
        if (*token == value) {
            ++count;
        }
    }
    return count;
}

void CheckPlacement(const std::string& source)
{
    const std::vector<std::string> tokens =
        Tokens(FunctionBody(source,
                            "bool DLLExportClass::Place("));
    const char* const x_assignment[] = {"int", "absolute_cell_x", "=", "map_cell_x", "+", "cell_x", ";"};
    const char* const y_assignment[] = {"int", "absolute_cell_y", "=", "map_cell_y", "+", "cell_y", ";"};
    const char* const bounds_guard[] = {"if", "(", "absolute_cell_x", "<", "0", "||",
                                        "absolute_cell_x", ">=", "MAP_MAX_CELL_WIDTH", "||",
                                        "absolute_cell_y", "<", "0", "||", "absolute_cell_y", ">=",
                                        "MAP_MAX_CELL_HEIGHT", ")", "{", "return", "false", ";", "}"};
    const char* const cell_conversion[] = {"CELL", "cell", "=", "(", "CELL", ")", "absolute_cell_x", "+",
                                           "(", "absolute_cell_y", "<<", "_map_width_shift_bits", ")", ";"};
    const char* const mutation[] = {"PlayerPtr", "->", "Place_Object", "(", "building", "->", "What_Am_I",
                                    "(", ")", ",", "cell", "+", "Map", ".", "ZoneOffset", ")"};

    const size_t x = RequireSequence(tokens, x_assignment);
    const size_t y = RequireSequence(tokens, y_assignment, x + 1u);
    const size_t guard = RequireSequence(tokens, bounds_guard, y + 1u);
    const size_t conversion = RequireSequence(tokens, cell_conversion, guard + 1u);
    const size_t place = RequireSequence(tokens, mutation, conversion + 1u);
    assert(x < y && y < guard && guard < conversion && conversion < place);
    for (size_t index = x; index < conversion; ++index) {
        /* The bounds guard must be active code, not hidden in a disabled
         * preprocessor branch like the historical sketch earlier in Place. */
        assert(tokens[index] != "#");
    }
    assert(CountToken(tokens, "absolute_cell_x") >= 4u);
    assert(CountToken(tokens, "absolute_cell_y") >= 4u);
}

void CheckSell(const std::string& source)
{
    const std::vector<std::string> tokens =
        Tokens(FunctionBody(source, "void DLLExportClass::Sell("));
    const char* const active_guard[] = {"if", "(", "!", "building", "->", "IsActive", ")"};
    const char* const demolition_guard[] = {"if", "(", "building", "->", "Can_Demolish", "(", ")", "&&",
                                            "building", "->", "House", "&&", "building", "->", "House", "->",
                                            "Class", "->", "House", "==", "PlayerPtr", "->", "Class", "->",
                                            "House", ")", "{", "building", "->", "Sell_Back", "(", "1", ")",
                                            ";", "}"};
    const size_t active = RequireSequence(tokens, active_guard);
    const size_t demolition = RequireSequence(tokens, demolition_guard, active + 1u);
    assert(active < demolition);
    assert(CountToken(tokens, "Can_Demolish") == 1u);
    assert(CountToken(tokens, "Sell_Back") == 1u);
}

void CheckSuperweapon(const std::string& source)
{
    const std::vector<std::string> tokens =
        Tokens(FunctionBody(source, "bool DLLExportClass::Place_Super_Weapon("));
    const char* const context_guard[] = {"DLLExportClass", ":", ":", "Set_Player_Context", "(", "player_id", ")"};
    const char* const ready_guard[] = {"if", "(", "!", "weapon", "->", "Is_Ready", "(", ")", ")"};
    const char* const radar_guard[] = {"if", "(", "!", "Map", ".", "In_Radar", "(", "cell", ")", ")"};
    const char* const mutation[] = {"OutList", ".", "Add", "(", "EventClass", "(", "EventClass", ":", ":",
                                    "SPECIAL_PLACE", ",", "weapon_type", ",", "cell", ")", ")"};
    const size_t context = RequireSequence(tokens, context_guard);
    const size_t ready = RequireSequence(tokens, ready_guard, context + 1u);
    const size_t radar = RequireSequence(tokens, radar_guard, ready + 1u);
    const size_t place = RequireSequence(tokens, mutation, radar + 1u);
    assert(context < ready && ready < radar && radar < place);
    assert(CountToken(tokens, "Is_Ready") == 1u);
    assert(CountToken(tokens, "SPECIAL_PLACE") == 1u);
}

void CheckVisiblePageExport(const std::string& source)
{
    const char* const signature = "CNC_Get_Visible_Page(";
    const size_t declaration = source.find(signature);
    assert(declaration != std::string::npos);
    const size_t definition = source.find(signature, declaration + 1u);
    assert(definition != std::string::npos);
    assert(source.find(signature, definition + 1u) == std::string::npos);
    const size_t opening = source.find('{', definition);
    assert(opening != std::string::npos);
    const size_t closing = MatchingBrace(source, opening);
    const std::vector<std::string> tokens = Tokens(source.substr(opening, closing - opening + 1u));

    const char* const export_buffer[] = {"GraphicBufferClass", "*", "gbuffer", "=", "HidPage", ".",
                                         "Get_Graphic_Buffer", "(", ")", ";"};
    RequireSequence(tokens, export_buffer);
    assert(CountToken(tokens, "SeenBuff") == 0u);
}

} // namespace

int main(int argc, char** argv)
{
    assert(argc == 2);
    const std::string source = ReadFile(argv[1]);
    CheckPlacement(source);
    CheckSell(source);
    CheckSuperweapon(source);
    CheckVisiblePageExport(source);
    return 0;
}
