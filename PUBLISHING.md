# Projekt auf GitHub veröffentlichen

Die Konten selbst müssen von Ihnen angelegt werden, weil Google und GitHub eine
persönliche Verifizierung (Telefon/CAPTCHA) und die Zustimmung zu ihren
Nutzungsbedingungen verlangen.

## 1. Gmail-Konto anlegen

1. <https://accounts.google.com/signup> öffnen.
2. Namen, gewünschte Adresse und Passwort eingeben.
3. Telefonnummer zur Verifizierung angeben und den Code eingeben.
4. Nutzungsbedingungen akzeptieren.

## 2. GitHub-Konto anlegen

1. <https://github.com/signup> öffnen.
2. Die neue Gmail-Adresse verwenden.
3. E-Mail-Adresse bestätigen (Link in der Gmail-Mail).
4. Gewünschten Benutzernamen wählen, z. B. `aliosman`.
5. Zwei-Faktor-Authentifizierung einrichten (empfohlen).

## 3. Repository anlegen

1. Auf GitHub: **New repository**.
2. Name: `npp-column-replace`
3. Sichtbarkeit: öffentlich oder privat.
4. **Kein** README, keine `.gitignore`, keine Lizenz hinzufügen – diese Dateien
   sind bereits im Projekt.
5. **Create repository**.

## 4. Lokal committen und pushen

Im Projektordner:

```bash
cd /Users/aliosman/Documents/npp-column-replace

git config user.name "Ihr Name"
git config user.email "ihre-neue-adresse@gmail.com"

git add -A
git commit -m "Initial commit: Notepad++ Spalten-Ersetzen 1.2.0"

git remote add origin https://github.com/<BENUTZERNAME>/npp-column-replace.git
git branch -M main
git push -u origin main
```

`<BENUTZERNAME>` durch den GitHub-Benutzernamen ersetzen.

## 5. Authentifizierung

Beim Push fragt GitHub nach Zugangsdaten. Empfohlen:

```bash
brew install gh
gh auth login
```

Dann `GitHub.com` → `HTTPS` → `Login with a web browser` wählen und den
angezeigten Code im Browser bestätigen. Alternativ ein Personal Access Token
mit `repo`-Rechte erzeugen und als Passwort verwenden.

**Wichtig:** Passwort, Token oder 2FA-Code niemals an Dritte weitergeben.

## 6. Release mit fertigen DLLs (optional)

```bash
git tag v1.2.0
git push origin v1.2.0
```

Auf GitHub unter **Releases → Draft a new release** den Tag `v1.2.0` wählen und
die beiden ZIPs aus `dist/` als Anhänge hochladen:

- `dist/ColumnReplace-1.2.0-x64.zip`
- `dist/ColumnReplace-1.2.0-x86.zip`

## 7. Automatischer Build

Die Datei `.github/workflows/build.yml` baut bei jedem Push automatisch beide
Architekturen mit MSVC. Die DLLs stehen anschließend unter **Actions → Build →
Artifacts** bereit.
