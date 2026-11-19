#include "script_recovery.h"

typedef struct SaveBuffer {
    void *vmt;
    int data_size;
    int capacity;
    int position;
    const Byte *data;
} SaveBuffer;

typedef struct {
    const Byte *next;
    const Byte *end;
} Reader;

typedef struct {
    void *vmt;
    void **items;
    int count;
    int capacity;
} NativeList;

typedef struct {
    Byte prefix[0x0c];
    const Wide *filename;
    Byte before_index[8];
    int active_script_index;
} ScriptTemplate;

typedef struct {
    Byte prefix[0x150];
    NativeList *scripts;
} LoadingGalaxy;

typedef struct {
    Byte before_binding[0x3cc];
    void *binding;
    Byte before_script_order[0x7c];
    Byte absolute_script_order;
} LoadedShip;

typedef struct {
    Byte before_marker_list[0x5c];
    void *marker_list;
} ActiveScript;

typedef struct {
    Byte before_kind[0x10];
    int kind;
    Byte before_read[0x2c];
    Byte was_read;
} QuestMessage;

typedef struct {
    Reader ships;
    Reader markers;
    Word32 ship_count;
    Word32 marker_count;
} PendingScript;

enum { key_capacity = 256, max_array_depth = 32, max_pending_scripts = 1024,
       quest_active = 3, quest_cancelled = 5 };

typedef struct {
    Word32 bytes;
    Wide value[key_capacity];
} ScriptKey;

_Static_assert(__builtin_offsetof(SaveBuffer, position) == 0x0c, "TBufEC.Position");
_Static_assert(__builtin_offsetof(NativeList, count) == 8, "TList.Count");
_Static_assert(__builtin_offsetof(ScriptTemplate, active_script_index) == 0x18, "ActiveScriptIndex");
_Static_assert(__builtin_offsetof(LoadingGalaxy, scripts) == 0x150, "TGalaxy.Scripts");

_Static_assert(__builtin_offsetof(LoadedShip, absolute_script_order) == 0x44c, "AbsoluteScriptOrder");
_Static_assert(__builtin_offsetof(QuestMessage, was_read) == 0x40, "Message.WasRead");

typedef void *(DELPHI *FindEntry)(void *, const Wide *);
typedef unsigned short (DELPHI *ReadWord)(SaveBuffer *);
typedef LoadedShip *(DELPHI *FindShip)(const LoadingGalaxy *, Word32, Byte);
typedef LoadedShip *(DELPHI *GetPlayer)(void);
typedef QuestMessage *(DELPHI *FindMessage)(const Wide *, Byte);
typedef int (DELPHI *IndexOfText)(void *, const Wide *);
static const FindEntry find_entry = (FindEntry)0x4d1934;
static const Word32 native_create __attribute__((used)) = 0x7d2ba5;
static const Word32 next_script __attribute__((used)) = 0x7d2c12;
static const ReadWord read_word = (ReadWord)0x7b8254;
static const FindShip find_ship = (FindShip)0x7d7c50;
static const GetPlayer get_player = (GetPlayer)0x747300;
static const FindMessage find_message = (FindMessage)0x7a5a20;
static const IndexOfText index_of_text = (IndexOfText)0x799b88;
typedef void (DELPHI *RunCode)(const void *, void *);
static const RunCode run_code = (RunCode)0x46cb84;
static VisualLog script_log;
static SaveBuffer *recovery_buffer;
static PendingScript pending[max_pending_scripts];
static unsigned int pending_count;
static unsigned int fem_rangers_reported;

void set_script_recovery_log(VisualLog log)
{
    script_log = log;
}

static unsigned int remaining(const Reader *reader)
{
    return (unsigned int)(reader->end - reader->next);
}

static int skip_bytes(Reader *reader, unsigned int count)
{
    if (count > remaining(reader))
        return 0;
    reader->next += count;
    return 1;
}

static int read_number(Reader *reader, unsigned int size, Word32 *out)
{
    const Byte *bytes = reader->next;
    if (!skip_bytes(reader, size))
        return 0;
    Word32 value = 0;
    for (unsigned int i = 0; i < size; ++i)
        value |= (Word32)bytes[i] << (i * 8);
    *out = value;
    return 1;
}

static int skip_string(Reader *reader)
{
    Word32 character;
    while (read_number(reader, 2, &character))
        if (!character)
            return 1;
    return 0;
}

static int path_separator(Word32 character)
{
    return character == '.' || character == '/' || character == '\\';
}

