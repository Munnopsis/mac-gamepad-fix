#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <mach-o/dyld.h>

#define MGF_VERSION "1.1.0"

typedef struct {
    char executable[PATH_MAX];
    char steamAppId[64];

    int leftYInvert;
    int rightYInvert;
    int gameControllerBlock;
    int injectDriver;

    int dryRun;
    int verbose;
} Config;


static void trim(char *s)
{
    char *p = s;

    while (*p == ' ' || *p == '\t')
        p++;

    if (p != s)
        memmove(s, p, strlen(p) + 1);

    size_t n = strlen(s);

    while (n > 0 &&
           (s[n - 1] == ' ' ||
            s[n - 1] == '\t' ||
            s[n - 1] == '\n' ||
            s[n - 1] == '\r')) {
        s[--n] = '\0';
    }
}


static int parse_y(const char *value, int *result)
{
    if (strcmp(value, "normal") == 0) {
        *result = 0;
        return 1;
    }

    if (strcmp(value, "invert") == 0) {
        *result = 1;
        return 1;
    }

    return 0;
}


static int parse_gc(const char *value, int *result)
{
    if (strcmp(value, "auto") == 0) {
        *result = 0;
        return 1;
    }

    if (strcmp(value, "block") == 0) {
        *result = 1;
        return 1;
    }

    return 0;
}


static int parse_driver(const char *value, int *result)
{
    if (strcmp(value, "unity") == 0) {
        *result = 1;
        return 1;
    }

    if (strcmp(value, "none") == 0) {
        *result = 0;
        return 1;
    }

    return 0;
}


static void get_base_dir(char *out, size_t outSize)
{
    char path[PATH_MAX];
    uint32_t size = sizeof(path);

    if (_NSGetExecutablePath(path, &size) != 0) {
        fprintf(stderr, "Executable path too long\n");
        exit(1);
    }

    char resolved[PATH_MAX];

    if (!realpath(path, resolved)) {
        perror("realpath");
        exit(1);
    }

    char *slash = strrchr(resolved, '/');

    if (!slash) {
        fprintf(stderr, "Could not determine executable directory\n");
        exit(1);
    }

    *slash = '\0';

    snprintf(out, outSize, "%s", resolved);
}


static void expand_home(
    const char *input,
    char *output,
    size_t outputSize)
{
    if (input[0] == '~' && input[1] == '/') {
        const char *home = getenv("HOME");

        if (!home)
            home = "";

        snprintf(
            output,
            outputSize,
            "%s/%s",
            home,
            input + 2
        );
    } else {
        snprintf(output, outputSize, "%s", input);
    }
}


static int load_profile(
    const char *base,
    const char *name,
    Config *cfg)
{
    char path[PATH_MAX];

    snprintf(
        path,
        sizeof(path),
        "%s/profiles/%s.conf",
        base,
        name
    );

    FILE *f = fopen(path, "r");

    if (!f) {
        fprintf(
            stderr,
            "Profile not found: %s\n",
            path
        );
        return 0;
    }

    char line[PATH_MAX * 2];

    while (fgets(line, sizeof(line), f)) {
        trim(line);

        if (!line[0] || line[0] == '#')
            continue;

        char *eq = strchr(line, '=');

        if (!eq)
            continue;

        *eq = '\0';

        char *key = line;
        char *value = eq + 1;

        trim(key);
        trim(value);

        if (strcmp(key, "executable") == 0) {
            snprintf(
                cfg->executable,
                sizeof(cfg->executable),
                "%s",
                value
            );
        }
        else if (strcmp(key, "steam_app_id") == 0) {
            snprintf(
                cfg->steamAppId,
                sizeof(cfg->steamAppId),
                "%s",
                value
            );
        }
        else if (strcmp(key, "left_y") == 0) {
            if (!parse_y(value, &cfg->leftYInvert)) {
                fprintf(stderr, "Invalid left_y: %s\n", value);
                fclose(f);
                return 0;
            }
        }
        else if (strcmp(key, "right_y") == 0) {
            if (!parse_y(value, &cfg->rightYInvert)) {
                fprintf(stderr, "Invalid right_y: %s\n", value);
                fclose(f);
                return 0;
            }
        }
        else if (strcmp(key, "gamecontroller") == 0) {
            if (!parse_gc(value, &cfg->gameControllerBlock)) {
                fprintf(
                    stderr,
                    "Invalid gamecontroller: %s\n",
                    value
                );
                fclose(f);
                return 0;
            }
        }
        else if (strcmp(key, "driver") == 0) {
            if (!parse_driver(value, &cfg->injectDriver)) {
                fprintf(
                    stderr,
                    "Invalid driver: %s\n",
                    value
                );
                fclose(f);
                return 0;
            }
        }
    }

    fclose(f);
    return 1;
}


