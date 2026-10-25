#include "ColumnReplace.h"

#include <algorithm>
#include <cwctype>

namespace columnreplace {
namespace {

// Decodes UTF-8 into code points and records the byte offset of every
// character. Invalid bytes are treated as one character each, so slicing the
// original string at the recorded offsets always round-trips safely.
void decodeUtf8(const std::string& s, std::vector<unsigned int>& cps,
                std::vector<std::size_t>& offsets) {
    cps.clear();
    offsets.clear();

    std::size_t i = 0;
    while (i < s.size()) {
        offsets.push_back(i);

        const unsigned char b = static_cast<unsigned char>(s[i]);
        unsigned int cp = b;
        std::size_t len = 1;

        if (b < 0x80) {
            cp = b;
            len = 1;
        } else if ((b & 0xE0) == 0xC0) {
            cp = b & 0x1Fu;
            len = 2;
        } else if ((b & 0xF0) == 0xE0) {
            cp = b & 0x0Fu;
            len = 3;
        } else if ((b & 0xF8) == 0xF0) {
            cp = b & 0x07u;
            len = 4;
        } else {
            cp = b;
            len = 1;
        }

        if (len > 1) {
            bool valid = (i + len <= s.size());
            for (std::size_t k = 1; valid && k < len; ++k) {
                const unsigned char c = static_cast<unsigned char>(s[i + k]);
                if ((c & 0xC0) != 0x80) {
                    valid = false;
                } else {
                    cp = (cp << 6) | (c & 0x3Fu);
                }
            }
            if (!valid) {
                cp = b;
                len = 1;
            }
        }

        cps.push_back(cp);
        i += len;
    }

    offsets.push_back(s.size());
}

unsigned int foldCodePoint(unsigned int cp) {
    if (cp < 0x80) {
        if (cp >= 'A' && cp <= 'Z') {
            return cp + ('a' - 'A');
        }
        return cp;
    }
    // Latin-1 letters (includes Ä/Ö/Ü). towlower alone is not reliable for
    // these in the "C" locale used by Windows GUI applications.
    if (cp >= 0x00C0 && cp <= 0x00DE && cp != 0x00D7) {
        return cp + 0x20;
    }
    // towlower works for the remaining BMP on both Windows (16-bit wchar_t)
    // and POSIX (32-bit wchar_t). Characters outside the BMP stay unchanged.
    if (cp <= 0xFFFF) {
        const std::wint_t folded = std::towlower(static_cast<std::wint_t>(cp));
        if (folded != static_cast<std::wint_t>(cp)) {
            return static_cast<unsigned int>(folded);
        }
    }
    return cp;
}

bool startsWith(const std::vector<unsigned int>& line,
                const std::vector<unsigned int>& prefix, bool caseSensitive) {
    if (line.size() < prefix.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); ++i) {
        unsigned int a = line[i];
        unsigned int b = prefix[i];
        if (!caseSensitive) {
            a = foldCodePoint(a);
            b = foldCodePoint(b);
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

struct FieldSpan {
    std::size_t start = 0; // byte offset of the first character (including quotes)
    std::size_t end = 0;   // byte offset after the last character
};

bool containsOnlyWhitespace(const std::string& text, std::size_t from,
                            std::size_t to) {
    for (std::size_t i = from; i < to; ++i) {
        if (text[i] != ' ' && text[i] != '\t') {
            return false;
        }
    }
    return true;
}

// Splits one line into fields. Delimiters inside double quotes are ignored;
// a doubled quote ("") is an escaped quote. The original byte offsets are
// kept so the rest of the line can be preserved verbatim.
std::vector<FieldSpan> parseDelimited(const std::string& line,
                                      char delimiter) {
    std::vector<FieldSpan> fields;
    std::size_t fieldStart = 0;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    ++i; // escaped quote
                } else {
                    inQuotes = false;
                }
            }
        } else if (c == '"' &&
                   containsOnlyWhitespace(line, fieldStart, i)) {
            inQuotes = true;
        } else if (c == delimiter) {
            fields.push_back({fieldStart, i});
            fieldStart = i + 1;
        }
    }

    fields.push_back({fieldStart, line.size()});
    return fields;
}

std::string encodeDelimitedValue(const std::string& value, char delimiter) {
    const bool needsQuotes =
        value.find(delimiter) != std::string::npos ||
        value.find('"') != std::string::npos ||
        value.find('\r') != std::string::npos ||
        value.find('\n') != std::string::npos ||
        (!value.empty() && (value.front() == ' ' || value.back() == ' '));

    if (!needsQuotes) {
        return value;
    }

    std::string encoded;
    encoded.reserve(value.size() + 2);
    encoded.push_back('"');
    for (const char c : value) {
        if (c == '"') {
            encoded += "\"\"";
        } else {
            encoded.push_back(c);
        }
    }
    encoded.push_back('"');
    return encoded;
}

bool replaceFixedWidth(const std::string& content,
                       const std::vector<unsigned int>& cps,
                       const std::vector<std::size_t>& offsets,
                       const Options& options, std::string& updated) {
    if (options.startColumn < 1 ||
        static_cast<std::size_t>(options.startColumn - 1) >= cps.size()) {
        return false;
    }

    const std::size_t startIndex =
        static_cast<std::size_t>(options.startColumn - 1);

    std::size_t endIndex = cps.size();
    if (options.endColumn > 0) {
        endIndex = static_cast<std::size_t>(options.endColumn);
        if (endIndex > cps.size()) {
            endIndex = cps.size();
        }
    }
    if (endIndex <= startIndex) {
        return false;
    }

    updated.clear();
    updated.reserve(content.size() + options.replacement.size());
    updated.append(content, 0, offsets[startIndex]);
    updated += options.replacement;
    updated.append(content, offsets[endIndex], std::string::npos);
    return updated != content;
}

