import asyncio, edge_tts

OUT = 'C:/Users/Trilex/vr-mods/SingularityVR-cutscene/audio/polish/cast_legal_20261014/casting_final/role_lektor/EDGE_fallback'

JOBS = [
    ('EDGE_6ba2_marek.mp3', 'pl-PL-MarekNeural',
     'Podobno doktor Barisow może zostać nowym szefem Katorgi dwanaście, ale podobno Moskwa nie ma do niego wielkiego zaufania. Gdyby tylko doktor Demiczew nie zginął w tym pożarze.'),
    ('EDGE_db5a_zofia.mp3', 'pl-PL-ZofiaNeural',
     'Ile jeszcze wytrzymam? Singularność wybuchła dwa dni temu i od tamtej pory nie odpoczywałam. Kilku medyków zdołało otworzyć punkt triażu w tych tunelach, ale ilu byśmy nie opatrzyli, przybywa jeszcze więcej chorych i rannych. Ta wyspa mnie wykończy.'),
    ('EDGE_825b_marek.mp3', 'pl-PL-MarekNeural',
     'Pomocy! Syreny! A potem nastąpił wybuch, pani Barisow krzyknęła i wybiegła z klasy. Zostawiła nas tam.'),
    ('EDGE_825b_zofia.mp3', 'pl-PL-ZofiaNeural',
     'Pomocy! Syreny! A potem nastąpił wybuch, pani Barisow krzyknęła i wybiegła z klasy. Zostawiła nas tam.'),
]

async def main():
    import os
    os.makedirs(OUT, exist_ok=True)
    for name, voice, text in JOBS:
        c = edge_tts.Communicate(text, voice)
        await c.save(OUT + '/' + name)
        print('saved', name, flush=True)

asyncio.run(main())
print('DONE ->', OUT)