static void list_profiles(const char *base)
{
    char path[PATH_MAX];

    snprintf(
        path,
        sizeof(path),
        "%s/profiles",
        base
    );

    DIR *dir = opendir(path);

    if (!dir) {
        perror(path);
        return;
    }

    printf("Available profiles:\n");

    struct dirent *entry;

    while ((entry = readdir(dir))) {
        const char *name = entry->d_name;
        size_t len = strlen(name);

        if (len > 5 &&
            strcmp(name + len - 5, ".conf") == 0) {

            printf("  %.*s\n", (int)(len - 5), name);
        }
    }

    closedir(dir);
}


static void help(void)
{
    printf(
        "MacGamepadFix %s\n"
        "\n"
        "Usage:\n"
        "  MacGamepadFix --profile NAME [options]\n"
        "  MacGamepadFix --game EXECUTABLE [options]\n"
        "\n"
        "Options:\n"
        "  --profile NAME\n"
        "  --game EXECUTABLE\n"
        "  --steam-app-id ID\n"
        "  --left-y normal|invert\n"
        "  --right-y normal|invert\n"
        "  --gamecontroller auto|block\n"
        "  --dry-run\n"
        "  --verbose\n"
        "  --list-profiles\n"
        "  --version\n"
        "  --help\n"
        "\n"
        "Arguments after -- are passed to the game.\n",
        MGF_VERSION
    );
}


