import pyperclip

def transliterate_gujarati(text_in: str) -> str:
    # Independent vowels
    VOWELS = {
        "અ": "a",
        "આ": "ā",
        "ઇ": "i",
        "ઈ": "ī",
        "ઉ": "u",
        "ઊ": "ū",
        "એ": "ē",
        "ઐ": "ai",
        "ઓ": "ō",
        "ઔ": "au",
    }

    # Consonants
    CONSONANTS = {
        "ક": "k",
        "ખ": "kh",
        "ગ": "g",
        "ઘ": "gh",
        "ચ": "c",
        "જ": "j",
        "ટ": "ṭ",
        "ડ": "ḍ",
        "ત": "t",
        "દ": "d",
        "ન": "n",
        "પ": "p",
        "બ": "b",
        "મ": "m",
        "ય": "y",
        "ર": "r",
        "લ": "l",
        "વ": "v",
        "શ": "ś",
        "ષ": "ṣ",
        "સ": "s",
        "હ": "h",
    }

    # Dependent vowel signs
    VOWEL_SIGNS = {
        "ા": "ā",
        "િ": "i",
        "ી": "ī",
        "ુ": "u",
        "ૂ": "ū",
        "ે": "ē",
        "ૈ": "ai",
        "ો": "ō",
        "ૌ": "au",
    }

    DIACRITICS = {
        "ં": "ṁ",
        "ઃ": "ḥ",
        "ઁ": "m̐",
    }

    VIRAMA = "્"

    result = []
    i = 0

    while i < len(text_in):
        ch = text_in[i]

        # Independent vowels
        if ch in VOWELS:
            result.append(VOWELS[ch])

        # Consonants
        elif ch in CONSONANTS:
            cons = CONSONANTS[ch]
            vowel = "a"  # inherent vowel

            # Look ahead
            if i + 1 < len(text_in):
                nxt = text_in[i + 1]

                # Virama suppresses vowel
                if nxt == VIRAMA:
                    vowel = ""
                    i += 1

                # Vowel sign replaces inherent vowel
                elif nxt in VOWEL_SIGNS:
                    vowel = VOWEL_SIGNS[nxt]
                    i += 1

            result.append(cons + vowel)

        # Diacritics
        elif ch in DIACRITICS:
            result.append(DIACRITICS[ch])

        i += 1

    return "".join(result)

def transliterate_arabic(text_in: str) -> str:
    ARABIC_MAP = {
        "ا": "ā",
        "ب": "b",
        "ت": "t",
        "ث": "th",
        "ج": "j",
        "ح": "ḥ",
        "خ": "kh",
        "د": "d",
        "ذ": "dh",
        "ر": "r",
        "ز": "z",
        "س": "s",
        "ش": "sh",
        "ص": "ṣ",
        "ض": "ḍ",
        "ط": "ṭ",
        "ظ": "ẓ",
        "ع": "ʿ",
        "غ": "gh",
        "ف": "f",
        "ق": "q",
        "ك": "k",
        "ل": "l",
        "م": "m",
        "ن": "n",
        "ه": "h",
        "و": "w",
        "ي": "y",
        "ء": "ʾ",

        # Variants
        "أ": "ʾa",
        "إ": "ʾi",
        "ؤ": "ʾu",
        "ئ": "ʾi",
        "ى": "ā",
        "ة": "a",

        # Persian additions
        "پ": "p",
        "چ": "ch",
        "ژ": "zh",
        "گ": "g",
    }

    HARAKAT = {
        "َ": "a",   # fatha
        "ِ": "i",   # kasra
        "ُ": "u",   # damma
        "ً": "an",
        "ٍ": "in",
        "ٌ": "un",
        "ْ": "",    # sukun
        "ّ": "",    # shadda handled separately
    }

    result = []
    i = 0

    while i < len(text_in):
        ch = text_in[i]

        # Shadda doubles consonant
        if ch == "ّ":
            if result:
                result[-1] = result[-1] + result[-1][-1]
            i += 1
            continue

        # Harakat
        if ch in HARAKAT:
            result.append(HARAKAT[ch])
            i += 1
            continue

        # Main letters
        result.append(ARABIC_MAP.get(ch, ch))

        i += 1

    return "".join(result)