static int read_script_key(Reader *reader, ScriptKey *key)
{
    static const Wide prefix[] = L"Script";
    Word32 character;
    unsigned int length = 0;
    while (length < key_capacity && read_number(reader, 2, &character)) {
        if (!character) {
            key->value[length] = 0;
            key->bytes = length * sizeof(Wide);
            return length > 7;
        }
        if (character < 0x20 || character == '?' ||
            (length < 6 && character != prefix[length]) ||
            (length == 6 && !path_separator(character)))
            return 0;
        key->value[length++] = (Wide)character;
    }
    return 0;
}

static int skip_variables(Reader *reader, unsigned int depth)
{
    Word32 count;
    // The two local variable lists use a word count, TVarArrayEC uses an Int32.
    if (depth > max_array_depth || !read_number(reader, depth ? 4 : 2, &count) ||
        count > remaining(reader) / 3)
        return 0;
    for (Word32 i = 0; i < count; ++i) {
        Word32 kind;
        if (!skip_string(reader) || !read_number(reader, 1, &kind))
            return 0;
        switch (kind) {
        case 0: // Empty
            break;
        case 1: // Int32
        case 2: // UInt32
            if (!skip_bytes(reader, 4))
                return 0;
            break;
        case 3: // Double
            if (!skip_bytes(reader, 8))
                return 0;
            break;
        case 4: // WideString
            if (!skip_string(reader))
                return 0;
            break;
        case 6: // A library binding has a name in script locals only.
            if (!depth && !skip_string(reader))
                return 0;
            break;
        case 9: // Array
            if (!skip_variables(reader, depth + 1))
                return 0;
            break;
        case 5: // These kinds have no payload in TVarEC.
        case 7:
        case 8:
        case 10:
            if (!depth)
                return 0;
            break;
        default:
            return 0;
        }
    }
    return 1;
}

static int skip_ether(Reader *reader)
{
    Word32 count;
    if (!read_number(reader, 4, &count) || count > remaining(reader) / 6)
        return 0;
    for (Word32 i = 0; i < count; ++i)
        if (!skip_string(reader) || !skip_bytes(reader, 4))
            return 0;
    return 1;
}

static int skip_map_aliases(Reader *reader)
{
    Word32 stars;
    if (!read_number(reader, 4, &stars) || stars > remaining(reader) / 14)
        return 0;
    for (Word32 i = 0; i < stars; ++i) {
        Word32 planets, reserved;
        if (!skip_string(reader) || !skip_bytes(reader, 4) ||
            !read_number(reader, 4, &planets) || planets > remaining(reader) / 6)
            return 0;
        for (Word32 j = 0; j < planets; ++j)
            if (!skip_string(reader) || !skip_bytes(reader, 4))
                return 0;
        if (!read_number(reader, 4, &reserved) || reserved)
            return 0;
    }
    return 1;
}

static int valid_list(const NativeList *list)
{
    return list && list->count >= 0 && list->capacity >= list->count &&
           (!list->count || list->items);
}

static int skip_items(Reader *reader)
{
    Word32 count;
    if (!read_number(reader, 4, &count) || count > remaining(reader) / 29)
        return 0;
    for (Word32 i = 0; i < count; ++i) {
        Word32 can_sell;
        if (!skip_string(reader) || !read_number(reader, 1, &can_sell) ||
            can_sell > 1 || !skip_bytes(reader, 12))
            return 0;
        // TextData[1..3], OnUseText and OnActionText.
        for (int field = 0; field < 5; ++field)
            if (!skip_string(reader))
                return 0;
        if (!skip_bytes(reader, 4)) // Item ID, which stays in the galaxy.
            return 0;
    }
    return 1;
}

static int read_ship_binding(Reader *reader, Word32 *ship)
{
    Word32 hit, hit_player;
    return skip_bytes(reader, 4) && read_number(reader, 4, ship) &&
           skip_bytes(reader, 20) && skip_string(reader) &&
           read_number(reader, 1, &hit) && hit <= 1 &&
           read_number(reader, 1, &hit_player) && hit_player <= 1;
}

