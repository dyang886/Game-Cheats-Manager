import hashlib
import json
import os
import re
import string
import threading

import zhon.hanzi
from pypinyin import Style, lazy_pinyin
from rapidfuzz import fuzz, process

from config import DATABASE_PATH, is_chinese

# --- Tunables -------------------------------------------------------------
MIN_QUERY_LEN = 2            # reject shorter latin queries (1 char matches nothing useful)
MIN_SUBSTR_LEN = 3           # below this: acronym/whole-word only (no substring/fuzzy)
MIN_FUZZY_LEN = 5            # below this: substring only (fuzzy is noise for short queries)
STRING_THRESHOLD = 80        # partial_ratio cutoff for fuzzy string match
TRANSLATE_THRESHOLD = 85     # cutoff for fuzzy name -> translation fallback
EXPAND_CAP = 80              # max games one pinyin query may pull in

MIN_PINYIN_LEN = 3           # ignore pinyin matching for shorter queries
PINYIN_THRESHOLD = 88        # partial_ratio cutoff: query pinyin vs Chinese-target pinyin
PINYIN_EXPAND_MIN_LEN = 4    # min romanized-query length to bridge via pinyin
PINYIN_EXPAND_THRESHOLD = 88  # leading-overlap cutoff for romanized pinyin (transliteration-tolerant)

CH_MIN_SYLLABLES = 3         # min Chinese query syllables for token matching
CH_MIN_SHARED = 3            # min syllables shared with the target
CH_COVERAGE = 0.8            # shared / shorter-side must reach this (containment, not partial overlap)

_PUNCTUATION = (string.punctuation + zhon.hanzi.punctuation).replace('&', '')
_ROMAN_MAP = [
    (1000, 'M'), (900, 'CM'), (500, 'D'), (400, 'CD'), (100, 'C'), (90, 'XC'),
    (50, 'L'), (40, 'XL'), (10, 'X'), (9, 'IX'), (5, 'V'), (4, 'IV'), (1, 'I'),
]


def _arabic_to_roman(num):
    if num == 0:
        return '0'
    roman = ''
    for value, symbol in _ROMAN_MAP:
        while num >= value:
            roman += symbol
            num -= value
    return roman


def sanitize(text):
    """Numbers -> roman, drop punctuation/space, lowercase."""
    text = re.sub(r'\d+', lambda m: _arabic_to_roman(int(m.group())), text)
    return ''.join(c for c in text if c not in _PUNCTUATION and not c.isspace()).lower()


# --- Pinyin / acronym helpers --------------------------------------------

def _fuzzy_collapse(s):
    """Merge fuzzy-sound pairs so homophones become identical: zh/ch/sh->z/c/s, ng->n."""
    s = s.replace('zh', 'z').replace('ch', 'c').replace('sh', 's')
    return s.replace('ng', 'n')


def latin_to_fuzzy(text):
    """Fuzzy-collapse an already-romanized pinyin query."""
    return _fuzzy_collapse(re.sub(r'[^a-z]', '', text.lower()))


def pinyin_forms(text):
    """Fuzzy pinyin of Chinese text from one conversion, as (concat, space-joined syllables)."""
    syllables = []
    for token in lazy_pinyin(text, style=Style.NORMAL):
        token = re.sub(r'[^a-z]', '', token.lower())
        if token:
            syllables.append(token)
    concat = _fuzzy_collapse(''.join(syllables))
    return concat, ' '.join(_fuzzy_collapse(s) for s in syllables)


def acronym_of(text):
    """Word initials, digits kept."""
    return ''.join(word[0] for word in re.findall(r'[A-Za-z0-9]+', text)).lower()


def _words(text):
    """Lowercased alphanumeric words of a title."""
    return set(re.findall(r'[a-z0-9]+', text.lower()))


def _acronym_hit(query, acronym):
    """query equals the acronym, or (3+ chars) is a prefix of it."""
    if len(acronym) < 2:
        return False
    return query == acronym or (len(query) >= 3 and acronym.startswith(query))


def _prefix_overlap(a, b):
    """High ratio over the common-length leading prefix; tolerant of extra text on
    either side."""
    if a == b:
        return True
    n = min(len(a), len(b))
    return fuzz.ratio(a[:n], b[:n]) >= PINYIN_EXPAND_THRESHOLD


def _syllables_contained(query_tokens, target_tokens):
    """The shorter syllable set is ~fully inside the longer (>= CH_COVERAGE, >= CH_MIN_SHARED
    shared): tolerates a missing/extra syllable, rejects a long name sharing only a few."""
    q, t = set(query_tokens.split()), set(target_tokens.split())
    shared = len(q & t)
    return shared >= CH_MIN_SHARED and shared >= min(len(q), len(t)) * CH_COVERAGE


