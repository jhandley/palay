
extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

#include <QApplication>
#include <stdio.h>
#include <getopt.h>
#include <QPrinter>
#include <QFile>

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [args] <script>\n", argv0);
    fprintf(stderr, "  -o Output file name\n");
    fprintf(stderr, "  -p Page size (Letter|A4)\n");
    fprintf(stderr, "  -f Output format (pdf|ps|odf|html|txt)\n");
}

/*!
 * Pushes lua stack trace for thread L onto lua stack as a string.
 * If there is already an error string on stack, it is
 * included in the stack trace string and the new string
 * replaces the error string.
 */
static int pushLuaStackTrace(lua_State *L)
{
    const char* msg = lua_tostring(L, -1);
    luaL_traceback(L, L, msg, 1);
    return 1;
}

static bool runLuaScript(lua_State *L, const QString &scriptFilename)
{
    QFile scriptFile(scriptFilename);
    if (!scriptFile.open(QFile::ReadOnly)) {
        fprintf(stderr, "Error opening file %s: %s", qPrintable(scriptFilename), qPrintable(scriptFile.errorString()));
        return -1;
    }

    QByteArray script = scriptFile.readAll();

    lua_pushcfunction(L, pushLuaStackTrace);
    int errhandlerIndex = lua_gettop(L);

    if (luaL_loadbuffer(L, script, script.count(), scriptFilename.toUtf8().constData())) {
        fprintf(stderr, "Error executing %s.\n%s", qPrintable(scriptFilename), lua_tostring(L, -1));
        return false;
    }

    if (lua_pcall(L, 0, LUA_MULTRET, errhandlerIndex)) {
        fprintf(stderr, "Error executing %s.\n%s", qPrintable(scriptFilename), lua_tostring(L, -1));
        return false;
    }

    lua_remove(L, errhandlerIndex); // clear error handler from stack

    return true;
}

static int savePalayDocument(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_getglobal(L, "saveAs");
    lua_pushvalue(L, 1);
    lua_call(L, 1, 0);

    return 0;
}

static int runPalayScript(const QString &scriptFilename, const QString &outputFilename, const QString &outputFormat, QPrinter::PageSize pageSize)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    if (!runLuaScript(L, ":/resources/scripts/init.lua")) {
        lua_close(L);
        return -1;
    }

    if (!runLuaScript(L, scriptFilename)) {
        lua_close(L);
        return -1;
    }

    lua_pushcfunction(L, savePalayDocument);
    lua_pushstring(L, outputFilename.toUtf8());
    if (lua_pcall(L, 1, 0, 0)) {
        fprintf(stderr, "Error writing file %s.\n%s", qPrintable(outputFilename), lua_tostring(L, -1));
        return -1;
    }

    lua_close(L);

    return 0;
}

int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QGuiApplication a(argc, argv);
#else
    QApplication a(argc, argv, false);
#endif

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    QString outputFilename;
    QPrinter::PageSize pageSize = QPrinter::Letter;
    QString outputFormat("pdf");

    int opt;
    while ((opt = getopt(argc, argv, "o:p:f:")) != -1) {
        switch (opt) {
        case 'o':
            outputFilename = optarg;
            break;
        case 'p':
            if (strcmp(optarg, "Letter") == 0)
                pageSize = QPrinter::Letter;
            else if (strcmp(optarg, "A4") == 0)
                pageSize = QPrinter::A4;
            else {
                fprintf(stderr, "Only 'Letter' and 'A4' are supported\n");
                return -1;
            }
            break;
        case 'f':
            if (strcmp(optarg, "pdf") == 0 ||
                strcmp(optarg, "ps") == 0 ||
                strcmp(optarg, "odf") == 0 ||
                strcmp(optarg, "html") == 0 ||
                strcmp(optarg, "txt"))
                outputFormat = optarg;
            else {
                fprintf(stderr, "Unsupported output format\n");
                return -1;
            }
            break;
        default:
            usage(argv[0]);
            return -1;
        }
    }

    if (outputFilename.isNull()) {
        fprintf(stderr, "Missing output file name\n");
        return -1;
    }
    if (optind + 1 > argc) {
        usage(argv[0]);
        return -1;
    }

    QString scriptFilename = argv[optind];


    return runPalayScript(scriptFilename, outputFilename, outputFormat, pageSize);

    return 0;
}
