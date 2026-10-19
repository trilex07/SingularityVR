# Manual reload — PSVR2 / PL + EN

These instructions describe the intended enabled manual-reload mode, not proof
of runtime acceptance. Centurion has prior physical acceptance. Valkyrie is
currently blocked by the live socket witness; Volk and Kasimov share that
path and require runtime validation. Do not fall back to R1 during a manual
reload test: record that as a failure instead.

## Polski

**Centurion:** Naciśnij kółko, aby otworzyć bęben. Unieś lufę, aby wysypać łuski.
Lewym chwytem weź nowy bęben zza lewego barku, włóż go w otwarty bęben broni
i puść chwyt. Machnij pistoletem, aby domknąć bęben. Sprawdź licznik i pierwszy
strzał; przy małym zapasie nie musi pojawić się pełne 6/6.

**Valkyrie:** Naciśnij kółko, aby zwolnić magazynek. Lewym chwytem weź nowy
magazynek zza lewego barku, włóż go w gniazdo broni i puść chwyt.
Nie wykonuj machnięcia domykającego. Sprawdź licznik i możliwość strzału.

**Volk S4:** Naciśnij kółko, aby zwolnić magazynek. Lewym chwytem weź nowy
magazynek zza lewego barku, włóż go w gniazdo broni i puść chwyt.
Ten wariant moda nie wymaga wkładania pojedynczych nabojów ani machnięcia.
Sprawdź licznik i możliwość strzału.

**Kasimov SNV-E99:** Naciśnij kółko, aby zwolnić magazynek. Lewym chwytem weź
nowy magazynek zza lewego barku, włóż go w gniazdo broni i puść chwyt.
Nie wykonuj machnięcia; ten wariant nie wymaga ręcznego ruchu zamkiem.
Sprawdź licznik i możliwość strzału.

## English

**Centurion:** Press Circle to open the cylinder. Tilt the muzzle up to dump
the cases. Use left grip to take a fresh drum from behind your left shoulder,
insert it into the open cylinder and release grip. Flick the pistol to close
the cylinder. Check the ammo counter and first shot; limited reserves may
produce fewer than six rounds.

**Valkyrie:** Press Circle to release the magazine. Use left grip to take a
fresh magazine from behind your left shoulder, insert it into the gun and
release grip. No closing flick. Check the ammo counter and firing.

**Volk S4:** Press Circle to release the magazine. Use left grip to take a
fresh magazine from behind your left shoulder, insert it into the gun and
release grip. This mod variant does not require individual shell insertion
or a closing flick. Check the ammo counter and firing.

**Kasimov SNV-E99:** Press Circle to release the magazine. Use left grip to
take a fresh magazine from behind your left shoulder, insert it into the gun
and release grip. No closing flick or manual bolt operation in this variant.
Check the ammo counter and firing.

## In-game integration

VR Settings Tutorial has a named page for each weapon in both languages.
The short Circle hint selects the same recipe by audited first-person mesh
identity. Unknown weapon identities receive no recipe. Native numbered
`Pad 3` labels are not globally replaced: that label is shared by unrelated
actions and still needs a proven reload-specific consumer.

## Test readiness

All four recipes and the English/Polish prompt tokens pass CPU tests. The
three magazine weapons pass the original-asset offline suite, but their shared
runtime socket is not ready for a physical acceptance test. The last live
Valkyrie session rejected the reconstructed socket as more than 0.75 m from
the right controller before Circle could eject anything. The original bind
translations are approximately 101, 109, and 110 UE units for Valkyrie,
Volk, and Kasimov. The offline roundtrip creates its own compensating local
matrix, so it cannot establish the live UE3 matrix. The next authorized run
must first capture the bounded native bind/root/local/controller rows; then
replay that exact snapshot offline to identify the missing transform term.
