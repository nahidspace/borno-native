# Borno Native (Windows TSF)

A native Windows Text Services Framework (TSF) input processor for Bangla
phonetic typing. No toolbar, no tray icon, no hooks -- it registers as a real
keyboard under `Win+Space`, same as Windows' built-in language keyboards.

Borno Native is an independent implementation inspired by the user experience
of Avro Phonetic, the Bangla phonetic input method by OmicronLab. Avro
Phonetic and OmicronLab are credited here as compatibility references; this
project does not vendor Avro's implementation or rule data. The independent
conversion engine is cross-checked against `pyAvroPhonetic` as a development
oracle; see `tools/oracle/README.md`.

Read the full documentation at [nahidspace.github.io/borno-native](https://nahidspace.github.io/borno-native/).

Status: **V2 of the roadmap** -- a working TSF skeleton with a candidate
popup. The phonetic engine (`Borno.TSF/Convert.cpp`) is independently written
(not vendored from OmicronLab -- see `tools/oracle/README.md` for why) but
cross-verified against the real pyAvroPhonetic engine as a black-box oracle:
109 test words/phrases in `tools/oracle/wordlist.txt`; the known remaining
gaps are listed below.

## License

Original Borno Native source code and documentation are released under the
Apache License, Version 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

The bundled Bangla word-frequency resource has separate CC BY-SA 4.0 terms;
see `Borno.TSF/Resources/ATTRIBUTION.md`. Development-only oracle materials
have their own licensing boundary and are not relicensed under Apache-2.0.

## Build

```powershell
.\scripts\build.ps1          # Debug (default)
.\scripts\build.ps1 Release
```

## Try it

```powershell
.\scripts\register.ps1       # needs Administrator -- will prompt via UAC
```

Then: **Settings > Time & language > Language & region** -> add/open
**Bangla** -> **Language options** -> **Add a keyboard** -> **Borno Native**.

Windows also registers its own built-in (non-phonetic) Bangla keyboard under
the same language, so `Win+Space` cycles through *three* stops (English ->
Windows' default Bangla layout -> Borno Native -> back to English) --
easy to land on the wrong one. To go straight to ours, click the language
indicator near the clock and pick **Borno Native** explicitly from the list.

To remove it:

```powershell
.\scripts\unregister.ps1
```

## What works right now

- Appears as a real keyboard under `Win+Space`, no separate app UI.
- Live transliteration as you type (e.g. `ami` -> `আমি`, `bangladesh` ->
  `বাংলাদেশ`, `kShomota` -> `ক্ষমতা`), including conjuncts formed via hasant.
- Backspace re-converts the shortened buffer instead of just deleting glyphs.
- Space/punctuation/Enter commit the word and start a fresh one.
- A candidate popup appears near the caret whenever a word has more than one
  legitimate spelling (Bangla's sound-alike letters: স/শ/ষ, and hrasva/dirgho
  ই/ঈ and উ/ঊ). Navigate with Up/Down, jump to an entry with 1-9, commit with
  Enter/Tab, dismiss with Escape. Self-drawn (not TSF's native candidate UI),
  Uniscribe-shaped so Bangla conjuncts render correctly, light/dark aware.
- Candidates are ranked by real Bangla word frequency (`Borno.TSF/Dictionary.cpp`,
  25k words embedded as a DLL resource from `wordfreq` -- see
  `Resources/ATTRIBUTION.md` for the CC BY-SA sourcing), not just generation
  order -- e.g. typing `bishesh` now ranks the real word বিশেষ above the
  mechanically-generated (non-word) বিশেশ.

## Known limitations (expected at this stage)

- **"w" handling** -- not mapped at all yet (`worldcup`, `password`,
  `windows` all diverge from the real engine's contextual handling of it).
- **Nasal assimilation before palatals** -- `shunchi` should give শুঞ্ছি
  (ন -> ঞ before ch/c/j/jh), we give শুন্ছি.
- Candidates only ever come from substituting sound-alike letters in the
  primary conversion -- words with no ambiguous letter (the vast majority)
  never show a popup at all, even though real Avro would still offer
  dictionary-based suggestions for them. Broadening this needs a
  phonetic-to-dictionary reverse index, not just ranking what we already
  generate.
- No next-word prediction yet. Needs a different asset than the word-frequency
  dictionary -- an actual bigram/trigram model requires a sentence corpus to
  count word-pair co-occurrence, not just unigram frequencies. Not yet sourced.
- No settings app yet (V4). Tested primarily in Notepad; behavior in
  Chromium/Electron/Office/RDP is not yet verified (also V3+).
- Registration is machine-wide (`HKEY_CLASSES_ROOT`) and requires admin.

## Layout

```
Borno.TSF/      C++/COM TSF text service
Borno.Tests/    console harness: runs Convert() over a word list for testing
scripts/        build / register / unregister helper scripts
tools/oracle/   pyAvroPhonetic reference oracle for cross-verification (dev-only,
                not shipped -- see its README for the licensing approach)
```