def transliterate_devanagari(text_in: str) -> str:
    # Independent vowels
    VOWELS = {
        "अ": "a",
        "आ": "ā",
        "इ": "i",
        "ई": "ī",
        "उ": "u",
        "ऊ": "ū",
        "ऋ": "ṛ",
        "ॠ": "ṝ",
        "ऌ": "ḷ",
        "ॡ": "ḹ",
        "ए": "ē",
        "ऐ": "ai",
        "ओ": "ō",
        "औ": "au",
    }

    # Consonants
    CONSONANTS = {
        "क": "k",
        "ख": "kh",
        "ग": "g",
        "घ": "gh",
        "ङ": "ṅ",

        "च": "c",
        "छ": "ch",
        "ज": "j",
        "झ": "jh",
        "ञ": "ñ",

        "ट": "ṭ",
        "ठ": "ṭh",
        "ड": "ḍ",
        "ढ": "ḍh",
        "ण": "ṇ",

        "त": "t",
        "थ": "th",
        "द": "d",
        "ध": "dh",
        "न": "n",

        "प": "p",
        "फ": "ph",
        "ब": "b",
        "भ": "bh",
        "म": "m",

        "य": "y",
        "र": "r",
        "ल": "l",
        "व": "v",

        "श": "ś",
        "ष": "ṣ",
        "स": "s",
        "ह": "h",

        # Additional consonants
        "ळ": "ḷ",
        "क्ष": "kṣ",
        "ज्ञ": "jñ",
    }

    # Dependent vowel signs
    VOWEL_SIGNS = {
        "ा": "ā",
        "ि": "i",
        "ी": "ī",
        "ु": "u",
        "ू": "ū",
        "ृ": "ṛ",
        "ॄ": "ṝ",
        "ॢ": "ḷ",
        "ॣ": "ḹ",
        "े": "ē",
        "ै": "ai",
        "ो": "ō",
        "ौ": "au",
    }

    # Misc signs
    SIGNS = {
        "ं": "ṁ",   # anusvara
        "ः": "ḥ",   # visarga
        "ँ": "m̐",  # candrabindu
        "ऽ": "'",   # avagraha
    }

    VIRAMA = "्"

    result = []
    i = 0

    while i < len(text_in):
        ch = text_in[i]

        # Independent vowels
        if ch in VOWELS:
            result.append(VOWELS[ch])
            i += 1
            continue

        # Consonants
        if ch in CONSONANTS:
            cons = CONSONANTS[ch]
            vowel = "a"  # inherent vowel

            # Look ahead
            if i + 1 < len(text_in):
                nxt = text_in[i + 1]

                # Virama suppresses vowel
                if nxt == VIRAMA:
                    vowel = ""
                    i += 1

                # Dependent vowel sign replaces inherent vowel
                elif nxt in VOWEL_SIGNS:
                    vowel = VOWEL_SIGNS[nxt]
                    i += 1

            result.append(cons + vowel)
            i += 1
            continue

        # Signs
        if ch in SIGNS:
            result.append(SIGNS[ch])
            i += 1
            continue

        # Numbers (optional)
        DEVANAGARI_DIGITS = {
            "०": "0",
            "१": "1",
            "२": "2",
            "३": "3",
            "४": "4",
            "५": "5",
            "६": "6",
            "७": "7",
            "८": "8",
            "९": "9",
        }

        if ch in DEVANAGARI_DIGITS:
            result.append(DEVANAGARI_DIGITS[ch])
            i += 1
            continue

        # Unknown chars copied directly
        result.append(ch)
        i += 1

    return "".join(result)

