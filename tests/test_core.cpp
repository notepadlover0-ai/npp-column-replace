#include "ColumnReplace.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        ++failures;
    }
}

void checkEqual(const std::string& actual, const std::string& expected,
                int line) {
    if (actual != expected) {
        std::cerr << "FAIL line " << line << "\n  expected: " << expected
                  << "\n  actual:   " << actual << '\n';
        ++failures;
    }
}

#define CHECK(expression) check((expression), #expression, __LINE__)
#define CHECK_EQ(actual, expected) checkEqual((actual), (expected), __LINE__)

} // namespace

int main() {
    using namespace columnreplace;

    // Prefix filter plus inclusive column range.
    {
        Options options;
        options.prefix = "ABC";
        options.startColumn = 4;
        options.endColumn = 6;
        options.replacement = "XY";

        const std::string text = "ABC123456\nABD123456\nABC12\nABC\n";
        const Analysis analysis = analyze(text, options);

        CHECK_EQ(std::to_string(analysis.matchedLines), "3");
        CHECK_EQ(std::to_string(analysis.changedLines), "2");
        CHECK_EQ(applyEdits(text, analysis),
                 "ABCXY456\nABD123456\nABCXY\nABC\n");
    }

    // Shorter and longer replacements touch only the configured columns; the
    // text after the range is preserved verbatim.
    {
        Options options;
        options.startColumn = 4;
        options.endColumn = 6;
        options.replacement = "X";

        const std::string text = "ABC001Hans\n";
        CHECK_EQ(applyEdits(text, analyze(text, options)), "ABCXHans\n");

        options.replacement = "LONGER";
        CHECK_EQ(applyEdits(text, analyze(text, options)),
                 "ABCLONGERHans\n");

        options.replacement = "";
        CHECK_EQ(applyEdits(text, analyze(text, options)), "ABCHans\n");
    }

    // "From column N to end of line" pads shorter lines with spaces so the
    // replacement starts at column N on every matching line.
    {
        Options options;
        options.startColumn = 5;
        options.replacement = "XY";

        const std::string text = "abcdef\nab\nabcd\n\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(std::to_string(analysis.matchedLines), "4");
        CHECK_EQ(std::to_string(analysis.changedLines), "3");
        CHECK_EQ(applyEdits(text, analysis), "abcdXY\nab  XY\nabcdXY\n\n");
    }

    // A line that only reaches the character before the start column is
    // extended without any padding.
    {
        Options options;
        options.startColumn = 5;
        options.replacement = "XY";

        const std::string text = "abcd\n";
        CHECK_EQ(applyEdits(text, analyze(text, options)), "abcdXY\n");
    }

    // Deleting from a column that a short line does not reach changes
    // nothing; an explicit end column also keeps short lines untouched.
    {
        Options options;
        options.startColumn = 5;
        options.replacement = "";

        const std::string text = "ab\n";
        CHECK_EQ(applyEdits(text, analyze(text, options)), "ab\n");

        options.replacement = "XY";
        options.endColumn = 6;
        const std::string narrower = "ab\n";
        CHECK_EQ(applyEdits(narrower, analyze(narrower, options)), "ab\n");
    }

    // Empty prefix = all lines; one character replaced.
    {
        Options options;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "#";

        const std::string text = "abc\ndef";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "#bc\n#ef");
    }

    // Open-ended range deletes to end of line.
    {
        Options options;
        options.startColumn = 3;
        options.replacement = "";

        const std::string text = "abcdef\nxyz\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "ab\nxy\n");
    }

    // CRLF line endings are preserved.
    {
        Options options;
        options.prefix = "A";
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "X";

        const std::string text = "A1\r\nA2\r\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "AX\r\nAX\r\n");
    }

    // Columns count UTF-8 characters, not bytes.
    {
        Options options;
        options.startColumn = 2;
        options.endColumn = 3;
        options.replacement = "XY";

        const std::string text = "äöüABC\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "äXYABC\n");
    }

    // Case-insensitive prefix comparison.
    {
        Options options;
        options.prefix = "abc";
        options.caseSensitive = false;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "Z";

        const std::string text = "ABCdef\nabcdef\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "ZBCdef\nZbcdef\n");
    }

    // Line range limits the operation.
    {
        Options options;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "X";
        options.firstLine = 1;
        options.lastLine = 1;

        const std::string text = "aaa\nbbb\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "Xaa\nbbb\n");
    }

    // Selection mode only touches lines overlapping the selection.
    {
        Options options;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "X";
        options.onlySelection = true;
        options.selectionStart = 4;
        options.selectionEnd = 7;

        const std::string text = "aaa\nbbb\nccc\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "aaa\nXbb\nccc\n");
    }

    // Preview limit is respected.
    {
        Options options;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "X";

        const std::string text = "a\nb\nc\n";
        const Analysis analysis = analyze(text, options, 2);
        CHECK_EQ(std::to_string(analysis.changedLines), "3");
        CHECK_EQ(std::to_string(analysis.previews.size()), "2");
    }

    // Identical replacement produces no edit.
    {
        Options options;
        options.prefix = "ABC";
        options.startColumn = 4;
        options.endColumn = 6;
        options.replacement = "123";

        const std::string text = "ABC123\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(std::to_string(analysis.matchedLines), "1");
        CHECK_EQ(std::to_string(analysis.changedLines), "0");
        CHECK(analysis.edits.empty());
    }

    // Case-insensitive comparison handles German umlauts.
    {
        Options options;
        options.prefix = "äbc";
        options.caseSensitive = false;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "Z";

        const std::string text = "ÄBCx\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "ZBCx\n");
    }

    // Four-byte UTF-8 characters count as one column.
    {
        Options options;
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "X";

        const std::string text = "😀ab\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "😀Xb\n");
    }

    // Delimited fields: replace one field.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "X";

        const std::string text = "a;b;c\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "a;X;c\n");
    }

    // Delimited fields: empty end column means only the start field.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 2;
        options.endColumn = 0;
        options.replacement = "X";

        const std::string text = "a;b;c\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "a;X;c\n");
    }

    // Delimited fields: a range is collapsed into one value.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 2;
        options.endColumn = 3;
        options.replacement = "X";

        const std::string text = "a;b;c\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "a;X\n");
    }

    // Delimited fields: quoted delimiters and escaped quotes are understood.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "Y";

        const std::string text = "\"a;b\";c\n";
        const Analysis first = analyze(text, options);
        CHECK_EQ(applyEdits(text, first), "Y;c\n");

        options.startColumn = 2;
        options.endColumn = 2;
        const std::string quoted = "\"a\"\"b\";c\n";
        const Analysis second = analyze(quoted, options);
        CHECK_EQ(applyEdits(quoted, second), "\"a\"\"b\";Y\n");
    }

    // Delimited fields: the replacement is quoted when necessary.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "x;y";

        const std::string text = "a;b;c\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "a;\"x;y\";c\n");
    }

    // Delimited fields: trailing empty field and tab delimiter.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 3;
        options.endColumn = 3;
        options.replacement = "X";

        const std::string text = "a;b;\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "a;b;X\n");

        options.delimiter = '\t';
        options.startColumn = 2;
        options.endColumn = 2;
        const std::string tsv = "a\tb\tc\n";
        const Analysis tabAnalysis = analyze(tsv, options);
        CHECK_EQ(applyEdits(tsv, tabAnalysis), "a\tX\tc\n");
    }

    // Delimited fields: a trailing newline must not create an extra record.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "X";

        const std::string text = "a;b;c\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(std::to_string(analysis.changedLines), "1");
        CHECK_EQ(applyEdits(text, analysis), "X;b;c\n");
    }

    // Delimited fields combined with a prefix filter.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.prefix = "A";
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "X";

        const std::string text = "A;b\nB;c\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "A;X\nB;c\n");
    }

    // Delimited edits with different lengths keep later offsets intact.
    {
        Options options;
        options.mode = ColumnMode::Delimited;
        options.delimiter = ';';
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "LONG";

        const std::string text = "a;bb;c\nd;e;f\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "a;LONG;c\nd;LONG;f\n");
    }

    // Multiple edits keep byte offsets consistent when applied together.
    {
        Options options;
        options.startColumn = 1;
        options.endColumn = 1;
        options.replacement = "XX";

        const std::string text = "a\nb\nc\n";
        const Analysis analysis = analyze(text, options);
        CHECK_EQ(applyEdits(text, analysis), "XX\nXX\nXX\n");
    }

    // ANSI documents (BytePerCharacter): every byte is one column, so a
    // two-byte UTF-8 umlaut occupies two columns while it is one character in
    // UTF-8 mode.
    {
        Options options;
        options.startColumn = 2;
        options.endColumn = 3;
        options.replacement = "XY";

        const std::string text = "a\xC3\xA4z\n";
        const Analysis ansi =
            analyze(text, options, 20, TextEncoding::BytePerCharacter);
        CHECK_EQ(applyEdits(text, ansi), "aXYz\n");

        const Analysis utf8 = analyze(text, options);
        CHECK_EQ(applyEdits(text, utf8), "aXY\n");
    }

    // ANSI mode pads shorter lines in bytes as well, so the replacement starts
    // at the same column on every line.
    {
        Options options;
        options.startColumn = 4;
        options.replacement = "X";

        // "a" plus the single CP1252 byte 0xE4 ("ä") = two columns.
        const std::string text = "a\xE4\n";
        const Analysis analysis =
            analyze(text, options, 20, TextEncoding::BytePerCharacter);
        CHECK_EQ(applyEdits(text, analysis), "a\xE4 X\n");
    }

    // ANSI prefix comparison works on code page bytes.
    {
        Options options;
        options.prefix = std::string("\xFC", 1); // CP1252 "ü"
        options.startColumn = 2;
        options.endColumn = 2;
        options.replacement = "X";

        const std::string text = "\xFC" "ber\nabc\n";
        const Analysis analysis =
            analyze(text, options, 20, TextEncoding::BytePerCharacter);
        CHECK_EQ(applyEdits(text, analysis), "\xFC" "Xer\nabc\n");
    }

    if (failures == 0) {
        std::cout << "All tests passed.\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