static int read_script_state(Reader *reader, PendingScript *record)
{
    if (!skip_ether(reader) || !skip_variables(reader, 0) ||
        !skip_variables(reader, 0) || !skip_map_aliases(reader) || !skip_items(reader) ||
        !read_number(reader, 2, &record->ship_count) ||
        record->ship_count > remaining(reader) / 32)
        return 0;
    record->ships.next = reader->next;
    for (Word32 i = 0; i < record->ship_count; ++i) {
        Word32 id;
        if (!read_ship_binding(reader, &id))
            return 0;
    }
    record->ships.end = reader->next;
    if (!read_number(reader, 2, &record->marker_count) ||
        record->marker_count > remaining(reader) / 2)
        return 0;
    record->markers.next = reader->next;
    for (Word32 i = 0; i < record->marker_count; ++i)
        if (!skip_string(reader))
            return 0;
    record->markers.end = reader->next;
    return remaining(reader) >= 2; // Following script or native list count.
}

unsigned short DELPHI start_script_recovery(SaveBuffer *buffer)
{
    pending_count = 0;
    __atomic_store_n(&fem_rangers_reported, 0, __ATOMIC_RELAXED);
    recovery_buffer = buffer == *(void *volatile *)0x81cc70 ? buffer : 0;
    return read_word(buffer);
}

static int player_has_binding(const LoadedShip *player)
{
    const NativeList *bindings = *(NativeList *const *)((const Byte *)player + 0x58c);
    return !valid_list(bindings) || bindings->count != 0;
}

static void release_script_order(const LoadingGalaxy *galaxy, LoadedShip *player, Word32 id)
{
    if (!id)
        return;
    LoadedShip *ship = find_ship(galaxy, id, 0);
    if (!ship || !ship->absolute_script_order || ship->binding ||
        (ship == player && player_has_binding(player)))
        return;
    ship->absolute_script_order = 0;
}

static int read_marker(Reader *reader, ScriptKey *key)
{
    Word32 character;
    unsigned int count = 0;
    while (read_number(reader, 2, &character)) {
        if (!character) {
            if (!count || count >= key_capacity)
                return 0;
            key->value[count] = 0;
            key->bytes = count * sizeof(Wide);
            return 1;
        }
        if (count < key_capacity)
            key->value[count] = (Wide)character;
        ++count;
    }
    return 0;
}

static int marker_used_by_active_script(const NativeList *scripts, const Wide *key)
{
    for (int i = 0; i < scripts->count; ++i) {
        const ActiveScript *script = scripts->items[i];
        if (script && script->marker_list && index_of_text(script->marker_list, key) >= 0)
            return 1;
    }
    return 0;
}

static void cancel_script_marker(const NativeList *scripts, const Wide *key)
{
    QuestMessage *message = find_message(key, 0);
    if (!message || message->kind != quest_active || marker_used_by_active_script(scripts, key))
        return;
    message->kind = quest_cancelled;
    message->was_read = 0;
}

static void DELPHI __attribute__((used)) finish_script_recovery(
    const LoadingGalaxy *galaxy, SaveBuffer *buffer)
{
    if (!buffer || buffer != recovery_buffer)
        return;
    recovery_buffer = 0;
    if (!pending_count || !galaxy || !valid_list(galaxy->scripts))
        return;
    LoadedShip *player = get_player();
    for (unsigned int i = 0; i < pending_count; ++i) {
        PendingScript *record = &pending[i];
        Reader ships = record->ships;
        for (Word32 j = 0; j < record->ship_count; ++j) {
            Word32 id;
            if (read_ship_binding(&ships, &id))
                release_script_order(galaxy, player, id);
        }
        Reader markers = record->markers;
        for (Word32 j = 0; j < record->marker_count; ++j) {
            ScriptKey key;
            if (read_marker(&markers, &key))
                cancel_script_marker(galaxy->scripts, key.value);
        }
    }
    pending_count = 0;
}

void __attribute__((naked)) finish_script_recovery_entry(void)
{
    __asm__ volatile("mov %esi, %eax\n mov %edi, %edx\n jmp finish_script_recovery");
}

static int same_path(const Wide *left, const Wide *right)
{
    if (!left || !right)
        return 0;
    while (*left && (*left == *right || (path_separator(*left) && path_separator(*right)))) {
        ++left;
        ++right;
    }
    return *left == *right;
}

static Wide mod_path_character(Wide character)
{
    if (character >= 'A' && character <= 'Z')
        return (Wide)(character + ('a' - 'A'));
    return character == '/' ? '\\' : character;
}

