# Cross-verification oracle (clean-room, not shipped)

This directory is dev tooling only -- nothing here ships in BornoTSF.dll.

## What it is

`pyAvroPhonetic` (OmicronLab's official Python port) installed here in
`.venv/`, patched to run on Python 3 (it's a 2013-era Python 2 codebase --
see the shim in `.venv/Lib/site-packages/pyavrophonetic/utils/__init__.py`).
Used as a black-box test oracle to cross-verify `Borno.TSF/Convert.cpp`'s
output against the real thing.

## Licensing approach

`pyAvroPhonetic` is GPL-3.0. Its rule data file (`avrodict.json`) carries its
own internal metadata claiming a separate MIT license, but that claim isn't
independently verifiable (the referenced upstream, `jsAvroPhonetic`, shows no
license at all) and doesn't override the enclosing package's GPL-3.0.

Given that, this project treats `pyAvroPhonetic` strictly as a **test
oracle**: we run it, observe its input/output behavior, and derive our own
independently-written rules/tables (like the conjunct-pair whitelist in
`Convert.cpp`) from those *observations*. Nothing from `avrodict.json` or
`avro.py` is copied into `Borno.TSF/`. The word-pair correspondences a
phonetic scheme produces are facts about the scheme, not copyrightable
expression -- but the specific GPL-licensed code/data files are not vendored
here, and `Borno.TSF/` remains independently licensable.

If a future need calls for directly vendoring OmicronLab's rule data instead
of clean-room re-deriving it, that requires deciding to adopt GPL-3.0 for the
whole project first -- see the discussion this traces back to.

## Files

- `wordlist.txt` -- ~100 test words/phrases across pronouns, verbs, retroflex
  vs dental consonants, sibilants, conjuncts, digits, punctuation, sentences.
- `theirs.tsv` -- oracle output for `wordlist.txt` (`word\tresult`).
- `ours.tsv` -- our `Convert()`'s output for the same list, generated via
  `../../Borno.Tests/BornoTests.exe wordlist.txt ours.tsv`.
- `conjunct_matrix.tsv` / `conjunct_pairs.txt` -- exhaustive test of all
  ~1100 consonant-key pair combinations against the oracle, and the 203
  pairs that came back forming a conjunct. This is the empirical source for
  `FormsConjunct()` in `Convert.cpp`.

## Re-running the comparison

```
cd tools/oracle
PYTHONIOENCODING=utf-8 ./.venv/Scripts/python.exe -c "from pyavrophonetic import avro; print(avro.parse('ami'))"
../../Borno.Tests/x64/Debug/BornoTests.exe wordlist.txt ours.tsv
# then diff ours.tsv vs theirs.tsv (see conversation history for the diff script,
# or just write a quick one -- read both TSVs as UTF-8, compare by word)
```

## Known remaining gaps (as of the last cross-check)

Diff went from 33 real divergences down to 5 after adding the conjunct
whitelist, digit/dari punctuation, case-sensitive vowel digraphs, and the
kkh/gg irregular clusters. Remaining:

- **"w" handling** -- not mapped at all; real Avro does something contextual
  with it we haven't reverse-engineered yet (`worldcup`, `password`,
  `windows` all diverge).
- **Nasal assimilation before palatals** -- `shunchi` should give শুঞ্ছি
  (ন -> ঞ before ch/c/j/jh), we give শুন্ছি. Not yet implemented.
