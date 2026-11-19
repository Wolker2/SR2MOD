# Сверка и публикация модов — 19.11.2026

Сфера: все доступные репозитории Wolker2, текущая папка Google Drive с модами и сохранённые архивы ChatGPT Library. Старые версии не удалялись.

## Опубликованы новые установочные комплекты (11)

| Мод | Новая папка в GitHub | Проверка |
|---|---|---|
| FHD_FastCore_Error 0.2.15 | [SR2MOD/0.2.15](../1FHD_FastCore_Error_0.2.15_Nexus-GitHub) | [Actions](https://github.com/Wolker2/SR2MOD/actions/runs/44608979364) |
| FHD_FindQuest 0.5.8 | [Репозиторий](https://github.com/Wolker2/FHD_FindQuest/tree/main/1FHD_FindQuest_0.5.8_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_FindQuest/actions/runs/44609189086) |
| FHD_Navigator 0.2.8 | [Репозиторий](https://github.com/Wolker2/FHD_Navigator/tree/main/1FHD_Navigator_0.2.8_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_Navigator/actions/runs/44609176864) |
| FHD_QuestMarker_Preview 0.3.15 | [Репозиторий](https://github.com/Wolker2/FHD_QuestMarker_Preview/tree/main/1FHD_QuestMarker_Preview_0.3.15_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_QuestMarker_Preview/actions/runs/44609476660) |
| FHD_LeftStats v1 — Google Drive | [Репозиторий](https://github.com/Wolker2/FHD_LeftStats/tree/main/1FHD_LeftStats_v1_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_LeftStats/actions/runs/44609485744) |
| FHD_Autopilot 2.16.1 — Google Drive | [Репозиторий](https://github.com/Wolker2/FHD_Autopilot/tree/main/1FHD_Autopilot_v2.16.1_DRIVE_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_Autopilot/actions/runs/44610782593) |
| FHD_Trade v2.0 — Google Drive | [Репозиторий](https://github.com/Wolker2/FHD_Trade/tree/main/1FHD_Trade_v2.0_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_Trade/actions/runs/44615153241) |
| FHD_RealClock v1.3 — полный пакет Drive | [Репозиторий](https://github.com/Wolker2/FHD_RealClock/tree/main/1FHD_RealClock_v1.3_D3D9_DRIVE_FULL_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_RealClock/actions/runs/44610924738) |
| FHD_OnlineChat 0.22 FIX23 — полный пакет Drive | [Репозиторий](https://github.com/Wolker2/FHD_OnlineChat/tree/main/1FHD_OnlineChat_0.22_FIX23_DRIVE_FULL_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_OnlineChat/actions/runs/44610932944) |
| FHD_GameScale2 0.5.7.31 — Google Drive | [Репозиторий](https://github.com/Wolker2/FHD_GameScale2/tree/main/1FHD_GameScale2_0.5.7.31_DRIVE_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_GameScale2/actions/runs/44611431113) |
| FHD_QuickQuest 1.2.5-fix24 (140 QMM) — Google Drive | [Репозиторий](https://github.com/Wolker2/FHD_QuickQuest/tree/main/1FHD_QuickQuest_v1.2.5-fix24_DRIVE_Nexus-GitHub) | [Actions](https://github.com/Wolker2/FHD_QuickQuest/actions/runs/44612436686) |

Оригинальные архивы сохранены побайтно. Для версий 0.5.7.31 и fix24 большие установочные ZIP передавались частями, затем в GitHub Actions собирались в оригинальные файлы с проверкой SHA-256 и удалением временных частей. Исходные текстовые и бинарные игровые ресурсы экспортированы в SOURCE; там, где исходный архив содержал только DLL, C-код DLL не восстанавливался. Работа модов в игре не тестировалась.

### Особые замечания

- **Trade v2.0:** все 56 оригинальных GI сохранены, 53 изображения преобразованы в PNG. Три изображения имеют GI frame type 1, для которого используемый конвертер не поддерживает полноценный PNG-экспорт.
- **RealClock (полный Drive):** два DAT имеют нестандартный для используемого конвертера формат; они сохранены оригинальными бинарными файлами. Этот комплект включает дополнительные загрузчики XenoNative, поэтому не заменяет компактный v1.3.
- **OnlineChat (полный Drive):** содержит дополнительные XenoNative-компоненты и не заменяет компактную версию с тем же названием.

## Крупные файлы: добавлены прямые ссылки на Google Drive (7)

- [FHD_Nanotech](https://github.com/Wolker2/FHD_Nanotech/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — V28.5 RELEASE, 101,7 МБ
- [FHD_Animation](https://github.com/Wolker2/FHD_Animation/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — 1,32 ГБ
- [FHD_SpaceBG](https://github.com/Wolker2/FHD_SpaceBG/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — 371,2 МБ
- [FHD_FON](https://github.com/Wolker2/FHD_FON/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — 810,9 МБ
- [FHD_Hangar](https://github.com/Wolker2/FHD_Hangar/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — 37,3 МБ
- [FHD_LOTOStations](https://github.com/Wolker2/FHD_LOTOStations/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — 5,4 МБ
- [FHD_BaseBuilder_Paid](https://github.com/Wolker2/FHD_BaseBuilder_Paid/blob/main/FULL_INSTALL_GOOGLE_DRIVE.md) — 1,25 МБ

Контрольные суммы больших файлов не пересчитывались, Google Drive-доступ может требовать разрешений. Исходники и метаданные остаются в GitHub, гигантские игровые ресурсы — на Drive.

## Осталось получить точные оригиналы

1. **FHD_NPC_AI v2.2.4 RADIO_BOUNTY_MARKUP_FIX_TEST** — приоритетное исключение среди отсутствующих проектов. Архив найден в Library, но невозможно получить оригинальные байты через доступный контейнерный путь. Отдельный репозиторий Wolker2/FHD_NPC_AI ещё не создан.
2. **GameScale2 0.5.7.34**, **LeftStats Fix 3 SKILL_BARS_MIRRORED**, **Autopilot 2.16.1 HOLOMAP_RENDER_FIX**, **Artifacts 0.8.24.3**, **QuickQuest fix24 SHU_JOHN_DOE** — более новые/специальные сборки имеются в Library, но не в доступных для прямого переноса исходных Drive-файлах; опубликованные версии не считаются заменой им.
3. **FHD_BIP** — полный 128,8 МБ архив есть в Library, но актуальная прямая ссылка Drive не получена; GitHub содержит облегчённые ресурсы.
4. **FHD_Nanogenesis 0.9** — полный пакет 159,3 МБ найден в Library, но актуальная прямая ссылка Drive не получена.
5. **Animation** — отдельные правки Government/PirateClan из библиотеки от 18 ноября не включены в первоначальный архив на Google Drive.

Все прочие отсутствующие экспериментальные проекты сознательно **пропущены** по указанию пользователя.

Ничего не удаляли из репозиториев и не изменяли в исходном каталоге Google Drive.
