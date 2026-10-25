# Spalten-Ersetzen für Notepad++

[![build](https://github.com/notepadlover0-ai/npp-column-replace/actions/workflows/build.yml/badge.svg)](https://github.com/notepadlover0-ai/npp-column-replace/actions/workflows/build.yml)

Ein Notepad++-Plugin (32- und 64-Bit), das in ausgewählten Zeilen einen festen
Spaltenbereich durch einen neuen Wert ersetzt.

Damit lassen sich zum Beispiel Datensätze bearbeiten, bei denen die ersten drei
Zeichen ein Satzart-/Präfixkennzeichen und bestimmte Spalten danach feste
Felder sind.

## Funktionen

- **Zwei Spaltenmodi:**
  - **Feste Zeichenpositionen** (Standard): Spalten werden als Zeichenpositionen
    gezählt.
  - **Trennzeichen-getrennte Felder (CSV)**: Spalten werden als Felder
    interpretiert, z. B. `;`- oder Tab-getrennt. Anführungszeichen und
    verdoppelte Anführungszeichen werden berücksichtigt.
- **Filter nach Zeilenanfang (Präfix):** Es werden nur Zeilen bearbeitet, die
  mit dem angegebenen Text beginnen. Für „die ersten 3 Zeichen“ einfach diese
  3 Zeichen als Präfix eingeben.
- **Optionaler Zeilenbereich:** z. B. nur Zeile 1 bis 3 („die ersten 3 Zeilen“).
- **Optional nur Zeilen in der aktuellen Auswahl.**
- **Spaltenbereich:** 1-basiert und inklusive.
  - Feste Positionen: Endspalte leer = bis zum Zeilenende.
  - CSV: Endspalte leer = nur die Startspalte.
- **Vorschau** der ersten 20 Änderungen vor dem Ersetzen.
- **Ein einziger Undo-Schritt:** Nach dem Ersetzen mit `Strg+Z` komplett
  rückgängig machen.
- Zeilenenden (CRLF/LF) und alle nicht betroffenen Zeichen bleiben unverändert.
- Feste Spalten zählen UTF-8-Zeichen, nicht Bytes. Umlaute und die meisten
  Emojis zählen als ein Zeichen; ein Tab zählt als ein Zeichen.
- CSV-Ersetzungen werden bei Bedarf automatisch in Anführungszeichen gesetzt
  und Anführungszeichen im Wert verdoppelt.

## Installation

1. Notepad++ schließen.
2. Die zur Notepad++-Version passende DLL kopieren:
   - **64-Bit-Notepad++**
     `C:\Program Files\Notepad++\plugins\ColumnReplace\ColumnReplace.dll`
   - **32-Bit-Notepad++**
     `C:\Program Files (x86)\Notepad++\plugins\ColumnReplace\ColumnReplace.dll`
   - **Portable Version**
     `<Notepad++-Ordner>\plugins\ColumnReplace\ColumnReplace.dll`
3. Notepad++ starten.
4. Im Menü **Plugins → Spalten-Ersetzen → Spalten-Ersetzen...** öffnen.

Welche Architektur die installierte Notepad++-Version hat, steht unter
**? → Info** (bzw. **Hilfe → Info**). Die DLL-Architektur muss dazu passen;
eine 64-Bit-DLL wird von 32-Bit-Notepad++ nicht geladen (und umgekehrt).

## Bedienung

| Feld | Bedeutung |
| --- | --- |
| **Zeilenanfang (Präfix)** | Nur Zeilen, die mit diesem Text beginnen. Leer = alle Zeilen. |
| **Groß-/Kleinschreibung beachten** | Vergleich des Präfixes case-sensitiv oder nicht. |
| **Nur Zeilen in Auswahl** | Nur Zeilen, die die aktuelle Textauswahl berühren. |
| **Zeilen von / bis** | Optionaler 1-basierter Zeilenbereich. Leer = keine Begrenzung. |
| **Feste Zeichenpositionen** | Spalten werden als Zeichenpositionen in der Zeile gezählt. |
| **Trennzeichen-getrennte Felder** | Spalten werden als Felder zwischen Trennzeichen gezählt (CSV/TSV). |
| **Trennzeichen** | Ein Zeichen, z. B. `;`, `,` oder `\|`. `\t` steht für Tab. |
| **Von Spalte** | Erste zu ersetzende Spalte bzw. erstes Feld, 1-basiert. |
| **Bis Spalte** | Letzte zu ersetzende Spalte bzw. letztes Feld, inklusive. Leer = siehe Modus. |
| **Ersetzen durch** | Neuer Text. Leer = Spalten löschen. |

**Vorschau** zeigt Trefferzahl und die ersten 20 Änderungen, ohne das Dokument
zu verändern. **Ersetzen** führt alle Änderungen als eine Undo-Gruppe aus.

### Beispiel

Ausgangstext:

```text
ABC001Hans
ABC002Anna
ABC003Max
XYZ004Peter
```

Einstellungen:

- Zeilenanfang: `ABC`
- Zeilen von: `1`, bis: `3` (nur die ersten drei Zeilen)
- Von Spalte: `4`
- Bis Spalte: `6`
- Ersetzen durch: `999`

Ergebnis:

```text
ABC999Hans
ABC999Anna
ABC999Max
XYZ004Peter
```

Die Spalten 4–6 enthalten `001`, `002` und `003`; sie werden durch `999`
ersetzt. `XYZ004Peter` bleibt unverändert, weil das Präfix nicht passt.

### Beispiel: CSV-/Trennzeichen-Modus

Ausgangstext:

```text
ABC;001;Hans
ABC;002;Anna
XYZ;003;Max
```

Einstellungen:

- Modus: **Trennzeichen-getrennte Felder**
- Trennzeichen: `;`
- Zeilenanfang: `ABC`
- Von Spalte: `2`
- Bis Spalte: `2`
- Ersetzen durch: `999`

Ergebnis:

```text
ABC;999;Hans
ABC;999;Anna
XYZ;003;Max
```

Soll der Wert ein Trennzeichen enthalten, wird er automatisch gequotet:

```text
Ersetzen durch: 999;Sonderfall
Ergebnis:       ABC;"999;Sonderfall";Hans
```

## Verhalten im Detail

### Feste Zeichenpositionen

- Spalten sind **1-basiert und inklusive**: „Von 4 bis 6“ ersetzt genau drei
  Zeichen.
- Ist die Zeile kürzer als die Startspalte, bleibt sie unverändert.
- Liegt die Endspalte hinter dem Zeilenende, wird bis zum Zeilenende ersetzt.
- Das Präfix wird **literal** verglichen (kein regulärer Ausdruck).
- „Nur Zeilen in Auswahl“ filtert ganze Zeilen; ersetzt werden weiterhin die
  angegebenen Spalten der gesamten Zeile, nicht nur der markierte Bereich.
- Die Ersetzung ist einzeilig; Zeilenumbrüche können nicht eingegeben werden.
- Die letzte Einstellung bleibt während der Notepad++-Sitzung erhalten.

### Trennzeichen-getrennte Felder (CSV)

- Felder sind **1-basiert und inklusive**. „Von 2 bis 3“ ersetzt Feld 2 und 3
  durch einen Wert; das Trennzeichen zwischen beiden entfällt dadurch.
- Endspalte leer = nur die Startspalte wird ersetzt.
- Standard-Anführungszeichen ist `"`. Trennzeichen innerhalb von
  Anführungszeichen werden nicht als Feldtrenner behandelt; `""` ist ein
  verdoppeltes (escaptes) Anführungszeichen im Feldwert.
- Der neue Wert wird automatisch gequotet, wenn er das Trennzeichen, ein
  Anführungszeichen, einen Zeilenumbruch oder führende/abschließende Leerzeichen
  enthält.
- Leere Felder am Zeilenende (z. B. `a;b;`) werden als Feld 3 erkannt.
- Die Verarbeitung ist zeilenbasiert. Felder mit Zeilenumbrüchen innerhalb von
  Anführungszeichen (mehrzeilige CSV-Felder) werden nicht unterstützt.
- Ein abschließender Zeilenumbruch erzeugt keinen zusätzlichen leeren Datensatz.
- Als Trennzeichen ist ein einzelnes ASCII-Zeichen erlaubt; `\t` steht für Tab.
  Das Anführungszeichen `"` kann nicht als Trennzeichen verwendet werden.
- Das Präfix wird **literal** verglichen (kein regulärer Ausdruck).
- „Nur Zeilen in Auswahl“ filtert ganze Zeilen; ersetzt werden weiterhin die
  angegebenen Felder der gesamten Zeile.
- Die Ersetzung ist einzeilig; Zeilenumbrüche können nicht eingegeben werden.
- Die letzte Einstellung bleibt während der Notepad++-Sitzung erhalten.

## Fertige Builds

In diesem Projekt liegen bereits gebaute Pakete:

- `dist/ColumnReplace-1.1.0-x64.zip` – 64-Bit
- `dist/ColumnReplace-1.1.0-x86.zip` – 32-Bit

Jedes ZIP enthält den Ordner `ColumnReplace` mit der DLL, dieser wird direkt
nach `<Notepad++>\plugins\` kopiert.

## Selbst bauen

### Windows (Visual Studio + CMake)

Voraussetzungen:

- Visual Studio 2022 mit „Desktopentwicklung mit C++“
- CMake 3.20 oder neuer

Im Projektordner:

```bat
build_windows.bat
```

oder manuell:

```bat
cmake -S . -B build\x64 -A x64
cmake --build build\x64 --config Release

cmake -S . -B build\x86 -A Win32
cmake --build build\x86 --config Release
```

Ergebnis:

- `build\x64\Release\ColumnReplace.dll` (64-Bit)
- `build\x86\Release\ColumnReplace.dll` (32-Bit)

### Cross-Compile unter macOS/Linux (MinGW-w64)

```bash
brew install mingw-w64 cmake   # macOS
./build_mingw.sh
```

Ergebnis:

- `build/x86_64/ColumnReplace.dll`
- `build/x86/ColumnReplace.dll`

Die MinGW-Builds linken libstdc++/libgcc statisch. Die Windows-UCRT
(`api-ms-win-crt-*`) wird von Windows 10/11 mitgebracht.

### Kernlogik testen

Die Spaltenlogik ist plattformunabhängig und lässt sich ohne Windows testen:

```bash
clang++ -std=c++17 -Wall -Wextra -I src src/ColumnReplace.cpp \
    tests/test_core.cpp -o test_core
./test_core
```

## Kompatibilität

- Notepad++ 8.6 oder neuer empfohlen (32- und 64-Bit), Windows 10/11.
- Verwendet `SCI_GETTEXTRANGEFULL` aus Scintilla 5.2.3+ mit Fallback auf
  `SCI_GETTEXT` für ältere Versionen.
- Die mitgelieferten Header stammen aus dem Notepad++-Quellcode.
- Bei sehr großen Dateien kann die Vorschau einen Moment dauern, weil das
  Dokument für die Analyse einmal eingelesen wird.

## Lizenz

GPLv3. Die Notepad++-Plugin-Header (`PluginInterface.h`,
`Notepad_plus_msgs.h`, `Scintilla.h`) stehen unter der GPLv3; dieses Plugin
verwendet sie und wird daher ebenfalls unter der GPLv3 veröffentlicht.
