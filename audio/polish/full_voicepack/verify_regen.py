import whisper

model = whisper.load_model('small')

files = [
    ('0011_43166fba7ce1.wav', 'Wynik remisowy po regulaminowym czasie. Dogrywka do nagłej śmierci!'),
    ('0018_1e4d181c0a7a.wav', 'Wyeliminuj wrogi gatunek. Wygrywa drużyna z największą liczbą zabójstw.'),
    ('0102_da8e7a134316.wav', 'Kompleks przetwarzania E 99, Katorga-12 27 października 2010 roku.'),
    ('0171_e58a6aacfe65.wav', 'Naciśnij [impulse] i [age] jednocześnie, aby włączyć spowolnienie czasu.'),
    ('0507_4ea3eb8014b5.wav', 'Naciśnij [flashlight], aby włączyć funkcję impulsu TMD. Odkryte ślady pomogą ci przejść przez Katorgę-12.'),
    ('0712_b5032f02297c.wav', 'Naciśnij [jump], aby wskoczyć na powaloną belkę.'),
]

for f, expected in files:
    result = model.transcribe(f, language='pl', fp16=False)
    lang = result.get('language')
    text = result.get('text')
    print(f + ': [' + lang + '] ' + text)
    print('  Expected: ' + expected)
    print()