def transliterate_malayam(text_in: str) -> str:
    # Independent vowels
    VOWELS = {
        "അ": "a",
        "ആ": "ā",
        "ഇ": "i",
        "ഈ": "ī",
        "ഉ": "u",
        "ഊ": "ū",
        "ഋ": "ṛ",
        "ൠ": "ṝ",
        "ഌ": "ḷ",
        "ൡ": "ḹ",
        "എ": "e",
        "ഏ": "ē",
        "ഐ": "ai",
        "ഒ": "o",
        "ഓ": "ō",
        "ഔ": "au",
    }

    # Consonants
    CONSONANTS = {
        "ക": "k",
        "ഖ": "kh",
        "ഗ": "g",
        "ഘ": "gh",
        "ങ": "ṅ",

        "ച": "c",
        "ഛ": "ch",
        "ജ": "j",
        "ഝ": "jh",
        "ഞ": "ñ",

        "ട": "ṭ",
        "ഠ": "ṭh",
        "ഡ": "ḍ",
        "ഢ": "ḍh",
        "ണ": "ṇ",

        "ത": "t",
        "ഥ": "th",
        "ദ": "d",
        "ധ": "dh",
        "ന": "n",

        "പ": "p",
        "ഫ": "ph",
        "ബ": "b",
        "ഭ": "bh",
        "മ": "m",

        "യ": "y",
        "ര": "r",
        "റ": "ṟ",
        "ല": "l",
        "ള": "ḷ",
        "ഴ": "ḻ",
        "വ": "v",

        "ശ": "ś",
        "ഷ": "ṣ",
        "സ": "s",
        "ഹ": "h",
    }

    # Chillu letters
    CHILLUS = {
        "ൺ": "ṇ",
        "ൻ": "n",
        "ർ": "r",
        "ൽ": "l",
        "ൾ": "ḷ",
        "ൿ": "k",
    }

    # Dependent vowel signs
    VOWEL_SIGNS = {
        "ാ": "ā",
        "ി": "i",
        "ീ": "ī",
        "ു": "u",
        "ൂ": "ū",
        "ൃ": "ṛ",
        "ൄ": "ṝ",
        "ൢ": "ḷ",
        "ൣ": "ḹ",
        "െ": "e",
        "േ": "ē",
        "ൈ": "ai",
        "ൊ": "o",
        "ോ": "ō",
        "ൌ": "au",
    }

    # Misc signs
    SIGNS = {
        "ം": "ṁ",   # anusvara
        "ഃ": "ḥ",   # visarga
        "ഽ": "'",   # avagraha
    }

    CHANDRAKKALA = "്"


    result = []
    i = 0

    while i < len(text_in):
        ch = text_in[i]

        # Independent vowels
        if ch in VOWELS:
            result.append(VOWELS[ch])
            i += 1
            continue

        # Chillus
        if ch in CHILLUS:
            result.append(CHILLUS[ch])
            i += 1
            continue

        # Consonants
        if ch in CONSONANTS:
            cons = CONSONANTS[ch]
            vowel = "a"  # inherent vowel

            if i + 1 < len(text_in):
                nxt = text_in[i + 1]

                # Chandrakkala suppresses vowel
                if nxt == CHANDRAKKALA:
                    vowel = ""
                    i += 1

                # Dependent vowel signs
                elif nxt in VOWEL_SIGNS:
                    vowel = VOWEL_SIGNS[nxt]
                    i += 1

            result.append(cons + vowel)
            i += 1
            continue

        # Signs
        if ch in SIGNS:
            result.append(SIGNS[ch])
            i += 1
            continue

        # Digits
        MALAYALAM_DIGITS = {
            "൦": "0",
            "൧": "1",
            "൨": "2",
            "൩": "3",
            "൪": "4",
            "൫": "5",
            "൬": "6",
            "൭": "7",
            "൮": "8",
            "൯": "9",
        }

        if ch in MALAYALAM_DIGITS:
            result.append(MALAYALAM_DIGITS[ch])
            i += 1
            continue

        # Unknown chars copied directly
        result.append(ch)
        i += 1

    return "".join(result)

