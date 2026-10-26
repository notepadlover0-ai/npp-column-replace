#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace columnreplace {

enum class ColumnMode {
    FixedWidth, // startColumn/endColumn are character positions
    Delimited   // startColumn/endColumn are 1-based field numbers
};

// How the document bytes are interpreted. UTF-8 counts code points, while
// BytePerCharacter treats every byte as one character (used for ANSI
// documents, whose Scintilla buffer contains raw code page bytes).
enum class TextEncoding {
    Utf8,
    BytePerCharacter
};

// 1-based column numbers, end column inclusive.
// FixedWidth: endColumn == 0 means "to the end of the line".
// Delimited:  endColumn == 0 means "only the start field".
struct Options {
    std::string prefix;          // UTF-8, empty = every line
    bool caseSensitive = true;
    ColumnMode mode = ColumnMode::FixedWidth;
    char delimiter = ';';        // used when mode == Delimited
    long long startColumn = 1;
    long long endColumn = 0;
    std::string replacement;     // UTF-8, may be empty (delete columns)
    bool onlySelection = false;
    long long selectionStart = 0; // byte offsets into the document
    long long selectionEnd = 0;
    long long firstLine = 0;      // 1-based, 0 = no limit
    long long lastLine = 0;       // 1-based, 0 = no limit
};

struct Edit {
    std::size_t start = 0;        // byte offset in the original document
    std::size_t end = 0;          // exclusive
    std::string replacement;      // UTF-8
    std::size_t lineNumber = 0;   // 1-based
};

struct Preview {
    std::size_t lineNumber = 0;
    std::string before;
    std::string after;
};

struct Analysis {
    std::vector<Edit> edits;
    std::vector<Preview> previews;
    std::size_t matchedLines = 0;
    std::size_t changedLines = 0;
};

// Analyzes the document without modifying it. Pass BytePerCharacter for
// single-byte ANSI documents.
Analysis analyze(const std::string& text, const Options& options,
                 std::size_t previewLimit = 20,
                 TextEncoding encoding = TextEncoding::Utf8);

// Applies the edits to a copy of the text (for tests / non-Scintilla use).
std::string applyEdits(const std::string& text, const Analysis& analysis);

} // namespace columnreplace
