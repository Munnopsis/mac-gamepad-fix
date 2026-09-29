#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>

typedef struct Il2CppDomain Il2CppDomain;
typedef struct Il2CppAssembly Il2CppAssembly;
typedef struct Il2CppImage Il2CppImage;
typedef struct Il2CppClass Il2CppClass;
typedef struct Il2CppObject Il2CppObject;
typedef struct Il2CppString Il2CppString;
typedef struct Il2CppType Il2CppType;
typedef struct MethodInfo MethodInfo;
typedef struct PropertyInfo PropertyInfo;

static void *ga;
static FILE *out;

static void *S(const char *n)
{
    return ga ? dlsym(ga, n) : NULL;
}

static void print_string(Il2CppString *s,
                         const uint16_t *(*chars)(Il2CppString *),
                         int32_t (*length)(Il2CppString *))
{
    if (!s) {
        fprintf(out, "<null>");
        return;
    }

    const uint16_t *p = chars(s);
    int32_t n = length(s);

    for (int32_t i = 0; i < n; i++)
        fputc(p[i] < 128 ? (char)p[i] : '?', out);
}

int cm_phase1_done = 0;

void cm_phase1(void)
{
    out = fopen("/tmp/cm-gamepad-phase1.log", "w");
    if (!out)
        return;

    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char *n = _dyld_get_image_name(i);
        if (n && strstr(n, "/GameAssembly.dylib")) {
            ga = dlopen(n, RTLD_NOW | RTLD_NOLOAD);
            if (!ga)
                ga = dlopen(n, RTLD_NOW);
            break;
        }
    }

    Il2CppDomain *(*domain_get)(void) = S("il2cpp_domain_get");
    const Il2CppAssembly **(*get_assemblies)(const Il2CppDomain *, size_t *) = S("il2cpp_domain_get_assemblies");
    const Il2CppImage *(*get_image)(const Il2CppAssembly *) = S("il2cpp_assembly_get_image");
    Il2CppClass *(*class_from_name)(const Il2CppImage *, const char *, const char *) = S("il2cpp_class_from_name");
    const MethodInfo *(*get_methods)(Il2CppClass *, void **) = S("il2cpp_class_get_methods");
    const char *(*method_name)(const MethodInfo *) = S("il2cpp_method_get_name");
    uint32_t (*param_count)(const MethodInfo *) = S("il2cpp_method_get_param_count");
    const Il2CppType *(*get_param)(const MethodInfo *, uint32_t) = S("il2cpp_method_get_param");
    char *(*type_name)(const Il2CppType *) = S("il2cpp_type_get_name");
    const PropertyInfo *(*get_prop)(Il2CppClass *, const char *) = S("il2cpp_class_get_property_from_name");
    const MethodInfo *(*prop_getter)(const PropertyInfo *) = S("il2cpp_property_get_get_method");
    Il2CppClass *(*get_parent)(Il2CppClass *) = S("il2cpp_class_get_parent");
    Il2CppClass *(*object_class)(Il2CppObject *) = S("il2cpp_object_get_class");
    void *(*unbox)(Il2CppObject *) = S("il2cpp_object_unbox");
    Il2CppString *(*string_new)(const char *) = S("il2cpp_string_new");
    Il2CppObject *(*invoke)(const MethodInfo *, void *, void **, Il2CppObject **) = S("il2cpp_runtime_invoke");
    const uint16_t *(*string_chars)(Il2CppString *) = S("il2cpp_string_chars");
    int32_t (*string_length)(Il2CppString *) = S("il2cpp_string_length");

    Il2CppDomain *domain = domain_get();
    size_t count = 0;
    const Il2CppAssembly **assemblies = get_assemblies(domain, &count);

    Il2CppClass *inputSystem = NULL;
    Il2CppClass *joystick = NULL;
    Il2CppClass *matcherClass = NULL;

    for (size_t i = 0; i < count; i++) {
        const Il2CppImage *img = get_image(assemblies[i]);
        if (!img)
            continue;

        if (!inputSystem)
            inputSystem = class_from_name(img, "UnityEngine.InputSystem", "InputSystem");
        if (!joystick)
            joystick = class_from_name(img, "UnityEngine.InputSystem", "Joystick");
        if (!matcherClass)
            matcherClass = class_from_name(img, "UnityEngine.InputSystem.Layouts", "InputDeviceMatcher");

        if (inputSystem && joystick && matcherClass)
            break;
    }

    Il2CppClass *k = joystick;
    const PropertyInfo *currentProp = NULL;

    while (k && !currentProp) {
        currentProp = get_prop(k, "current");
        k = get_parent(k);
    }

    Il2CppObject *exc = NULL;
    Il2CppObject *device = invoke(prop_getter(currentProp), NULL, NULL, &exc);

    if (!device || exc) {
        fprintf(out, "Joystick.current unavailable\n");
        fclose(out);
        return;
    }

    k = object_class(device);
    const PropertyInfo *descriptionProp = NULL;

    while (k && !descriptionProp) {
        descriptionProp = get_prop(k, "description");
        k = get_parent(k);
    }

    exc = NULL;
    Il2CppObject *description =
        invoke(prop_getter(descriptionProp), device, NULL, &exc);

    if (!description || exc) {
        fprintf(out, "description unavailable\n");
        fclose(out);
        return;
    }

    const MethodInfo *fromDescription = NULL;
    void *iter = NULL;
    const MethodInfo *m;

    while ((m = get_methods(matcherClass, &iter))) {
        if (strcmp(method_name(m), "FromDeviceDescription") == 0 &&
            param_count(m) == 1) {
            fromDescription = m;
            break;
        }
    }

    void *descArgs[1] = { unbox(description) };
    exc = NULL;
    Il2CppObject *matcher =
        invoke(fromDescription, NULL, descArgs, &exc);

    if (!matcher || exc) {
        fprintf(out, "FromDeviceDescription failed\n");
        fclose(out);
        return;
    }

    const char *json =
        "{\"name\":\"HID::Microsoft GamePad-1\",\"extend\":\"Gamepad\",\"format\":\"HID \",\"displayName\":\"Xbox 360 Controller (macOS HID)\",\"controls\":[{\"name\":\"dpad\",\"layout\":\"Dpad\",\"offset\":2,\"bit\":0,\"sizeInBits\":4,\"format\":\"BIT \"},{\"name\":\"dpad/up\",\"layout\":\"Button\",\"offset\":0,\"bit\":0,\"sizeInBits\":1,\"format\":\"BIT \"},{\"name\":\"dpad/down\",\"layout\":\"Button\",\"offset\":0,\"bit\":1,\"sizeInBits\":1,\"format\":\"BIT \"},{\"name\":\"dpad/left\",\"layout\":\"Button\",\"offset\":0,\"bit\":2,\"sizeInBits\":1,\"format\":\"BIT \"},{\"name\":\"dpad/right\",\"layout\":\"Button\",\"offset\":0,\"bit\":3,\"sizeInBits\":1,\"format\":\"BIT \"},{\"name\":\"start\",\"layout\":\"Button\",\"offset\":2,\"bit\":4,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"START\"},{\"name\":\"select\",\"layout\":\"Button\",\"offset\":2,\"bit\":5,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"BACK\"},{\"name\":\"leftStickPress\",\"layout\":\"Button\",\"offset\":2,\"bit\":6,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"Left Stick Press\"},{\"name\":\"rightStickPress\",\"layout\":\"Button\",\"offset\":2,\"bit\":7,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"Right Stick Press\"},{\"name\":\"leftShoulder\",\"layout\":\"Button\",\"offset\":3,\"bit\":0,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"LB\"},{\"name\":\"rightShoulder\",\"layout\":\"Button\",\"offset\":3,\"bit\":1,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"RB\"},{\"name\":\"xbox\",\"layout\":\"Button\",\"offset\":3,\"bit\":2,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"Xbox\"},{\"name\":\"buttonSouth\",\"layout\":\"Button\",\"offset\":3,\"bit\":4,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"A\"},{\"name\":\"buttonEast\",\"layout\":\"Button\",\"offset\":3,\"bit\":5,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"B\"},{\"name\":\"buttonWest\",\"layout\":\"Button\",\"offset\":3,\"bit\":6,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"X\"},{\"name\":\"buttonNorth\",\"layout\":\"Button\",\"offset\":3,\"bit\":7,\"sizeInBits\":1,\"format\":\"BIT \",\"displayName\":\"Y\"},{\"name\":\"leftTrigger\",\"layout\":\"Button\",\"offset\":4,\"sizeInBits\":8,\"format\":\"BYTE\",\"displayName\":\"LT\"},{\"name\":\"rightTrigger\",\"layout\":\"Button\",\"offset\":5,\"sizeInBits\":8,\"format\":\"BYTE\",\"displayName\":\"RT\"},{\"name\":\"leftStick\",\"layout\":\"Stick\",\"offset\":6,\"sizeInBits\":32,\"format\":\"VC2S\",\"processors\":\"stickDeadzone,invertVector2(invertX=false,invertY=true)\"},{\"name\":\"leftStick/x\",\"layout\":\"Axis\",\"offset\":0,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=1\"},{\"name\":\"leftStick/left\",\"layout\":\"Button\",\"offset\":0,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=0,invert=true\"},{\"name\":\"leftStick/right\",\"layout\":\"Button\",\"offset\":0,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=0,clampMax=1,invert=false\"},{\"name\":\"leftStick/y\",\"layout\":\"Axis\",\"offset\":2,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=1\",\"processors\":\"invert\"},{\"name\":\"leftStick/up\",\"layout\":\"Button\",\"offset\":2,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=0,invert=true\"},{\"name\":\"leftStick/down\",\"layout\":\"Button\",\"offset\":2,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=0,clampMax=1,invert=false\"},{\"name\":\"rightStick\",\"layout\":\"Stick\",\"offset\":10,\"sizeInBits\":32,\"format\":\"VC2S\",\"processors\":\"stickDeadzone,invertVector2(invertX=false,invertY=true)\"},{\"name\":\"rightStick/x\",\"layout\":\"Axis\",\"offset\":0,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=1\"},{\"name\":\"rightStick/left\",\"layout\":\"Button\",\"offset\":0,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=0,invert=true\"},{\"name\":\"rightStick/right\",\"layout\":\"Button\",\"offset\":0,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=0,clampMax=1,invert=false\"},{\"name\":\"rightStick/y\",\"layout\":\"Axis\",\"offset\":2,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=1\",\"processors\":\"invert\"},{\"name\":\"rightStick/up\",\"layout\":\"Button\",\"offset\":2,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=-1,clampMax=0,invert=true\"},{\"name\":\"rightStick/down\",\"layout\":\"Button\",\"offset\":2,\"sizeInBits\":16,\"format\":\"SHRT\",\"parameters\":\"normalize=true,normalizeMin=-1,normalizeMax=1,normalizeZero=0,clamp=true,clampMin=0,clampMax=1,invert=false\"}]}";

    const MethodInfo *registerLayout = NULL;
    const MethodInfo *registerMatcher = NULL;
    const MethodInfo *tryMatch = NULL;

    iter = NULL;

    while ((m = get_methods(inputSystem, &iter))) {
        const char *n = method_name(m);
        uint32_t pc = param_count(m);

        if (!registerLayout && strcmp(n, "RegisterLayout") == 0 && pc == 3) {
            char *a = type_name(get_param(m, 0));
            char *b = type_name(get_param(m, 1));
            if (a && b &&
                strcmp(a, "System.String") == 0 &&
                strcmp(b, "System.String") == 0)
                registerLayout = m;
        }

        if (!registerMatcher &&
            strcmp(n, "RegisterLayoutMatcher") == 0 && pc == 2)
            registerMatcher = m;

        if (!tryMatch &&
            strcmp(n, "TryFindMatchingLayout") == 0 && pc == 1)
            tryMatch = m;
    }

    unsigned char noMatcher[512];
    memset(noMatcher, 0, sizeof(noMatcher));

    Il2CppString *jsonStr = string_new(json);
    Il2CppString *layoutName = string_new("HID::Microsoft GamePad-1");

    void *layoutArgs[3] = {
        jsonStr,
        layoutName,
        noMatcher
    };

    exc = NULL;
    invoke(registerLayout, NULL, layoutArgs, &exc);
    fprintf(out, "RegisterLayout exception=%p\n", (void *)exc);

    void *matcherArgs[2] = {
        layoutName,
        unbox(matcher)
    };

    exc = NULL;
    invoke(registerMatcher, NULL, matcherArgs, &exc);
    fprintf(out, "RegisterLayoutMatcher exception=%p\n", (void *)exc);

    void *matchArgs[1] = { unbox(description) };

    exc = NULL;
    Il2CppObject *result =
        invoke(tryMatch, NULL, matchArgs, &exc);

    fprintf(out, "TryFindMatchingLayout = ");
    print_string((Il2CppString *)result, string_chars, string_length);
    fprintf(out, "\nexception=%p\n", (void *)exc);

    if (!exc && result)
        cm_phase1_done = 1;

    fclose(out);
}