def transliterate_tamil(text_in: str) -> str:
    # Independent vowels
    VOWELS = {
        "அ": "a",
        "ஆ": "ā",
        "இ": "i",
        "ஈ": "ī",
        "உ": "u",
        "ஊ": "ū",
        "எ": "e",
        "ஏ": "ē",
        "ஐ": "ai",
        "ஒ": "o",
        "ஓ": "ō",
        "ஔ": "au",
    }

    # Consonants
    CONSONANTS = {
        "க": "k",
        "ங": "ṅ",

        "ச": "c",
        "ஞ": "ñ",

        "ட": "ṭ",
        "ண": "ṇ",

        "த": "t",
        "ந": "n",

        "ப": "p",
        "ம": "m",

        "ய": "y",
        "ர": "r",
        "ல": "l",
        "வ": "v",

        "ழ": "ḻ",
        "ள": "ḷ",
        "ற": "ṟ",
        "ன": "ṉ",

        "ஷ": "ṣ",
        "ஸ": "s",
        "ஜ": "j",
        "ஹ": "h",
        "க்ஷ": "kṣ",
    }

    # Dependent vowel signs
    VOWEL_SIGNS = {
        "ா": "ā",
        "ி": "i",
        "ீ": "ī",
        "ு": "u",
        "ூ": "ū",
        "ெ": "e",
        "ே": "ē",
        "ை": "ai",
        "ொ": "o",
        "ோ": "ō",
        "ௌ": "au",
    }

    # Misc signs
    SIGNS = {
        "ம்": "m",
        "ஃ": "ḵ",  # aytham
    }

    VIRAMA = "்"

    result = []
    i = 0

    while i < len(text_in):
        ch = text_in[i]

        # Independent vowels
        if ch in VOWELS:
            result.append(VOWELS[ch])
            i += 1
            continue

        # Consonants
        if ch in CONSONANTS:
            cons = CONSONANTS[ch]
            vowel = "a"  # inherent vowel

            if i + 1 < len(text_in):
                nxt = text_in[i + 1]

                # Pulli suppresses vowel
                if nxt == VIRAMA:
                    vowel = ""
                    i += 1

                # Vowel sign overrides inherent vowel
                elif nxt in VOWEL_SIGNS:
                    vowel = VOWEL_SIGNS[nxt]
                    i += 1

            result.append(cons + vowel)
            i += 1
            continue

        # Special signs
        if ch in SIGNS:
            result.append(SIGNS[ch])
            i += 1
            continue

        # Tamil digits
        TAMIL_DIGITS = {
            "௦": "0",
            "௧": "1",
            "௨": "2",
            "௩": "3",
            "௪": "4",
            "௫": "5",
            "௬": "6",
            "௭": "7",
            "௮": "8",
            "௯": "9",
        }

        if ch in TAMIL_DIGITS:
            result.append(TAMIL_DIGITS[ch])
            i += 1
            continue

        # Unknown chars copied directly
        result.append(ch)
        i += 1

    return "".join(result)