bool replaceDelimited(const std::string& content, const Options& options,
                      std::size_t& startByte, std::size_t& endByte,
                      std::string& encodedValue, std::string& updated) {
    if (options.startColumn < 1 || options.delimiter == '\0' ||
        options.delimiter == '"' || options.delimiter == '\r' ||
        options.delimiter == '\n') {
        return false;
    }

    const std::vector<FieldSpan> fields =
        parseDelimited(content, options.delimiter);

    const std::size_t startField =
        static_cast<std::size_t>(options.startColumn - 1);
    if (startField >= fields.size()) {
        return false;
    }

    std::size_t endField = (options.endColumn > 0)
                               ? static_cast<std::size_t>(options.endColumn)
                               : startField + 1;
    if (endField <= startField) {
        return false;
    }
    if (endField > fields.size()) {
        endField = fields.size();
    }

    startByte = fields[startField].start;
    endByte = fields[endField - 1].end;
    encodedValue =
        encodeDelimitedValue(options.replacement, options.delimiter);

    updated.clear();
    updated.reserve(content.size() + encodedValue.size());
    updated.append(content, 0, startByte);
    updated += encodedValue;
    updated.append(content, endByte, std::string::npos);
    return updated != content;
}

} // namespace

Analysis analyze(const std::string& text, const Options& options,
                 std::size_t previewLimit) {
    Analysis result;

    std::vector<unsigned int> prefixCps;
    std::vector<std::size_t> prefixOffsets;
    decodeUtf8(options.prefix, prefixCps, prefixOffsets);

    std::vector<unsigned int> lineCps;
    std::vector<std::size_t> lineOffsets;

    std::size_t pos = 0;
    std::size_t lineNumber = 1;

    for (;;) {
        const std::size_t nl = text.find('\n', pos);
        const bool lastLine = (nl == std::string::npos);

        // A trailing newline does not create an extra editable line. Without
        // this, replacing field 1 in delimited mode would append a value after
        // the final newline.
        if (lastLine && pos == text.size() && pos > 0) {
            break;
        }

        const std::size_t lineEnd = lastLine ? text.size() : nl;
        const std::size_t lineWithEolEnd = lastLine ? text.size() : nl + 1;

        std::size_t contentEnd = lineEnd;
        if (contentEnd > pos && text[contentEnd - 1] == '\r') {
            --contentEnd;
        }

        const bool inLineRange =
            (options.firstLine <= 0 ||
             static_cast<long long>(lineNumber) >= options.firstLine) &&
            (options.lastLine <= 0 ||
             static_cast<long long>(lineNumber) <= options.lastLine);

        bool inSelection = true;
        if (options.onlySelection) {
            inSelection =
                static_cast<long long>(pos) < options.selectionEnd &&
                static_cast<long long>(lineWithEolEnd) > options.selectionStart;
        }

        if (inLineRange && inSelection) {
            const std::string content = text.substr(pos, contentEnd - pos);
            decodeUtf8(content, lineCps, lineOffsets);

            if (startsWith(lineCps, prefixCps, options.caseSensitive)) {
                ++result.matchedLines;

                std::string updated;
                bool changed = false;
                std::size_t editStart = 0;
                std::size_t editEnd = 0;
                std::string replacement;

                if (options.mode == ColumnMode::Delimited) {
                    changed = replaceDelimited(content, options, editStart,
                                               editEnd, replacement, updated);
                    if (changed) {
                        editStart += pos;
                        editEnd += pos;
                    }
                } else {
                    changed = replaceFixedWidth(content, lineCps, lineOffsets,
                                                options, updated);
                    if (changed) {
                        editStart = pos + lineOffsets[static_cast<std::size_t>(
                                                 options.startColumn - 1)];
                        std::size_t endIndex = lineCps.size();
                        if (options.endColumn > 0) {
                            endIndex =
                                static_cast<std::size_t>(options.endColumn);
                            if (endIndex > lineCps.size()) {
                                endIndex = lineCps.size();
                            }
                        }
                        editEnd = pos + lineOffsets[endIndex];
                        replacement = options.replacement;
                    }
                }

                if (changed) {
                    Edit edit;
                    edit.start = editStart;
                    edit.end = editEnd;
                    edit.replacement = replacement;
                    edit.lineNumber = lineNumber;
                    result.edits.push_back(edit);
                    ++result.changedLines;

                    if (result.previews.size() < previewLimit) {
                        Preview preview;
                        preview.lineNumber = lineNumber;
                        preview.before = content;
                        preview.after = updated;
                        result.previews.push_back(preview);
                    }
                }
            }
        }

        if (lastLine) {
            break;
        }
        pos = nl + 1;
        ++lineNumber;
    }

    return result;
}

std::string applyEdits(const std::string& text, const Analysis& analysis) {
    std::string output = text;
    for (auto it = analysis.edits.rbegin(); it != analysis.edits.rend(); ++it) {
        if (it->start > it->end || it->end > output.size()) {
            continue;
        }
        output.replace(it->start, it->end - it->start, it->replacement);
    }
    return output;
}

} // namespace columnreplace