class QueryMatcher:
    """Precompute a query's match forms once, then test against many targets."""

    def __init__(self, keyword_list, raw_keyword):
        self.sanitized = [s for s in (sanitize(k) for k in keyword_list) if len(s) >= 2]
        alnum = re.sub(r'[^a-z0-9]', '', raw_keyword.lower())
        # acronym drops trailing digits but keeps embedded ones
        self.acronym_q = re.sub(r'\d+$', '', alnum)
        # 2-char queries match only via acronym/whole-word, never substring (avoids flooding)
        self.short_token = alnum if 2 <= len(alnum) < MIN_SUBSTR_LEN else ''
        if is_chinese(raw_keyword):
            self.pinyin_q, self.pinyin_tokens = pinyin_forms(raw_keyword)
        else:
            self.pinyin_q, self.pinyin_tokens = latin_to_fuzzy(raw_keyword), ''
        if len(self.pinyin_q) < MIN_PINYIN_LEN:
            self.pinyin_q = ''
        self.n_syllables = len(self.pinyin_tokens.split())

    def matches(self, target):
        target_s = sanitize(target)
        if len(target_s) < 2:
            return False

        # 1. String: short latin skipped (acronym/whole-word handle it), substring for
        #    medium, fuzzy for long.
        for kw in self.sanitized:
            if kw.isascii() and len(kw) < MIN_SUBSTR_LEN:
                continue
            if len(kw) < MIN_FUZZY_LEN:
                if kw in target_s:
                    return True
            elif fuzz.partial_ratio(kw, target_s) >= STRING_THRESHOLD:
                return True

        # 2. Acronym
        if len(self.acronym_q) >= 2 and _acronym_hit(self.acronym_q, acronym_of(target)):
            return True

        # 3. Whole word for 2-char queries
        if self.short_token and self.short_token in _words(target):
            return True

        # 4. Pinyin against a Chinese target
        if is_chinese(target):
            if self.n_syllables >= CH_MIN_SYLLABLES:            # tolerant syllable match
                if _syllables_contained(self.pinyin_tokens, pinyin_forms(target)[1]):
                    return True
            elif self.pinyin_q:                                 # homophone / romanized
                tp = pinyin_forms(target)[0]
                if tp and (self.pinyin_q in tp
                           or fuzz.partial_ratio(self.pinyin_q, tp) >= PINYIN_THRESHOLD):
                    return True
        return False


class TranslationIndex:
    """In-memory, hash-cached view of translations.json for fast bilingual lookup."""

    def __init__(self):
        self._lock = threading.Lock()
        self._hash = None
        self._built = False
        self.en_to_zh = {}        # sanitized en -> zh
        self.zh_to_en = {}        # sanitized zh -> en
        self.en_keys = []         # sanitized en, for fuzzy fallback
        self.zh_keys = []
        self.en_items = []        # (sanitized_en, zh) for substring expansion
        self.zh_items = []        # (sanitized_zh, en)
        self.zh_pinyin = []       # (concat, tokens, en) for pinyin expansion

    def _path(self):
        return os.path.join(DATABASE_PATH, "translations.json")

    def _read(self):
        try:
            with open(self._path(), 'rb') as f:
                return f.read()
        except OSError:
            return None

    def refresh(self):
        """Rebuild if translations.json content changed. Call once per search; it reads the
        file, so per-result lookups rely on _ensure_built() instead."""
        raw = self._read()
        digest = hashlib.md5(raw).hexdigest() if raw is not None else None
        if self._built and digest == self._hash:
            return
        with self._lock:
            if not (self._built and digest == self._hash):
                self._build(raw, digest)

    def _ensure_built(self):
        if not self._built:
            self.refresh()

    def _build(self, raw, digest):
        try:
            data = json.loads(raw) if raw else []
        except ValueError:
            data = []
        self.en_to_zh, self.zh_to_en = {}, {}
        self.en_items, self.zh_items, self.zh_pinyin = [], [], []
        for trainer in data:
            en, zh = trainer.get('en_US'), trainer.get('zh_CN')
            if not en or not zh:
                continue
            se, sz = sanitize(en), sanitize(zh)
            self.en_to_zh.setdefault(se, zh)
            self.zh_to_en.setdefault(sz, en)
            self.en_items.append((se, zh))
            self.zh_items.append((sz, en))
            concat, tokens = pinyin_forms(zh)
            self.zh_pinyin.append((concat, tokens, en))
        self.en_keys, self.zh_keys = list(self.en_to_zh), list(self.zh_to_en)
        self._hash, self._built = digest, True

    def has_data(self):
        self._ensure_built()
        return bool(self.en_items)

    def translate(self, name, target_language, threshold=TRANSLATE_THRESHOLD):
        """Translate name into target_language ('en'/'zh'): exact dict, then fuzzy fallback."""
        self._ensure_built()
        key = sanitize(name)
        if target_language == 'zh':
            mapping, keys = self.en_to_zh, self.en_keys
        else:
            mapping, keys = self.zh_to_en, self.zh_keys
        if key in mapping:
            return mapping[key]
        if not keys:
            return None
        match = process.extractOne(key, keys, scorer=fuzz.WRatio, score_cutoff=threshold)
        return mapping[match[0]] if match else None

    def expand_keyword(self, keyword):
        """Query -> cross-language equivalents plus pinyin-matched English titles."""
        self._ensure_built()
        results = {keyword}
        key = sanitize(keyword)
        chinese = is_chinese(keyword)

        # cross-language by exact containment; skip short latin keys (they substring-flood)
        if key and (chinese or len(key) >= MIN_SUBSTR_LEN):
            for name, other in (self.zh_items if chinese else self.en_items):
                if key in name:
                    results.add(other)

        # pinyin bridge to English titles: Chinese -> tolerant syllable match, romanized -> prefix
        if chinese:
            _, tokens = pinyin_forms(keyword)
            if len(tokens.split()) >= CH_MIN_SYLLABLES:
                self._bridge(results, lambda _concat, t: _syllables_contained(tokens, t))
        else:
            q = latin_to_fuzzy(keyword)
            if len(q) >= PINYIN_EXPAND_MIN_LEN:
                self._bridge(results, lambda concat, _t: bool(concat) and _prefix_overlap(q, concat))

        return [r for r in results if r]

    def _bridge(self, results, accept):
        """Add up to EXPAND_CAP English titles whose pinyin (concat, tokens) accept()s."""
        added = 0
        for concat, tokens, en in self.zh_pinyin:
            if en in results or not accept(concat, tokens):
                continue
            results.add(en)
            added += 1
            if added >= EXPAND_CAP:
                return


translation_index = TranslationIndex()
