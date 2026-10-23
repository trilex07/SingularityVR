# Physical feedback after MAIN deployment of 05a94ab

User reports right gun/glove alignment acceptable; knife pickup still corrupts presentation. Remaining regression checks reported OK. Do not mark knife fixed from its offline route tests.

New requested scope: unreliable Valkyrie shoulder-ammo pickup and better two-handed accuracy. Existing NeedAmmo code in weapon_reload_prototypes.h requires gripEdge while inside the pouch; arriving with Grip already held does not acquire. This is a concrete candidate explanation, not yet correlated to the user's failed attempts. Preserve knife/support ownership if making acquisition more forgiving.

MAIN archive needed: the completed session from DLL 4B34CE06F93DDB2387D75C5A4152BCD6AADD900ED3DBA9E6459F86A677BE496C, full view_matrix.log plus hash and installed INI snapshot. No additional headset session is needed for this step. The existing main-review handoff still says AWAITING and no matching fresh session archive was found during this follow-up.

Correlate knife reserve/native-joint (including tmdCuff, frame, eye)/standalone receipts per pickup. If the native route is exclusive and corruption persists, the route-overlap hypothesis is falsified as a sufficient explanation; inspect consumed material-0 palette, IB range and the depth-only glove replay. Do not repeat the route patch without that evidence.

For accuracy, distinguish native angular spread/recoil from rendered-barrel/native-ray disagreement. User authorizes reduced native recoil/spread during two-hand support, but no engine field or value has yet been proven for that change. Retain single-handed behavior and actual native damage calls.

No new rendering or gameplay change in this follow-up; no Steam access, launch or deployment. Latest candidate remains unaccepted for knife use.
