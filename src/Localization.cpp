#include "Localization.h"

#include <windows.h>

#include <algorithm>
#include <cctype>

#include "PluginState.h"

namespace columnreplace {
namespace {

constexpr int kTextCount = static_cast<int>(Text::Count);

const wchar_t* const kEnglish[kTextCount] = {
    /* PluginName        */ L"Column Replace",
    /* MenuItem          */ L"Column Replace...",
    /* DialogCaption     */ L"Column Replace",
    /* GroupFilter       */ L"Filter",
    /* LabelPrefix       */ L"Line prefix:",
    /* CheckCase         */ L"Match case",
    /* CheckSelection    */ L"Only lines in selection",
    /* LabelLineFrom     */ L"Lines from:",
    /* LabelLineTo       */ L"to:",
    /* HintAllLines      */ L"(empty = all lines)",
    /* HintPrefix        */ L"Hint: prefix e.g. \"ABC\" = first 3 characters of a line.",
    /* GroupColumns      */ L"Columns / fields (1-based, inclusive)",
    /* RadioFixed        */ L"Fixed character positions",
    /* RadioDelimited    */ L"Delimiter-separated fields",
    /* LabelStartColumn  */ L"From column:",
    /* LabelEndColumn    */ L"To column:",
    /* HintEndFixed      */ L"(empty = to end of line)",
    /* HintEndDelimited  */ L"(empty = start column only)",
    /* LabelDelimiter    */ L"Delimiter:",
    /* HintDelimiter     */ L"e.g. ; or \\t (tab)",
    /* LabelReplacement  */ L"Replace with:",
    /* ButtonPreview     */ L"Preview",
    /* ButtonReplace     */ L"Replace",
    /* ButtonClose       */ L"Close",
    /* LabelPreview      */ L"Preview (max. 20 changes):",
    /* LabelLanguage     */ L"Language:",
    /* StatusReady       */ L"Ready.",
    /* StatusCalculating */ L"Calculating preview ...",
    /* StatusNoChangesList */ L"(no changes)",
    /* StatusNoChanges   */ L"No changes.",
    /* StatusAlreadyApplied */ L"No changes - already replaced.",
    /* ErrorReadOnly     */ L"The current document is read-only.",
    /* ErrorDelimiterRequired */ L"Please enter a delimiter (e.g. ; or \\t for tab).",
    /* ErrorDelimiterAscii */ L"The delimiter must be a single ASCII character (or \\t for tab).",
    /* ErrorDelimiterSingle */ L"The delimiter must be a single character (or \\t for tab).",
    /* ErrorStartRequired */ L"Please enter a start column (1 or greater).",
    /* ErrorStartNumber  */ L"The start column must be a number of at least 1.",
    /* ErrorEndNumber    */ L"The end column is not a valid number.",
    /* ErrorEndBeforeStart */ L"The end column must not be smaller than the start column.",
    /* ErrorLineFromMin  */ L"\"Line from\" must be at least 1.",
    /* ErrorLineToMin    */ L"\"Line to\" must be at least 1.",
    /* ErrorLineRange    */ L"\"Line from\" must not be greater than \"Line to\".",
    /* ErrorNoSelection  */ L"\"Only lines in selection\" is enabled, but no text is selected.",
    /* LabelLineFromShort */ L"Line from",
    /* LabelLineToShort  */ L"Line to",
    /* AnsiWarningTitle  */ L"ANSI file",
    /* CodePagePrefix    */ L"Code page ",
    /* LanguageGerman    */ L"Deutsch",
    /* LanguageEnglish   */ L"English",
};

const wchar_t* const kGerman[kTextCount] = {
    /* PluginName        */ L"Spalten-Ersetzen",
    /* MenuItem          */ L"Spalten-Ersetzen...",
    /* DialogCaption     */ L"Spalten-Ersetzen",
    /* GroupFilter       */ L"Filter",
    /* LabelPrefix       */ L"Zeilenanfang (Präfix):",
    /* CheckCase         */ L"Groß-/Kleinschreibung beachten",
    /* CheckSelection    */ L"Nur Zeilen in Auswahl",
    /* LabelLineFrom     */ L"Zeilen von:",
    /* LabelLineTo       */ L"bis:",
    /* HintAllLines      */ L"(leer = alle Zeilen)",
    /* HintPrefix        */ L"Hinweis: Präfix z. B. \"ABC\" = erste 3 Zeichen der Zeile.",
    /* GroupColumns      */ L"Spalten / Felder (1-basiert, inklusive)",
    /* RadioFixed        */ L"Feste Zeichenpositionen",
    /* RadioDelimited    */ L"Trennzeichen-getrennte Felder",
    /* LabelStartColumn  */ L"Von Spalte:",
    /* LabelEndColumn    */ L"Bis Spalte:",
    /* HintEndFixed      */ L"(leer = bis Zeilenende)",
    /* HintEndDelimited  */ L"(leer = nur Von-Spalte)",
    /* LabelDelimiter    */ L"Trennzeichen:",
    /* HintDelimiter     */ L"z. B. ; oder \\t (Tab)",
    /* LabelReplacement  */ L"Ersetzen durch:",
    /* ButtonPreview     */ L"Vorschau",
    /* ButtonReplace     */ L"Ersetzen",
    /* ButtonClose       */ L"Schließen",
    /* LabelPreview      */ L"Vorschau (max. 20 Änderungen):",
    /* LabelLanguage     */ L"Sprache:",
    /* StatusReady       */ L"Bereit.",
    /* StatusCalculating */ L"Vorschau wird berechnet ...",
    /* StatusNoChangesList */ L"(keine Änderungen)",
    /* StatusNoChanges   */ L"Keine Änderungen.",
    /* StatusAlreadyApplied */ L"Keine Änderungen - bereits ersetzt.",
    /* ErrorReadOnly     */ L"Das aktuelle Dokument ist schreibgeschützt.",
    /* ErrorDelimiterRequired */ L"Bitte ein Trennzeichen angeben (z. B. ; oder \\t für Tab).",
    /* ErrorDelimiterAscii */ L"Das Trennzeichen muss ein einzelnes ASCII-Zeichen sein (oder \\t für Tab).",
    /* ErrorDelimiterSingle */ L"Das Trennzeichen muss ein einzelnes Zeichen sein (oder \\t für Tab).",
    /* ErrorStartRequired */ L"Bitte eine Startspalte angeben (1 oder größer).",
    /* ErrorStartNumber  */ L"Die Startspalte muss eine Zahl ab 1 sein.",
    /* ErrorEndNumber    */ L"Die Endspalte ist keine gültige Zahl.",
    /* ErrorEndBeforeStart */ L"Die Endspalte darf nicht kleiner als die Startspalte sein.",
    /* ErrorLineFromMin  */ L"\"Zeile von\" muss mindestens 1 sein.",
    /* ErrorLineToMin    */ L"\"Zeile bis\" muss mindestens 1 sein.",
    /* ErrorLineRange    */ L"\"Zeile von\" darf nicht größer als \"Zeile bis\" sein.",
    /* ErrorNoSelection  */ L"\"Nur Zeilen in Auswahl\" ist aktiv, aber es ist kein Text ausgewählt.",
    /* LabelLineFromShort */ L"Zeile von",
    /* LabelLineToShort  */ L"Zeile bis",
    /* AnsiWarningTitle  */ L"ANSI-Datei",
    /* CodePagePrefix    */ L"Codepage ",
    /* LanguageGerman    */ L"Deutsch",
    /* LanguageEnglish   */ L"English",
};

static_assert(sizeof(kEnglish) / sizeof(kEnglish[0]) == kTextCount,
              "English table does not match the Text enum");
static_assert(sizeof(kGerman) / sizeof(kGerman[0]) == kTextCount,
              "German table does not match the Text enum");

Language g_language = Language::English;

bool containsIgnoreCase(const std::string& text, const char* needle) {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return lower.find(needle) != std::string::npos;
}

const wchar_t* const* table() {
    return g_language == Language::German ? kGerman : kEnglish;
}

} // namespace

Language detectLanguage() {
    if (g_nppData._nppHandle != nullptr) {
        char nativeLanguage[64] = {};
        SendMessageW(g_nppData._nppHandle, NPPM_GETNATIVELANGFILENAME,
                     static_cast<WPARAM>(sizeof(nativeLanguage)),
                     reinterpret_cast<LPARAM>(nativeLanguage));

        const std::string name(nativeLanguage);
        if (containsIgnoreCase(name, "german") ||
            containsIgnoreCase(name, "deutsch")) {
            return Language::German;
        }
        if (containsIgnoreCase(name, "english")) {
            return Language::English;
        }
    }

    const LANGID uiLanguage = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(uiLanguage) == LANG_GERMAN) {
        return Language::German;
    }
    return Language::English;
}