def transliterate_gurmukhi(text_in: str) -> str:
    # Independent vowels
    VOWELS = {
        "ਅ": "a",
        "ਆ": "ā",
        "ਇ": "i",
        "ਈ": "ī",
        "ਉ": "u",
        "ਊ": "ū",
        "ਏ": "ē",
        "ਐ": "ai",
        "ਓ": "ō",
        "ਔ": "au",
    }

    # Consonants
    CONSONANTS = {
        "ਕ": "k",
        "ਖ": "kh",
        "ਗ": "g",
        "ਘ": "gh",
        "ਙ": "ṅ",

        "ਚ": "c",
        "ਛ": "ch",
        "ਜ": "j",
        "ਝ": "jh",
        "ਞ": "ñ",

        "ਟ": "ṭ",
        "ਠ": "ṭh",
        "ਡ": "ḍ",
        "ਢ": "ḍh",
        "ਣ": "ṇ",

        "ਤ": "t",
        "ਥ": "th",
        "ਦ": "d",
        "ਧ": "dh",
        "ਨ": "n",

        "ਪ": "p",
        "ਫ": "ph",
        "ਬ": "b",
        "ਭ": "bh",
        "ਮ": "m",

        "ਯ": "y",
        "ਰ": "r",
        "ਲ": "l",
        "ਵ": "v",

        "ਸ਼": "ś",
        "ਸ": "s",
        "ਹ": "h",

        "ੜ": "ṛ",

        # Nukta letters
        "ਖ਼": "x",
        "ਗ਼": "ġ",
        "ਜ਼": "z",
        "ਫ਼": "f",
        "ਲ਼": "ḷ",
    }

    # Dependent vowel signs
    VOWEL_SIGNS = {
        "ਾ": "ā",
        "ਿ": "i",
        "ੀ": "ī",
        "ੁ": "u",
        "ੂ": "ū",
        "ੇ": "ē",
        "ੈ": "ai",
        "ੋ": "ō",
        "ੌ": "au",
    }

    # Special signs
    SIGNS = {
        "ਂ": "ṁ",   # bindi
        "ੰ": "ṁ",   # tippi
        "ਃ": "ḥ",   # visarga
    }

    VIRAMA = "੍"
    ADHAK = "ੱ"

    result = []
    i = 0

    while i < len(text_in):
        ch = text_in[i]

        # Adhak doubles next consonant
        if ch == ADHAK:
            if i + 1 < len(text_in):
                nxt = text_in[i + 1]
                if nxt in CONSONANTS:
                    result.append(CONSONANTS[nxt])
            i += 1
            continue

        # Independent vowels
        if ch in VOWELS:
            result.append(VOWELS[ch])
            i += 1
            continue

        # Consonants
        if ch in CONSONANTS:
            cons = CONSONANTS[ch]
            vowel = "a"  # inherent vowel

            if i + 1 < len(text_in):
                nxt = text_in[i + 1]

                # Virama suppresses vowel
                if nxt == VIRAMA:
                    vowel = ""
                    i += 1

                # Vowel signs override inherent vowel
                elif nxt in VOWEL_SIGNS:
                    vowel = VOWEL_SIGNS[nxt]
                    i += 1

            result.append(cons + vowel)
            i += 1
            continue

        # Signs
        if ch in SIGNS:
            result.append(SIGNS[ch])
            i += 1
            continue

        # Digits
        GURMUKHI_DIGITS = {
            "੦": "0",
            "੧": "1",
            "੨": "2",
            "੩": "3",
            "੪": "4",
            "੫": "5",
            "੬": "6",
            "੭": "7",
            "੮": "8",
            "੯": "9",
        }

        if ch in GURMUKHI_DIGITS:
            result.append(GURMUKHI_DIGITS[ch])
            i += 1
            continue

        # Unknown characters copied directly
        result.append(ch)
        i += 1

    return "".join(result)

def main():
    transliteration_result: str = ""
    user_input: str = ""

    print("Commands:\nab - Arabic\ndv - Devangari (Hindi, Sanskrit)\ngk - Gurmukhi (Punjabi)\ngu - Gujarati\nmm - Malayam\ntm - Tamil\n")
    
    while True:
        user_input = input("")
        

        if user_input.lower() == "q":
            break

        user_input_list: list[str] = user_input.split(" ")

        if len(user_input_list) > 1:
            match user_input_list[0].lower():
                case "ab":
                    transliteration_result = transliterate_arabic(user_input[3:]).title()
                case "gu":
                    transliteration_result = transliterate_gujarati(user_input[3:]).title()
                case "dv":
                    transliteration_result = transliterate_devanagari(user_input[3:]).title()
                case "mm":
                    transliteration_result = transliterate_malayam(user_input[3:]).title()
                case "tm":
                    transliteration_result = transliterate_tamil(user_input[3:]).title()
                case "gk":
                    transliteration_result = transliterate_gurmukhi(user_input[3:]).title()
                case _:
                    pass
            
            pyperclip.copy(transliteration_result)
            print(f"{transliteration_result}\n")

        

if __name__ == "__main__":
    main()
