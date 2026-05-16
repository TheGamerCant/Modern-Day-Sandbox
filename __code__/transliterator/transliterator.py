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

def main():
    while True:
        user_input: str = input("")

        if user_input.lower() == "q":
            break

        user_input_list: list[str] = user_input.split(" ")

        if len(user_input_list) > 1:
            if  user_input_list[0].lower() == "ab":
                print(transliterate_arabic(user_input[3:]).title())
                
            elif  user_input_list[0].lower() == "gu":
                print(transliterate_gujarati(user_input[3:]).title())

            elif user_input_list[0].lower() == "dv":
                print(transliterate_devanagari(user_input[3:]).title())

            elif user_input_list[0].lower() == "mm":
                print(transliterate_malayam(user_input[3:]).title())

if __name__ == "__main__":
    main()