static int has_fem_rangers_mod(const Wide *list)
{
    static const Wide name[] = L"evolution\\evofemrangers";
    while (list && *list) {
        while (*list == ' ' || *list == '\t' || *list == ',')
            ++list;
        const Wide *word = list;
        while (*list && *list != ',')
            ++list;
        const Wide *end = list;
        while (end > word && (end[-1] == ' ' || end[-1] == '\t'))
            --end;
        unsigned int i = 0;
        while (word < end && name[i] && mod_path_character(*word) == name[i]) {
            ++word;
            ++i;
        }
        if (word == end && !name[i])
            return 1;
    }
    return 0;
}

static int affected_fem_rangers_turn(const void *code)
{
    const Byte *script = *(const Byte *volatile *)0x81b848;
    return code && script && *(const void *const *)(script + 0x38) == code &&
           same_path(*(const Wide *const *)(script + 0x08), L"Script.PC_fem_rangers") &&
           !has_fem_rangers_mod(*(const Wide *volatile *)0x81cfc8);
}

void DELPHI guarded_script_turn_code(const void *code, void *process)
{
    if (affected_fem_rangers_turn(code) &&
        has_fem_rangers_mod(*(const Wide *volatile *)0x81cfd0)) {
        if (script_log && !__atomic_exchange_n(&fem_rangers_reported, 1, __ATOMIC_RELAXED))
            script_log(L"FHD_FastCore_Error",
                       L"Save recovery: skipping PC_fem_rangers turn code after removal of EvoFemRangers");
        return;
    }
    run_code(code, process);
}

static int is_registered(const NativeList *templates, const ScriptKey *key)
{
    for (int i = 0; i < templates->count; ++i) {
        const ScriptTemplate *entry = templates->items[i];
        if (!entry || same_path(entry->filename, key->value))
            return 1;
    }
    return 0;
}

static void log_script(const ScriptKey *key, const Wide *status)
{
    if (!script_log)
        return;
    Wide message[key_capacity + 90];
    unsigned int at = 0;
    while (*status)
        message[at++] = *status++;
    for (unsigned int i = 0; i <= key->bytes / sizeof(Wide); ++i)
        message[at + i] = key->value[i];
    script_log(L"FHD_FastCore_Error", message);
}

static int DELPHI __attribute__((used)) recover_missing_script(SaveBuffer *buffer,
                                                              const LoadingGalaxy *galaxy)
{
    if (!buffer || buffer != recovery_buffer || !galaxy)
        return 0;
    const int version = **(const int **)0x813580;
    if (version < 162 || version > 167 || !buffer->data || buffer->position < 0 ||
        buffer->data_size > buffer->capacity || buffer->position >= buffer->data_size)
        return 0;
    void *root = *(void *volatile *)0x81cfe4;
    NativeList *templates = *(NativeList *volatile *)0x81cc40;
    if (!root || !valid_list(templates) || !valid_list(galaxy->scripts))
        return 0;

    Reader reader = {buffer->data + buffer->position, buffer->data + buffer->data_size};
    ScriptKey key;
    if (!read_script_key(&reader, &key) || find_entry(root, key.value))
        return 0;
    if (is_registered(templates, &key)) {
        log_script(&key, L"Save recovery: missing file for installed script ");
        return 0;
    }
    PendingScript record;
    if (!read_script_state(&reader, &record)) {
        log_script(&key, L"Save recovery: unreadable or unsupported script record ");
        return 0;
    }
    if (record.ship_count || record.marker_count) {
        if (pending_count == max_pending_scripts) {
            log_script(&key, L"Save recovery: too many removed script bindings ");
            return 0;
        }
        pending[pending_count++] = record;
    }

    // The native loader has not created this TScript yet. Rebase saved template
    // indices exactly as the engine does when it removes a script during a turn.
    const int removed_index = galaxy->scripts->count;
    for (int i = 0; i < templates->count; ++i) {
        ScriptTemplate *entry = templates->items[i];
        if (entry->active_script_index == removed_index)
            entry->active_script_index = -1;
        else if (entry->active_script_index > removed_index)
            --entry->active_script_index;
    }
    buffer->position = (int)(reader.next - buffer->data);
    log_script(&key, L"Save recovery: skipped missing script ");
    return 1;
}

void __attribute__((naked)) missing_script_entry(void)
{
    __asm__ volatile(
        "mov %edi, %eax\n"
        "mov %esi, %edx\n"
        "call recover_missing_script\n"
        "test %eax, %eax\n"
        "jnz 1f\n"
        "mov $1, %dl\n"
        "mov 0x645de4, %eax\n"
        "jmp *native_create\n"
        "1: jmp *next_script\n"
    );
}