int main(int argc, char **argv)
{
    Config cfg = {0};

    cfg.leftYInvert = 0;
    cfg.rightYInvert = 0;
    cfg.gameControllerBlock = 0;
    cfg.injectDriver = 1;

    char base[PATH_MAX];
    get_base_dir(base, sizeof(base));

    const char *profile = NULL;

    /*
     * First pass:
     * load profile before CLI overrides.
     */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--profile") == 0 &&
            i + 1 < argc) {

            profile = argv[++i];
        }
    }

    if (profile) {
        if (!load_profile(base, profile, &cfg))
            return 1;
    }

    int gameArgsIndex = -1;

    /*
     * Second pass:
     * command-line values override profile values.
     */
    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "--") == 0) {
            gameArgsIndex = i + 1;
            break;
        }

        if (strcmp(argv[i], "--profile") == 0) {
            i++;
            continue;
        }

        if (strcmp(argv[i], "--game") == 0 &&
            i + 1 < argc) {

            snprintf(
                cfg.executable,
                sizeof(cfg.executable),
                "%s",
                argv[++i]
            );
        }

        else if (strcmp(argv[i], "--steam-app-id") == 0 &&
                 i + 1 < argc) {

            snprintf(
                cfg.steamAppId,
                sizeof(cfg.steamAppId),
                "%s",
                argv[++i]
            );
        }

        else if (strcmp(argv[i], "--left-y") == 0 &&
                 i + 1 < argc) {

            if (!parse_y(argv[++i], &cfg.leftYInvert)) {
                fprintf(stderr, "Invalid --left-y value\n");
                return 1;
            }
        }

        else if (strcmp(argv[i], "--right-y") == 0 &&
                 i + 1 < argc) {

            if (!parse_y(argv[++i], &cfg.rightYInvert)) {
                fprintf(stderr, "Invalid --right-y value\n");
                return 1;
            }
        }

        else if (strcmp(argv[i], "--gamecontroller") == 0 &&
                 i + 1 < argc) {

            if (!parse_gc(
                    argv[++i],
                    &cfg.gameControllerBlock)) {

                fprintf(
                    stderr,
                    "Invalid --gamecontroller value\n"
                );
                return 1;
            }
        }

        else if (strcmp(argv[i], "--dry-run") == 0) {
            cfg.dryRun = 1;
        }

        else if (strcmp(argv[i], "--verbose") == 0) {
            cfg.verbose = 1;
        }

        else if (strcmp(argv[i], "--version") == 0) {
            printf("MacGamepadFix %s\n", MGF_VERSION);
            return 0;
        }

        else if (strcmp(argv[i], "--list-profiles") == 0) {
            list_profiles(base);
            return 0;
        }

        else if (strcmp(argv[i], "--help") == 0) {
            help();
            return 0;
        }

        else {
            fprintf(
                stderr,
                "Unknown option: %s\n",
                argv[i]
            );
            return 1;
        }
    }

    if (!cfg.executable[0]) {
        fprintf(
            stderr,
            "No game executable specified.\n"
            "Use --profile NAME or --game EXECUTABLE.\n"
        );
        return 1;
    }

    char executable[PATH_MAX];

    expand_home(
        cfg.executable,
        executable,
        sizeof(executable)
    );

    if (executable[0] != '/') {
        char relativeExecutable[PATH_MAX];
        snprintf(
            relativeExecutable,
            sizeof(relativeExecutable),
            "%s/%s",
            base,
            executable
        );
        snprintf(
            executable,
            sizeof(executable),
            "%s",
            relativeExecutable
        );
    }

    if (!cfg.injectDriver && cfg.gameControllerBlock) {
        fprintf(
            stderr,
            "Invalid configuration: gamecontroller=block requires driver=unity\n"
        );
        return 1;
    }

    char driver[PATH_MAX];
    char gcblock[PATH_MAX];

    snprintf(
        driver,
        sizeof(driver),
        "%s/lib/libMacGamepadFix.dylib",
        base
    );

    snprintf(
        gcblock,
        sizeof(gcblock),
        "%s/lib/libMacGamepadFixGCBlock.dylib",
        base
    );

    if (cfg.injectDriver && access(driver, R_OK) != 0) {
        fprintf(stderr, "Driver not found: %s\n", driver);
        return 1;
    }

    if (cfg.gameControllerBlock &&
        access(gcblock, R_OK) != 0) {

        fprintf(
            stderr,
            "GameController blocker not found: %s\n",
            gcblock
        );
        return 1;
    }

    setenv(
        "MGF_LEFT_Y_INVERT",
        cfg.leftYInvert ? "1" : "0",
        1
    );

    setenv(
        "MGF_RIGHT_Y_INVERT",
        cfg.rightYInvert ? "1" : "0",
        1
    );

    setenv(
        "MGF_VERBOSE",
        cfg.verbose ? "1" : "0",
        1
    );

    if (cfg.steamAppId[0]) {
        setenv("SteamAppId", cfg.steamAppId, 1);
        setenv("SteamGameId", cfg.steamAppId, 1);
    }

    const char *oldDyld =
        getenv("DYLD_INSERT_LIBRARIES");

    char dyld[PATH_MAX * 4];

    if (cfg.gameControllerBlock) {
        if (oldDyld && oldDyld[0]) {
            snprintf(
                dyld,
                sizeof(dyld),
                "%s:%s:%s",
                gcblock,
                driver,
                oldDyld
            );
        } else {
            snprintf(
                dyld,
                sizeof(dyld),
                "%s:%s",
                gcblock,
                driver
            );
        }
    } else {
        if (oldDyld && oldDyld[0]) {
            snprintf(
                dyld,
                sizeof(dyld),
                "%s:%s",
                driver,
                oldDyld
            );
        } else {
            snprintf(
                dyld,
                sizeof(dyld),
                "%s",
                driver
            );
        }
    }

    if (cfg.injectDriver)
        setenv("DYLD_INSERT_LIBRARIES", dyld, 1);
    else
        unsetenv("DYLD_INSERT_LIBRARIES");

    if (cfg.dryRun || cfg.verbose) {
        printf("MacGamepadFix %s\n", MGF_VERSION);
        printf("Game:           %s\n", executable);
        printf(
            "Left Stick Y:   %s\n",
            cfg.leftYInvert ? "invert" : "normal"
        );
        printf(
            "Right Stick Y:  %s\n",
            cfg.rightYInvert ? "invert" : "normal"
        );
        printf(
            "GameController: %s\n",
            cfg.gameControllerBlock ? "block" : "auto"
        );

        if (cfg.steamAppId[0])
            printf("Steam App ID:   %s\n", cfg.steamAppId);

        if (cfg.injectDriver)
            printf("Driver:         %s\n", driver);
        else
            printf("Driver:         none\n");

        if (cfg.gameControllerBlock)
            printf("GC blocker:     %s\n", gcblock);
    }

    if (cfg.dryRun)
        return 0;

    int extraCount = 0;

    if (gameArgsIndex >= 0)
        extraCount = argc - gameArgsIndex;

    char **childArgs =
        calloc((size_t)extraCount + 2, sizeof(char *));

    if (!childArgs) {
        perror("calloc");
        return 1;
    }

    childArgs[0] = executable;

    for (int i = 0; i < extraCount; i++)
        childArgs[i + 1] = argv[gameArgsIndex + i];

    childArgs[extraCount + 1] = NULL;

    execv(executable, childArgs);

    fprintf(
        stderr,
        "Could not start game: %s\n",
        strerror(errno)
    );

    free(childArgs);
    return 1;
}
