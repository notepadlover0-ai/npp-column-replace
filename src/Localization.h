#pragma once

#include <cstddef>
#include <string>

namespace columnreplace {

enum class Language { English, German };

// All user-visible strings. The order must match the tables in
// Localization.cpp; Text::Count is used for the size check there.
enum class Text {
    PluginName,
    MenuItem,
    DialogCaption,
    GroupFilter,
    LabelPrefix,
    CheckCase,
    CheckSelection,
    CheckLineRange,
    LabelLineFrom,
    LabelLineTo,
    HintAllLines,
    HintPrefix,
    GroupColumns,
    RadioFixed,
    RadioDelimited,
    LabelStartColumn,
    LabelEndColumn,
    HintEndFixed,
    HintEndDelimited,
    LabelDelimiter,
    HintDelimiter,
    LabelReplacement,
    ButtonPreview,
    ButtonReplace,
    ButtonClose,
    LabelPreview,
    LabelLanguage,
    StatusReady,
    StatusCalculating,
    StatusNoChangesList,
    StatusNoChanges,
    StatusAlreadyApplied,
    ErrorReadOnly,
    ErrorDelimiterRequired,
    ErrorDelimiterAscii,
    ErrorDelimiterSingle,
    ErrorStartRequired,
    ErrorStartNumber,
    ErrorEndNumber,
    ErrorEndBeforeStart,
    ErrorLineFromMin,
    ErrorLineToMin,
    ErrorLineRange,
    ErrorNoSelection,
    LabelLineFromShort,
    LabelLineToShort,
    AnsiWarningTitle,
    CodePagePrefix,
    LanguageGerman,
    LanguageEnglish,
    Count
};

// Uses the Notepad++ native language file when available and falls back to
// the Windows UI language.
Language detectLanguage();
Language currentLanguage();
void setLanguage(Language language);

const wchar_t* tr(Text id);

std::wstring formatInvalidNumber(const std::wstring& label);
std::wstring formatPreviewLine(long long lineNumber, const std::wstring& before,
                               const std::wstring& after);
std::wstring formatPreviewStatus(std::size_t matched, std::size_t changed,
                                 std::size_t shown);
std::wstring formatReplaceStatus(std::size_t changed, std::size_t matched);
std::wstring formatReadyStatus(const std::wstring& encoding);
std::wstring formatAnsiWarning(const std::wstring& codePageName);

} // namespace columnreplace
