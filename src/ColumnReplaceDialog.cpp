#include "ColumnReplaceDialog.h"

#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>

#include "ColumnReplace.h"
#include "Localization.h"
#include "PluginState.h"
#include "resource.h"

namespace {

struct Settings {
    std::wstring prefix;
    bool caseSensitive = true;
    bool onlySelection = false;
    bool delimitedMode = false;
    std::wstring delimiter = L";";
    std::wstring firstLine;
    std::wstring lastLine;
    std::wstring startColumn = L"1";
    std::wstring endColumn;
    std::wstring replacement;
    columnreplace::Language language = columnreplace::Language::English;
    bool languageInitialized = false;
};

Settings g_settings;

std::wstring getWindowText(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    if (length <= 0) {
        return std::wstring();
    }
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(hwnd, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return text;
}

std::string wideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return std::string();
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                                         static_cast<int>(text.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return std::string();
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                        static_cast<int>(text.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return std::wstring();
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                                         static_cast<int>(text.size()),
                                         nullptr, 0);
    if (size <= 0) {
        return std::wstring();
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(),
                        static_cast<int>(text.size()), result.data(), size);
    return result;
}

std::wstring trimForDisplay(const std::string& utf8,
                            std::size_t maxCharacters = 100) {
    std::wstring text = utf8ToWide(utf8);
    for (wchar_t& ch : text) {
        if (ch == L'\t') {
            ch = L' ';
        }
    }
    if (text.size() > maxCharacters) {
        text.resize(maxCharacters);
        // Do not cut a UTF-16 surrogate pair in half.
        if (!text.empty()) {
            const wchar_t last = text.back();
            if (last >= 0xD800 && last <= 0xDBFF) {
                text.pop_back();
            }
        }
        text += L"...";
    }
    return text;
}

std::wstring codePageDisplayName(int codePage) {
    if (codePage == 0) {
        codePage = static_cast<int>(GetACP());
    }
    switch (codePage) {
    case 874:
        return L"Windows-874";
    case 932:
        return L"Shift-JIS (932)";
    case 936:
        return L"GBK (936)";
    case 949:
        return L"Korean (949)";
    case 950:
        return L"Big5 (950)";
    case 1250:
    case 1251:
    case 1252:
    case 1253:
    case 1254:
    case 1255:
    case 1256:
    case 1257:
    case 1258:
        return L"Windows-" + std::to_wstring(codePage);
    default:
        return columnreplace::tr(columnreplace::Text::CodePagePrefix) +
               std::to_wstring(codePage);
    }
}

// Notepad++ stores ANSI documents internally as UTF-8 as well, so the plugin
// always edits the UTF-8 view. This reports only which encoding is used when
// the document is saved.
std::wstring documentEncodingName(HWND scintilla) {
    const int codePage = static_cast<int>(
        SendMessageW(scintilla, SCI_GETCODEPAGE, 0, 0));
    if (codePage == SC_CP_UTF8) {
        return L"UTF-8";
    }
    return L"ANSI (" + codePageDisplayName(codePage) + L")";
}

bool isRepresentableInCodePage(const std::wstring& text, int codePage) {
    const UINT cp = codePage == 0 ? GetACP() : static_cast<UINT>(codePage);
    if (text.empty() || cp == CP_UTF8) {
        return true;
    }
    BOOL usedDefault = FALSE;
    const int length = WideCharToMultiByte(
        cp, WC_NO_BEST_FIT_CHARS, text.c_str(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, &usedDefault);
    return length > 0 && usedDefault == FALSE;
}

class WaitCursor {
public:
    WaitCursor() : previous_(SetCursor(LoadCursorW(nullptr, IDC_WAIT))) {}
    ~WaitCursor() { SetCursor(previous_); }

    WaitCursor(const WaitCursor&) = delete;
    WaitCursor& operator=(const WaitCursor&) = delete;

private:
    HCURSOR previous_;
};

std::string readDocument(HWND scintilla) {
    const Sci_Position length = static_cast<Sci_Position>(
        SendMessageW(scintilla, SCI_GETLENGTH, 0, 0));
    if (length <= 0) {
        return std::string();
    }

    std::string buffer(static_cast<std::size_t>(length) + 1, '\0');
    Sci_TextRangeFull range{};
    range.chrg.cpMin = 0;
    range.chrg.cpMax = length;
    range.lpstrText = buffer.data();
    const LRESULT copied = SendMessageW(
        scintilla, SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<LPARAM>(&range));

    if (copied <= 0) {
        // Fallback for older Scintilla versions that do not implement
        // SCI_GETTEXTRANGEFULL yet.
        const LRESULT fallback = SendMessageW(
            scintilla, SCI_GETTEXT,
            static_cast<WPARAM>(static_cast<std::size_t>(length) + 1),
            reinterpret_cast<LPARAM>(buffer.data()));
        if (fallback <= 0) {
            return std::string();
        }
    }

    buffer.resize(static_cast<std::size_t>(length));
    return buffer;
}

long long mapPosition(long long position,
                      const std::vector<columnreplace::Edit>& edits) {
    long long delta = 0;
    for (const auto& edit : edits) {
        const long long start = static_cast<long long>(edit.start);
        const long long end = static_cast<long long>(edit.end);
        if (end <= position) {
            delta += static_cast<long long>(edit.replacement.size()) -
                     (end - start);
        } else if (start < position) {
            return start + delta +
                   static_cast<long long>(edit.replacement.size());
        } else {
            break;
        }
    }
    return position + delta;
}

void applyToScintilla(HWND scintilla,
                      const columnreplace::Analysis& analysis) {
    if (analysis.edits.empty()) {
        return;
    }

    const long long oldCaret =
        static_cast<long long>(SendMessageW(scintilla, SCI_GETCURRENTPOS, 0, 0));
    const long long oldSelectionStart = static_cast<long long>(
        SendMessageW(scintilla, SCI_GETSELECTIONSTART, 0, 0));
    const long long oldSelectionEnd = static_cast<long long>(
        SendMessageW(scintilla, SCI_GETSELECTIONEND, 0, 0));
    const long long firstVisibleLine = static_cast<long long>(
        SendMessageW(scintilla, SCI_GETFIRSTVISIBLELINE, 0, 0));

    SendMessageW(scintilla, SCI_BEGINUNDOACTION, 0, 0);
    for (auto it = analysis.edits.rbegin(); it != analysis.edits.rend(); ++it) {
        SendMessageW(scintilla, SCI_SETTARGETSTART,
                     static_cast<WPARAM>(it->start), 0);
        SendMessageW(scintilla, SCI_SETTARGETEND,
                     static_cast<WPARAM>(it->end), 0);
        SendMessageW(scintilla, SCI_REPLACETARGET,
                     static_cast<WPARAM>(it->replacement.size()),
                     reinterpret_cast<LPARAM>(it->replacement.c_str()));
    }
    SendMessageW(scintilla, SCI_ENDUNDOACTION, 0, 0);

    const long long newLength = static_cast<long long>(
        SendMessageW(scintilla, SCI_GETLENGTH, 0, 0));
    const long long newCaret =
        std::min(std::max(mapPosition(oldCaret, analysis.edits), 0LL),
                 newLength);

    if (oldSelectionStart != oldSelectionEnd) {
        const long long newStart =
            std::min(std::max(mapPosition(oldSelectionStart, analysis.edits),
                              0LL),
                     newLength);
        const long long newEnd =
            std::min(std::max(mapPosition(oldSelectionEnd, analysis.edits), 0LL),
                     newLength);
        SendMessageW(scintilla, SCI_SETSEL, static_cast<WPARAM>(newStart),
                     static_cast<LPARAM>(newEnd));
    } else {
        SendMessageW(scintilla, SCI_SETCURRENTPOS,
                     static_cast<WPARAM>(newCaret), 0);
    }

    // Keep the scroll position stable; replacements never add newlines.
    SendMessageW(scintilla, SCI_SETFIRSTVISIBLELINE,
                 static_cast<WPARAM>(firstVisibleLine), 0);
}

void setStatus(HWND dialog, const std::wstring& text) {
    SetDlgItemTextW(dialog, IDC_STATUS, text.c_str());
}

void clearPreview(HWND dialog) {
    SendMessageW(GetDlgItem(dialog, IDC_PREVIEWLIST), LB_RESETCONTENT, 0, 0);
    SendMessageW(GetDlgItem(dialog, IDC_PREVIEWLIST),
                 LB_SETHORIZONTALEXTENT, 0, 0);
}

void addPreviewLine(HWND list, const std::wstring& text) {
    SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
}

void updateHorizontalExtent(HWND list) {
    const int count =
        static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
    if (count <= 0) {
        return;
    }

    HDC dc = GetDC(list);
    if (dc == nullptr) {
        return;
    }
    const HGDIOBJ oldFont =
        SelectObject(dc, reinterpret_cast<HGDIOBJ>(
                             SendMessageW(list, WM_GETFONT, 0, 0)));

    int maximum = 0;
    for (int i = 0; i < count; ++i) {
        const int length =
            static_cast<int>(SendMessageW(list, LB_GETTEXTLEN,
                                          static_cast<WPARAM>(i), 0));
        if (length <= 0) {
            continue;
        }
        std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
        SendMessageW(list, LB_GETTEXT, static_cast<WPARAM>(i),
                     reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<std::size_t>(length));
        SIZE size{};
        if (GetTextExtentPoint32W(dc, text.c_str(), length, &size)) {
            maximum = std::max(maximum, static_cast<int>(size.cx));
        }
    }

    SelectObject(dc, oldFont);
    ReleaseDC(list, dc);
    SendMessageW(list, LB_SETHORIZONTALEXTENT,
                 static_cast<WPARAM>(maximum + 24), 0);
}

std::wstring formatPreview(const columnreplace::Preview& preview) {
    return columnreplace::formatPreviewLine(
        static_cast<long long>(preview.lineNumber),
        trimForDisplay(preview.before), trimForDisplay(preview.after));
}

struct InputValues {
    columnreplace::Options options;
    unsigned int firstLine = 0;
    unsigned int lastLine = 0;
    bool hasFirstLine = false;
    bool hasLastLine = false;
};

bool parseOptionalNumber(HWND dialog, int controlId, const wchar_t* label,
                         unsigned int& value, bool& present,
                         std::wstring& error) {
    const std::wstring text = getWindowText(GetDlgItem(dialog, controlId));
    if (text.empty()) {
        present = false;
        value = 0;
        return true;
    }

    BOOL ok = FALSE;
    const UINT parsed = GetDlgItemInt(dialog, controlId, &ok, FALSE);
    if (!ok) {
        error = columnreplace::formatInvalidNumber(label);
        return false;
    }
    present = true;
    value = parsed;
    return true;
}

bool readInput(HWND dialog, HWND scintilla, InputValues& values,
               std::wstring& error) {
    values.options.prefix =
        wideToUtf8(getWindowText(GetDlgItem(dialog, IDC_PREFIX)));
    values.options.replacement =
        wideToUtf8(getWindowText(GetDlgItem(dialog, IDC_REPLACEMENT)));
    values.options.caseSensitive =
        IsDlgButtonChecked(dialog, IDC_CASE) == BST_CHECKED;
    values.options.onlySelection =
        IsDlgButtonChecked(dialog, IDC_ONLYSEL) == BST_CHECKED;
    values.options.mode =
        IsDlgButtonChecked(dialog, IDC_MODE_DELIMITED) == BST_CHECKED
            ? columnreplace::ColumnMode::Delimited
            : columnreplace::ColumnMode::FixedWidth;

    if (values.options.mode == columnreplace::ColumnMode::Delimited) {
        const std::wstring delimiterText =
            getWindowText(GetDlgItem(dialog, IDC_DELIMITER));
        if (delimiterText.empty()) {
            error = columnreplace::tr(
                columnreplace::Text::ErrorDelimiterRequired);
            return false;
        }
        if (delimiterText == L"\\t") {
            values.options.delimiter = '\t';
        } else if (delimiterText.size() == 1) {
            const wchar_t ch = delimiterText[0];
            if (ch == L'\t') {
                values.options.delimiter = '\t';
            } else if (ch < 0x20 || ch > 0x7E || ch == L'"') {
                error = columnreplace::tr(
                    columnreplace::Text::ErrorDelimiterAscii);
                return false;
            } else {
                values.options.delimiter = static_cast<char>(ch);
            }
        } else {
            error = columnreplace::tr(
                columnreplace::Text::ErrorDelimiterSingle);
            return false;
        }
    }

    const std::wstring startText =
        getWindowText(GetDlgItem(dialog, IDC_STARTCOL));
    if (startText.empty()) {
        error = columnreplace::tr(columnreplace::Text::ErrorStartRequired);
        return false;
    }
    BOOL startOk = FALSE;
    const UINT start = GetDlgItemInt(dialog, IDC_STARTCOL, &startOk, FALSE);
    if (!startOk || start == 0) {
        error = columnreplace::tr(columnreplace::Text::ErrorStartNumber);
        return false;
    }
    values.options.startColumn = static_cast<long long>(start);

    const std::wstring endText =
        getWindowText(GetDlgItem(dialog, IDC_ENDCOL));
    if (endText.empty()) {
        values.options.endColumn = 0;
    } else {
        BOOL endOk = FALSE;
        const UINT end = GetDlgItemInt(dialog, IDC_ENDCOL, &endOk, FALSE);
        if (!endOk) {
            error = columnreplace::tr(columnreplace::Text::ErrorEndNumber);
            return false;
        }
        if (end != 0 && end < start) {
            error = columnreplace::tr(columnreplace::Text::ErrorEndBeforeStart);
            return false;
        }
        values.options.endColumn = static_cast<long long>(end);
    }

    if (!parseOptionalNumber(dialog, IDC_FIRSTLINE,
                             columnreplace::tr(
                                 columnreplace::Text::LabelLineFromShort),
                             values.firstLine, values.hasFirstLine, error) ||
        !parseOptionalNumber(dialog, IDC_LASTLINE,
                             columnreplace::tr(
                                 columnreplace::Text::LabelLineToShort),
                             values.lastLine, values.hasLastLine, error)) {
        return false;
    }

    if (values.hasFirstLine && values.firstLine == 0) {
        error = columnreplace::tr(columnreplace::Text::ErrorLineFromMin);
        return false;
    }
    if (values.hasLastLine && values.lastLine == 0) {
        error = columnreplace::tr(columnreplace::Text::ErrorLineToMin);
        return false;
    }
    if (values.hasFirstLine && values.hasLastLine &&
        values.firstLine > values.lastLine) {
        error = columnreplace::tr(columnreplace::Text::ErrorLineRange);
        return false;
    }

    values.options.firstLine =
        values.hasFirstLine ? static_cast<long long>(values.firstLine) : 0;
    values.options.lastLine =
        values.hasLastLine ? static_cast<long long>(values.lastLine) : 0;

    if (values.options.onlySelection) {
        const long long selectionStart = static_cast<long long>(
            SendMessageW(scintilla, SCI_GETSELECTIONSTART, 0, 0));
        const long long selectionEnd = static_cast<long long>(
            SendMessageW(scintilla, SCI_GETSELECTIONEND, 0, 0));
        if (selectionStart == selectionEnd) {
            error = columnreplace::tr(
                columnreplace::Text::ErrorNoSelection);
            return false;
        }
        values.options.selectionStart = selectionStart;
        values.options.selectionEnd = selectionEnd;
    }

    return true;
}

void updateModeControls(HWND dialog) {
    const bool delimited =
        IsDlgButtonChecked(dialog, IDC_MODE_DELIMITED) == BST_CHECKED;

    EnableWindow(GetDlgItem(dialog, IDC_DELIMITER), delimited ? TRUE : FALSE);
    SetDlgItemTextW(dialog, IDC_COLUMNHINT,
                    columnreplace::tr(
                        delimited ? columnreplace::Text::HintEndDelimited
                                  : columnreplace::Text::HintEndFixed));
}

void applyLanguage(HWND dialog) {
    SetWindowTextW(dialog,
                   columnreplace::tr(columnreplace::Text::DialogCaption));

    SetDlgItemTextW(dialog, IDC_GROUP_FILTER,
                    columnreplace::tr(columnreplace::Text::GroupFilter));
    SetDlgItemTextW(dialog, IDC_LABEL_PREFIX,
                    columnreplace::tr(columnreplace::Text::LabelPrefix));
    SetDlgItemTextW(dialog, IDC_CASE,
                    columnreplace::tr(columnreplace::Text::CheckCase));
    SetDlgItemTextW(dialog, IDC_ONLYSEL,
                    columnreplace::tr(columnreplace::Text::CheckSelection));
    SetDlgItemTextW(dialog, IDC_LABEL_LINEFROM,
                    columnreplace::tr(columnreplace::Text::LabelLineFrom));
    SetDlgItemTextW(dialog, IDC_LABEL_LINETO,
                    columnreplace::tr(columnreplace::Text::LabelLineTo));
    SetDlgItemTextW(dialog, IDC_HINT_ALLLINES,
                    columnreplace::tr(columnreplace::Text::HintAllLines));
    SetDlgItemTextW(dialog, IDC_HINT_PREFIX,
                    columnreplace::tr(columnreplace::Text::HintPrefix));

    SetDlgItemTextW(dialog, IDC_GROUP_COLUMNS,
                    columnreplace::tr(columnreplace::Text::GroupColumns));
    SetDlgItemTextW(dialog, IDC_MODE_FIXED,
                    columnreplace::tr(columnreplace::Text::RadioFixed));
    SetDlgItemTextW(dialog, IDC_MODE_DELIMITED,
                    columnreplace::tr(columnreplace::Text::RadioDelimited));
    SetDlgItemTextW(dialog, IDC_LABEL_STARTCOL,
                    columnreplace::tr(columnreplace::Text::LabelStartColumn));
    SetDlgItemTextW(dialog, IDC_LABEL_ENDCOL,
                    columnreplace::tr(columnreplace::Text::LabelEndColumn));
    SetDlgItemTextW(dialog, IDC_LABEL_DELIMITER,
                    columnreplace::tr(columnreplace::Text::LabelDelimiter));
    SetDlgItemTextW(dialog, IDC_HINT_DELIMITER,
                    columnreplace::tr(columnreplace::Text::HintDelimiter));

    SetDlgItemTextW(dialog, IDC_LABEL_REPLACEMENT,
                    columnreplace::tr(columnreplace::Text::LabelReplacement));
    SetDlgItemTextW(dialog, IDC_PREVIEW,
                    columnreplace::tr(columnreplace::Text::ButtonPreview));
    SetDlgItemTextW(dialog, IDC_REPLACE,
                    columnreplace::tr(columnreplace::Text::ButtonReplace));
    SetDlgItemTextW(dialog, IDCANCEL,
                    columnreplace::tr(columnreplace::Text::ButtonClose));

    SetDlgItemTextW(dialog, IDC_LABEL_PREVIEW,
                    columnreplace::tr(columnreplace::Text::LabelPreview));
    SetDlgItemTextW(dialog, IDC_LABEL_LANGUAGE,
                    columnreplace::tr(columnreplace::Text::LabelLanguage));

    updateModeControls(dialog);
}

void populateLanguageCombo(HWND dialog) {
    HWND combo = GetDlgItem(dialog, IDC_LANGUAGE);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    SendMessageW(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(
                     columnreplace::tr(columnreplace::Text::LanguageGerman)));
    SendMessageW(combo, CB_ADDSTRING, 0,
                 reinterpret_cast<LPARAM>(
                     columnreplace::tr(columnreplace::Text::LanguageEnglish)));
    SendMessageW(combo, CB_SETCURSEL,
                 columnreplace::currentLanguage() ==
                         columnreplace::Language::German
                     ? 0
                     : 1,
                 0);
}

std::wstring readyStatus(HWND scintilla) {
    if (scintilla == nullptr) {
        return columnreplace::tr(columnreplace::Text::StatusReady);
    }
    return columnreplace::formatReadyStatus(documentEncodingName(scintilla));
}

void saveSettings(HWND dialog) {
    g_settings.prefix = getWindowText(GetDlgItem(dialog, IDC_PREFIX));
    g_settings.caseSensitive =
        IsDlgButtonChecked(dialog, IDC_CASE) == BST_CHECKED;
    g_settings.onlySelection =
        IsDlgButtonChecked(dialog, IDC_ONLYSEL) == BST_CHECKED;
    g_settings.delimitedMode =
        IsDlgButtonChecked(dialog, IDC_MODE_DELIMITED) == BST_CHECKED;
    g_settings.delimiter = getWindowText(GetDlgItem(dialog, IDC_DELIMITER));
    g_settings.firstLine = getWindowText(GetDlgItem(dialog, IDC_FIRSTLINE));
    g_settings.lastLine = getWindowText(GetDlgItem(dialog, IDC_LASTLINE));
    g_settings.startColumn = getWindowText(GetDlgItem(dialog, IDC_STARTCOL));
    g_settings.endColumn = getWindowText(GetDlgItem(dialog, IDC_ENDCOL));
    g_settings.replacement = getWindowText(GetDlgItem(dialog, IDC_REPLACEMENT));
}

void loadSettings(HWND dialog) {
    SetDlgItemTextW(dialog, IDC_PREFIX, g_settings.prefix.c_str());
    CheckDlgButton(dialog, IDC_CASE,
                   g_settings.caseSensitive ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_ONLYSEL,
                   g_settings.onlySelection ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(dialog, IDC_MODE_FIXED,
                   g_settings.delimitedMode ? BST_UNCHECKED : BST_CHECKED);
    CheckDlgButton(dialog, IDC_MODE_DELIMITED,
                   g_settings.delimitedMode ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextW(dialog, IDC_DELIMITER, g_settings.delimiter.c_str());
    SetDlgItemTextW(dialog, IDC_FIRSTLINE, g_settings.firstLine.c_str());
    SetDlgItemTextW(dialog, IDC_LASTLINE, g_settings.lastLine.c_str());
    SetDlgItemTextW(dialog, IDC_STARTCOL, g_settings.startColumn.c_str());
    SetDlgItemTextW(dialog, IDC_ENDCOL, g_settings.endColumn.c_str());
    SetDlgItemTextW(dialog, IDC_REPLACEMENT, g_settings.replacement.c_str());
    updateModeControls(dialog);
}

void runPreview(HWND dialog) {
    HWND scintilla = currentScintilla();
    if (scintilla == nullptr) {
        return;
    }

    InputValues values;
    std::wstring error;
    if (!readInput(dialog, scintilla, values, error)) {
        MessageBoxW(dialog, error.c_str(),
                    columnreplace::tr(columnreplace::Text::PluginName),
                    MB_ICONWARNING | MB_OK);
        return;
    }

    clearPreview(dialog);
    SetDlgItemTextW(dialog, IDC_STATUS,
                    columnreplace::tr(
                        columnreplace::Text::StatusCalculating));

    WaitCursor cursor;
    const std::string document = readDocument(scintilla);
    const columnreplace::Analysis analysis =
        columnreplace::analyze(document, values.options, 20);

    HWND list = GetDlgItem(dialog, IDC_PREVIEWLIST);
    if (analysis.previews.empty()) {
        addPreviewLine(list, columnreplace::tr(
                                 columnreplace::Text::StatusNoChangesList));
    } else {
        for (const auto& preview : analysis.previews) {
            addPreviewLine(list, formatPreview(preview));
        }
    }
    updateHorizontalExtent(list);

    setStatus(dialog, columnreplace::formatPreviewStatus(
                          analysis.matchedLines, analysis.changedLines, 20));
}

void runReplace(HWND dialog) {
    HWND scintilla = currentScintilla();
    if (scintilla == nullptr) {
        return;
    }

    if (SendMessageW(scintilla, SCI_GETREADONLY, 0, 0) != 0) {
        MessageBoxW(dialog,
                    columnreplace::tr(columnreplace::Text::ErrorReadOnly),
                    columnreplace::tr(columnreplace::Text::PluginName),
                    MB_ICONWARNING | MB_OK);
        return;
    }

    InputValues values;
    std::wstring error;
    if (!readInput(dialog, scintilla, values, error)) {
        MessageBoxW(dialog, error.c_str(),
                    columnreplace::tr(columnreplace::Text::PluginName),
                    MB_ICONWARNING | MB_OK);
        return;
    }

    saveSettings(dialog);

    // ANSI documents are edited as UTF-8 internally. Characters that do not
    // exist in the document code page would be turned into '?' when saving,
    // so ask before doing that.
    const int codePage = static_cast<int>(
        SendMessageW(scintilla, SCI_GETCODEPAGE, 0, 0));
    if (codePage != SC_CP_UTF8 &&
        !isRepresentableInCodePage(
            getWindowText(GetDlgItem(dialog, IDC_REPLACEMENT)), codePage)) {
        const std::wstring warning =
            columnreplace::formatAnsiWarning(codePageDisplayName(codePage));
        if (MessageBoxW(dialog, warning.c_str(),
                        columnreplace::tr(
                            columnreplace::Text::AnsiWarningTitle),
                        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
            return;
        }
    }

    WaitCursor cursor;
    const std::string document = readDocument(scintilla);
    const columnreplace::Analysis analysis =
        columnreplace::analyze(document, values.options, 0);

    if (analysis.edits.empty()) {
        clearPreview(dialog);
        setStatus(dialog,
                  columnreplace::tr(columnreplace::Text::StatusNoChanges));
        return;
    }

    applyToScintilla(scintilla, analysis);

    clearPreview(dialog);
    setStatus(dialog, columnreplace::formatReplaceStatus(
                          analysis.changedLines, analysis.matchedLines));
}

INT_PTR CALLBACK dialogProc(HWND dialog, UINT message, WPARAM wParam,
                            LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        if (!g_settings.languageInitialized) {
            g_settings.language = columnreplace::detectLanguage();
            g_settings.languageInitialized = true;
        }
        columnreplace::setLanguage(g_settings.language);
        loadSettings(dialog);
        populateLanguageCombo(dialog);
        applyLanguage(dialog);
        SetDlgItemTextW(dialog, IDC_STATUS,
                        readyStatus(currentScintilla()).c_str());
        SetFocus(GetDlgItem(dialog, IDC_PREFIX));
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_PREVIEW:
            runPreview(dialog);
            return TRUE;

        case IDC_REPLACE:
            runReplace(dialog);
            return TRUE;

        case IDC_MODE_FIXED:
        case IDC_MODE_DELIMITED:
            updateModeControls(dialog);
            return TRUE;

        case IDC_LANGUAGE:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                const int selection = static_cast<int>(SendMessageW(
                    GetDlgItem(dialog, IDC_LANGUAGE), CB_GETCURSEL, 0, 0));
                const columnreplace::Language language =
                    selection == 0 ? columnreplace::Language::German
                                   : columnreplace::Language::English;
                if (language != columnreplace::currentLanguage()) {
                    g_settings.language = language;
                    columnreplace::setLanguage(language);
                    applyLanguage(dialog);
                    clearPreview(dialog);
                    SetDlgItemTextW(
                        dialog, IDC_STATUS,
                        readyStatus(currentScintilla()).c_str());
                }
            }
            return TRUE;

        case IDCANCEL:
            saveSettings(dialog);
            EndDialog(dialog, 0);
            return TRUE;

        default:
            break;
        }
        break;

    case WM_CLOSE:
        saveSettings(dialog);
        EndDialog(dialog, 0);
        return TRUE;

    default:
        break;
    }

    (void)lParam;
    return FALSE;
}

} // namespace

void showColumnReplaceDialog() {
    if (g_nppData._nppHandle == nullptr) {
        return;
    }
    DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_COLUMN_REPLACE),
                    g_nppData._nppHandle, dialogProc, 0);
}
