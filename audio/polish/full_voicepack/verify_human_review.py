import whisper

model = whisper.load_model('small')

files = [
    ('0011_43166fba7ce1.wav', 'Wynik remisowy po regulaminowym czasie. Dogrywka do nagłej śmierci!!!'),
    ('0018_1e4d181c0a7a.wav', 'Wyeliminuj wrogi gatunek. Wygrywa drużyna z największą liczbą zabójstw.'),
    ('0102_da8e7a134316.wav', 'Wymagany Kod Aktywacyjny Do Naładowania Bomby E 99'),
    ('0171_e58a6aacfe65.wav', 'Naciśnij klucz wiek, aby rozstroić Zeki w czasie. To kosztuje energię E 99.'),
    ('0172_0005fcf47156.wav', 'Naciśnij klucz wiek, aby postarzyć Rewertów, przyspieszyć ich rozpad i doprowadzić do wybuchu. To kosztuje energię E 99.'),
    ('0174_e63b02880451.wav', 'Postarzanie klucz wiek lub impulsowanie klucz impuls fazowych kleszczy to bardzo skuteczna strategia.'),
    ('0175_a8ea3b99ffff.wav', 'Naciśnij klucz wiek, aby rozstroić Zeki w czasie. To kosztuje energię E 99.'),
    ('0176_8775dc4ee866.wav', 'Podnieś dodatkową amunicję, przechodząc po upuszczonej broni lub pakach z amunicją.'),
    ('0177_498900210f5f.wav', 'Naciśnij klucz wiek, aby odnowić skrzynie z amunicją. W środku znajdziesz duży zapas amunicji do wszystkich broni.'),
    ('0178_674ff92ad51e.wav', 'Naciśnij klucz wiek, aby odnowić apteczki i odzyskać znajdujące się w nich pakiety zdrowia.'),
    ('0180_646e1432951a.wav', 'Użyj zbiorników kriogenicznych, aby zamrozić wrogów na krótki czas.'),
    ('0181_6f088a2dbaa7.wav', 'Naciśnij i przytrzymaj klucz chwyt, aby naładować dedlok i spowolnić czas wokół wrogów.'),
    ('0182_e2c3a7f502e0.wav', 'Grawitacyjny chwyt klucz chwyt pozwala łapać beczki rzucone w twoją stronę.'),
    ('0186_c53fea8fb9e2.wav', 'Gdy wrogowie są poza fazą, są niewrażliwi na obrażenia.'),
    ('0190_0fbab60ac913.wav', 'Przejdź po upuszczonej broni, aby zdobyć do niej amunicję.'),
]

with open('human_review_list.txt', 'w', encoding='utf-8') as out:
    for f, expected in files:
        result = model.transcribe(f, language='pl', fp16=False)
        lang = result.get('language')
        text = result.get('text')
        out.write(f"FILE: {f}\n")
        out.write(f"EXPECTED: {expected}\n")
        out.write(f"ASR: {text}\n")
        out.write(f"LANG: {lang}\n")
        out.write("\n")

print("Done - check human_review_list.txt")