Language currentLanguage() { return g_language; }

void setLanguage(Language language) { g_language = language; }

const wchar_t* tr(Text id) {
    const int index = static_cast<int>(id);
    if (index < 0 || index >= kTextCount) {
        return L"";
    }
    return table()[index];
}

std::wstring formatInvalidNumber(const std::wstring& label) {
    std::wstring result = g_language == Language::German
                              ? L"Ungültiger Wert für \""
                              : L"Invalid value for \"";
    result += label;
    result += L"\".";
    return result;
}

std::wstring formatPreviewLine(long long lineNumber, const std::wstring& before,
                               const std::wstring& after) {
    std::wstring result = g_language == Language::German ? L"Zeile " : L"Line ";
    result += std::to_wstring(lineNumber);
    result += L":  ";
    result += before;
    result += L"   ->   ";
    result += after;
    return result;
}

std::wstring formatPreviewStatus(std::size_t matched, std::size_t changed,
                                 std::size_t shown) {
    std::wstring result = g_language == Language::German ? L"Treffer: "
                                                         : L"Matches: ";
    result += std::to_wstring(matched);
    result += g_language == Language::German ? L"   |   Änderungen: "
                                             : L"   |   Changes: ";
    result += std::to_wstring(changed);

    if (shown > 0 && changed > shown) {
        result += g_language == Language::German ? L"   (" : L"   (showing ";
        result += std::to_wstring(shown);
        result += g_language == Language::German ? L" von " : L" of ";
        result += std::to_wstring(changed);
        result += g_language == Language::German ? L" angezeigt)" : L")";
    }
    return result;
}

