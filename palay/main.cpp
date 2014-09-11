
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
#include "libpalay.h"

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

/*!
 * Forwards call to global methods to a method on the palay document
 * object. Meant to be used as a closure with the method and the document
 * object as upvalues.
 */
static int callWithDoc(lua_State *L)
{
    // lua_call wants us to first push the method, then
    // the arguments in order, we currently have the args in
    // order on top of the stack so we need to first push
    // the method, then the document object (self parameter as
    // first arg) and finally the arguments.

    int nargs = lua_gettop(L);
    lua_pushvalue(L, lua_upvalueindex(1)); // method to call
    lua_pushvalue(L, lua_upvalueindex(2)); // document object
    for (int i = 0; i < nargs; ++i) {
        lua_pushvalue(L, i + 1);
    }

    lua_call(L, nargs + 1, LUA_MULTRET);
    int nret = lua_gettop(L) - nargs;
    return nret;
}

static int runPalayScript(const QString &scriptFilename, const QString &outputFilename,
                          const QString &outputFormat, const QString &pageSize)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // require "libpalay"
    luaL_requiref(L, "libpalay", luaopen_libpalay, 0);

    // Set constants defined in libpalay in global environment
    // Anything the libpalay table that isn't function we treat
    // as a constant.
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        if (!lua_isfunction(L, -1)) {
            lua_setglobal(L, lua_tostring(L, -2));
        } else {
            lua_pop(L, 1);
        }
    }

    // Call libpalay.newDocument
    lua_getfield(L, 1, "newDocument");
    lua_call(L, 0, 1);

    // Expose all the methods in the document
    // as global functions (closures with the document as an upvalue).
    lua_getmetatable(L, -1);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, 2); // palaydocument
            lua_pushcclosure(L, callWithDoc, 2);
            lua_setglobal(L, lua_tostring(L, -2)); // _G[key] = closure
        } else {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 3);

    // Set the page size
    lua_getglobal(L, "pageSize");
    lua_pushstring(L, pageSize.toUtf8());
    if (lua_pcall(L, 1, 0, 0)) {
        fprintf(stderr, "Error setting page size.\n%s", lua_tostring(L, -1));
        return -1;
    }

    // Run the Lua script
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
    QApplication a(argc, argv, true);
#endif

    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }

    QString outputFilename;
    QString pageSize = "Letter";
    QString outputFormat("pdf");

    int opt;
    while ((opt = getopt(argc, argv, "o:p:f:")) != -1) {
        switch (opt) {
        case 'o':
            outputFilename = optarg;
            break;
        case 'p':
            pageSize = optarg;
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

}