std::wstring formatReplaceStatus(std::size_t changed, std::size_t matched) {
    std::wstring result = g_language == Language::German ? L"Ersetzt: "
                                                         : L"Replaced: ";
    result += std::to_wstring(changed);
    result += g_language == Language::German ? L" Zeile(n)" : L" line(s)";
    result += g_language == Language::German ? L"   |   Treffer: "
                                             : L"   |   Matches: ";
    result += std::to_wstring(matched);
    result += g_language == Language::German ? L"   |   Rückgängig mit Strg+Z"
                                             : L"   |   Undo with Ctrl+Z";
    return result;
}

std::wstring formatReadyStatus(const std::wstring& encoding) {
    std::wstring result = g_language == Language::German
                              ? L"Bereit. Datei-Kodierung: "
                              : L"Ready. File encoding: ";
    result += encoding;
    result += L".";
    return result;
}

std::wstring formatAnsiWarning(const std::wstring& codePageName) {
    std::wstring result;
    if (g_language == Language::German) {
        result = L"Die Datei wird als ANSI gespeichert (";
        result += codePageName;
        result += L").\n\n";
        result += L"Der Ersatztext enthält Zeichen, die in dieser Codepage "
                  L"nicht vorkommen. Notepad++ ersetzt sie beim Speichern "
                  L"durch \"?\".\n\nTrotzdem ersetzen?";
    } else {
        result = L"The file is stored as ANSI (";
        result += codePageName;
        result += L").\n\n";
        result += L"The replacement text contains characters that do not "
                  L"exist in this code page. Notepad++ will replace them "
                  L"with \"?\" when saving.\n\nReplace anyway?";
    }
    return result;
}

} // namespace columnreplace
